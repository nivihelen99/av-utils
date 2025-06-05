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
    uint8_t dscp;           // DSCP value to be applied by this route
    
    RouteAttributes() : nextHop(0), asPath(0), med(0), localPref(100), 
                       tag(0), protocol(0), adminDistance(1), isActive(true), dscp(0) {} // Default DSCP 0
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
        // hash_combine(seed, packet.flowLabel); // Optional: include FlowLabel in hash
        return seed;
    }

public:
    PolicyRoutingTree() : root(std::make_unique<PolicyRadixNode>()) {}
    
    // Convert IP string to uint32_t
    uint32_t ipStringToInt(const std::string& ip) const { // Added const
        struct in_addr addr;
        if (inet_aton(ip.c_str(), &addr) == 0) {
            throw std::runtime_error("Invalid IP address string: " + ip);
        }
        return ntohl(addr.s_addr);
    }
    
    // Convert uint32_t to IP string
    std::string ipIntToString(uint32_t ip) const { // Added const
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
        //           << " (priority: " << policy.priority << ")" << std::endl; // Removed cout
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
        std::cout << "\n=== Policy-Based Routing Table ===" << std::endl;
        std::cout << std::left
                  << std::setw(18) << "Route Prefix"
                  << std::setw(15) << "Next Hop" 
                  << std::setw(12) << "PolicyPrio"
                  << std::setw(10) << "AdminDist"
                  << std::setw(10) << "LocalPref"
                  << std::setw(8) << "MED"
                  << std::setw(9) << "SetDSCP" // Added SetDSCP column
                  << " Policy Details" << std::endl;
        std::cout << std::string(110, '-') << std::endl; // Adjusted width
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
            std::cerr << "Error creating packet: " << e.what() << std::endl;
            return;
        }
        packet.srcPort = srcPort;
        packet.dstPort = dstPort;
        packet.protocol = protocol;
        packet.tos = tos;
        packet.flowLabel = flowLabel;
        
        std::cout << "\n=== Packet Lookup Simulation ===" << std::endl;
        std::cout << "Packet: SrcIP=" << srcIPStr << ", DstIP=" << dstIPStr
                  << ", SrcPort=" << srcPort << ", DstPort=" << dstPort
                  << ", Proto=" << (int)protocol << ", ToS=0x" << std::hex << (int)tos << std::dec
                  << ", FlowLabel=" << flowLabel << std::endl;
        
        auto selectedRouteOpt = selectEcmpPathUsingFlowHash(packet); // Using new ECMP selection
        
        if (!selectedRouteOpt) {
            std::cout << "  No matching route found." << std::endl;
            return;
        }

        const auto& selectedRoute = *selectedRouteOpt;
        std::cout << "  Selected Next Hop: " << ipIntToString(selectedRoute.nextHop)
                  << " (Admin: " << (int)selectedRoute.adminDistance
                  << ", LP: " << selectedRoute.localPref
                  << ", MED: " << selectedRoute.med
                  << ", Tag: " << selectedRoute.tag
                  << ")" << std::endl;
        std::cout << "  Applying DSCP: 0x" << std::hex << (int)selectedRoute.dscp << std::dec
                  << " (Value: " << (int)selectedRoute.dscp << ")" << std::endl; // Display applied DSCP

         auto ecmpCandidates = getEqualCostPaths(packet);
         if (ecmpCandidates.size() > 1) {
             std::cout << "  ECMP candidates considered for this flow (" << ecmpCandidates.size() << "):" << std::endl;
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
                std::cout << std::endl;
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
        // if (policy.flowLabel != 0 && packet.flowLabel != policy.flowLabel) return false; // Optional
        
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
                          << std::setw(8) << attrs.med
                          << std::setw(9) << ("0x" + [](uint8_t val){ std::stringstream ss; ss << std::hex << (int)val; return ss.str(); }(attrs.dscp)); // Display SetDSCP

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
                policyDetailsStr += "]";
                if (policyDetailsStr.length() > 2) { // Contains more than "[]"
                     std::cout << policyDetailsStr;
                }
                std::cout << std::endl;
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

