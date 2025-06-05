#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <bitset>
#include <sstream>
#include <algorithm>
#include <arpa/inet.h>

// Route attributes for policy-based routing
struct RouteAttributes {
    uint32_t nextHop;
    uint16_t asPath;
    uint32_t med;           // Multi-Exit Discriminator
    uint32_t localPref;     // Local preference
    uint16_t tag;           // Route tag
    uint8_t protocol;       // Routing protocol (OSPF, BGP, etc.)
    uint8_t adminDistance;  // Administrative distance
    bool isActive;
    
    RouteAttributes() : nextHop(0), asPath(0), med(0), localPref(100), 
                       tag(0), protocol(0), adminDistance(1), isActive(true) {}
};

// Policy matching criteria
struct PolicyRule {
    uint32_t srcPrefix;
    uint8_t srcPrefixLen;
    uint32_t dstPrefix;
    uint8_t dstPrefixLen;
    uint16_t srcPort;
    uint16_t dstPort;
    uint8_t protocol;
    uint8_t tos;            // Type of Service
    uint32_t flowLabel;     // IPv6 flow label equivalent
    uint32_t priority;      // Rule priority (lower = higher priority)
    
    PolicyRule() : srcPrefix(0), srcPrefixLen(0), dstPrefix(0), dstPrefixLen(0),
                   srcPort(0), dstPort(0), protocol(0), tos(0), flowLabel(0), priority(100) {}
};

// Packet classification info
struct PacketInfo {
    uint32_t srcIP;
    uint32_t dstIP;
    uint16_t srcPort;
    uint16_t dstPort;
    uint8_t protocol;
    uint8_t tos;
    uint32_t flowLabel;
    
    PacketInfo() : srcIP(0), dstIP(0), srcPort(0), dstPort(0), 
                   protocol(0), tos(0), flowLabel(0) {}
};

class PolicyRadixNode {
public:
    std::unique_ptr<PolicyRadixNode> left;   // 0 bit
    std::unique_ptr<PolicyRadixNode> right;  // 1 bit
    std::vector<std::pair<PolicyRule, RouteAttributes>> routes;
    uint32_t prefix;
    uint8_t prefixLen;
    bool isValid;
    
    PolicyRadixNode() : prefix(0), prefixLen(0), isValid(false) {}
};

class PolicyRoutingTree {
private:
    std::unique_ptr<PolicyRadixNode> root;
    std::unordered_map<uint32_t, std::string> routeTable;  // For debugging
    
public:
    PolicyRoutingTree() : root(std::make_unique<PolicyRadixNode>()) {}
    
    // Convert IP string to uint32_t
    uint32_t ipStringToInt(const std::string& ip) {
        struct in_addr addr;
        inet_aton(ip.c_str(), &addr);
        return ntohl(addr.s_addr);
    }
    
    // Convert uint32_t to IP string
    std::string ipIntToString(uint32_t ip) {
        struct in_addr addr;
        addr.s_addr = htonl(ip);
        return std::string(inet_ntoa(addr));
    }
    
    // Add a policy-based route
    void addRoute(const std::string& prefix, uint8_t prefixLen, 
                  const PolicyRule& policy, const RouteAttributes& attrs) {
        uint32_t prefixInt = ipStringToInt(prefix);
        
        // Create mask for prefix length
        uint32_t mask = (prefixLen == 0) ? 0 : (0xFFFFFFFF << (32 - prefixLen));
        prefixInt &= mask;
        
        insertRoute(root.get(), prefixInt, prefixLen, 0, policy, attrs);
        
        std::cout << "Added policy route: " << prefix << "/" << (int)prefixLen 
                  << " -> " << ipIntToString(attrs.nextHop) 
                  << " (priority: " << policy.priority << ")" << std::endl;
    }
    
    // Lookup with policy-based routing
    std::vector<std::pair<PolicyRule, RouteAttributes>> 
    lookup(const PacketInfo& packet) {
        std::vector<std::pair<PolicyRule, RouteAttributes>> candidates;
        
        // First, do longest prefix match on destination
        findMatchingRoutes(root.get(), packet.dstIP, 0, 32, candidates);
        
        // Then apply policy filtering
        std::vector<std::pair<PolicyRule, RouteAttributes>> validRoutes;
        
        for (const auto& [policy, attrs] : candidates) {
            if (matchesPolicy(packet, policy)) {
                validRoutes.push_back({policy, attrs});
            }
        }
        
        // Sort by priority (lower number = higher priority)
        std::sort(validRoutes.begin(), validRoutes.end(),
                 [](const auto& a, const auto& b) {
                     return a.first.priority < b.first.priority;
                 });
        
        return validRoutes;
    }
    
