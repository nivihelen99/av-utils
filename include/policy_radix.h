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
#include <functional> // For std::hash
#include <optional>   // For std::optional
#include <iomanip>    // For std::setw
#include <stdexcept>  // For std::runtime_error

// Forward declaration (if needed by types below, but PolicyRoutingTree itself is now defined earlier)
// class PolicyRoutingTree;

// Moved definitions to the top

// Route attributes for policy-based routing
struct RouteAttributes {
    uint32_t nextHop;
    std::vector<uint32_t> asPath; // Changed from uint16_t to std::vector<uint32_t>
    uint32_t med;           // Multi-Exit Discriminator
    uint32_t localPref;     // Local preference
    uint16_t tag;           // Route tag
    uint8_t protocol;       // Routing protocol (OSPF, BGP, etc.)
    uint8_t adminDistance;  // Administrative distance
    bool isActive;
    uint8_t dscp;           // DSCP value to be applied by this route
    uint64_t rateLimitBps;  // Rate limit in bits per second
    uint64_t burstSizeBytes; // Burst size in bytes
    
    RouteAttributes() : nextHop(0), asPath(), med(0), localPref(100),
                       tag(0), protocol(0), adminDistance(1), isActive(true), dscp(0),
                       rateLimitBps(0), burstSizeBytes(0) {}
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
    // std::unordered_map<uint32_t, std::string> routeTable;  // Removed, not used per new design focus

    // Enhanced hash function for PacketInfo
    size_t generateFlowHash(const PacketInfo& packet) const {
        size_t seed = 0;
        auto hash_combine = [&](size_t& current_seed, auto val) {
            current_seed ^= std::hash<decltype(val)>{}(val) + 0x9e3779b9 + (current_seed << 6) + (current_seed >> 2);
        };
        hash_combine(seed, packet.srcIP);
        hash_combine(seed, packet.dstIP);
        hash_combine(seed, packet.srcPort);
        hash_combine(seed, packet.dstPort);
        hash_combine(seed, packet.protocol);
        // hash_combine(seed, packet.tos); // Optional: include ToS in hash
        hash_combine(seed, packet.flowLabel); // Optional: include FlowLabel in hash
        return seed;
    }

public:
    PolicyRoutingTree() : root(std::make_unique<PolicyRadixNode>()) {}
    
    // Convert IP string to uint32_t
    static uint32_t ipStringToInt(const std::string& ip) { // Made static
        struct in_addr addr;
        if (inet_aton(ip.c_str(), &addr) == 0) {
            throw std::runtime_error("Invalid IP address string: " + ip);
        }
        return ntohl(addr.s_addr);
    }
    
    // Convert uint32_t to IP string
    static std::string ipIntToString(uint32_t ip) { // Made static
        struct in_addr addr;
        addr.s_addr = htonl(ip);
        const char* ip_str = inet_ntoa(addr);
        if (ip_str == nullptr) {
            throw std::runtime_error("Failed to convert IP integer to string.");
        }
        return std::string(ip_str);
    }
    
    // Add a policy-based route
    void addRoute(const std::string& prefixStr, uint8_t prefixLen,
                  PolicyRule policy, const RouteAttributes& attrs) { // Policy passed by value
        uint32_t prefixInt = ipStringToInt(prefixStr);
        
        uint32_t mask = (prefixLen == 0) ? 0 : (0xFFFFFFFF << (32 - prefixLen));
        prefixInt &= mask; // Ensure prefix is clean
        
        // insertRoute will handle associating policy.dstPrefix if not set
        insertRoute(root.get(), prefixInt, prefixLen, 0, policy, attrs);
        
        // std::cout << "Added policy route: " << prefix << "/" << (int)prefixLen
        //           << " -> " << ipIntToString(attrs.nextHop)
        //           << " (priority: " << policy.priority << ")" << '\n'; // Removed cout
    }
    
