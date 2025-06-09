#pragma once

#include <iomanip>
#include <optional>
#include <regex>
#include <sstream>
#include <string>

#define MAC_ADDR_SZ 6   

struct MAC_ADR
{
  unsigned char macAdr[MAC_ADDR_SZ];                                                                                                                       
};

#define MAC_ADR_LEN sizeof(MAC_ADR)



enum class MacFormat
{
   COLON_SEPARATED, // AA:BB:CC:DD:EE:FF
   HYPHEN_SEPARATED, // AA-BB-CC-DD-EE-FF
   DOT_SEPARATED, // AA.BB.CC.DD.EE.FF
   DOTTED_QUAD, // AAAA.BBBB.CCCC
   NO_SEPARATOR // AABBCCDDEEFF
};

/**
 * Convert MAC_ADR struct to string representation
 * @param mac The MAC_ADR struct to convert
 * @param format The desired output format (default: colon-separated)
 * @param uppercase Whether to use uppercase hex digits (default: true)
 * @param add_0x Whether to prefix only the first octet with "0x" (default: false)
 * @return Formatted MAC address string
 */
std::string macToString(const MAC_ADR& mac,
                        MacFormat format = MacFormat::COLON_SEPARATED,
                        bool uppercase = true,
                        bool add_0x = false)
{
   std::stringstream ss;
   const char* hex_chars = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

   // Helper lambda to format a single byte
   auto formatByte = [&](unsigned char byte, bool add_prefix = false) -> std::string {
      std::string result;
      if (add_prefix)
         result += "0x";
      result += hex_chars[(byte >> 4) & 0x0F];
      result += hex_chars[byte & 0x0F];
      return result;
   };

   switch (format)
   {
      case MacFormat::COLON_SEPARATED:
         for (int i = 0; i < MAC_ADDR_SZ; ++i)
         {
            if (i > 0)
               ss << ":";
            ss << formatByte(mac.macAdr[i], add_0x && i == 0);
         }
         break;

      case MacFormat::HYPHEN_SEPARATED:
         for (int i = 0; i < MAC_ADDR_SZ; ++i)
         {
            if (i > 0)
               ss << "-";
            ss << formatByte(mac.macAdr[i], add_0x && i == 0);
         }
         break;

      case MacFormat::DOT_SEPARATED:
         for (int i = 0; i < MAC_ADDR_SZ; ++i)
         {
            if (i > 0)
               ss << ".";
            ss << formatByte(mac.macAdr[i], add_0x && i == 0);
         }
         break;

      case MacFormat::DOTTED_QUAD:
         // Format as AAAA.BBBB.CCCC (3 groups of 4 hex chars)
         for (int i = 0; i < 3; ++i)
         {
            if (i > 0)
               ss << ".";
            if (add_0x && i == 0)
               ss << "0x";
            // First byte of the pair
            ss << hex_chars[(mac.macAdr[i * 2] >> 4) & 0x0F];
            ss << hex_chars[mac.macAdr[i * 2] & 0x0F];
            // Second byte of the pair
            ss << hex_chars[(mac.macAdr[i * 2 + 1] >> 4) & 0x0F];
            ss << hex_chars[mac.macAdr[i * 2 + 1] & 0x0F];
         }
         break;

      case MacFormat::NO_SEPARATOR:
         if (add_0x)
            ss << "0x";
         for (int i = 0; i < MAC_ADDR_SZ; ++i)
         {
            ss << hex_chars[(mac.macAdr[i] >> 4) & 0x0F];
            ss << hex_chars[mac.macAdr[i] & 0x0F];
         }
         break;
   }

   return ss.str();
}

/**
 * Convenience functions for common formats
 */
std::string macToColonString(const MAC_ADR& mac, bool uppercase = true, bool add_0x = false)
{
   return macToString(mac, MacFormat::COLON_SEPARATED, uppercase, add_0x);
}

std::string macToHyphenString(const MAC_ADR& mac, bool uppercase = true, bool add_0x = false)
{
   return macToString(mac, MacFormat::HYPHEN_SEPARATED, uppercase, add_0x);
}

std::string macToDottedQuadString(const MAC_ADR& mac, bool uppercase = true, bool add_0x = false)
{
   return macToString(mac, MacFormat::DOTTED_QUAD, uppercase, add_0x);
}

std::string macToPlainString(const MAC_ADR& mac, bool uppercase = true, bool add_0x = false)
{
   return macToString(mac, MacFormat::NO_SEPARATOR, uppercase, add_0x);
}

/**
 * Helper function to determine if input MAC string is predominantly lowercase
 */
bool isInputLowercase(const std::string& mac_str)
{
   int lower_count = 0, upper_count = 0;
   for (char c : mac_str)
   {
      if (c >= 'a' && c <= 'f')
         lower_count++;
      else if (c >= 'A' && c <= 'F')
         upper_count++;
   }
   return lower_count >= upper_count;
}

/**
 * Format MAC address as string with specified separator and case
 * @param mac The MAC_ADR struct to format
 * @param separator Character to use as separator (':' or '-' or '.')
 * @param lowercase Whether to use lowercase hex digits
 * @return Formatted MAC address string
 */