    // Find best route for a packet
    RouteAttributes* findBestRoute(const PacketInfo& packet) {
        auto routes = lookup(packet);
        
        if (routes.empty()) {
            return nullptr;
        }
        
        // Additional tie-breaking logic (administrative distance, metrics, etc.)
        auto bestRoute = std::min_element(routes.begin(), routes.end(),
            [](const auto& a, const auto& b) {
                const auto& attrsA = a.second;
                const auto& attrsB = b.second;
                
                // First by administrative distance
                if (attrsA.adminDistance != attrsB.adminDistance) {
                    return attrsA.adminDistance < attrsB.adminDistance;
                }
                
                // Then by local preference (higher is better)
                if (attrsA.localPref != attrsB.localPref) {
                    return attrsA.localPref > attrsB.localPref;
                }
                
                // Then by MED (lower is better)
                return attrsA.med < attrsB.med;
            });
        
        return &(bestRoute->second);
    }
    
    // Display routing table with policies
    void displayRoutes() {
        std::cout << "\n=== Policy-Based Routing Table ===" << std::endl;
        std::cout << std::left << std::setw(18) << "Prefix" 
                  << std::setw(15) << "Next Hop" 
                  << std::setw(8) << "Priority"
                  << std::setw(8) << "Admin"
                  << std::setw(8) << "LocalPref"
                  << std::setw(8) << "MED" << std::endl;
        std::cout << std::string(75, '-') << std::endl;
        
        displayRoutesHelper(root.get(), 0, 0);
    }
    
    // Simulate route lookup for debugging
    void simulatePacket(const std::string& srcIP, const std::string& dstIP, 
                       uint16_t srcPort, uint16_t dstPort, uint8_t protocol) {
        PacketInfo packet;
        packet.srcIP = ipStringToInt(srcIP);
        packet.dstIP = ipStringToInt(dstIP);
        packet.srcPort = srcPort;
        packet.dstPort = dstPort;
        packet.protocol = protocol;
        
        std::cout << "\n=== Packet Lookup ===" << std::endl;
        std::cout << "Packet: " << srcIP << ":" << srcPort << " -> " 
                  << dstIP << ":" << dstPort << " (proto: " << (int)protocol << ")" << std::endl;
        
        auto routes = lookup(packet);
        
        if (routes.empty()) {
            std::cout << "No matching routes found" << std::endl;
            return;
        }
        
        std::cout << "Matching routes (in priority order):" << std::endl;
        for (const auto& [policy, attrs] : routes) {
            std::cout << "  Priority " << policy.priority 
                      << " -> " << ipIntToString(attrs.nextHop)
                      << " (admin: " << (int)attrs.adminDistance << ")" << std::endl;
        }
        
        RouteAttributes* best = findBestRoute(packet);
        if (best) {
            std::cout << "Selected: " << ipIntToString(best->nextHop) << std::endl;
        }
    }
    
    // Add traffic engineering constraints
    void addTrafficEngineering(const std::string& prefix, uint8_t prefixLen,
                              uint32_t primaryNextHop, uint32_t backupNextHop,
                              uint32_t bandwidth, uint32_t delay) {
        PolicyRule policy;
        policy.dstPrefix = ipStringToInt(prefix);
        policy.dstPrefixLen = prefixLen;
        policy.priority = 50;  // Medium priority
        
        RouteAttributes primary;
        primary.nextHop = primaryNextHop;
        primary.localPref = 200;  // High preference
        primary.tag = bandwidth;  // Abuse tag field for bandwidth
        
        RouteAttributes backup;
        backup.nextHop = backupNextHop;
        backup.localPref = 100;   // Lower preference
        backup.tag = bandwidth / 2;  // Half bandwidth for backup
        
        addRoute(prefix, prefixLen, policy, primary);
        
        // Add backup route with lower priority
        policy.priority = 100;
        addRoute(prefix, prefixLen, policy, backup);
        
        std::cout << "Added TE route: " << prefix << "/" << (int)prefixLen
                  << " primary=" << ipIntToString(primaryNextHop)
                  << " backup=" << ipIntToString(backupNextHop) << std::endl;
    }
    
