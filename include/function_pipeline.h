#pragma once

#include <utility>
#include <type_traits>

namespace pipeline {

// Forward declarations
template <typename F, typename G>
struct Composed;

template <typename F>
class FunctionPipeline;

// Core composition helper - applies f then g
template <typename F, typename G>
struct Composed {
    F f;
    G g;

    explicit Composed(F f_, G g_) : f(std::move(f_)), g(std::move(g_)) {}

    template <typename... Args>
    auto operator()(Args&&... args) const -> decltype(g(f(std::forward<Args>(args)...))) {
        return g(f(std::forward<Args>(args)...));
    }
};

// Main pipeline class that supports chaining
template <typename F>
class FunctionPipeline {
private:
    F func;

public:
    explicit FunctionPipeline(F f) : func(std::move(f)) {}

    // Chain another function after the current one
    template <typename G>
    auto then(G g) const -> FunctionPipeline<Composed<F, G>> {
        return FunctionPipeline<Composed<F, G>>{
            Composed<F, G>{func, std::move(g)}
        };
    }

    // Execute the pipeline
    template <typename... Args>
    auto operator()(Args&&... args) const -> decltype(func(std::forward<Args>(args)...)) {
        return func(std::forward<Args>(args)...);
    }
};

// Factory function to create a pipeline
template <typename F>
auto pipe(F f) -> FunctionPipeline<F> {
    return FunctionPipeline<F>{std::move(f)};
}

// Variadic pipe - shorthand for pipe(f1).then(f2).then(f3)...
template <typename F1, typename F2, typename... Fs>
auto pipe(F1 f1, F2 f2, Fs... fs) {
    if constexpr (sizeof...(fs) == 0) {
        return pipe(std::move(f1)).then(std::move(f2));
    } else {
        return pipe(std::move(f1)).then(std::move(f2)).then(std::move(fs)...);
    }
}

// Right-to-left composition helper (traditional mathematical composition)
template <typename F, typename G>
struct RightComposed {
    F f;
    G g;

    explicit RightComposed(F f_, G g_) : f(std::move(f_)), g(std::move(g_)) {}

    template <typename... Args>
    auto operator()(Args&&... args) const -> decltype(f(g(std::forward<Args>(args)...))) {
        return f(g(std::forward<Args>(args)...));
    }
};

// Right-to-left composition: compose(f, g, h) creates f(g(h(x)))
template <typename F>
auto compose(F f) -> FunctionPipeline<F> {
    return FunctionPipeline<F>{std::move(f)};
}

template <typename F, typename G>
auto compose(F f, G g) -> FunctionPipeline<RightComposed<F, G>> {
    return FunctionPipeline<RightComposed<F, G>>{
        RightComposed<F, G>{std::move(f), std::move(g)}
    };
}

template <typename F, typename G, typename... Fs>
auto compose(F f, G g, Fs... fs) {
    return compose(std::move(f), compose(std::move(g), std::move(fs)...));
}

} // namespace pipeline

// Usage examples and tests
#ifdef PIPELINE_EXAMPLES

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace examples {

// Helper functions for examples
std::string trim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
    return s;
}

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = s.find(delimiter);
    
    while (end != std::string::npos) {
        tokens.push_back(s.substr(start, end - start));
        start = end + delimiter.length();
        end = s.find(delimiter, start);
    }
    tokens.push_back(s.substr(start));
    return tokens;
}

void run_examples() {
    using namespace pipeline;
    
    std::cout << "=== FunctionPipeline Examples ===\n\n";
    
    // Example 1: Basic transform pipeline
    std::cout << "1. Transform pipeline:\n";
    auto pipeline1 = pipe([](int x) { return x * 2; })
                        .then([](int x) { return x + 3; })
                        .then([](int x) { return x * x; });
    
    int result1 = pipeline1(5);  // (((5 * 2) + 3) ^ 2) = 169
    std::cout << "   pipeline1(5) = " << result1 << " (expected: 169)\n\n";
    
    // Example 2: String processing pipeline
    std::cout << "2. String processing pipeline:\n";
    auto clean = [](std::string s) { return trim(to_lower(s)); };
    auto tokenize = [](const std::string& s) { return split(s, " "); };
    
    auto pipeline2 = pipe(clean).then(tokenize);
    auto words = pipeline2("  Hello WORLD  ");
    
    std::cout << "   Input: \"  Hello WORLD  \"\n";
    std::cout << "   Output: [";
    for (size_t i = 0; i < words.size(); ++i) {
        std::cout << "\"" << words[i] << "\"";
        if (i < words.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n\n";
    
    // Example 3: Type transforming chain
    std::cout << "3. Type transforming chain:\n";
    auto pipeline3 = pipe([](int i) { return std::to_string(i); })
                        .then([](const std::string& s) { return "Num: " + s; });
    
    std::string result3 = pipeline3(42);
    std::cout << "   pipeline3(42) = \"" << result3 << "\"\n\n";
    
    // Example 4: Variadic pipe shorthand
    std::cout << "4. Variadic pipe shorthand:\n";
    auto pipeline4 = pipe(
        [](int x) { return x + 1; },
        [](int x) { return x * 2; },
        [](int x) { return x - 5; }
    );
    
    int result4 = pipeline4(10);  // ((10 + 1) * 2) - 5 = 17
    std::cout << "   pipeline4(10) = " << result4 << " (expected: 17)\n\n";
    
    // Example 5: Right-to-left composition
    std::cout << "5. Right-to-left composition:\n";
    auto composed = compose(
        [](int x) { return x * x; },      // applied last
        [](int x) { return x + 1; },      // applied second
        [](int x) { return x * 2; }       // applied first
    );
    
    int result5 = composed(3);  // ((3 * 2) + 1)^2 = 49
    std::cout << "   composed(3) = " << result5 << " (expected: 49)\n\n";
    
    // Example 6: Complex chaining
    std::cout << "6. Complex chaining:\n";
    auto complex_pipeline = pipe([](double x) { return x / 2.0; })
                               .then([](double x) { return static_cast<int>(x); })
                               .then([](int x) { return std::to_string(x); })
                               .then([](const std::string& s) { return "Result: " + s; });
    
    std::string result6 = complex_pipeline(42.8);
    std::cout << "   complex_pipeline(42.8) = \"" << result6 << "\"\n\n";
}

} // namespace examples

#endif // PIPELINE_EXAMPLES
