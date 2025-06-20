// redis_id_allocator.hpp
#pragma once
#include "swss/dbconnector.h"
#include "swss/rediscommand.h"
#include <string>
#include <optional>
#include <vector>
#include <chrono>
#include <stdexcept>
#include <mutex>
#include <memory>
#include <swss/logger.h>

class RedisIDAllocator {
public:
    struct Config {
        int min_id;
        int max_id;
        int ttl_seconds = 3600;  // Default 1 hour TTL
        int cleanup_interval = 300;  // Cleanup every 5 minutes
        bool enable_auto_cleanup = true;
        std::string prefix = "alloc";
        int init_lock_timeout = 30;  // Seconds to wait for initialization lock
        int heartbeat_interval = 10;  // Heartbeat every 10 seconds
        int creator_ttl = 60;  // Creator lease TTL
    };

    enum class Role {
        CREATOR,    // This instance created and manages the allocator
        CONSUMER    // This instance only uses the allocator
    };

    // Factory method that handles distributed initialization
    static std::unique_ptr<RedisIDAllocator> createOrAttach(
        swss::DBConnector* db, 
        const std::string& name, 
        const Config& config) {
        
        std::string initKey = config.prefix + ":" + name + ":init";
        std::string creatorKey = config.prefix + ":" + name + ":creator";
        std::string configKey = config.prefix + ":" + name + ":config";
        
        // Try to become the creator with distributed lock
        Role role = attemptBecomeCreator(db, initKey, creatorKey, configKey, config);
        
        if (role == Role::CREATOR) {
            SWSS_LOG_NOTICE("Became CREATOR for allocator '%s' (range: %d-%d)", 
                name.c_str(), config.min_id, config.max_id);
        } else {
            SWSS_LOG_NOTICE("Became CONSUMER for allocator '%s'", name.c_str());
        }
        
        return std::unique_ptr<RedisIDAllocator>(new RedisIDAllocator(db, name, config, role));
    }

    ~RedisIDAllocator() {
        if (m_role == Role::CREATOR) {
            // Release creator lease
            swss::RedisCommand cmd;
            cmd.format("DEL %s", m_creatorKey.c_str());
            m_db->sendCommand(&cmd, 0);
            SWSS_LOG_INFO("Released creator lease for allocator '%s'", m_name.c_str());
        }
    }

    // Allocate a single ID with optional TTL
    std::optional<int> allocate(int ttl_seconds = 0) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (!ensureAllocatorReady()) return std::nullopt;
        
        maybeCleanupExpired();
        if (ttl_seconds == 0) ttl_seconds = m_config.ttl_seconds;
        
        swss::RedisCommand cmd;
        cmd.format("EVAL %s 2 %s %s %d %d %ld", 
            m_allocateScript.c_str(),
            m_bitmapKey.c_str(), 
            m_metaKey.c_str(),
            m_config.min_id, 
            m_config.max_id,
            nowSeconds() + ttl_seconds);
            
