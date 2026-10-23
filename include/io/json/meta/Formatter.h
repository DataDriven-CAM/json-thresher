#pragma once
#include <cstdio>
#include <string>
#include <string_view>
#include <sstream>

#include "metaphrase/fixed_string.h"
#include "metaphrase/fixed_stack.h"
#include "metaphrase/fixed_accumulator.h"

namespace sylvanmats::io::json::meta{

#include <meta>
#include <string_view>
#include <algorithm>

// Statically estimates the JSON primitive footprint string-width 
template<typename T>
consteval std::size_t estimate_max_value_width() {
    if constexpr (std::is_integral_v<T>) {
        return 20; // Maximum string footprint for uint64_t / int64_t digits
    } else if constexpr (std::is_floating_point_v<T>) {
        return 24; // Scientific notation buffer allowance
    } else if constexpr (std::is_same_v<T, bool>) {
        return 5;  // "false"
    } else {
        return sizeof(T) * 2; // Upper fallback for raw nested buffers/fixed strings
    }
}

// Scans structural layout using C++26 meta reflection to output exact buffer boundaries
template<typename S>
consteval std::size_t calculate_max_json_buffer_size(std::size_t indent_multiplier = 4) {
    // 1. Explicitly fetch an unrestricted access context to expose all struct layouts
    constexpr auto ctx = std::meta::access_context::unchecked(); 
    
    // 2. Materialize the data members into a static compile-time array via define_static_array
    static constexpr auto members = std::define_static_array(
        std::meta::nonstatic_data_members_of(^^S, ctx)
    );
    
    std::size_t bytes_accumulator = 2; // initial padding placeholder: "{\n"
    
    template for (constexpr auto m : members) {
        bytes_accumulator += indent_multiplier + 2 + 2 + 2; 

        // std::meta::name_of similarly maps to your standard string parsing tracking
        constexpr std::size_t key_len = std::meta::identifier_of(m).size();
        bytes_accumulator += (key_len * 2); 

        using MemberType = [: std::meta::type_of(m) :]; 
        bytes_accumulator += estimate_max_value_width<MemberType>();
    }
    
    return bytes_accumulator;
}
    template<size_t N>
    constexpr void transform_key_format(std::string_view source, ::sylvanmats::metaphrase::fixed_accumulator<N>& acc) {

        for (char c : source) {
            if (c == '-') {
                acc.push_back('_');
            } else if (c == '_') {
                acc.push_back('_');
                acc.push_back('_');
            } else {
                acc.push_back(c);
            }
        }
    }

constexpr auto transform_key_format(std::string_view source) {
    // Basic allocation tracker for compile-time string construction
    struct { char data[128]{}; size_t len = 0; } result;
    for (char c : source) {
        if (c == '-') { result.data[result.len++] = '_'; }
        else if (c == '_') { result.data[result.len++] = '_'; result.data[result.len++] = '_'; }
        else { result.data[result.len++] = c; }
    }
    return std::string_view(result.data, result.len);
}

    template<typename T>
    class Formatter{
        private:
           static constexpr std::size_t RequiredCapacity = calculate_max_json_buffer_size<T>();
        public:
        Formatter() = default;
        Formatter(const Formatter& orig) = delete;
        Formatter(Formatter&& orig) = delete;
        Formatter& operator=(const Formatter& orig) = delete;
        Formatter& operator=(Formatter&& orig) = delete;
        ~Formatter() = default;
        constexpr ::sylvanmats::metaphrase::fixed_accumulator<RequiredCapacity> operator ()(T value, int depth = 0){
            ::sylvanmats::metaphrase::fixed_accumulator<RequiredCapacity> acc;
            std::string indent(depth * 4, ' ');
            if constexpr (std::meta::is_type(^^T) && std::is_class_v<typename [: ^^T :]>) {
                acc.append("{{\n");
                template for (constexpr auto m : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))) {
                    auto identifier = std::meta::identifier_of(m);
                    acc.append(indent);
                    acc.append("\"");
                    transform_key_format(identifier, acc);
                    acc.append("\": {");
                    acc.append(identifier);
                    acc.append("}}\n");
                }
                acc.append("}}\n");
            }

