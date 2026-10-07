#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_map>
#include <span>
#include <tuple>
#include <concepts>
#include <meta>
#include <variant>
#include <format>
#include <iostream>
#include <functional>
#include <ranges>
#include <vector>
#include <map>
#include <typeindex>
#include <cmath>
#include <stack>
#include <algorithm>
#include <utility>

namespace sylvanmats::metaphrase{

    // Structural fixed_string template matching our performance pipeline
    template<size_t N>
    struct fixed_string {
        char data[N]{};
        std::size_t length = 0;
        consteval fixed_string(const char (&str)[N]) { std::copy_n(str, N, data); }
        constexpr fixed_string(const char* ptr, size_t len) {
            length = (len < N) ? len : (N - 1);
            for (std::size_t i = 0; i < length; ++i) {
                data[i] = ptr[i];
            }
            data[len] = '\0';
        }
        constexpr std::string_view view() const { return std::string_view(data, length); }

        template<size_t M>
        constexpr auto operator+(const fixed_string<M>& other) const {
            char merged[N + M - 1]{};
            std::copy_n(data, N - 1, merged);
            std::copy_n(other.data, M - 1, merged + N - 1);
            return fixed_string<N + M - 1>(merged, N + M - 2);
        }
    };

    template<size_t N>
    fixed_string(const char (&)[N]) -> fixed_string<N>;

    // Allocation-free rule generator using structural concatenation
    template<fixed_string RuleName, fixed_string Body>
    consteval auto make_gbnf_rule() {
        constexpr auto sep = fixed_string(" ::= ");
        constexpr auto nl  = fixed_string("\n");
        return RuleName + sep + Body + nl;
    }

    // A zero-allocation, compile-time stack container
    template<typename T, size_t Capacity>
    struct fixed_stack : std::array<T, Capacity> {
        size_t current_size = 0;

   constexpr fixed_stack() = default;

        constexpr void push(T val) {
            if (current_size < Capacity) {
                (*this)[current_size] = val;
                current_size++;
            }
        }

        template<typename... Args>
        constexpr void emplace(Args&&... args) {
            if (current_size >= Capacity) {
                throw std::out_of_range(std::format("fixed_stack overflow : Cannot emplace element into a full buffer of size {}", Capacity));
            }
            // Construct the element perfectly in place
            (*this)[current_size] = T(std::forward<Args>(args)...);
            current_size++;
        }
        
        constexpr void pop() {
            if (current_size > 0) {
                current_size--;
            }
        }

        constexpr T& back() { 
            return (*this)[current_size - 1]; 
        }

        constexpr T back() const {
            return (current_size > 0) ? (*this)[current_size - 1] : T{};
        }

        constexpr size_t size() const { return current_size; }
        constexpr bool empty() const { return current_size == 0; }
        constexpr void clear() { current_size = 0; }

        constexpr bool anyOf(const T& val) const {
            for (size_t i = 0; i < current_size; ++i) {
                if ((*this)[i] == val) {
                    return true;
                }
            }
            return false;
        }

    };

// Explicit master slice-appender that accepts dynamic lengths safely
template<size_t MaxCapacity = 8192>
struct fixed_accumulator {
    char data_buffer[MaxCapacity]{};
    size_t current_length = 0;

    constexpr fixed_accumulator() = default;

    constexpr void append(std::string_view sv) {
        // Enforce boundary space for the incoming data and the null terminator
        if (current_length + sv.length() + 1 <= MaxCapacity) {
            std::copy_n(sv.data(), sv.length(), data_buffer + current_length);
            current_length += sv.length();
            data_buffer[current_length] = '\0';
        } else {
            // Triggers a hard compilation error if hit during constexpr evaluation
            throw std::out_of_range("fixed_accumulator overflow on append");
        }
    }

    constexpr void append(std::u8string_view u8sv) {
        if (current_length + u8sv.length() + 1 <= MaxCapacity) {
            for (size_t i = 0; i < u8sv.length(); ++i) {
                // static_cast from char8_t to char is fully permitted in constexpr
                data_buffer[current_length + i] = static_cast<char>(u8sv[i]);
            }
            current_length += u8sv.length();
            data_buffer[current_length] = '\0';
        } else {
            throw std::out_of_range(std::format("fixed_accumulator overflow on u8 append {} {}", u8sv.length(), MaxCapacity));
        }
    }

        // Appends a u8 string wrapped in quotes, automatically escaping GBNF conflicts
    constexpr void append_quoted_escaped(std::u8string_view u8sv) {
        if (current_length + u8sv.length() + 1 <= MaxCapacity) {

            // 1. Opening structural quote for GBNF literal matching
            data_buffer[current_length++] = '"';

            // 2. Transcoding pass with automatic inline escaping
            for (char8_t code_unit : u8sv) {
                const char c = static_cast<char>(code_unit);

                switch (c) {
                    case '"':
                        // Map a literal JSON quote to \" for GBNF terminal matching
                        data_buffer[current_length++] = '\\';
                        data_buffer[current_length++] = '"';
                        break;
                    case '\\':
                        // Escape backslashes
                        data_buffer[current_length++] = '\\';
                        data_buffer[current_length++] = '\\';
                        break;
                    case '\n':
                        // Flatten real newlines so they don't break the GBNF layout line structure
                        data_buffer[current_length++] = '\\';
                        data_buffer[current_length++] = 'n';
                        break;
                    case '\t':
                        data_buffer[current_length++] = '\\';
                        data_buffer[current_length++] = 't';
                        break;
                    default:
                        data_buffer[current_length++] = c;
                        break;
                }
            }

            // 3. Closing structural quote for GBNF literal matching
            data_buffer[current_length++] = '"';
        } else {
            throw std::out_of_range("fixed_accumulator overflow on u8 append_quoted_escaped");
        }
    }
    
    constexpr void push_back(char c) {
        if (current_length + 2 <= MaxCapacity) {
            data_buffer[current_length++] = c;
            data_buffer[current_length] = '\0';
        } else {
            throw std::out_of_range("fixed_accumulator overflow on push_back");
        }
    }

    
    constexpr const char* data() const { return data_buffer; }
    constexpr size_t size() const { return current_length; }
    constexpr std::string_view view() const { return std::string_view(data_buffer, current_length); }
};

// Layout descriptors
    enum OBJECT_TYPE{
        JSON_ROOT,
        JSON_OBJECT,
        JSON_ARRAY,
        JSON_STRING,
        JSON_NUMBER,
        JSON_INTEGER,
        JSON_BOOLEAN,
        PAIR_VALUE,
        JSON_NULL
    };

    struct jobject{
        OBJECT_TYPE obj_type;
        size_t id=0;
        size_t parent_id=0;
        size_t syntax_id=0;
        std::u8string_view key{};
        std::u8string_view value{};
        size_t depth=0;
        bool is_choice_child = false; // Flag to indicate if this node is syntax for alternation children
        std::u8string_view deferred_ref_path{}; // e.g., "#/definitions/graph"
        size_t resolved_target_idx = 0;     // Left at 0 for now
    };