    // Lookup with policy-based routing (refined for LPM and policy application)
    std::vector<std::pair<PolicyRule, RouteAttributes>>
    lookup(const PacketInfo& packet) const { // Made const
        PolicyRadixNode* currentNode = root.get();
        PolicyRadixNode* bestMatchNode = nullptr;

        // Check root for 0.0.0.0/0 default route if it's valid
        if (root->isValid && root->prefixLen == 0) {
            bestMatchNode = root.get();
        }

        for (uint8_t depth = 0; depth < 32 && currentNode != nullptr; ++depth) {
            bool bit = (packet.dstIP >> (31 - depth)) & 1;
            PolicyRadixNode* nextNode = bit ? currentNode->right.get() : currentNode->left.get();

            if (nextNode == nullptr) break; // Path ends
            currentNode = nextNode;

            if (currentNode->isValid) {
                // Check if this node's prefix matches the packet's destination IP
                uint32_t mask = (currentNode->prefixLen == 0) ? 0 : (0xFFFFFFFF << (32 - currentNode->prefixLen));
                if ((packet.dstIP & mask) == (currentNode->prefix & mask)) {
                    // If this node's prefix is more specific, it becomes the new bestMatchNode
                    if (bestMatchNode == nullptr || currentNode->prefixLen > bestMatchNode->prefixLen) {
                        bestMatchNode = currentNode;
                    }
                }
            }
        }
        
        std::vector<std::pair<PolicyRule, RouteAttributes>> lpmRouteCandidates;
        if (bestMatchNode) {
            for (const auto& route_pair : bestMatchNode->routes) {
                lpmRouteCandidates.push_back(route_pair);
            }
        }

        std::vector<std::pair<PolicyRule, RouteAttributes>> validRoutes;
        if (!lpmRouteCandidates.empty()) {
            for (const auto& route_pair : lpmRouteCandidates) {
                if (matchesPolicy(packet, route_pair.first)) {
                    validRoutes.push_back(route_pair);
                }
            }
        }

        std::sort(validRoutes.begin(), validRoutes.end(),
            [](const auto& a, const auto& b) {
                if (a.first.priority != b.first.priority) {
                    return a.first.priority < b.first.priority;
                }
                const auto& attrsA = a.second;
                const auto& attrsB = b.second;
                if (attrsA.adminDistance != attrsB.adminDistance) {
                    return attrsA.adminDistance < attrsB.adminDistance;
                }
                if (attrsA.localPref != attrsB.localPref) {
                    return attrsA.localPref > attrsB.localPref;
                }
                return attrsA.med < attrsB.med;
            });
        return validRoutes;
    }
    
    RouteAttributes* findBestRoute(const PacketInfo& packet) { // Not const due to static thread_local
        auto routes = lookup(packet); // lookup is const
        if (routes.empty()) {
            return nullptr;
        }
        // The lookup method already sorts routes correctly. The first one is the best.
        static thread_local RouteAttributes bestRouteCopy;
        bestRouteCopy = routes[0].second;
        return &bestRouteCopy;
    }

    void displayRoutes() const { // Made const
        std::cout << "\n=== Policy-Based Routing Table ===" << '\n';
        std::cout << std::left
                  << std::setw(18) << "Route Prefix"
                  << std::setw(15) << "Next Hop" 
                  << std::setw(12) << "PolicyPrio"
                  << std::setw(10) << "AdminDist"
                  << std::setw(10) << "LocalPref"
                  << std::setw(8) << "MED"
                  << std::setw(15) << "AS Path"
                  << std::setw(12) << "RateLimit"   // Added RateLimit column
                  << std::setw(12) << "BurstSize"   // Added BurstSize column
                  << std::setw(9) << "SetDSCP"
                  << " Policy Details" << '\n';
        std::cout << std::string(160, '-') << '\n'; // Adjusted width +24 for new columns + some spacing
        displayRoutesHelper(root.get(), 0, 0, ""); // Initial call with empty path string
    }
    
