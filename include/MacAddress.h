#pragma once

#include <array>
#include <functional>
#include <iomanip>
#include <iostream>
#include <random>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

class MacAddress
{
   public:
   // Constants
   static constexpr size_t MAC_LENGTH = 6;
   static constexpr char DEFAULT_SEPARATOR = ':';

   using MacArray = std::array<uint8_t, MAC_LENGTH>;

   // Constructors
   MacAddress()
       : octets_ {0, 0, 0, 0, 0, 0}
   {
   }

   explicit MacAddress(const MacArray& octets)
       : octets_(octets)
   {
   }

   explicit MacAddress(std::string_view mac_str)
   {
      if (!parse(mac_str))
      {
         throw std::invalid_argument("Invalid MAC address format: " + std::string(mac_str));
      }
   }

   MacAddress(uint8_t o1, uint8_t o2, uint8_t o3, uint8_t o4, uint8_t o5, uint8_t o6)
       : octets_ {o1, o2, o3, o4, o5, o6}
   {
   }

   MacAddress(std::initializer_list<uint8_t> init)
   {
      if (init.size() != 6)
      {
         throw std::invalid_argument("MAC address must have exactly 6 octets");
      }
      std::copy(init.begin(), init.end(), octets_.begin());
   }

   // Copy constructor and assignment operator (defaulted)
   MacAddress(const MacAddress&) = default;
   MacAddress& operator=(const MacAddress&) = default;

   // Move constructor and assignment operator (defaulted)
   MacAddress(MacAddress&&) = default;
   MacAddress& operator=(MacAddress&&) = default;

   // Destructor
   ~MacAddress() = default;

   // Add this method
   const unsigned char* data() const
   {
      return octets_.data();
   }

   // Also useful - non-const version if you need to modify
   unsigned char* data()
   {
      return octets_.data();
   }

   // Factory methods
   static MacAddress fromString(std::string_view mac_str)
   {
      return MacAddress(mac_str);
   }

   static MacAddress fromBytes(const uint8_t* bytes)
   {
      if (!bytes)
      {
         throw std::invalid_argument("Null pointer provided");
      }
      MacArray octets;
      std::copy(bytes, bytes + MAC_LENGTH, octets.begin());
      return MacAddress(octets);
   }

   static MacAddress random()
   {
      static std::random_device rd;
      static std::mt19937 gen(rd());
      static std::uniform_int_distribution<uint8_t> dis(0, 255);

      MacArray octets;
      for (auto& octet : octets)
      {
         octet = dis(gen);
      }

      // Set locally administered bit and clear multicast bit for a valid unicast MAC
      octets[0] = (octets[0] & 0xFE) | 0x02;

      return MacAddress(octets);
   }

   static MacAddress broadcast()
   {
      MacArray broadcast_addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      return MacAddress(broadcast_addr);
   }

   static MacAddress zero()
   {
      return MacAddress();
   }

   // Parsing methods
   bool parse(std::string_view mac_str)
   {
      // Remove whitespace
      std::string clean_mac;
      for (char c : mac_str)
      {
         if (!std::isspace(c))
         {
            clean_mac += c;
         }
      }

      // Try different formats
        if (clean_mac.find(':') != std::string::npos) {
            return parseWithSeparator(clean_mac, ':');
        }
        if (clean_mac.find('-') != std::string::npos) {
            return parseWithSeparator(clean_mac, '-');
        }
        // Cisco format AABB.CCDD.EEFF (length 14) or AA.BB.CC.DD.EE.FF (length 17, less common for MACs)
        if (clean_mac.find('.') != std::string::npos) {
            return parseWithSeparator(clean_mac, '.');
        }
        // If no separators are found, assume it might be a 12-character hex string
        if (clean_mac.length() == 12 && clean_mac.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos) {
            return parseWithoutSeparator(clean_mac);
        }

        // If none of the above specific formats match, return false.
        return false;
   }

   // Accessors
   const MacArray& getOctets() const noexcept
   {
      return octets_;
   }

   uint8_t operator[](size_t index) const
   {
      if (index >= MAC_LENGTH)
      {
         throw std::out_of_range("Index out of range");
      }
      return octets_[index];
   }

   uint8_t& operator[](size_t index)
   {
      if (index >= MAC_LENGTH)
      {
         throw std::out_of_range("Index out of range");
      }
      return octets_[index];
   }

   uint8_t at(size_t index) const
   {
      if (index >= MAC_LENGTH)
      {
         throw std::out_of_range("Index out of range");
      }
      return octets_[index];
   }

   uint8_t& at(size_t index)
   {
      if (index >= MAC_LENGTH)
      {
         throw std::out_of_range("Index out of range");
      }
      return octets_[index];
   }