std::string formatMAC(const MAC_ADR& mac, char separator = ':', bool lowercase = false)
{
   std::string result;
   const char* hex_chars = lowercase ? "0123456789abcdef" : "0123456789ABCDEF";

   for (int i = 0; i < MAC_ADDR_SZ; ++i)
   {
      if (i > 0)
         result += separator;
      result += hex_chars[(mac.macAdr[i] >> 4) & 0x0F];
      result += hex_chars[mac.macAdr[i] & 0x0F];
   }
   return result;
}

/**
 * Parse MAC address with separator (colon, hyphen, or dot)
 * Supports formats: AA:BB:CC:DD:EE:FF, AA-BB-CC-DD-EE-FF, AAAA.BBBB.CCCC, AA.BB.CC.DD.EE.FF
 * @param mac_str MAC address string
 * @param separator The separator character (':', '-', or '.')
 * @return Optional MAC_ADR struct if parsing successful, std::nullopt otherwise
 */
std::optional<MAC_ADR> parseMAC_WithSeparator(const std::string& mac_str, char separator)
{
   // Using static const std::regex for performance
   static const std::regex colon_pattern(
      "^([0-9A-Fa-f]{1,2}):([0-9A-Fa-f]{1,2}):([0-9A-Fa-f]{1,2}):([0-9A-Fa-f]{1,2}):([0-9A-Fa-f]{1,"
      "2}):([0-9A-Fa-f]{1,2})$");
   static const std::regex hyphen_pattern(
      "^([0-9A-Fa-f]{1,2})-([0-9A-Fa-f]{1,2})-([0-9A-Fa-f]{1,2})-([0-9A-Fa-f]{1,2})-([0-9A-Fa-f]{1,"
      "2})-([0-9A-Fa-f]{1,2})$");
   static const std::regex dotted_quad_pattern(
      "^([0-9A-Fa-f]{4})\\.([0-9A-Fa-f]{4})\\.([0-9A-Fa-f]{4})$");
   static const std::regex dot_pattern(
      "^([0-9A-Fa-f]{1,2})\\.([0-9A-Fa-f]{1,2})\\.([0-9A-Fa-f]{1,2})\\.([0-9A-Fa-f]{1,2})\\.([0-9A-"
      "Fa-f]{1,2})\\.([0-9A-Fa-f]{1,2})$");

   std::smatch matches;
   const std::regex* current_pattern = nullptr;
   MAC_ADR result = {};

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
      if (std::regex_match(mac_str, matches, dotted_quad_pattern))
      {
         try
         {
            for (int i = 0; i < 3; ++i)
            {
               std::string hex_pair = matches[i + 1].str();
               result.macAdr[i * 2] =
                  static_cast<unsigned char>(std::stoul(hex_pair.substr(0, 2), nullptr, 16));
               result.macAdr[i * 2 + 1] =
                  static_cast<unsigned char>(std::stoul(hex_pair.substr(2, 2), nullptr, 16));
            }
            return result;
         }
         catch (...)
         {
            return std::nullopt;
         }
      }
      // If Cisco didn't match, try generic dot pattern
      current_pattern = &dot_pattern;
   }
   else
   {
      // Unsupported separator
      return std::nullopt;
   }

   if (!current_pattern)
      return std::nullopt;

   if (std::regex_match(mac_str, matches, *current_pattern))
   {
      try
      {
         for (size_t i = 0; i < MAC_ADDR_SZ; ++i)
         {
            result.macAdr[i] =
               static_cast<unsigned char>(std::stoul(matches[i + 1].str(), nullptr, 16));
         }
         return result;
      }
      catch (...)
      {
         return std::nullopt;
      }
   }

   return std::nullopt;
}

/**
 * Parse MAC address without separator (12 consecutive hex characters)
 * Supports format: AABBCCDDEEFF
 * @param mac_str MAC address string (must be exactly 12 hex characters)
 * @return Optional MAC_ADR struct if parsing successful, std::nullopt otherwise
 */
std::optional<MAC_ADR> parseMAC_WithoutSeparator(const std::string& mac_str)
{
   if (mac_str.length() != 12)
   {
      return std::nullopt;
   }

   // Validate hex characters
   static const std::regex pattern("^[0-9A-Fa-f]{12}$");
   if (!std::regex_match(mac_str, pattern))
   {
      return std::nullopt;
   }

   MAC_ADR result = {};

   try
   {
      for (size_t i = 0; i < MAC_ADDR_SZ; ++i)
      {
         std::string hex_pair = mac_str.substr(i * 2, 2);
         result.macAdr[i] = static_cast<unsigned char>(std::stoul(hex_pair, nullptr, 16));
      }
      return result;
   }
   catch (...)
   {
      return std::nullopt;
   }
}

/**
 * Generic MAC address parser that handles multiple formats
 * @param mac_str MAC address string in any supported format
 * @return Optional MAC_ADR struct if parsing successful, std::nullopt otherwise
 */
std::optional<MAC_ADR> parseMAC(const std::string& mac_str)
{
   if (mac_str.empty())
      return std::nullopt;

   // Try different separator formats
   if (mac_str.find(':') != std::string::npos)
   {
      return parseMAC_WithSeparator(mac_str, ':');
   }
   else if (mac_str.find('-') != std::string::npos)
   {
      return parseMAC_WithSeparator(mac_str, '-');
   }
   else if (mac_str.find('.') != std::string::npos)
   {
      return parseMAC_WithSeparator(mac_str, '.');
   }
   else if (mac_str.length() == 12)
   {
      return parseMAC_WithoutSeparator(mac_str);
   }

   return std::nullopt;
}