    void simulatePacket(const std::string& srcIPStr, const std::string& dstIPStr,
                       uint16_t srcPort, uint16_t dstPort, uint8_t protocol,
                       uint8_t tos = 0, uint32_t flowLabel = 0) { // Added tos and flowLabel
        PacketInfo packet;
        try {
            packet.srcIP = ipStringToInt(srcIPStr);
            packet.dstIP = ipStringToInt(dstIPStr);
        } catch (const std::runtime_error& e) {
            std::cerr << "Error creating packet: " << e.what() << '\n';
            return;
        }
        packet.srcPort = srcPort;
        packet.dstPort = dstPort;
        packet.protocol = protocol;
        packet.tos = tos;
        packet.flowLabel = flowLabel;
        
        std::cout << "\n=== Packet Lookup Simulation ===" << '\n';
        std::cout << "Packet: SrcIP=" << srcIPStr << ", DstIP=" << dstIPStr
                  << ", SrcPort=" << srcPort << ", DstPort=" << dstPort
                  << ", Proto=" << (int)protocol << ", ToS=0x" << std::hex << (int)tos << std::dec
                  << ", FlowLabel=" << flowLabel << '\n';
        
        auto selectedRouteOpt = selectEcmpPathUsingFlowHash(packet); // Using new ECMP selection
        
        if (!selectedRouteOpt) {
            std::cout << "  No matching route found." << '\n';
            return;
        }

        const auto& selectedRoute = *selectedRouteOpt;
        std::cout << "  Selected Next Hop: " << ipIntToString(selectedRoute.nextHop)
                  << " (Admin: " << (int)selectedRoute.adminDistance
                  << ", LP: " << selectedRoute.localPref
                  << ", MED: " << selectedRoute.med
                  << ", Tag: " << selectedRoute.tag
                  << ")" << '\n';
        std::cout << "  Applying DSCP: 0x" << std::hex << (int)selectedRoute.dscp << std::dec
                  << " (Value: " << (int)selectedRoute.dscp << ")" << '\n';
        // Display Rate Limiting Information
        std::cout << "  Rate Limit: " << selectedRoute.rateLimitBps << " bps, Burst: " << selectedRoute.burstSizeBytes << " bytes" << '\n';

         auto ecmpCandidates = getEqualCostPaths(packet);
         if (ecmpCandidates.size() > 1) {
             std::cout << "  ECMP candidates considered for this flow (" << ecmpCandidates.size() << "):" << '\n';
             size_t flowHashIndex = generateFlowHash(packet) % ecmpCandidates.size();
             for(size_t i = 0; i < ecmpCandidates.size(); ++i) {
                 const auto& path = ecmpCandidates[i];
                 std::cout << "    -> " << ipIntToString(path.nextHop)
                           << " (Admin: " << (int)path.adminDistance
                           << ", LP: " << path.localPref << ", MED: " << path.med
                           << ", DSCP: 0x" << std::hex << (int)path.dscp << std::dec << ")"; // Added DSCP to ECMP list
                if (i == flowHashIndex) {
                    std::cout << " [*SELECTED* by flow hash]";
                }
                std::cout << '\n';
             }
         }
    }
    
    void addTrafficEngineering(const std::string& prefixStr, uint8_t prefixLen,
                              uint32_t primaryNextHopIp, uint32_t backupNextHopIp,
                              uint32_t bandwidth, uint32_t /*delay unused*/) { // delay marked unused
        PolicyRule commonPolicy;

        RouteAttributes primaryAttrs;
        primaryAttrs.nextHop = primaryNextHopIp;
        primaryAttrs.localPref = 200;
        primaryAttrs.tag = bandwidth;
        primaryAttrs.dscp = 0x12; // AF21 (DSCP 18) for primary TE path
        PolicyRule primaryPolicy = commonPolicy;
        primaryPolicy.priority = 50;
        addRoute(prefixStr, prefixLen, primaryPolicy, primaryAttrs);

        RouteAttributes backupAttrs;
        backupAttrs.nextHop = backupNextHopIp;
        backupAttrs.localPref = 100;
        backupAttrs.tag = bandwidth / 2;
        backupAttrs.dscp = 0x00; // Best effort for backup
        PolicyRule backupPolicy = commonPolicy;
        backupPolicy.priority = 100;
        addRoute(prefixStr, prefixLen, backupPolicy, backupAttrs);
    }
    