   unsigned char* get()
   {
      return &octets_[0];
   }
   const unsigned char* get() const
   {
      return &octets_[0];
   }

   // String conversion methods
   std::string toString(char separator = DEFAULT_SEPARATOR) const
   {
      std::ostringstream oss;
      oss << std::hex << std::uppercase << std::setfill('0');

      for (size_t i = 0; i < MAC_LENGTH; ++i)
      {
         if (i > 0)
            oss << separator;
         oss << std::setw(2) << static_cast<int>(octets_[i]);
      }

      return oss.str();
   }

   std::string toStringLower(char separator = DEFAULT_SEPARATOR) const
   {
      std::ostringstream oss;
      oss << std::hex << std::nouppercase << std::setfill('0');

      for (size_t i = 0; i < MAC_LENGTH; ++i)
      {
         if (i > 0)
            oss << separator;
         oss << std::setw(2) << static_cast<int>(octets_[i]);
      }

      return oss.str();
   }

   std::string toCiscoFormat() const
   {
      std::ostringstream oss;
      oss << std::hex << std::nouppercase << std::setfill('0');

      for (size_t i = 0; i < MAC_LENGTH; i += 2)
      {
         if (i > 0)
            oss << '.';
         oss << std::setw(2) << static_cast<int>(octets_[i]);
         oss << std::setw(2) << static_cast<int>(octets_[i + 1]);
      }

      return oss.str();
   }

   std::string toWindowsFormat() const
   {
      return toString('-');
   }

   std::string toUnixFormat() const
   {
      return toString(':');
   }

   // Utility methods
   bool isValid() const noexcept
   {
      return !isZero();
   }

   bool isZero() const noexcept
   {
      for (uint8_t octet : octets_)
      {
         if (octet != 0)
            return false;
      }
      return true;
   }

   bool isBroadcast() const noexcept
   {
      for (uint8_t octet : octets_)
      {
         if (octet != 0xFF)
            return false;
      }
      return true;
   }

   bool isMulticast() const noexcept
   {
      return (octets_[0] & 0x01) != 0;
   }

   bool isUnicast() const noexcept
   {
      return !isMulticast();
   }

   bool isLocallyAdministered() const noexcept
   {
      return (octets_[0] & 0x02) != 0;
   }

   bool isUniversallyAdministered() const noexcept
   {
      return !isLocallyAdministered();
   }

   // Get OUI (Organizationally Unique Identifier) - first 3 octets
   uint32_t getOUI() const noexcept
   {
      return (static_cast<uint32_t>(octets_[0]) << 16) | (static_cast<uint32_t>(octets_[1]) << 8) |
             static_cast<uint32_t>(octets_[2]);
   }

   // Get NIC (Network Interface Controller) specific part - last 3 octets
   uint32_t getNIC() const noexcept
   {
      return (static_cast<uint32_t>(octets_[3]) << 16) | (static_cast<uint32_t>(octets_[4]) << 8) |
             static_cast<uint32_t>(octets_[5]);
   }

   // Convert to 64-bit integer (with padding)
   uint64_t toUInt64() const noexcept
   {
      uint64_t result = 0;
      for (size_t i = 0; i < MAC_LENGTH; ++i)
      {
         result = (result << 8) | octets_[i];
      }
      return result;
   }

   // Create from 64-bit integer (uses lower 48 bits)
   static MacAddress fromUInt64(uint64_t value)
   {
      MacArray octets;
      for (int i = MAC_LENGTH - 1; i >= 0; --i)
      {
         octets[i] = static_cast<uint8_t>(value & 0xFF);
         value >>= 8;
      }
      return MacAddress(octets);
   }

   // Iterators
   auto begin() noexcept
   {
      return octets_.begin();
   }
   auto end() noexcept
   {
      return octets_.end();
   }
   auto begin() const noexcept
   {
      return octets_.begin();
   }
   auto end() const noexcept
   {
      return octets_.end();
   }
   auto cbegin() const noexcept
   {
      return octets_.cbegin();
   }
   auto cend() const noexcept
   {
      return octets_.cend();
   }

   // Comparison operators
   bool operator==(const MacAddress& other) const noexcept
   {
      return octets_ == other.octets_;
   }

   bool operator!=(const MacAddress& other) const noexcept
   {
      return !(*this == other);
   }

   bool operator<(const MacAddress& other) const noexcept
   {
      return octets_ < other.octets_;
   }

   bool operator<=(const MacAddress& other) const noexcept
   {
      return octets_ <= other.octets_;
   }

   bool operator>(const MacAddress& other) const noexcept
   {
      return octets_ > other.octets_;
   }

   bool operator>=(const MacAddress& other) const noexcept
   {
      return octets_ >= other.octets_;
   }

   // Stream operators
   friend std::ostream& operator<<(std::ostream& os, const MacAddress& mac)
   {
      return os << mac.toString();
   }