        return executeIntegerCommand(cmd);
    }

    // Allocate a contiguous range of IDs
    std::optional<int> allocateRange(int count, int ttl_seconds = 0) {
        if (count <= 0 || count > (m_config.max_id - m_config.min_id + 1)) {
            return std::nullopt;
        }
        
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!ensureAllocatorReady()) return std::nullopt;
        
        maybeCleanupExpired();
        if (ttl_seconds == 0) ttl_seconds = m_config.ttl_seconds;
        
        swss::RedisCommand cmd;
        cmd.format("EVAL %s 2 %s %s %d %d %d %ld",
            m_allocateRangeScript.c_str(),
            m_bitmapKey.c_str(),
            m_metaKey.c_str(),
            m_config.min_id,
            m_config.max_id,
            count,
            nowSeconds() + ttl_seconds);
            
        return executeIntegerCommand(cmd);
    }

    // Reserve a specific ID
    bool reserve(int id, int ttl_seconds = 0) {
        if (!isValidId(id)) return false;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!ensureAllocatorReady()) return false;
        
        maybeCleanupExpired();
        if (ttl_seconds == 0) ttl_seconds = m_config.ttl_seconds;
        
        swss::RedisCommand cmd;
        cmd.format("EVAL %s 2 %s %s %d %ld",
            m_reserveScript.c_str(),
            m_bitmapKey.c_str(),
            m_metaKey.c_str(),
            id,
            nowSeconds() + ttl_seconds);
            
        auto result = executeIntegerCommand(cmd);
        return result.has_value() && result.value() == 1;
    }

    // Reserve a range of specific IDs
    bool reserveRange(int start, int end, int ttl_seconds = 0) {
        if (start > end || !isValidId(start) || !isValidId(end)) {
            return false;
        }
        
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!ensureAllocatorReady()) return false;
        
        maybeCleanupExpired();
        if (ttl_seconds == 0) ttl_seconds = m_config.ttl_seconds;
        
        swss::RedisCommand cmd;
        cmd.format("EVAL %s 2 %s %s %d %d %ld",
            m_reserveRangeScript.c_str(),
            m_bitmapKey.c_str(),
            m_metaKey.c_str(),
            start,
            end,
            nowSeconds() + ttl_seconds);
            
        auto result = executeIntegerCommand(cmd);
        return result.has_value() && result.value() == 1;
    }

    // Free an ID
    bool free(int id) {
        if (!isValidId(id)) return false;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!ensureAllocatorReady()) return false;
        
        swss::RedisCommand cmd;
        cmd.format("EVAL %s 2 %s %s %d",
            m_freeScript.c_str(),
            m_bitmapKey.c_str(),
            m_metaKey.c_str(),
            id);
            
        auto result = executeIntegerCommand(cmd);
        return result.has_value() && result.value() == 1;
    }

    // Free a range of IDs
    bool freeRange(int start, int end) {
        if (start > end || !isValidId(start) || !isValidId(end)) {
            return false;
        }
        
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!ensureAllocatorReady()) return false;
        
        swss::RedisCommand cmd;
        cmd.format("EVAL %s 2 %s %s %d %d",
            m_freeRangeScript.c_str(),
            m_bitmapKey.c_str(),
            m_metaKey.c_str(),
            start,
            end);
            
        auto result = executeIntegerCommand(cmd);
        return result.has_value() && result.value() == (end - start + 1);
    }

    // Check if an ID is allocated
    bool isAllocated(int id) const {
        if (!isValidId(id)) return false;
        
        swss::RedisCommand cmd;
        cmd.format("GETBIT %s %d", m_bitmapKey.c_str(), id);
        m_db->sendCommand(&cmd, 0);
        auto reply = cmd.getReply();
        return reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1;
    }

    // Extend TTL for an ID
    bool extendTTL(int id, int additional_seconds) {
        if (!isValidId(id)) return false;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!ensureAllocatorReady()) return false;
        
        swss::RedisCommand cmd;
        cmd.format("EVAL %s 1 %s %d %d",
            m_extendTTLScript.c_str(),
            m_metaKey.c_str(),
            id,
            additional_seconds);
            
        auto result = executeIntegerCommand(cmd);
        return result.has_value() && result.value() == 1;
    }

    // Force cleanup of expired IDs (only works for CREATOR)
    int cleanupExpired() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_role != Role::CREATOR) {
            SWSS_LOG_WARN("Only CREATOR can perform manual cleanup");
            return 0;
        }
        return performCleanup();
    }

    // Get statistics about allocator usage
    struct Stats {
        int total_range;
        int allocated_count;
        int available_count;
        int expired_count;
        double utilization_percent;
        Role role;
        bool creator_alive;
        std::string creator_id;
    };

    Stats getStats() const {
        Stats stats;
        stats.total_range = m_config.max_id - m_config.min_id + 1;
        stats.role = m_role;
        
        // Check if creator is alive
        stats.creator_alive = isCreatorAlive(stats.creator_id);
        
        // Count allocated IDs
        swss::RedisCommand cmd;
        cmd.format("EVAL %s 1 %s %d %d",
            m_getStatsScript.c_str(),
            m_bitmapKey.c_str(),
            m_config.min_id,
            m_config.max_id);
            
        m_db->sendCommand(&cmd, 0);
        auto reply = cmd.getReply();
        
        if (reply && reply->type == REDIS_REPLY_INTEGER) {
            stats.allocated_count = static_cast<int>(reply->integer);
        } else {
            stats.allocated_count = 0;
        }
        
        stats.available_count = stats.total_range - stats.allocated_count;
        stats.utilization_percent = (static_cast<double>(stats.allocated_count) / stats.total_range) * 100.0;
        stats.expired_count = countExpiredIds();
        
        return stats;
    }

    // Get all allocated IDs
    std::vector<int> getAllocatedIds() const {
        std::vector<int> result;
        
        for (int id = m_config.min_id; id <= m_config.max_id; ++id) {
            if (isAllocated(id)) {
                result.push_back(id);
            }
        }
        
        return result;
    }

    // Get role of this instance
    Role getRole() const { return m_role; }

    // Clear all allocations (dangerous - use with caution, CREATOR only)
    bool clearAll() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_role != Role::CREATOR) {
            SWSS_LOG_ERROR("Only CREATOR can clear all allocations");
            return false;
        }
        
        swss::RedisCommand cmd1, cmd2;
        cmd1.format("DEL %s", m_bitmapKey.c_str());
        cmd2.format("DEL %s", m_metaKey.c_str());
        
        m_db->sendCommand(&cmd1, 0);
        m_db->sendCommand(&cmd2, 0);
        
        auto reply1 = cmd1.getReply();
        auto reply2 = cmd2.getReply();
        
        return (reply1 && reply1->type == REDIS_REPLY_INTEGER) &&
               (reply2 && reply2->type == REDIS_REPLY_INTEGER);
    }