    enum SCHEMA_CONTEXT : size_t {
        SCHEMA_MODE,     // Keywords active (type, properties)
        DATAKEY_MODE,    // Keywords suspended, treat keys as literal string fragments
        REGEXKEY_MODE,   // Keywords suspended, treat keys as regex token rules
        SYMBOLDEF_MODE   // Keys are internal references (under $defs)
    };

    enum EDGE_KIND : size_t {
        EDGE_KIND_AST=1,
        EDGE_KIND_AST_CHOICE=2,
        EDGE_KIND_PROPERTIES=3,
        EDGE_KIND_ADDITIONAL_PROPERTIES=4,
        EDGE_KIND_DEFINITIONS=5,
        EDGE_KIND_REQUIRED=6,
        EDGE_KIND_ITEMS=7,
        EDGE_KIND_PREFIX_ITEMS=8,
        EDGE_KIND_ENUM=9,
    };

    enum CHOICE_KIND{
        CHOICE_KIND_NONE=0,
        CHOICE_KIND_ONEOF=1,
        CHOICE_KIND_ANYOF=2,
        CHOICE_KIND_ALLOF=3,
        CHOICE_KIND_ENUM=4,
        CHOICE_KIND_TYPE=5,
        CHOICE_KIND_ITEMS=6,
        CHOICE_KIND_DEFINITIONS=7,
    };

    struct RawEdge {
         size_t source;
         size_t destination;
         EDGE_KIND edge_kind=EDGE_KIND_AST;
         SCHEMA_CONTEXT context=SCHEMA_CONTEXT::SCHEMA_MODE;
    };

    // Helper to keep track of traversal state without recursion
    struct DFSState {
        size_t vertex_id;
        size_t current_edge_idx; // Tracks which outgoing edge we are processing
    };