            return acc;
        }

    template<typename F>
    static bool is_empty_field(const F& field_value) {
        if constexpr (std::is_same_v<F, std::string>) {
            return field_value.empty();
        } else if constexpr (requires { field_value.empty(); }) {
            return field_value.empty(); // Catches vectors or other custom containers
        } else if constexpr (std::is_same_v<F, std::variant<int, std::string>>) {
            // Checks if the variant is holding an empty string or standard default state
            return std::visit([](const auto& val) {
                if constexpr (std::is_same_v<std::decay_t<decltype(val)>, std::string>) {
                    return val.empty();
                }
                return false; // Valid numbers are never skipped
            }, field_value);
        }
        return false;
    }

    // Generic structural buffer writer: compiles into specialized steps for ANY type shape
    template<typename S>
    constexpr std::string emit_json_buffer(const S& obj, size_t indent_level = 1) {
        constexpr auto ctx = std::meta::access_context::unchecked(); 
        std::string buffer = "{\n";
        std::string indent(indent_level * 2, ' ');
        bool first_entry = true;

        // Fetch compile-time members of your custom target struct shape
        static constexpr auto members = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^S, ctx)
        );

        // --- THE C++26 UNROLLED INLINE LOOP ---
        template for (constexpr auto field : members) {
            // Unroll individual fields at compile time to create specialized runtime branches
            const auto& runtime_val = obj.[:field:];

            // 1. Skip completely if the user left the optional field blank at runtime
            if (!is_empty_field(runtime_val)) {
                
                // 2. Pre-bake the transformed schema naming labels at compile time
                constexpr auto raw_name = std::meta::identifier_of(field);
                static constexpr auto transformed_name = [&raw_name]() {
                    ::sylvanmats::metaphrase::fixed_accumulator<RequiredCapacity> acc;
                    transform_key_format(raw_name, acc);
                    return std::string_view(std::define_static_string(acc.view())); // Baked inside .rodata
                }();

                // Append comma separation logic between active attributes safely
                if (!first_entry) { buffer += ",\n"; }
                first_entry = false;

                buffer += indent + std::format("\"{}\": ", transformed_name);

                // 3. Match execution strategy to exact data type layouts at compile time
                using FieldType = std::decay_t<decltype(runtime_val)>;

                if constexpr (std::is_same_v<FieldType, std::string>) {
                    buffer += std::format("\"{}\"", runtime_val);
                }
                else if constexpr (std::is_same_v<FieldType, std::variant<int, std::string>>) {
                    buffer += std::visit([](const auto& val) -> std::string {
                        using V = std::decay_t<decltype(val)>;
                        if constexpr (std::is_same_v<V, int>) return std::to_string(val);
                        else return std::format("\"{}\"", val);
                    }, runtime_val);
                }
                else if constexpr (std::is_same_v<FieldType, std::vector<std::string>>) {
                    buffer += "[";
                    for (size_t i = 0; i < runtime_val.size(); ++i) {
                        buffer += std::format("\"{}\"{}", runtime_val[i], (i + 1 < runtime_val.size() ? ", " : ""));
                    }
                    buffer += "]";
                }
                else if constexpr (std::is_aggregate_v<FieldType>) {
                    // Recursively process nested structs (like your 'issued' block) safely
                    buffer += emit_json_buffer(runtime_val, indent_level + 1);
                }
            }
        }

        buffer += "\n" + std::string((indent_level - 1) * 2, ' ') + "}";
        return buffer;
    }

    template<typename S>
    constexpr bool deserialize_field(S& obj, std::string_view target_key, std::string_view raw_value) {
        constexpr auto ctx = std::meta::access_context::unchecked(); 
        bool key_matched = false;

        // 1. Fetch the static array of fields belonging to the target struct layout
        static constexpr auto members = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^S, ctx)
        );

        // 2. Unroll the loop to perform flat string lookups at compile time
        template for (constexpr auto field : members) {
            constexpr auto raw_name = std::meta::identifier_of(field);
            static constexpr auto transformed_name = []<auto F>() {
                constexpr auto r_name = std::meta::identifier_of(F);
                ::sylvanmats::metaphrase::fixed_accumulator<RequiredCapacity> acc;
                transform_key_format(r_name, acc);
                return std::string_view(std::define_static_string(acc.view()));
            }.template operator()<field>();

            // 3. Match the current JSON token against the pre-baked schema key
            if (target_key == transformed_name) {
                key_matched = true;
                auto& runtime_field = obj.[:field:];
                using FieldType = std::decay_t<decltype(runtime_field)>;

                // 4. Safely deduce and assign data down into the runtime types
                if constexpr (std::is_same_v<FieldType, std::string>) {
                    runtime_field = std::string(raw_value);
                }
                else if constexpr (std::is_same_v<FieldType, std::variant<int, std::string>>) {
                    // Check if the raw string represents an integer or a quoted literal
                    if (!raw_value.empty() && std::isdigit(raw_value[0])) {
                        runtime_field = std::stoi(std::string(raw_value));
                    } else {
                        runtime_field = std::string(raw_value);
                    }
                }
                else if constexpr (std::is_same_v<FieldType, std::vector<std::string>>) {
                    // Primitive comma-separated array token trimmer example
                    runtime_field.clear();
                    std::string_view remaining = raw_value;
                    if (remaining.starts_with('[')) remaining.remove_prefix(1);
                    if (remaining.ends_with(']')) remaining.remove_suffix(1);

                    // A real loop would split strings around commas into your vector
                    if (!remaining.empty()) {
                        runtime_field.emplace_back(remaining);
                    }
                }
            }
        }
        return key_matched;
    }
    };
}