    std::vector<RouteAttributes> getEqualCostPaths(const PacketInfo& packet) const { // Made const
        auto sortedValidRoutes = lookup(packet); // lookup is already sorted
        std::vector<RouteAttributes> ecmp;

        if (sortedValidRoutes.empty()) return ecmp;

        const auto& bestRoutePairCriteria = sortedValidRoutes[0];

        for (const auto& currentRoutePair : sortedValidRoutes) {
            if (currentRoutePair.first.priority == bestRoutePairCriteria.first.priority &&
                currentRoutePair.second.adminDistance == bestRoutePairCriteria.second.adminDistance &&
                currentRoutePair.second.localPref == bestRoutePairCriteria.second.localPref &&
                currentRoutePair.second.med == bestRoutePairCriteria.second.med &&
                currentRoutePair.second.isActive) {
                ecmp.push_back(currentRoutePair.second);
            } else {
                break;
            }
        }
        return ecmp;
    }

    // New method as per subtask
    std::optional<RouteAttributes> selectEcmpPathUsingFlowHash(const PacketInfo& packet) const { // Made const
        std::vector<RouteAttributes> ecmpCandidates = getEqualCostPaths(packet);

        if (ecmpCandidates.empty()) return std::nullopt;
        if (ecmpCandidates.size() == 1) return ecmpCandidates[0];

        size_t hashVal = generateFlowHash(packet);
        int selectedIndex = hashVal % ecmpCandidates.size();
        return ecmpCandidates[selectedIndex];
    }

private:
    // insertRoute: PolicyRule passed by value to allow modification for dstPrefix/dstPrefixLen
    void insertRoute(PolicyRadixNode* node, uint32_t targetPrefix, uint8_t targetPrefixLen,
                    uint8_t currentBitDepth, PolicyRule policy, // Passed by value
                    const RouteAttributes& attrs) {
        if (currentBitDepth == targetPrefixLen) {
            node->prefix = targetPrefix;
            node->prefixLen = targetPrefixLen;
            node->isValid = true;
            // If policy's destination prefix is not explicitly set, use the route's prefix.
            if (policy.dstPrefix == 0 && policy.dstPrefixLen == 0 && targetPrefixLen > 0) {
                policy.dstPrefix = targetPrefix;
                policy.dstPrefixLen = targetPrefixLen;
            }
            node->routes.push_back({policy, attrs});
            return;
        }
        
        bool bit = (targetPrefix >> (31 - currentBitDepth)) & 1;
        
        if (bit == 0) { // Go left
            if (!node->left) {
                node->left = std::make_unique<PolicyRadixNode>();
            }
            insertRoute(node->left.get(), targetPrefix, targetPrefixLen, currentBitDepth + 1, policy, attrs);
        } else { // Go right
            if (!node->right) {
                node->right = std::make_unique<PolicyRadixNode>();
            }
            insertRoute(node->right.get(), targetPrefix, targetPrefixLen, currentBitDepth + 1, policy, attrs);
        }
    }
    
    // Removed findMatchingRoutes as its logic is incorporated into the new lookup method.
    
    bool matchesPolicy(const PacketInfo& packet, const PolicyRule& policy) const { // Made const
        // Source Prefix Check
        if (policy.srcPrefixLen > 0) {
            uint32_t srcPolicyMask = (policy.srcPrefixLen == 32) ? 0xFFFFFFFF : ((1U << policy.srcPrefixLen) -1) << (32 - policy.srcPrefixLen) ;
            if (policy.srcPrefixLen == 0) srcPolicyMask = 0;
            if ((packet.srcIP & srcPolicyMask) != (policy.srcPrefix & srcPolicyMask)) {
                return false;
            }
        }

        // Destination Prefix Check (specific to policy, not route prefix)
        if (policy.dstPrefixLen > 0) {
            uint32_t dstPolicyMask = (policy.dstPrefixLen == 32) ? 0xFFFFFFFF : ((1U << policy.dstPrefixLen)-1) << (32-policy.dstPrefixLen);
            if(policy.dstPrefixLen == 0) dstPolicyMask = 0;
             if ((packet.dstIP & dstPolicyMask) != (policy.dstPrefix & dstPolicyMask)) {
                return false;
            }
        }
        
        if (policy.srcPort != 0 && packet.srcPort != policy.srcPort) return false;
        if (policy.dstPort != 0 && packet.dstPort != policy.dstPort) return false;
        if (policy.protocol != 0 && packet.protocol != policy.protocol) return false;
        if (policy.tos != 0 && packet.tos != policy.tos) return false;
        if (policy.flowLabel != 0 && packet.flowLabel != policy.flowLabel) return false; // Optional
        
        return true;
    }
    