    class GBackusNaurFormation{
        public:
        GBackusNaurFormation() = default;
        GBackusNaurFormation(const GBackusNaurFormation& orig) = delete;
        GBackusNaurFormation(GBackusNaurFormation&& orig) = delete;
        GBackusNaurFormation& operator=(const GBackusNaurFormation& orig) = delete;
        GBackusNaurFormation& operator=(GBackusNaurFormation&& orig) = delete;
        ~GBackusNaurFormation() = default;
        constexpr fixed_string<16384> operator ()(std::u8string_view jsonBuffer){
            fixed_stack<std::tuple<size_t, jobject>, 8192> vertices;
            fixed_stack<std::tuple<size_t, size_t, EDGE_KIND, SCHEMA_CONTEXT>, 8191> edges;
            fixed_stack<size_t, 64> parentStack;
            fixed_stack<RawEdge, 8191> raw_edges;

            std::array<size_t, 8192> child_counts{};
            child_counts.fill(0);

            size_t cursor = 0;
            size_t keyStart=0;
            size_t keyEnd=0;
            bool hitColon=false;
            bool hitComma=false;
            bool firstObject=true;
            fixed_accumulator<65536> gbnfAcc{};
            while(cursor<jsonBuffer.size()){
                while (cursor < jsonBuffer.size() && 
                    (jsonBuffer[cursor] == ' ' || jsonBuffer[cursor] == '\t' || 
                    jsonBuffer[cursor] == '\n' || jsonBuffer[cursor] == '\r')) {
                    cursor++;
                }
                if (cursor >= jsonBuffer.size()) break;
                if(jsonBuffer[cursor]=='n' && cursor<jsonBuffer.size()-4 && jsonBuffer.substr(cursor, 4)==u8"null"){
                    size_t idx1 = vertices.size();
                    std::u8string_view current_key = (keyEnd > keyStart) ? jsonBuffer.substr(keyStart, keyEnd - keyStart) : u8"";
                    size_t parent_id=parentStack.empty() ? 0 : parentStack.back();
                    bool choice_flag=(parentStack.empty())? false: getChoiceSyntax(std::get<1>(vertices.back()).key)>CHOICE_KIND_NONE;
                    size_t syntax_id=(choice_flag) ? parent_id : 0;
                    EDGE_KIND edge_kind=parentStack.empty() ? EDGE_KIND_AST : getEdgeKind(std::get<1>(vertices[parentStack.back()]).key);
                    vertices.emplace(idx1, jobject{.obj_type=JSON_NULL, .id=idx1, .parent_id=parentStack.back(), .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(cursor, 1), .depth=parentStack.size(), .is_choice_child=choice_flag});
                    if(hasParentalSyntax(vertices, parentStack)){
                        parent_id=parentStack[parentStack.size()-2];
                    }
                    if(vertices.size()>1 && edge_kind==EDGE_KIND_AST){
                        // std::get<1>(vertices[parent_id]).deferred_ref_path=deferred_ref_path;
                        raw_edges.push(RawEdge{std::get<1>(vertices.back()).parent_id, idx1, EDGE_KIND_AST});
                        child_counts[std::get<1>(vertices.back()).parent_id]++;
                    }
                    hitColon=false;
                    hitComma=false;
                    keyStart = 0;
                    keyEnd = 0;
                    cursor+=4;
                    continue;
                }
                else if(jsonBuffer[cursor]=='t' && cursor<jsonBuffer.size()-4 && jsonBuffer.substr(cursor, 4)==u8"true"){
                    size_t idx1 = vertices.size();
                    std::u8string_view current_key = (keyEnd > keyStart) ? jsonBuffer.substr(keyStart, keyEnd - keyStart) : u8"";
                    size_t parent_id=parentStack.empty() ? 0 : parentStack.back();
                    bool choice_flag=(parentStack.empty())? false: getChoiceSyntax(std::get<1>(vertices.back()).key)>CHOICE_KIND_NONE;
                    size_t syntax_id=(choice_flag) ? parent_id : 0;
                    EDGE_KIND edge_kind=parentStack.empty() ? EDGE_KIND_AST : getEdgeKind(std::get<1>(vertices[parentStack.back()]).key);
                    if(hasParentalSyntax(vertices, parentStack)){
                        parent_id=parentStack[parentStack.size()-2];
                    }
                    vertices.emplace(idx1, jobject{.obj_type=JSON_BOOLEAN, .id=idx1, .parent_id=parent_id, .syntax_id=syntax_id, .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(cursor, 4), .depth=parentStack.size(), .is_choice_child=choice_flag});
                    if(vertices.size()>1 && edge_kind==EDGE_KIND_AST){
                        // std::get<1>(vertices[parent_id]).deferred_ref_path=deferred_ref_path;
                        raw_edges.push(RawEdge{parent_id, idx1});
                        child_counts[parent_id]++;
                    }
                    hitColon=false;
                    hitComma=false;
                    keyStart = 0;
                    keyEnd = 0;
                    cursor+=4;
                    continue;
                }
                else if(jsonBuffer[cursor]=='f' && cursor<jsonBuffer.size()-5 && jsonBuffer.substr(cursor, 5)==u8"false"){
                    size_t idx1 = vertices.size();
                    std::u8string_view current_key = (keyEnd > keyStart) ? jsonBuffer.substr(keyStart, keyEnd - keyStart) : u8"";
                    size_t parent_id=parentStack.empty() ? 0 : parentStack.back();
                    bool choice_flag=(parentStack.empty())? false: getChoiceSyntax(std::get<1>(vertices.back()).key)>CHOICE_KIND_NONE;
                    size_t syntax_id=(choice_flag) ? parent_id : 0;
                    SCHEMA_CONTEXT schemaContext=(!parentStack.empty() && std::get<1>(vertices[parentStack.back()]).key==u8"properties") ? DATAKEY_MODE : SCHEMA_MODE;
                    EDGE_KIND edge_kind=parentStack.empty() ? EDGE_KIND_AST : getEdgeKind(std::get<1>(vertices[parentStack.back()]).key);
                    if(hasParentalSyntax(vertices, parentStack)){
                        parent_id=parentStack[parentStack.size()-2];
                    }
                    vertices.emplace(idx1, jobject{.obj_type=JSON_BOOLEAN, .id=idx1, .parent_id=parent_id, .syntax_id=syntax_id, .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(cursor, 5), .depth=parentStack.size(), .is_choice_child=choice_flag});
                    if(vertices.size()>1 && (schemaContext==DATAKEY_MODE || (!is_schema_keyword(current_key) && edge_kind!=EDGE_KIND_DEFINITIONS))){
                        // std::get<1>(vertices[parent_id]).deferred_ref_path=deferred_ref_path;
                        raw_edges.push(RawEdge{parent_id, idx1, EDGE_KIND_AST, schemaContext});
                        child_counts[parent_id]++;
                    }
                    hitColon=false;
                    hitComma=false;
                    keyStart = 0;
                    keyEnd = 0;
                    cursor+=5;
                    continue;
                }
                else if(jsonBuffer[cursor]=='{'){
                    size_t idx1 = vertices.size();
                    std::u8string_view current_key = (keyEnd > keyStart) ? jsonBuffer.substr(keyStart, keyEnd - keyStart) : u8"";
                    if(firstObject){
                        current_key=u8"root";
                    }
                    // Check the key immediately to set the flag inline
                    bool choice_flag=(parentStack.empty())? false: getChoiceSyntax(std::get<1>(vertices[parentStack.back()]).key)>CHOICE_KIND_NONE;
                    size_t parent_id=parentStack.empty() ? 0 : parentStack.back();
                    size_t syntax_id=(choice_flag) ? parent_id : 0;
                    SCHEMA_CONTEXT schemaContext=(!parentStack.empty() && std::get<1>(vertices[parentStack.back()]).key==u8"properties") ? DATAKEY_MODE : SCHEMA_MODE;
                    EDGE_KIND edge_kind=parentStack.empty() ? EDGE_KIND_AST : getEdgeKind(std::get<1>(vertices[parentStack.back()]).key);
                    if(hasParentalSyntax(vertices, parentStack)){
                        parent_id=parentStack[parentStack.size()-2];
                    }
                    // gbnfAcc.append("# Object ");
                    // gbnfAcc.append(current_key);
                    // gbnfAcc.append(" ");
                    // gbnfAcc.append(jsonBuffer.substr(cursor, 1));
                    // gbnfAcc.append(" ");
                    // gbnfAcc.append(std::format("{}", idx1));
                    // gbnfAcc.append(" ");
                    // gbnfAcc.append(std::format("{}", parent_id));
                    // gbnfAcc.append(" ");
                    // gbnfAcc.append(std::format("{}", syntax_id));
                    // gbnfAcc.append(" ");
                    // gbnfAcc.append(std::format("{}", choice_flag));
                    // gbnfAcc.append(" ");
                    // gbnfAcc.append(std::format("{}", std::to_underlying(schemaContext)));
                    // gbnfAcc.append("\n");
                    vertices.emplace(idx1, jobject{.obj_type=JSON_OBJECT, .id=idx1, .parent_id=parent_id, .syntax_id=syntax_id, .key=current_key, .value=jsonBuffer.substr(cursor, 1), .depth=parentStack.size(), .is_choice_child=choice_flag});
                    // std::get<1>(vertices[parent_id]).deferred_ref_path=deferred_ref_path;
                    keyStart = 0;
                    keyEnd = 0;
                   if(!firstObject && vertices.size()>1 && (schemaContext==DATAKEY_MODE || (!is_schema_keyword(current_key) && edge_kind!=EDGE_KIND_DEFINITIONS))){
                        raw_edges.push(RawEdge{parent_id, idx1, edge_kind, schemaContext});
                        child_counts[parent_id]++;
                        // gbnfAcc.append(" # Edge ");
                        // gbnfAcc.append(std::get<1>(vertices.back()).key);
                        // gbnfAcc.append(" ");
                        // gbnfAcc.append(std::format("{}", std::get<1>(vertices.back()).id));
                        // gbnfAcc.append(" ");
                        // gbnfAcc.append(std::format("{}", parent_id));
                        // gbnfAcc.append(" ");
                        // gbnfAcc.append(std::format("{}", std::to_underlying(edge_kind)));
                        // gbnfAcc.append(" ");
                        // gbnfAcc.append(std::format("{}", child_counts[parent_id]));
                        // gbnfAcc.append("\n");
                    }
                    // else {
                    //     gbnfAcc.append(" # no edge \n");
                    // }
                    parentStack.push(std::get<1>(vertices.back()).id);
                    hitColon=false;
                    hitComma=false;
                    firstObject=false;
                }
                else if(jsonBuffer[cursor]=='}'){
                    parentStack.pop();
                }
                if(jsonBuffer[cursor]=='['){
                   size_t idx1 = vertices.size();
                    std::u8string_view current_key = (keyEnd > keyStart) ? jsonBuffer.substr(keyStart, keyEnd - keyStart) : u8"";
                    
                    // Check the key immediately to set the flag inline
                    bool choice_flag=(parentStack.empty())? false: getChoiceSyntax(std::get<1>(vertices[parentStack.back()]).key)>CHOICE_KIND_NONE;
                    size_t parent_id=parentStack.empty() ? 0 : parentStack.back();
                    size_t syntax_id=(choice_flag) ? parent_id : 0;
                    SCHEMA_CONTEXT schemaContext=(!parentStack.empty() && std::get<1>(vertices[parentStack.back()]).key==u8"properties") ? DATAKEY_MODE : SCHEMA_MODE;
                    EDGE_KIND edge_kind=parentStack.empty() ? EDGE_KIND_AST : getEdgeKind(std::get<1>(vertices[parentStack.back()]).key);
                    if(hasParentalSyntax(vertices, parentStack)){
                        parent_id=parentStack[parentStack.size()-2];
                    }
                    // gbnfAcc.append("# Array ");
                    // gbnfAcc.append(current_key);
                    // gbnfAcc.append(" ");
                    // gbnfAcc.append(std::format("{}", idx1));
                    // gbnfAcc.append(" ");
                    // gbnfAcc.append(jsonBuffer.substr(cursor, 1));
                    // gbnfAcc.append(" ");
                    // gbnfAcc.append(std::format("{}", parent_id));
                    // gbnfAcc.append(" ");
                    // gbnfAcc.append(std::format("{}", syntax_id));
                    // gbnfAcc.append(" ");
                    // gbnfAcc.append(std::format("{}", choice_flag));
                    // gbnfAcc.append("\n");
                    vertices.emplace(idx1, jobject{.obj_type=JSON_ARRAY, .id=idx1, .parent_id=parent_id, .syntax_id=syntax_id, .key=current_key, .value=jsonBuffer.substr(cursor, 1), .depth=parentStack.size(), .is_choice_child=choice_flag});
                    // std::get<1>(vertices[parent_id]).deferred_ref_path=deferred_ref_path;
                    keyStart = 0;
                    keyEnd = 0;
                    if(vertices.size()>1 && (schemaContext==DATAKEY_MODE || !is_schema_keyword(current_key))){
                        raw_edges.push(RawEdge{parent_id, idx1, edge_kind, schemaContext});
                        child_counts[parent_id]++;
                        // gbnfAcc.append(" # Edge ");
                        // gbnfAcc.append(std::get<1>(vertices.back()).key);
                        // gbnfAcc.append(" ");
                        // gbnfAcc.append(std::format("{}", parent_id));
                        // gbnfAcc.append(" ");
                        // gbnfAcc.append(std::format("{}", std::get<1>(vertices.back()).id));
                        // gbnfAcc.append(" ");
                        // gbnfAcc.append(std::format("{}", std::to_underlying(edge_kind)));
                        // gbnfAcc.append(" ");
                        // gbnfAcc.append(std::format("{}", child_counts[parent_id]));
                        // gbnfAcc.append("\n");
                    }
                    // else {
                    //     gbnfAcc.append(" # no edge \n");
                    // }
                    parentStack.push(std::get<1>(vertices.back()).id);
                    hitColon=false;
                    hitComma=false;
                }
                else if(jsonBuffer[cursor]==']'){
                    parentStack.pop();
                }
                else if(jsonBuffer[cursor]=='"'){
                    cursor++;
                    size_t startOffset=cursor;
                    size_t c=0;
                    while(cursor<jsonBuffer.size() && jsonBuffer[cursor]!='"'){if(jsonBuffer[cursor]=='\\'){cursor++;};cursor++;c++;};
                    if(std::get<1>(vertices[parentStack.back()]).obj_type==JSON_ARRAY){
                        size_t idx1 = vertices.size();
                        size_t parent_id=parentStack.empty() ? 0 : parentStack.back();
                        size_t syntax_id=parent_id;
                        if(parentStack.size()>=2){
                            parent_id=parentStack[parentStack.size()-2];
                        }
                        vertices.emplace(idx1, jobject{.obj_type=JSON_STRING, .id=idx1, .parent_id=parent_id, .syntax_id=syntax_id, .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(startOffset, cursor - startOffset), .depth=parentStack.size()});
                        // std::get<1>(vertices[parent_id]).deferred_ref_path=deferred_ref_path;
                        EDGE_KIND edge_kind=EDGE_KIND_AST;
                        if(std::get<1>(vertices[parentStack.back()]).is_choice_child){
                            edge_kind=EDGE_KIND_AST_CHOICE;
                        }
                        else if(std::get<1>(vertices[parentStack.back()]).key==u8"required"){
                            edge_kind=EDGE_KIND_REQUIRED;
                        }
                        else if(std::get<1>(vertices[parentStack.back()]).key==u8"enum"){
                            edge_kind=EDGE_KIND_ENUM;
                        }
                        raw_edges.push(RawEdge{parent_id, idx1, edge_kind});
                        child_counts[parent_id]++;
                    }
                    else if(!hitColon){
                        keyStart=startOffset;
                        keyEnd=cursor;
                    }
                    else{
                        size_t idx1 = vertices.size();
                        std::u8string_view current_key = (keyEnd > keyStart) ? jsonBuffer.substr(keyStart, keyEnd - keyStart) : u8"";
                        bool choice_flag=(parentStack.empty())? false: getChoiceSyntax(std::get<1>(vertices[parentStack.back()]).key)>CHOICE_KIND_NONE;
                        size_t parent_id=parentStack.empty() ? 0 : parentStack.back();
                        SCHEMA_CONTEXT schemaContext=(!parentStack.empty() && std::get<1>(vertices[parentStack.back()]).key==u8"properties") ? DATAKEY_MODE : SCHEMA_MODE;
                        EDGE_KIND edge_kind=getEdgeKind(std::get<1>(vertices[parentStack.back()]).key);
                        size_t syntax_id=(std::get<1>(vertices[parentStack.back()]).key==u8"properties") ? parent_id : 0;
                        std::u8string_view deferred_ref_path=u8"";
                        if(hasParentalSyntax(vertices, parentStack)){
                            parent_id=parentStack[parentStack.size()-2];
                        }
                        else if (current_key == u8"$schema" || current_key == u8"$id" || current_key == u8"title" || current_key == u8"description") {
                            cursor++;
                            continue;
                        }
                        else if(current_key==u8"$ref"){
                            schemaContext=DATAKEY_MODE;
                            deferred_ref_path=jsonBuffer.substr(startOffset, cursor - startOffset);
                        }
                         vertices.emplace(idx1, jobject{.obj_type=JSON_STRING, .id=idx1, .parent_id=parent_id, .syntax_id=syntax_id, .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(startOffset, cursor - startOffset), .depth=parentStack.size(), .deferred_ref_path=deferred_ref_path});
                         std::get<1>(vertices[parent_id]).deferred_ref_path=deferred_ref_path;
                        // edges.push(std::tuple<size_t, size_t, int>{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id, 1});
                        if(vertices.size()>1 && (schemaContext==SCHEMA_MODE)){
                            raw_edges.push(RawEdge{parent_id, idx1, edge_kind, schemaContext});
                            child_counts[parent_id]++;
                        }
                        hitColon=false;
                        hitComma=false;
                    }
                    //cursor++;
                    
                }
                else if(jsonBuffer[cursor]=='-' || jsonBuffer[cursor]=='.' || (jsonBuffer[cursor]>='0' && jsonBuffer[cursor]<='9')){
                    size_t itStart=cursor;
                    cursor++;
                    bool hitPeriod=(jsonBuffer[cursor]=='.') ? true : false;
                    size_t c=0;
                    while((jsonBuffer[cursor]>='0' && jsonBuffer[cursor]<='9') || jsonBuffer[cursor]=='.'){if(!hitPeriod && jsonBuffer[cursor]=='.')hitPeriod=true;cursor++;c++;};
                    double val{};
                    if(hitPeriod){
                        size_t idx1 = vertices.size();
                        vertices.emplace(idx1, jobject{.obj_type=JSON_NUMBER, .id=idx1, .parent_id=parentStack.back(), .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(itStart, cursor - itStart), .depth=parentStack.size()});
                        // std::get<1>(vertices[parent_id]).deferred_ref_path=deferred_ref_path;
                        EDGE_KIND edge_kind=EDGE_KIND_AST;
                        raw_edges.push(RawEdge{std::get<1>(vertices.back()).parent_id, idx1, edge_kind});
                        child_counts[std::get<1>(vertices.back()).parent_id]++;
                    }
                    else{
                        size_t idx1 = vertices.size();
                        vertices.emplace(idx1, jobject{.obj_type=JSON_NUMBER, .id=idx1, .parent_id=parentStack.back(), .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(itStart, cursor - itStart), .depth=parentStack.size()});
                        // std::get<1>(vertices[parent_id]).deferred_ref_path=deferred_ref_path;
                        raw_edges.push(RawEdge{std::get<1>(vertices.back()).parent_id, idx1, EDGE_KIND_AST});
                        child_counts[std::get<1>(vertices.back()).parent_id]++;
                    }
                    hitColon=false;
                    hitComma=false;
                    keyStart = 0;
                    keyEnd = 0;
                    
                }
                else if(jsonBuffer[cursor]==':'){
                    hitColon=true;
                    hitComma=false;
                }
                else if(jsonBuffer[cursor]==','){
                    hitComma=true;
                    hitColon=false;
                    keyStart = 0;
                    keyEnd = 0;
                }
                else if(jsonBuffer[cursor]=='\\'){
                    cursor++;
                }
                cursor++;
            }
            size_t deferred_sum = 0;
            for (auto& vertex : vertices) {
                if (!std::get<1>(vertex).deferred_ref_path.empty()) {
                    // Look up which vertex index owns this definition name
                    std::get<1>(vertex).resolved_target_idx = resolve_ref_pointer(vertices, std::get<1>(vertex).deferred_ref_path);
                    deferred_sum++;
                }
            }
            // gbnfAcc.append("# Deferred sum ");
            // gbnfAcc.append(std::format("{}", deferred_sum));
            // gbnfAcc.append("\n\n");
            std::array<size_t, 8192> edge_offsets{};
            size_t running_sum = 0;

            for (size_t i = 0; i < vertices.size(); ++i) {
                edge_offsets[i] = running_sum;
                running_sum += child_counts[i];
            }

            // Set the final edge size bounds explicitly
            edges.current_size = raw_edges.size();

            // Populate the final edges table in a single linear pass.
            // Because we use the pre-calculated offsets, all edges share the same source 
            // are automatically written into a perfectly contiguous block!
            for (size_t i = 0; i < raw_edges.size(); ++i) {
                size_t src = raw_edges[i].source;
                size_t dest = raw_edges[i].destination;
                
                size_t target_slot = edge_offsets[src];
                edge_offsets[src]++;
                edges[target_slot] = std::tuple<size_t, size_t, EDGE_KIND, SCHEMA_CONTEXT>{src, dest, raw_edges[i].edge_kind, raw_edges[i].context};
            }
            // 1. Storage bounds matching your structural limits
            std::array<size_t, 8192> edge_start_idx{};
            size_t sum = 0;
            for (size_t i = 0; i < vertices.size(); ++i) {
                edge_start_idx[i] = sum;
                sum += child_counts[i];
            }

            std::array<fixed_accumulator<512>, 8192> vertex_rules{};
            traverse_gbnf_graph(vertices, edges, edge_start_idx, child_counts, vertex_rules);
            // fixed_accumulator<65536> gbnfAcc{};
            gbnfAcc.append("# --- METAPHRASED LLAMA COMPATIBLE GBNF GRAMMAR ---\n");
            gbnfAcc.append("# Generated automatically from source JSON Schema definitions\n\n");
            // 1. Core Entry Point Rule
            gbnfAcc.append(vertex_rules[0].view());

            for (size_t i = 1; i < vertices.size(); ++i) {
                if (vertex_rules[i].size() > 0) {
                    gbnfAcc.append(vertex_rules[i].view());
                }
            }
            
            gbnfAcc.append(R"(
ws ::= [ \t\n\r]*
string ::= "\"" [^"\\]* "\""
number ::= [0-9]+ ("." [0-9]+)?
integer ::= [0-9]+
boolean ::= "true" | "false"
null ::= "null"

)");
            return sylvanmats::metaphrase::fixed_string<16384>(gbnfAcc.data(), gbnfAcc.size());
        }

    private:
        template<size_t VCapacity, size_t ECapacity>
        constexpr void traverse_gbnf_graph(
            const fixed_stack<std::tuple<size_t, jobject>, VCapacity>& vertices,
            const fixed_stack<std::tuple<size_t, size_t, EDGE_KIND, SCHEMA_CONTEXT>, ECapacity>& edges,
            const std::array<size_t, VCapacity>& edge_start_idx,
            std::array<size_t, VCapacity>& child_counts,
            std::array<fixed_accumulator<512>, 8192>& vertex_rules
        ) {
            if (vertices.size() <= 1) return;
            // 1. Explicit visitor stack replacing runtime/compiler call recursion
            fixed_stack<DFSState, 128> traversal_stack;
            
            // 2. Visited map to guard against recursive schema definitions ($ref cycles)
            std::array<bool, VCapacity> visited{};
            visited.fill(false);

            // Push root vertex (assumed to be index 0)
            traversal_stack.push(DFSState{0, 0});
            visited[0] = true;

            std::array<size_t, 8192> printed_fields{};
            printed_fields.fill(0);
            
            size_t offset_edge_idx=0;
            size_t highest_vertex_id=0;
            while (!traversal_stack.empty()) {
                auto& state = traversal_stack.back();
                size_t u = state.vertex_id;
                const auto& u_obj = std::get<1>(vertices[u]);

                if(u_obj.obj_type==JSON_OBJECT && child_counts[u]==1 && std::get<1>(vertices[std::get<1>(edges[edge_start_idx[u]])]).key==u8"type"){
                    vertex_rules[u].append("root ::= \"{\" ws ( string ws \":\" ws value ( ws \",\" ws string ws \":\" ws value )* )? \"}\"\n");
                    vertex_rules[u].append("value  ::= object | array | string | number | \"true\" | \"false\" | \"null\"\n");
                    vertex_rules[u].append("object ::= \"{\" ws ( string ws \":\" ws value ( ws \",\" ws string ws \":\" ws value )* )? \"}\"\n");
                    vertex_rules[u].append("array  ::= \"[\" ws ( value ( ws \",\" ws value )* )? \"]\"\n");
                    if (!visited[u]) {
                    visited[u] = true;
                    }
                    traversal_stack.pop();
                    break;
                }
                
                if(is_schema_keyword(u_obj.key) && u_obj.key!=u8"type"){
                    // vertex_rules[u].append("\n# ");
                    // vertex_rules[u].append(u_obj.key);
                    // vertex_rules[u].append(" should not be connected to anything\n");
                    if (!visited[u]) {
                    visited[u] = true;
                    }
                    traversal_stack.pop();
                }
                else{
                std::u8string_view rootType = findType(u, child_counts, vertices, edges, edge_start_idx);
                // 1. Declare the rule name on entry
                if (state.current_edge_idx == 0) {
                    vertex_rules[u].append(u_obj.key);
                    if(u_obj.key.empty()){
                        vertex_rules[u].append("-branch-");
                        emit_size_t_as_string(vertex_rules[u], u);
                        vertex_rules[u].append("-rule");
                    }
                    vertex_rules[u].append(" ::= ");
                    if (u_obj.obj_type == JSON_OBJECT && rootType==u8"object") vertex_rules[u].append("\"{\" ws ");
                    else if (u_obj.obj_type == JSON_OBJECT && rootType==u8"array") vertex_rules[u].append("\"[\" ws ");
                }

                // 2. Linear traversal over perfectly mapped edges
                if (state.current_edge_idx < child_counts[u]) {
                    size_t next_edge = edge_start_idx[u] + state.current_edge_idx;
                    size_t v = std::get<1>(edges[next_edge]);
                    const auto& v_obj = std::get<1>(vertices[v]);
                    if(highest_vertex_id<=v)highest_vertex_id=v+1;

                    if(v_obj.resolved_target_idx>0){
                        size_t r=v_obj.resolved_target_idx;
                        const auto& r_obj=std::get<1>(vertices[r]);
                        vertex_rules[u].append("\"\\\"");
                        vertex_rules[u].append(v_obj.key);
                        vertex_rules[u].append("\\\"\" ws \":\" ws ");
                        vertex_rules[u].append(r_obj.key);
                        vertex_rules[u].append(" ws ");
                        visited[v]=true;

                    }
                    else if(std::get<2>(edges[next_edge]) == EDGE_KIND_DEFINITIONS || (std::get<1>(vertices[v_obj.syntax_id]).key==u8"definitions" || std::get<1>(vertices[v_obj.syntax_id]).key==u8"$defs")){
                        // vertex_rules[u].append("\n# definitions syntax sugar\n");
                        visited[v]=true;
                    }
                    else if(std::get<2>(edges[next_edge]) == EDGE_KIND_REQUIRED || std::get<2>(edges[next_edge]) == EDGE_KIND_ITEMS || std::get<2>(edges[next_edge]) == EDGE_KIND_ADDITIONAL_PROPERTIES){
                        // if(state.current_edge_idx > 0) vertex_rules[u].append(" \",\" ws ");
                        // vertex_rules[u].append("\"\\\"");
                        // vertex_rules[u].append(v_obj.key);
                        // vertex_rules[u].append("\\\"\" ws \":\" ws ");
                        // vertex_rules[u].append(v_obj.value);
                        //vertex_rules[u].append("\n# required impl here\n");
                        visited[v]=true;

                    }
                    else if(std::get<2>(edges[next_edge]) == EDGE_KIND_PROPERTIES){
                        fixed_stack<std::u8string_view, 128> required_stack;
                        findRequiredArray(u, child_counts, vertices, edges, edge_start_idx, required_stack);
                        size_t inner_edge = edge_start_idx[v] + 0;
                        size_t w = std::get<1>(edges[inner_edge]);
                        const auto& w_obj = std::get<1>(vertices[w]);
                        if(highest_vertex_id<=w)highest_vertex_id=w+1;
                        std::u8string_view type = findType(v, child_counts, vertices, edges, edge_start_idx);
                        fixed_stack<std::u8string_view, 128> enumerated_array;
                        findEnumerationArray(v, child_counts, vertices, edges, edge_start_idx, enumerated_array);
                        fixed_stack<std::u8string_view, 128> prefix_items_array;
                        findPrefixItemsArray(v, child_counts, vertices, edges, edge_start_idx, prefix_items_array);
                        if(!type.empty()){
                            if(required_stack.size()>0 && !required_stack.anyOf(v_obj.key)){
                                vertex_rules[u].append(" (");
                            }
                          if (state.current_edge_idx > offset_edge_idx) vertex_rules[u].append(" ws \",\" ws ");
                          vertex_rules[u].append("\"\\\"");
                          vertex_rules[u].append(v_obj.key);
                          vertex_rules[u].append("\\\"\" ws \":\" ws ");
                        }
                        if(w_obj.resolved_target_idx>0){
                            // vertex_rules[u].append("# deferred ref path ");
                            // vertex_rules[u].append(std::get<1>(vertices[v_obj.resolved_target_idx]).key);
                            // vertex_rules[u].append("\n");
                            if (state.current_edge_idx > offset_edge_idx) vertex_rules[u].append(" \",\" ");
                            vertex_rules[u].append("\"\\\"");
                            vertex_rules[u].append(v_obj.key);
                            vertex_rules[u].append("\\\"\" ws \":\" ws ");
                            vertex_rules[u].append(std::get<1>(vertices[v_obj.resolved_target_idx]).key);
                            vertex_rules[u].append("\n");
                            visited[v]=true;
                            offset_edge_idx++;
                        }
                        if(prefix_items_array.size()>0){
                            // vertex_rules[u].append("# prefixItemsArray ");
                            // vertex_rules[u].append(std::format("{}", prefix_items_array.size()));
                            // vertex_rules[u].append("\n");
                            vertex_rules[u].append("\"[\" ws (");
                            vertex_rules[u].append("\"[\" ");
                            for(size_t piCount=0;piCount<prefix_items_array.size();piCount++){
                                if(piCount>0){
                                    vertex_rules[u].append("\",\" ");
                                }
                                vertex_rules[u].append(" ws ");
                                vertex_rules[u].append(prefix_items_array[piCount]);
                                vertex_rules[u].append(" ws ");
                            }
                            vertex_rules[u].append("\"]\" ");
                            vertex_rules[u].append(" ( ws \",\" ws ");
                            vertex_rules[u].append("\"[\" ");
                            for(size_t piCount=0;piCount<prefix_items_array.size();piCount++){
                                if(piCount>0){
                                    vertex_rules[u].append("\",\" ");
                                }
                                vertex_rules[u].append(" ws ");
                                vertex_rules[u].append(prefix_items_array[piCount]);
                                vertex_rules[u].append(" ws ");
                            }
                            vertex_rules[u].append("\"]\")*)? ws \"]\" ");
                            visited[w]=true;
                            visited[v]=true;
                        }
                        else if(enumerated_array.size()>0){
                            vertex_rules[u].append(" (");
                            for(size_t eCount=0;eCount<enumerated_array.size();eCount++){
                                if(eCount>0){
                                    vertex_rules[u].append(" |");
                                }
                                vertex_rules[u].append(" ");
                                if(type==u8"string")vertex_rules[u].append("\"\\\"");
                                vertex_rules[u].append(enumerated_array[eCount]);
                                if(type==u8"string")vertex_rules[u].append("\\\"\"");
                                vertex_rules[u].append(" ");
                            }
                            vertex_rules[u].append(")");
                            visited[w]=true;
                            visited[v]=true;
                        }
                        else if(child_counts[v]>=2 && w_obj.value==u8"array"){
                            vertex_rules[u].append("-branch-");
                            emit_size_t_as_string(vertex_rules[u], v);
                            vertex_rules[u].append("-rule ");
                            vertex_rules[v].append("-branch-");
                            emit_size_t_as_string(vertex_rules[v], v);
                            vertex_rules[v].append("-rule ::= ");
                            size_t x=std::get<1>(edges[edge_start_idx[v] + 1]);
                            const auto& x_obj = std::get<1>(vertices[x]);
                            if(highest_vertex_id<=x)highest_vertex_id=x+1;
                            if(w_obj.value==u8"array" && x_obj.key==u8"type"){
                                vertex_rules[v].append("\"[\" ws ( string ( ws \",\" ws string )* )? ws \"]\"\n");
                              //vertex_rules[u].append(x_obj.value);
                              visited[x]=true;
                            }
                            visited[w]=true;
                            visited[v]=true;
                            // traversal_stack.pop();
                          }
                          else if(w_obj.value==u8"array"){
                            //vertex_rules[u].append("\n# array syntax sugar\n");
                            
                          }
                          else{
                            vertex_rules[u].append(w_obj.value);
                            if(required_stack.size()>0 && !required_stack.anyOf(v_obj.key)){
                                vertex_rules[u].append(")?");
                            }
                            visited[w]=true;
                            visited[v]=true;
                          }
                        required_stack.clear();
                    }
                    else if(u_obj.obj_type==JSON_OBJECT && v_obj.key==u8"type")offset_edge_idx++;
                    else if(u_obj.obj_type==JSON_OBJECT && (v_obj.obj_type==JSON_STRING || v_obj.obj_type==JSON_NUMBER || v_obj.obj_type==JSON_BOOLEAN || v_obj.obj_type==JSON_NULL)){
                        vertex_rules[u].append(" \"");
                        vertex_rules[u].append(getPrimitiveName(v_obj.obj_type));
                        vertex_rules[u].append("\"");
                    }
                    else if (v_obj.is_choice_child) {
                        if (state.current_edge_idx > offset_edge_idx) vertex_rules[u].append(" | ");
                        vertex_rules[u].append(v_obj.key);
                        if(v_obj.key.empty()){
                            vertex_rules[u].append("_branch_");
                            emit_size_t_as_string(vertex_rules[u], v);
                        }
                        vertex_rules[u].append("_rule");
                    } else {
                        if (state.current_edge_idx > offset_edge_idx) vertex_rules[u].append(" ws \",\" ws ");
                        // Emit normal sequential keys and reference links...
                        if(v_obj.key.empty()){
                            vertex_rules[u].append("-branch-");
                            emit_size_t_as_string(vertex_rules[u], v);
                            vertex_rules[u].append("-rule");
                        }
                        else{
                        //   vertex_rules[u].append("\"\\\"");
                        //   vertex_rules[u].append(v_obj.key);
                        //   vertex_rules[u].append("\\\"\" ws \":\" ws ");
                          //vertex_rules[u].append(v_obj.value);
                        }

                    }
                    
                    if (!visited[v]) {
                        visited[v] = true;
                        if(v_obj.obj_type!=JSON_STRING && v_obj.obj_type!=JSON_NUMBER && v_obj.obj_type!=JSON_BOOLEAN && v_obj.obj_type!=JSON_NULL)
                            traversal_stack.push(DFSState{v, 0});
                    }
                    state.current_edge_idx++;
                } else {
                    // 3. Close rule on exit
                    if (u_obj.obj_type == JSON_OBJECT &&  rootType==u8"object")
                     vertex_rules[u].append(" ws \"}\"");
                    else if (u_obj.obj_type == JSON_OBJECT &&  rootType==u8"array")
                     vertex_rules[u].append(" ws \"]\"");
                    vertex_rules[u].append("\n");
                    visited[u] = true;
                    if(highest_vertex_id<=u)highest_vertex_id=u+1;
                    traversal_stack.pop();
                }
                }
                if(traversal_stack.empty() && highest_vertex_id<vertices.size()-1){
                    // vertex_rules[u].append("# highest_vertex_id ");
                    // vertex_rules[u].append(std::format("{}", highest_vertex_id));
                    // vertex_rules[u].append(" ");
                    // vertex_rules[u].append(std::format("{}", vertices.size()));
                    // if(!visited[highest_vertex_id])vertex_rules[u].append(" !visited ");
                    // vertex_rules[u].append("\n");
                    while(highest_vertex_id<vertices.size()-1 &&  visited[highest_vertex_id]){
                        highest_vertex_id++;
                    }
                    if(highest_vertex_id<vertices.size()-1){
                        traversal_stack.push(DFSState{highest_vertex_id, 0});
                        offset_edge_idx=0;
                    }
                }
            }
        };

        // A simple constexpr flag helper to identify JSON Schema keywords
        constexpr bool is_schema_keyword(std::u8string_view key) {
            return key == u8"properties" || key == u8"addionalProperties" || key == u8"oneOf"    || key == u8"anyOf"             || key == u8"allOf" || key == u8"enum" || 
                key == u8"items"         || key == u8"required"           || key == u8"additionalProperties" ||
                key == u8"$schema"       || key == u8"$id"                || key == u8"defintions"           || key == u8"$defs" || key == u8"$ref"  || key == u8"title" || key == u8"type" ||
                key == u8"prefixItems";
        }    

        template<size_t VCapacity>
        constexpr bool hasParentalSyntax(const fixed_stack<std::tuple<size_t, jobject>, VCapacity>& vertices, fixed_stack<size_t, 64>& parentStack){
            if(parentStack.size()>=2 && (getEdgeKind(std::get<1>(vertices[parentStack.back()]).key)>EDGE_KIND_AST_CHOICE || getChoiceSyntax(std::get<1>(vertices[parentStack.back()]).key)>CHOICE_KIND_NONE)){
                if(parentStack.size()>=3 && getEdgeKind(std::get<1>(vertices[parentStack[parentStack.size()-2]]).key)==EDGE_KIND_PROPERTIES)
                    return false;
                // if(parentStack.size()>=3 && getEdgeKind(std::get<1>(vertices[parentStack[parentStack.size()-2]]).key)==EDGE_KIND_REQUIRED)
                //     return false;
                    return true;
            }
            else return false;
        }

        constexpr EDGE_KIND getEdgeKind(std::u8string_view key) {
            if (key == u8"properties") return EDGE_KIND_PROPERTIES;
            if (key == u8"additionalProperties") return EDGE_KIND_ADDITIONAL_PROPERTIES;
            if (key == u8"definitions" || key == u8"$defs") return EDGE_KIND_DEFINITIONS;
            if (key == u8"required") return EDGE_KIND_REQUIRED;
            if (key == u8"items") return EDGE_KIND_ITEMS;
            if (key == u8"prefixItems") return EDGE_KIND_PREFIX_ITEMS;
            if (getChoiceSyntax(key)!=CHOICE_KIND_NONE) return EDGE_KIND_AST_CHOICE;
            return EDGE_KIND_AST;
        }

        constexpr CHOICE_KIND getChoiceSyntax(std::u8string_view key) {
            if (key == u8"oneOf") return CHOICE_KIND_ONEOF;
            if (key == u8"anyOf") return CHOICE_KIND_ANYOF;
            if (key == u8"allOf") return CHOICE_KIND_ALLOF;
            // if (key == u8"enum") return CHOICE_KIND_ENUM;
            // if (key == u8"type") return CHOICE_KIND_TYPE;
            // if (key == u8"items") return CHOICE_KIND_ITEMS;
            // if (key == u8"definitions" || key == u8"$defs") return CHOICE_KIND_DEFINITIONS;
            return CHOICE_KIND_NONE;
        }

        constexpr std::u8string_view getPrimitiveName(OBJECT_TYPE type) {
            switch (type) {
                case JSON_STRING: return u8"string";
                case JSON_NUMBER: return u8"number";
                case JSON_INTEGER: return u8"integer";
                case JSON_BOOLEAN: return u8"boolean";
                case JSON_NULL: return u8"null";
                default: return u8"";
            }
        }

        template<size_t VCapacity, size_t ECapacity>
        constexpr std::u8string_view findType(const size_t u, std::array<size_t, VCapacity>& child_counts,
            const fixed_stack<std::tuple<size_t, jobject>, VCapacity>& vertices,
            const fixed_stack<std::tuple<size_t, size_t, EDGE_KIND, SCHEMA_CONTEXT>, ECapacity>& edges,
            const std::array<size_t, VCapacity>& edge_start_idx)
        {
            std::u8string_view type = u8"";
            size_t current_edge_idx = 0;
            while(current_edge_idx<child_counts[u]){
                size_t inner_edge = edge_start_idx[u] + current_edge_idx;
                size_t w = std::get<1>(edges[inner_edge]);
                const auto& w_obj = std::get<1>(vertices[w]);
                if(w_obj.key == u8"type"){
                    type=w_obj.value;
                    break;
                }
                current_edge_idx++;
            }
            return type;
        };

        template<size_t VCapacity, size_t ECapacity>
        constexpr void findRequiredArray(const size_t u, std::array<size_t, VCapacity>& child_counts,
            const fixed_stack<std::tuple<size_t, jobject>, VCapacity>& vertices,
            const fixed_stack<std::tuple<size_t, size_t, EDGE_KIND, SCHEMA_CONTEXT>, ECapacity>& edges,
            const std::array<size_t, VCapacity>& edge_start_idx, fixed_stack<std::u8string_view, 128>& required_array)
        {
            size_t current_edge_idx = 0;
            while(current_edge_idx<child_counts[u]){
                size_t inner_edge = edge_start_idx[u] + current_edge_idx;
                size_t w = std::get<1>(edges[inner_edge]);
                const auto& w_obj = std::get<1>(vertices[w]);
                if(std::get<2>(edges[inner_edge]) == EDGE_KIND_REQUIRED){
                    required_array.emplace(w_obj.value);
                }
                current_edge_idx++;
            }
        };

        template<size_t VCapacity, size_t ECapacity>
        constexpr void findEnumerationArray(const size_t u, std::array<size_t, VCapacity>& child_counts,
            const fixed_stack<std::tuple<size_t, jobject>, VCapacity>& vertices,
            const fixed_stack<std::tuple<size_t, size_t, EDGE_KIND, SCHEMA_CONTEXT>, ECapacity>& edges,
            const std::array<size_t, VCapacity>& edge_start_idx, fixed_stack<std::u8string_view, 128>& enumerated_array)
        {
            size_t current_edge_idx = 0;
            while(current_edge_idx<child_counts[u]){
                size_t inner_edge = edge_start_idx[u] + current_edge_idx;
                size_t w = std::get<1>(edges[inner_edge]);
                const auto& w_obj = std::get<1>(vertices[w]);
                if(std::get<2>(edges[inner_edge]) == EDGE_KIND_ENUM){
                    enumerated_array.emplace(w_obj.value);
                }
                current_edge_idx++;
            }
        };

        template<size_t VCapacity, size_t ECapacity>
        constexpr void findPrefixItemsArray(const size_t u, std::array<size_t, VCapacity>& child_counts,
            const fixed_stack<std::tuple<size_t, jobject>, VCapacity>& vertices,
            const fixed_stack<std::tuple<size_t, size_t, EDGE_KIND, SCHEMA_CONTEXT>, ECapacity>& edges,
            const std::array<size_t, VCapacity>& edge_start_idx, fixed_stack<std::u8string_view, 128>& prefix_items_array)
        {
            size_t current_edge_idx = 0;
            while(current_edge_idx<child_counts[u]){
                size_t inner_edge = edge_start_idx[u] + current_edge_idx;
                size_t v = std::get<1>(edges[inner_edge]);
                const auto& v_obj = std::get<1>(vertices[v]);
                if(std::get<2>(edges[inner_edge]) == EDGE_KIND_PREFIX_ITEMS){
                    size_t current_edge_v_idx = 0;
                    while(current_edge_v_idx<child_counts[v]){
                        size_t inner_v_edge = edge_start_idx[v] + current_edge_v_idx;
                        size_t v = std::get<1>(edges[inner_v_edge]);
                        const auto& w_obj = std::get<1>(vertices[v]);
                        prefix_items_array.emplace(w_obj.value);
                        current_edge_v_idx++;
                    }
                }
                current_edge_idx++;
            }
        };

        template<size_t VCapacity>
        constexpr size_t resolve_ref_pointer(
            const fixed_stack<std::tuple<size_t, jobject>, VCapacity>& vertices,
            std::u8string_view ref_path
        ) {
            // Stripping the leading #/ if present
            if (ref_path.starts_with(u8"#/")) {
                ref_path.remove_prefix(2);
            }

            // A standard JSON pointer uses '/' as a structural delimiter.
            // For flat schema architectures, the final segment is typically the target key.
            size_t last_slash = ref_path.find_last_of('/');
            std::u8string_view target_key = (last_slash == std::u8string_view::npos)
                                        ? ref_path 
                                        : ref_path.substr(last_slash + 1);

            // Scan vertices to find the matching definition anchor node
            for (size_t i = 0; i < vertices.size(); ++i) {
                const auto& obj = std::get<1>(vertices[i]);
                if (obj.key == target_key) {
                    return obj.id; // Return matching vertex index
                }
            }

            return 0; // Fallback to root if unresolvable
        };

        constexpr std::string_view sanitize_rule_name(std::u8string_view u8key) {
            // Cast the u8 string view to a standard string view for manipulation
            std::string_view key(reinterpret_cast<const char*>(u8key.data()), u8key.size());
            if (key.starts_with("#/")) {
                size_t last_slash = key.find_last_of('/');
                if (last_slash != std::string_view::npos) {
                    return key.substr(last_slash + 1);
                }
            }
            return key;
        }

        template<size_t MaxCapacity>
        constexpr void emit_size_t_as_string(fixed_accumulator<MaxCapacity>& acc, size_t value) {
            if (value == 0) {
                acc.push_back('0');
                return;
            }

            // 1. A maximum 64-bit integer takes at most 20 digits. 
            // Allocate a small, fixed array buffer on the compile-time stack frame.
            char temp_digits[20]{};
            size_t digit_count = 0;

            // 2. Extract digits backward from right to left using modulo arithmetic
            while (value > 0) {
                temp_digits[digit_count++] = static_cast<char>('0' + (value % 10));
                value /= 10;
            }

            // 3. Push the characters into the accumulator in the correct forward order
            for (size_t i = digit_count; i > 0; --i) {
                acc.push_back(temp_digits[i - 1]);
            }
        }
    };
}