private:
    swss::DBConnector* m_db;
    std::string m_name;
    Config m_config;
    Role m_role;
    std::string m_bitmapKey;
    std::string m_metaKey;
    std::string m_initKey;
    std::string m_creatorKey;
    std::string m_configKey;
    std::string m_creatorId;
    mutable std::mutex m_mutex;
    int64_t m_lastCleanup;
    int64_t m_lastHeartbeat;

    // Lua scripts for atomic operations
    std::string m_allocateScript;
    std::string m_allocateRangeScript;
    std::string m_reserveScript;
    std::string m_reserveRangeScript;
    std::string m_freeScript;
    std::string m_freeRangeScript;
    std::string m_extendTTLScript;
    std::string m_getStatsScript;
    std::string m_cleanupScript;
    std::string m_initScript;

    // Private constructor - use createOrAttach factory method
    RedisIDAllocator(swss::DBConnector* db, const std::string& name, const Config& config, Role role)
        : m_db(db), m_name(name), m_config(config), m_role(role),
          m_bitmapKey(m_config.prefix + ":" + name + ":bitmap"),
          m_metaKey(m_config.prefix + ":" + name + ":meta"),
          m_initKey(m_config.prefix + ":" + name + ":init"),
          m_creatorKey(m_config.prefix + ":" + name + ":creator"),
          m_configKey(m_config.prefix + ":" + name + ":config"),
          m_creatorId(generateCreatorId()),
          m_lastCleanup(nowSeconds()),
          m_lastHeartbeat(nowSeconds())
    {
        if (m_config.min_id < 0 || m_config.max_id < m_config.min_id) {
            throw std::invalid_argument("Invalid ID range: min=" + 
                std::to_string(m_config.min_id) + ", max=" + std::to_string(m_config.max_id));
        }
        
        if (m_config.max_id - m_config.min_id > 1000000) {
            SWSS_LOG_WARN("Large ID range (%d-%d) may impact performance", 
                m_config.min_id, m_config.max_id);
        }
        
        initializeLuaScripts();
        
        if (m_role == Role::CREATOR) {
            startHeartbeat();
        }
    }

    static Role attemptBecomeCreator(swss::DBConnector* db, const std::string& initKey, 
                                   const std::string& creatorKey, const std::string& configKey,
                                   const Config& config) {
        
        std::string creatorId = generateCreatorId();
        
        // Lua script for atomic initialization attempt
        std::string initScript = R"(
            local init_key = KEYS[1]
            local creator_key = KEYS[2]
            local config_key = KEYS[3]
            local creator_id = ARGV[1]
            local creator_ttl = tonumber(ARGV[2])
            local config_data = ARGV[3]
            
            -- Check if already initialized
            if redis.call('EXISTS', init_key) == 1 then
                -- Check if creator is still alive
                if redis.call('EXISTS', creator_key) == 1 then
                    return 'CONSUMER'  -- Creator exists, become consumer
                else
                    -- Creator died, try to take over
                    local result = redis.call('SET', creator_key, creator_id, 'EX', creator_ttl, 'NX')
                    if result then
                        return 'CREATOR'  -- Successfully took over
                    else
                        return 'CONSUMER'  -- Someone else took over
                    end
                end
            else
                -- Try to initialize
                local result = redis.call('SET', init_key, '1', 'NX')
                if result then
                    -- Successfully initialized, become creator
                    redis.call('SET', creator_key, creator_id, 'EX', creator_ttl)
                    redis.call('SET', config_key, config_data)
                    return 'CREATOR'
                else
                    -- Someone else initialized first
                    return 'CONSUMER'
                end
            end
        )";

        // Serialize config for storage
        std::string configData = std::to_string(config.min_id) + "," + 
                                std::to_string(config.max_id) + "," + 
                                std::to_string(config.ttl_seconds);

        swss::RedisCommand cmd;
        cmd.format("EVAL %s 3 %s %s %s %s %d %s",
            initScript.c_str(),
            initKey.c_str(),
            creatorKey.c_str(),
            configKey.c_str(),
            creatorId.c_str(),
            config.creator_ttl,
            configData.c_str());

        db->sendCommand(&cmd, 0);
        auto reply = cmd.getReply();

        if (reply && reply->type == REDIS_REPLY_STRING) {
            std::string result(reply->str);
            if (result == "CREATOR") {
                return Role::CREATOR;
            }
        }

        // Wait a bit to ensure initialization is complete
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return Role::CONSUMER;
    }

    void initializeLuaScripts() {
        // Single ID allocation with TTL
        m_allocateScript = R"(
            local bitmap_key = KEYS[1]
            local meta_key = KEYS[2]
            local min_id = tonumber(ARGV[1])
            local max_id = tonumber(ARGV[2])
            local expire_time = tonumber(ARGV[3])
            
            for i = min_id, max_id do
                if redis.call('GETBIT', bitmap_key, i) == 0 then
                    redis.call('SETBIT', bitmap_key, i, 1)
                    redis.call('HSET', meta_key, i, expire_time)
                    return i
                end
            end
            return nil
        )";

        // Range allocation
        m_allocateRangeScript = R"(
            local bitmap_key = KEYS[1]
            local meta_key = KEYS[2]
            local min_id = tonumber(ARGV[1])
            local max_id = tonumber(ARGV[2])
            local count = tonumber(ARGV[3])
            local expire_time = tonumber(ARGV[4])
            
            for i = min_id, max_id - count + 1 do
                local available = true
                for j = 0, count - 1 do
                    if redis.call('GETBIT', bitmap_key, i + j) == 1 then
                        available = false
                        break
                    end
                end
                if available then
                    for j = 0, count - 1 do
                        redis.call('SETBIT', bitmap_key, i + j, 1)
                        redis.call('HSET', meta_key, i + j, expire_time)
                    end
                    return i
                end
            end
            return nil
        )";

        // Reserve specific ID
        m_reserveScript = R"(
            local bitmap_key = KEYS[1]
            local meta_key = KEYS[2]
            local id = tonumber(ARGV[1])
            local expire_time = tonumber(ARGV[2])
            
            if redis.call('GETBIT', bitmap_key, id) == 0 then
                redis.call('SETBIT', bitmap_key, id, 1)
                redis.call('HSET', meta_key, id, expire_time)
                return 1
            end
            return 0
        )";

        // Reserve range
        m_reserveRangeScript = R"(
            local bitmap_key = KEYS[1]
            local meta_key = KEYS[2]
            local start_id = tonumber(ARGV[1])
            local end_id = tonumber(ARGV[2])
            local expire_time = tonumber(ARGV[3])
            
            for i = start_id, end_id do
                if redis.call('GETBIT', bitmap_key, i) == 1 then
                    return 0
                end
            end
            
            for i = start_id, end_id do
                redis.call('SETBIT', bitmap_key, i, 1)
                redis.call('HSET', meta_key, i, expire_time)
            end
            return 1
        )";

        // Free single ID
        m_freeScript = R"(
            local bitmap_key = KEYS[1]
            local meta_key = KEYS[2]
            local id = tonumber(ARGV[1])
            
            local was_set = redis.call('GETBIT', bitmap_key, id)
            if was_set == 1 then
                redis.call('SETBIT', bitmap_key, id, 0)
                redis.call('HDEL', meta_key, id)
                return 1
            end
            return 0
        )";

        // Free range
        m_freeRangeScript = R"(
            local bitmap_key = KEYS[1]
            local meta_key = KEYS[2]
            local start_id = tonumber(ARGV[1])
            local end_id = tonumber(ARGV[2])
            local freed = 0
            
            for i = start_id, end_id do
                if redis.call('GETBIT', bitmap_key, i) == 1 then
                    redis.call('SETBIT', bitmap_key, i, 0)
                    redis.call('HDEL', meta_key, i)
                    freed = freed + 1
                end
            end
            return freed
        )";

        // Extend TTL
        m_extendTTLScript = R"(
            local meta_key = KEYS[1]
            local id = tonumber(ARGV[1])
            local additional_seconds = tonumber(ARGV[2])
            
            local current_expiry = redis.call('HGET', meta_key, id)
            if current_expiry then
                local new_expiry = tonumber(current_expiry) + additional_seconds
                redis.call('HSET', meta_key, id, new_expiry)
                return 1
            end
            return 0
        )";

        // Get statistics
        m_getStatsScript = R"(
            local bitmap_key = KEYS[1]
            local min_id = tonumber(ARGV[1])
            local max_id = tonumber(ARGV[2])
            local count = 0
            
            for i = min_id, max_id do
                if redis.call('GETBIT', bitmap_key, i) == 1 then
                    count = count + 1
                end
            end
            return count
        )";

        // Cleanup expired IDs
        m_cleanupScript = R"(
            local bitmap_key = KEYS[1]
            local meta_key = KEYS[2]
            local current_time = tonumber(ARGV[1])
            local cleaned = 0
            
            local meta_data = redis.call('HGETALL', meta_key)
            for i = 1, #meta_data, 2 do
                local id = tonumber(meta_data[i])
                local expire_time = tonumber(meta_data[i + 1])
                if current_time > expire_time then
                    redis.call('SETBIT', bitmap_key, id, 0)
                    redis.call('HDEL', meta_key, id)
                    cleaned = cleaned + 1
                end
            end
            return cleaned
        )";
    }

    bool ensureAllocatorReady() {
        // Check if allocator exists
        swss::RedisCommand cmd;
        cmd.format("EXISTS %s", m_initKey.c_str());
        m_db->sendCommand(&cmd, 0);
        auto reply = cmd.getReply();
        
        if (!reply || reply->type != REDIS_REPLY_INTEGER || reply->integer != 1) {
            SWSS_LOG_ERROR("Allocator '%s' not initialized", m_name.c_str());
            return false;
        }

        // If we're the creator, send heartbeat
        if (m_role == Role::CREATOR) {
            sendHeartbeat();
        } else {
            // If we're a consumer, check if creator is alive
            if (!isCreatorAlive()) {
                SWSS_LOG_WARN("Creator for allocator '%s' appears to be dead", m_name.c_str());
                // Could implement creator takeover logic here
            }
        }

        return true;
    }

    void sendHeartbeat() {
        int64_t now = nowSeconds();
        if (now - m_lastHeartbeat >= m_config.heartbeat_interval) {
            swss::RedisCommand cmd;
            cmd.format("EXPIRE %s %d", m_creatorKey.c_str(), m_config.creator_ttl);
            m_db->sendCommand(&cmd, 0);
            m_lastHeartbeat = now;
        }
    }

    void startHeartbeat() {
        // Set initial creator key with TTL
        swss::RedisCommand cmd;
        cmd.format("SET %s %s EX %d", m_creatorKey.c_str(), m_creatorId.c_str(), m_config.creator_ttl);
        m_db->sendCommand(&cmd, 0);
    }

    bool isCreatorAlive(std::string& creatorId) const {
        swss::RedisCommand cmd;
        cmd.format("GET %s", m_creatorKey.c_str());
        m_db->sendCommand(&cmd, 0);
        auto reply = cmd.getReply();
        
        if (reply && reply->type == REDIS_REPLY_STRING) {
            creatorId = std::string(reply->str);
            return true;
        }
        
        creatorId.clear();
        return false;
    }

    bool isCreatorAlive() const {
        std::string dummy;
        return isCreatorAlive(dummy);
    }

    bool isValidId(int id) const {
        return id >= m_config.min_id && id <= m_config.max_id;
    }

    std::optional<int> executeIntegerCommand(swss::RedisCommand& cmd) const {
        try {
            m_db->sendCommand(&cmd, 0);
            auto reply = cmd.getReply();
            if (reply && reply->type == REDIS_REPLY_INTEGER) {
                return static_cast<int>(reply->integer);
            }
            if (reply && reply->type == REDIS_REPLY_NIL) {
                return std::nullopt;
            }
        } catch (const std::exception& e) {
            SWSS_LOG_ERROR("Redis command failed: %s", e.what());
        }
        return std::nullopt;
    }

    void maybeCleanupExpired() {
        if (m_role != Role::CREATOR || !m_config.enable_auto_cleanup) {
            return;
        }
        
        int64_t now = nowSeconds();
        if (now - m_lastCleanup >= m_config.cleanup_interval) {
            performCleanup();
            m_lastCleanup = now;
        }
    }

    int performCleanup() {
        swss::RedisCommand cmd;
        cmd.format("EVAL %s 2 %s %s %ld",
            m_cleanupScript.c_str(),
            m_bitmapKey.c_str(),
            m_metaKey.c_str(),
            nowSeconds());
            
        auto result = executeIntegerCommand(cmd);
        int cleaned = result.value_or(0);
        
        if (cleaned > 0) {
            SWSS_LOG_INFO("Cleaned up %d expired IDs from allocator '%s'", 
                cleaned, m_name.c_str());
        }
        
        return cleaned;
    }

    int countExpiredIds() const {
        swss::RedisCommand cmd;
        cmd.format("HGETALL %s", m_metaKey.c_str());
        m_db->sendCommand(&cmd, 0);
        auto reply = cmd.getReply();
        
        int expired = 0;
        int64_t now = nowSeconds();
        
        if (reply && reply->type == REDIS_REPLY_ARRAY) {
            for (size_t i = 0; i + 1 < reply->elements; i += 2) {
                int64_t exp = std::stoll(reply->element[i + 1]->str);
                if (now > exp) {
                    expired++;
                }
            }
        }
        
        return expired;
    }

    static std::string generateCreatorId() {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        
        // Include process ID and timestamp for uniqueness
        return "creator_" + std::to_string(getpid()) + "_" + std::to_string(timestamp);
    }

    static int64_t nowSeconds() {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};