    void displayRoutesHelper(PolicyRadixNode* node, uint32_t /*currentPrefixVal*/, uint8_t /*depth*/, const std::string& pathStr) const { // Made const, params updated
        if (!node) return;
        
        if (node->isValid) {
            std::string prefixStr = ipIntToString(node->prefix) + "/" + std::to_string(node->prefixLen);
            for (const auto& [policy, attrs] : node->routes) {
                std::cout << std::left
                          << std::setw(18) << prefixStr
                          << std::setw(15) << ipIntToString(attrs.nextHop)
                          << std::setw(12) << policy.priority
                          << std::setw(10) << (int)attrs.adminDistance
                          << std::setw(10) << attrs.localPref
                           << std::setw(8) << attrs.med;

                // AS Path printing
                std::string asPathStr;
                if (attrs.asPath.empty()) {
                    asPathStr = "-";
                } else {
                    for (size_t i = 0; i < attrs.asPath.size(); ++i) {
                        asPathStr += std::to_string(attrs.asPath[i]) + (i < attrs.asPath.size() - 1 ? " " : "");
                    }
                }
                std::cout << std::setw(15) << asPathStr;

                // RateLimit and BurstSize printing
                std::cout << std::setw(12) << (attrs.rateLimitBps == 0 ? "-" : std::to_string(attrs.rateLimitBps));
                std::cout << std::setw(12) << (attrs.burstSizeBytes == 0 ? "-" : std::to_string(attrs.burstSizeBytes));

                std::cout << std::setw(9) << ("0x" + [](uint8_t val){ std::stringstream ss; ss << std::hex << (int)val; return ss.str(); }(attrs.dscp));

                std::string policyDetailsStr = " [";
                bool firstDetail = true;
                auto append_detail = [&](const std::string& detail) {
                    if (!firstDetail) policyDetailsStr += ", ";
                    policyDetailsStr += detail;
                    firstDetail = false;
                };

                if (policy.srcPrefixLen > 0) {
                    append_detail("SrcPfx: " + ipIntToString(policy.srcPrefix) + "/" + std::to_string(policy.srcPrefixLen));
                }
                // Display policy's dstPrefix only if it's different from the route's prefix
                if (policy.dstPrefixLen > 0 && (policy.dstPrefix != node->prefix || policy.dstPrefixLen != node->prefixLen)) {
                    append_detail("DstPfx: " + ipIntToString(policy.dstPrefix) + "/" + std::to_string(policy.dstPrefixLen));
                }
                if (policy.srcPort > 0) append_detail("SrcPort: " + std::to_string(policy.srcPort));
                if (policy.dstPort > 0) append_detail("DstPort: " + std::to_string(policy.dstPort));
                if (policy.protocol > 0) append_detail("Proto: " + std::to_string(policy.protocol));
                if (policy.tos > 0) {
                     std::stringstream ss;
                     ss << "ToS: 0x" << std::hex << (int)policy.tos;
                     append_detail(ss.str());
                }
                if (policy.flowLabel != 0) {
                    append_detail("FlowLabel: " + std::to_string(policy.flowLabel));
                }
                policyDetailsStr += "]";
                if (policyDetailsStr.length() > 2) { // Contains more than "[]"
                     std::cout << policyDetailsStr;
                }
                std::cout << '\n';
            }
        }
        
        if (node->left) {
            displayRoutesHelper(node->left.get(), 0, 0, pathStr + "0"); // Params simplified
        }
        if (node->right) {
            displayRoutesHelper(node->right.get(), 0, 0, pathStr + "1"); // Params simplified
        }
    }
};

