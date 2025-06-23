#include "split_view.h" // Assuming split_view.h is in the include path (e.g., via -I../include)
#include <iostream>
#include <vector>
#include <string_view>

void print_tokens(std::string_view description, SplitView& view) {
    std::cout << description << " (full string: \"" << view.get_input() << "\"):\n";
    // Note: view.begin()->data() is not ideal for printing the original string if view stores input_ directly.
    // Let's assume SplitView has a way to access its input or we pass it separately.
    // For now, I'll just print the description.
    // The header was updated to add private member input_ to SplitView, let's use that for demo.
    // However, direct access to private members is not possible from here.
    // The plan is to add a public getter or pass the original string for printing.
    // For now, let's just reconstruct:
    // std::cout << description << " (for input like: " << view.begin().input_ ... no, that's private.
    // Let's just print the tokens.

    // std::cout << description << "\n"; // Original line, replaced by the one above for context
    for (std::string_view token : view) {
        std::cout << "  [\"" << token << "\"]\n";
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "===== SplitView Examples =====\n\n";

    // 1. CSV or TSV parsing (from Requirement.md)
    std::string csv_data = "col1,col2,col3";
    SplitView csv_row(csv_data, ',');
    print_tokens("CSV Parsing Example (\"col1,col2,col3\" by ','):", csv_row);

    std::string tsv_data = "field1\tfield2\tfield3";
    SplitView tsv_row(tsv_data, '\t');
    print_tokens("TSV Parsing Example (\"field1\\tfield2\\tfield3\" by '\\t'):", tsv_row);

    // Example from requirements output
    std::string req_example_data = "one,two,,three";
    SplitView req_tokens(req_example_data, ',');
    print_tokens("Requirements Output Example (\"one,two,,three\" by ','):", req_tokens);

    // 2. CLI or config parsing (from Requirement.md)
    std::string cli_args_str = "a:b:c:d";
    SplitView args_view(cli_args_str, ':');
    print_tokens("CLI/Config Parsing Example (\"a:b:c:d\" by ':'):", args_view);

    std::cout << "Collecting CLI args into a vector:\n";
    std::vector<std::string_view> fields(args_view.begin(), args_view.end());
    for (size_t i = 0; i < fields.size(); ++i) {
        std::cout << "  Field " << i << ": \"" << fields[i] << "\"\n";
    }
    std::cout << std::endl;

    // 3. Key-Value pair parsing (from Requirement.md)
    std::string_view kv_line = "key=value";
    SplitView kv_parts(kv_line, '=');
    auto it = kv_parts.begin();
    std::string_view key, val;

    std::cout << "Key-Value Pair Example (\"key=value\" by '='):\n";
    if (it != kv_parts.end()) {
        key = *it;
        ++it;
        if (it != kv_parts.end()) {
            val = *it;
        } else {
            val = ""; // Or handle as error / missing value
        }
         std::cout << "  Key: \"" << key << "\", Value: \"" << val << "\"\n";
    } else {
        std::cout << "  Could not parse key-value pair.\n";
    }
    std::cout << std::endl;

    // More examples:
    std::string path_data = "/usr/local/bin";
    SplitView path_tokens(path_data, '/');
    print_tokens("Path Splitting Example (\"/usr/local/bin\" by '/'):", path_tokens);
    // Expected: "", "usr", "local", "bin"

    std::string multi_char_delim_data = "item1--item2--item3";
    SplitView multi_char_tokens(multi_char_delim_data, "--");
    print_tokens("Multi-character Delimiter Example (\"item1--item2--item3\" by '--'):", multi_char_tokens);
    // Expected: "item1", "item2", "item3"

    std::string empty_input_data = "";
    SplitView empty_tokens(empty_input_data, ',');
    print_tokens("Empty Input Example (\"\" by ','):", empty_tokens);
    // Expected: ""

    std::string only_delimiters_data = ",,,";
    SplitView only_delims_tokens(only_delimiters_data, ',');
    print_tokens("Only Delimiters Example (\",,,\" by ','):", only_delims_tokens);
    // Expected: "", "", "", ""

    std::string no_delimiter_data = "HelloWorld";
    SplitView no_delim_tokens(no_delimiter_data, ';');
    print_tokens("No Delimiter Example (\"HelloWorld\" by ';'):", no_delim_tokens);
    // Expected: "HelloWorld"

    std::string trailing_delimiter_data = "end,";
    SplitView trailing_delim_tokens(trailing_delimiter_data, ',');
    print_tokens("Trailing Delimiter Example (\"end,\" by ','):", trailing_delim_tokens);
    // Expected: "end", ""

    std::cout << "===== End of Examples =====\n";

    return 0;
}