   friend std::istream& operator>>(std::istream& is, MacAddress& mac)
   {
      std::string mac_str;
      is >> mac_str;
      if (!mac.parse(mac_str))
      {
         is.setstate(std::ios::failbit);
      }
      return is;
   }

   private:
   MacArray octets_;

   bool parseWithSeparator(const std::string& mac_str, char separator)
   {
      // Using static const std::regex for performance
      static const std::regex colon_pattern("^([0-9A-Fa-f]{1,2}):([0-9A-Fa-f]{1,2}):([0-9A-Fa-f]{1,2}):([0-9A-Fa-f]{1,2}):([0-9A-Fa-f]{1,2}):([0-9A-Fa-f]{1,2})$");
      static const std::regex hyphen_pattern("^([0-9A-Fa-f]{1,2})-([0-9A-Fa-f]{1,2})-([0-9A-Fa-f]{1,2})-([0-9A-Fa-f]{1,2})-([0-9A-Fa-f]{1,2})-([0-9A-Fa-f]{1,2})$");
      static const std::regex cisco_pattern("^([0-9A-Fa-f]{4})\\.([0-9A-Fa-f]{4})\\.([0-9A-Fa-f]{4})$");
      // Generic dot pattern for AA.BB.CC.DD.EE.FF style, less common but possible
      static const std::regex dot_pattern("^([0-9A-Fa-f]{1,2})\\.([0-9A-Fa-f]{1,2})\\.([0-9A-Fa-f]{1,2})\\.([0-9A-Fa-f]{1,2})\\.([0-9A-Fa-f]{1,2})\\.([0-9A-Fa-f]{1,2})$");

      std::smatch matches;
      const std::regex* current_pattern = nullptr;

      if (separator == ':')
      {
         current_pattern = &colon_pattern;
      }
      else if (separator == '-')
      {
         current_pattern = &hyphen_pattern;
      }
      else if (separator == '.')
      {
         // Try Cisco format first for '.'
         if (std::regex_match(mac_str, matches, cisco_pattern))
         {
            try
            {
               for (int i = 0; i < 3; ++i)
               {
                  std::string hex_pair = matches[i + 1].str();
                  octets_[i * 2] =
                     static_cast<uint8_t>(std::stoul(hex_pair.substr(0, 2), nullptr, 16));
                  octets_[i * 2 + 1] =
                     static_cast<uint8_t>(std::stoul(hex_pair.substr(2, 2), nullptr, 16));
               }
               return true;
            }
            catch (...)
            {
               // Error during conversion
               return false;
            }
         }
         // If Cisco didn't match, try generic dot pattern
         current_pattern = &dot_pattern;
      }
      else
      {
         // Should not happen if separator is one of ':', '-', '.'
         // but as a fallback or if new separators are added without static regexes:
         return false;
      }

      if (!current_pattern) return false; // Should be caught by above else

      if (std::regex_match(mac_str, matches, *current_pattern))
      {
         try
         {
            for (size_t i = 0; i < MAC_LENGTH; ++i)
            {
               octets_[i] = static_cast<uint8_t>(std::stoul(matches[i + 1].str(), nullptr, 16));
            }
            return true;
         }
         catch (...)
         {
            // Error during conversion
            return false;
         }
      }
      return false;
   }

   bool parseWithoutSeparator(const std::string& mac_str)
   {
      // Length check is already done in the calling `parse` method for this specific path.
      // However, keeping it here makes parseWithoutSeparator safe if called directly.
      if (mac_str.length() != 12)
      {
         return false;
      }

      // Regex check for hex characters is also partly done by find_first_not_of
      // in `parse` for this path. Static regex is still good for robustness.
      static const std::regex pattern("^[0-9A-Fa-f]{12}$");
      if (!std::regex_match(mac_str, pattern))
      {
         return false;
      }

      try
      {
         for (size_t i = 0; i < MAC_LENGTH; ++i)
         {
            std::string hex_pair = mac_str.substr(i * 2, 2);
            octets_[i] = static_cast<uint8_t>(std::stoul(hex_pair, nullptr, 16));
         }
         return true;
      }
      catch (...)
      {
         // Error during conversion
         return false;
      }
   }
};

// Hash function for use in std::unordered_map, std::unordered_set, etc.
namespace std
{
template <>
struct hash<MacAddress>
{
   std::size_t operator()(const MacAddress& mac) const noexcept
   {
      std::size_t result = 0;
      const auto& octets = mac.getOctets();

      for (size_t i = 0; i < MacAddress::MAC_LENGTH; ++i)
      {
         result ^= std::hash<uint8_t> {}(octets[i]) + 0x9e3779b9 + (result << 6) + (result >> 2);
      }

      return result;
   }
};
} // namespace std