class VrfRoutingTableManager {
private:
    std::unordered_map<uint32_t, std::unique_ptr<PolicyRoutingTree>> vrfTables;

    PolicyRoutingTree* getVrfTable(uint32_t vrfId, bool createIfNotFound = true) {
        auto it = vrfTables.find(vrfId);
        if (it != vrfTables.end()) {
            return it->second.get();
        }

        if (createIfNotFound) {
            auto newTable = std::make_unique<PolicyRoutingTree>();
            PolicyRoutingTree* newTablePtr = newTable.get();
            vrfTables[vrfId] = std::move(newTable);
            return newTablePtr;
        }
        return nullptr;
    }

    // Const version for const methods
    const PolicyRoutingTree* getVrfTable(uint32_t vrfId, bool createIfNotFound = false) const {
        auto it = vrfTables.find(vrfId);
        if (it != vrfTables.end()) {
            return it->second.get();
        }
        // Cannot create if not found in a const method if creation modifies the map
        return nullptr;
    }

public:
    VrfRoutingTableManager() = default;

    void addRoute(uint32_t vrfId, const std::string& prefixStr, uint8_t prefixLen,
                  PolicyRule policy, const RouteAttributes& attrs) {
        PolicyRoutingTree* table = getVrfTable(vrfId, true);
        if (table) {
            table->addRoute(prefixStr, prefixLen, policy, attrs);
        }
    }

    std::optional<RouteAttributes> selectEcmpPathUsingFlowHash(uint32_t vrfId, const PacketInfo& packet) {
        PolicyRoutingTree* table = getVrfTable(vrfId, false);
        if (table) {
            return table->selectEcmpPathUsingFlowHash(packet);
        }
        return std::nullopt;
    }

    void displayRoutes(uint32_t vrfId) const {
        const PolicyRoutingTree* table = getVrfTable(vrfId, false);
        if (table) {
            std::cout << "\n--- Routing Table for VRF ID: " << vrfId << " ---" << '\n';
            table->displayRoutes();
        } else {
            std::cout << "\n--- VRF ID: " << vrfId << " not found or has no routes ---" << '\n';
        }
    }

    void displayAllRoutes() const {
        if (vrfTables.empty()) {
            std::cout << "\n--- No VRFs configured ---" << '\n';
            return;
        }
        for (const auto& pair : vrfTables) {
            std::cout << "\n--- Routing Table for VRF ID: " << pair.first << " ---" << '\n';
            pair.second->displayRoutes();
        }
    }

    void simulatePacket(uint32_t vrfId, const std::string& srcIPStr, const std::string& dstIPStr,
                       uint16_t srcPort, uint16_t dstPort, uint8_t protocol,
                       uint8_t tos = 0, uint32_t flowLabel = 0) {
        PolicyRoutingTree* table = getVrfTable(vrfId, false); // Don't create VRF on simulation if it doesn't exist
        if (table) {
            std::cout << "\n=== Simulating Packet in VRF ID: " << vrfId << " ===" << '\n';
            table->simulatePacket(srcIPStr, dstIPStr, srcPort, dstPort, protocol, tos, flowLabel);
        } else {
            std::cout << "\n=== VRF ID: " << vrfId << " not found for packet simulation. Packet dropped. ===" << '\n';
            // Optionally, print packet details here too
            std::cout << "Packet Details: SrcIP=" << srcIPStr << ", DstIP=" << dstIPStr
                      << ", SrcPort=" << srcPort << ", DstPort=" << dstPort
                      << ", Proto=" << (int)protocol << ", ToS=0x" << std::hex << (int)tos << std::dec
                      << ", FlowLabel=" << flowLabel << '\n';
        }
    }
};

// This content is now moved to the top of the file.
// Ensure VrfRoutingTableManager is defined AFTER PolicyRoutingTree and other necessary types.