    // Multi-path routing support
    std::vector<RouteAttributes> getEqualCostPaths(const PacketInfo& packet) {
        auto routes = lookup(packet);
        std::vector<RouteAttributes> ecmp;
        
        if (routes.empty()) return ecmp;
        
        // Find all routes with same cost metrics
        uint32_t bestMed = routes[0].second.med;
        uint32_t bestLocalPref = routes[0].second.localPref;
        uint8_t bestAdminDist = routes[0].second.adminDistance;
        
        for (const auto& [policy, attrs] : routes) {
            if (attrs.adminDistance == bestAdminDist &&
                attrs.localPref == bestLocalPref &&
                attrs.med == bestMed) {
                ecmp.push_back(attrs);
            }
        }
        
        return ecmp;
    }

private:
    void insertRoute(PolicyRadixNode* node, uint32_t prefix, uint8_t prefixLen, 
                    uint8_t currentBit, const PolicyRule& policy, 
                    const RouteAttributes& attrs) {
        if (currentBit == prefixLen) {
            node->prefix = prefix;
            node->prefixLen = prefixLen;
            node->isValid = true;
            node->routes.push_back({policy, attrs});
            return;
        }
        
        bool bit = (prefix >> (31 - currentBit)) & 1;
        
        if (bit == 0) {
            if (!node->left) {
                node->left = std::make_unique<PolicyRadixNode>();
            }
            insertRoute(node->left.get(), prefix, prefixLen, currentBit + 1, policy, attrs);
        } else {
            if (!node->right) {
                node->right = std::make_unique<PolicyRadixNode>();
            }
            insertRoute(node->right.get(), prefix, prefixLen, currentBit + 1, policy, attrs);
        }
    }
    
    void findMatchingRoutes(PolicyRadixNode* node, uint32_t ip, uint8_t currentBit, 
                           uint8_t maxBits, 
                           std::vector<std::pair<PolicyRule, RouteAttributes>>& candidates) {
        if (!node || currentBit > maxBits) return;
        
        if (node->isValid) {
            // Check if this prefix matches the IP
            uint32_t mask = (node->prefixLen == 0) ? 0 : (0xFFFFFFFF << (32 - node->prefixLen));
            if ((ip & mask) == (node->prefix & mask)) {
                candidates.insert(candidates.end(), node->routes.begin(), node->routes.end());
            }
        }
        
        if (currentBit < maxBits) {
            bool bit = (ip >> (31 - currentBit)) & 1;
            
            if (bit == 0 && node->left) {
                findMatchingRoutes(node->left.get(), ip, currentBit + 1, maxBits, candidates);
            } else if (bit == 1 && node->right) {
                findMatchingRoutes(node->right.get(), ip, currentBit + 1, maxBits, candidates);
            }
        }
    }
    
    bool matchesPolicy(const PacketInfo& packet, const PolicyRule& policy) {
        // Source IP matching
        if (policy.srcPrefixLen > 0) {
            uint32_t srcMask = 0xFFFFFFFF << (32 - policy.srcPrefixLen);
            if ((packet.srcIP & srcMask) != (policy.srcPrefix & srcMask)) {
                return false;
            }
        }
        
        // Port matching
        if (policy.srcPort != 0 && packet.srcPort != policy.srcPort) {
            return false;
        }
        if (policy.dstPort != 0 && packet.dstPort != policy.dstPort) {
            return false;
        }
        
        // Protocol matching
        if (policy.protocol != 0 && packet.protocol != policy.protocol) {
            return false;
        }
        
        // TOS matching
        if (policy.tos != 0 && packet.tos != policy.tos) {
            return false;
        }
        
        return true;
    }
    
    void displayRoutesHelper(PolicyRadixNode* node, uint32_t prefix, uint8_t depth) {
        if (!node) return;
        
        if (node->isValid) {
            std::string prefixStr = ipIntToString(node->prefix) + "/" + std::to_string(node->prefixLen);
            
            for (const auto& [policy, attrs] : node->routes) {
                std::cout << std::left << std::setw(18) << prefixStr
                          << std::setw(15) << ipIntToString(attrs.nextHop)
                          << std::setw(8) << policy.priority
                          << std::setw(8) << (int)attrs.adminDistance
                          << std::setw(8) << attrs.localPref
                          << std::setw(8) << attrs.med << std::endl;
            }
        }
        
        if (node->left) {
            displayRoutesHelper(node->left.get(), prefix, depth + 1);
        }
        if (node->right) {
            displayRoutesHelper(node->right.get(), prefix | (1 << (31 - depth)), depth + 1);
        }
    }
};

