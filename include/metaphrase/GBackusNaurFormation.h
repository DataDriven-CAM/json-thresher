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
#include <any>
#include <iostream>
#include <functional>
#include <ranges>
#include <vector>
#include <map>
#include <typeindex>
#include <cmath>
#include <stack>
#include <algorithm>

#include "graph/container/dynamic_graph.hpp"
#include <graph/container/traits/vov_graph_traits.hpp>
#include "graph/views/incidence.hpp"
#include "graph/views/vertexlist.hpp"

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
                (*this)[current_size++] = val;
            }
        }

        template<typename... Args>
        constexpr void emplace(Args&&... args) {
            if (current_size >= Capacity) {
                throw std::out_of_range("fixed_stack overflow");
            }
            // Construct the element perfectly in place
            (*this)[current_size++] = T(std::forward<Args>(args)...);
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
    };

// Layout descriptors
    enum OBJECT_TYPE{
        JSON_ROOT,
        JSON_OBJECT,
        JSON_ARRAY,
        JSON_STRING,
        JSON_NUMBER,
        VALUE,
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
        bool is_choice_node = false; // Flag to indicate if this node is syntax for alternation children
    };

    using G = graph::container::dynamic_adjacency_graph<graph::container::vov_graph_traits<int, jobject>>;

    consteval int gbnfBuffer(std::u8string_view jsonBuffer){
        G dagGraph;
        fixed_stack<graph::copyable_vertex_t<size_t, jobject>, 4096> vertices;
        fixed_stack<graph::copyable_edge_t<size_t, int>, 4095> edges;
        fixed_stack<size_t, 64> parentStack;

        std::span s={jsonBuffer};
        std::span<const char8_t>::iterator it=s.begin();
        size_t keyStart=0;
        size_t keyEnd=0;
        bool inString = false;
        while(it!=s.end()){
            if (*it == '"' && (it == s.begin() || *std::prev(it) != '\\')) {
                if(inString){
                    keyEnd=it-s.begin();
                }
                else{
                    keyStart=it-s.begin();
                }
                inString = !inString;
                it++;
                continue;
            }
            if(*it=='{'){
                vertices.push(graph::copyable_vertex_t<size_t, jobject>{vertices.size(), jobject{.id=vertices.size(), .parent_id=parentStack.back(), .key=std::u8string_view(s.begin()+keyStart, s.begin()+keyEnd), .value=std::u8string_view(it, it+1), .depth=parentStack.size()}});
                if(vertices.size()>1)edges.push(graph::copyable_edge_t<size_t, int>{vertices.back().value.parent_id, vertices.back().value.id, 1});
                parentStack.push(vertices.back().id);

            }
            else if(*it=='}'){
                parentStack.pop();
            }
            it++;
        }
        // dagGraph.reserve_vertices(vertices.size());
        // dagGraph.load_vertices(vertices, std::identity{});//, [&](const auto& nm) -> graph::copyable_vertex_t<size_t, sylvanmats::io::json::jobject> {
        // dagGraph.reserve_edges(edges.size());
        // dagGraph.load_edges(edges, std::identity{});//[](const auto& c) -> graph::copyable_edge_t<graph::vertex_id_t<G>, int> { return {std::get<0>(c), std::get<1>(c), std::get<2>(c)}; });
        // std::string gbnf;
        // for(size_t i=0;i<objCount;i++){
        //     gbnf.append(sylvanmats::metaphrase::make_gbnf_rule<"node", R"("{ \"id\": " string ", \"type\": " type ", \"title\": " string ", \"container-title\": " string ", \"issued\": " dateBlock ", \"author\": " authorList " }")">());
        // }
        return vertices.size();
    }

template<typename T, size_t Capacity>
struct fixed_pool {
    T data[Capacity]{};
    size_t count = 0;
};

// Generates perfectly matching rule indentation entirely inside compile-time boundaries
constexpr auto make_indent(size_t depth) {
    // 1. Create a baseline master string containing our maximum expected indentation size
    constexpr auto master_spaces = fixed_string("                                                            "); // 60 spaces
    
    // 2. Compute exact required string length bounds
    size_t count = std::min(depth * 2, size_t(60));
    
    // 3. Return an explicitly bound string slice using the exact constructor signature
    return fixed_string<61>(master_spaces.data, count);
}

// Explicit master slice-appender that accepts dynamic lengths safely
template<size_t MaxCapacity = 4096>
struct fixed_accumulator {
    char data_buffer[MaxCapacity]{};
    size_t current_len = 0;

    constexpr fixed_accumulator() = default;

    constexpr void append(std::string_view sv) {
        // Enforce boundary space for the incoming data and the null terminator
        if (current_len + sv.length() + 1 <= MaxCapacity) {
            std::copy_n(sv.data(), sv.length(), data_buffer + current_len);
            current_len += sv.length();
            data_buffer[current_len] = '\0';
        } else {
            // Triggers a hard compilation error if hit during constexpr evaluation
            throw std::out_of_range("fixed_accumulator overflow on append");
        }
    }

    constexpr void append(std::u8string_view u8sv) {
        if (current_len + u8sv.length() + 1 <= MaxCapacity) {
            for (size_t i = 0; i < u8sv.length(); ++i) {
                // static_cast from char8_t to char is fully permitted in constexpr
                data_buffer[current_len + i] = static_cast<char>(u8sv[i]);
            }
            current_len += u8sv.length();
            data_buffer[current_len] = '\0';
        } else {
            throw std::out_of_range("fixed_accumulator overflow on u8 append");
        }
    }

    constexpr void push_back(char c) {
        if (current_len + 2 <= MaxCapacity) {
            data_buffer[current_len++] = c;
            data_buffer[current_len] = '\0';
        } else {
            throw std::out_of_range("fixed_accumulator overflow on push_back");
        }
    }

    constexpr const char* data() const { return data_buffer; }
    constexpr size_t size() const { return current_len; }
    constexpr std::string_view view() const { return std::string_view(data_buffer, current_len); }
};

enum EDGE_KIND{
    EDGE_KIND_AST=1,
    EDGE_KIND_AST_CHOICE=2,
    EDGE_KIND_PROPERTIES=3,
    EDGE_KIND_ADDITIONAL_PROPERTIES=4,
    EDGE_KIND_DEFINITIONS=5,
    EDGE_KIND_REQUIRED=6,
};

enum CHOICE_KIND{
    CHOICE_KIND_NONE=0,
    CHOICE_KIND_ONEOF=1,
    CHOICE_KIND_ANYOF=2,
    CHOICE_KIND_ALLOF=3,
    CHOICE_KIND_ENUM=4,
    CHOICE_KIND_TYPE=5,
};

    struct RawEdge {
         size_t source;
         size_t destination;
         EDGE_KIND edge_kind=EDGE_KIND_AST;
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
        constexpr fixed_string<8193> operator ()(std::u8string_view jsonBuffer){
            fixed_stack<std::tuple<size_t, jobject>, 4096> vertices;
            fixed_stack<std::tuple<size_t, size_t, int>, 4095> edges;
            fixed_stack<size_t, 64> parentStack;
            fixed_stack<RawEdge, 4095> raw_edges;

            std::array<size_t, 4096> child_counts{};
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
                    vertices.emplace(idx1, jobject{.obj_type=JSON_NULL, .id=idx1, .parent_id=parentStack.back(), .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(cursor, 1), .depth=parentStack.size()});
                    if(vertices.size()>1){
                        // edges.push(std::tuple<size_t, size_t, int>{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id, 1});
                        raw_edges.push(RawEdge{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id});
                        child_counts[std::get<1>(vertices.back()).parent_id]++;
                    }
                    hitColon=false;
                    hitComma=false;
                    cursor+=4;
                    continue;
                }
                else if(jsonBuffer[cursor]=='t' && cursor<jsonBuffer.size()-4 && jsonBuffer.substr(cursor, 4)==u8"true"){
                    size_t idx1 = vertices.size();
                    std::u8string_view current_key = (keyEnd > keyStart) ? jsonBuffer.substr(keyStart, keyEnd - keyStart) : u8"";
                    size_t parent_id=parentStack.empty() ? 0 : parentStack.back();
                    bool choice_flag=getChoiceSyntax(current_key)>CHOICE_KIND_NONE;
                    size_t syntax_id=(choice_flag) ? parent_id : 0;
                    EDGE_KIND edge_kind=getSchemaSyntax(current_key);
                    if(hasParentalSyntax(vertices, parentStack)){
                        parent_id=parentStack[parentStack.size()-2];
                    }
                    vertices.emplace(idx1, jobject{.obj_type=JSON_BOOLEAN, .id=idx1, .parent_id=parent_id, .syntax_id=syntax_id, .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(cursor, 4), .depth=parentStack.size()});
                    // edges.push(std::tuple<size_t, size_t, int>{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id, 1});
                    raw_edges.push(RawEdge{parent_id, std::get<1>(vertices.back()).id});
                    child_counts[parent_id]++;
                    hitColon=false;
                    hitComma=false;
                    cursor+=4;
                    continue;
                }
                else if(jsonBuffer[cursor]=='f' && cursor<jsonBuffer.size()-5 && jsonBuffer.substr(cursor, 5)==u8"false"){
                    size_t idx1 = vertices.size();
                    std::u8string_view current_key = (keyEnd > keyStart) ? jsonBuffer.substr(keyStart, keyEnd - keyStart) : u8"";
                    size_t parent_id=parentStack.empty() ? 0 : parentStack.back();
                    bool choice_flag=getChoiceSyntax(current_key)>CHOICE_KIND_NONE;
                    size_t syntax_id=(choice_flag) ? parent_id : 0;
                    EDGE_KIND edge_kind=getSchemaSyntax(current_key);
                    if(hasParentalSyntax(vertices, parentStack)){
                        parent_id=parentStack[parentStack.size()-2];
                    }
                    vertices.emplace(idx1, jobject{.obj_type=JSON_BOOLEAN, .id=idx1, .parent_id=parent_id, .syntax_id=syntax_id, .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(cursor, 5), .depth=parentStack.size()});
                    // edges.push(std::tuple<size_t, size_t, int>{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id, 1});
                    raw_edges.push(RawEdge{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id, EDGE_KIND_AST});
                    child_counts[parent_id]++;
                    hitColon=false;
                    hitComma=false;
                    cursor+=5;
                    continue;
                }
                else if(jsonBuffer[cursor]=='{'){
                    size_t idx1 = vertices.size();
                    std::u8string_view current_key = (keyEnd > keyStart) ? jsonBuffer.substr(keyStart, keyEnd - keyStart) : u8"";
                    if(firstObject){
                        firstObject=false;
                        current_key=u8"root";
                    }
                    // Check the key immediately to set the flag inline
                    bool choice_flag=getChoiceSyntax(current_key)>CHOICE_KIND_NONE;
                    size_t parent_id=parentStack.empty() ? 0 : parentStack.back();
                    size_t syntax_id=(choice_flag) ? parent_id : 0;
                    EDGE_KIND edge_kind=getSchemaSyntax(current_key);
                    if(hasParentalSyntax(vertices, parentStack)){
                        parent_id=parentStack[parentStack.size()-2];
                    }
                    gbnfAcc.append("# Object ");
                    gbnfAcc.append(current_key);
                    gbnfAcc.append(" ");
                    gbnfAcc.append(jsonBuffer.substr(cursor, 1));
                    gbnfAcc.append(" ");
                    gbnfAcc.append(std::to_string(idx1));
                    gbnfAcc.append(" ");
                    gbnfAcc.append(std::to_string(parent_id));
                    // gbnfAcc.append("\n");
                    vertices.emplace(idx1, jobject{.obj_type=JSON_OBJECT, .id=idx1, .parent_id=parent_id, .syntax_id=syntax_id, .key=current_key, .value=jsonBuffer.substr(cursor, 1), .depth=parentStack.size(), .is_choice_node=choice_flag});
                    keyStart = 0;
                    keyEnd = 0;
                   if(vertices.size()>1 && edge_kind==EDGE_KIND_AST){
                        raw_edges.push(RawEdge{parent_id, std::get<1>(vertices.back()).id, edge_kind});
                        child_counts[parent_id]++;
                        gbnfAcc.append(" # Edge ");
                        gbnfAcc.append(std::get<1>(vertices.back()).key);
                        gbnfAcc.append(" ");
                        gbnfAcc.append(std::to_string(std::get<1>(vertices.back()).id));
                        gbnfAcc.append(" ");
                        gbnfAcc.append(std::to_string(parent_id));
                        gbnfAcc.append(" ");
                        gbnfAcc.append(std::to_string(edge_kind));
                        gbnfAcc.append(" ");
                        gbnfAcc.append(std::to_string(child_counts[parent_id]));
                        gbnfAcc.append("\n");
                    }
                    else {
                        gbnfAcc.append(" # no edge \n");
                    }
                    parentStack.push(std::get<1>(vertices.back()).id);
                    hitColon=false;
                    hitComma=false;
                }
                else if(jsonBuffer[cursor]=='}'){
                    parentStack.pop();
                }
                if(jsonBuffer[cursor]=='['){
                   size_t idx1 = vertices.size();
                    std::u8string_view current_key = (keyEnd > keyStart) ? jsonBuffer.substr(keyStart, keyEnd - keyStart) : u8"";
                    
                    // Check the key immediately to set the flag inline
                    bool choice_flag = getChoiceSyntax(current_key)>CHOICE_KIND_NONE;
                    size_t parent_id=parentStack.empty() ? 0 : parentStack.back();
                    size_t syntax_id=(choice_flag) ? parent_id : 0;
                    EDGE_KIND edge_kind=getSchemaSyntax(current_key);
                    if(hasParentalSyntax(vertices, parentStack)){
                        parent_id=parentStack[parentStack.size()-2];
                    }
                    gbnfAcc.append("# Array ");
                    gbnfAcc.append(current_key);
                    gbnfAcc.append(" ");
                    gbnfAcc.append(std::to_string(idx1));
                    gbnfAcc.append(" ");
                    gbnfAcc.append(jsonBuffer.substr(cursor, 1));
                    gbnfAcc.append(" ");
                    gbnfAcc.append(std::to_string(parent_id));
                    gbnfAcc.append("\n");
                    vertices.emplace(idx1, jobject{.obj_type=JSON_ARRAY, .id=idx1, .parent_id=parent_id, .syntax_id=syntax_id, .key=current_key, .value=jsonBuffer.substr(cursor, 1), .depth=parentStack.size(), .is_choice_node=choice_flag});
                    keyStart = 0;
                    keyEnd = 0;
                    if(vertices.size()>1 && edge_kind==EDGE_KIND_AST){
                        raw_edges.push(RawEdge{parent_id, std::get<1>(vertices.back()).id, edge_kind});
                        child_counts[parent_id]++;
                        gbnfAcc.append("# Edge ");
                        gbnfAcc.append(std::get<1>(vertices.back()).key);
                        gbnfAcc.append(" ");
                        gbnfAcc.append(std::to_string(parent_id));
                        gbnfAcc.append(" ");
                        gbnfAcc.append(std::to_string(std::get<1>(vertices.back()).id));
                        gbnfAcc.append(" ");
                        gbnfAcc.append(std::to_string(edge_kind));
                        gbnfAcc.append(" ");
                        gbnfAcc.append(std::to_string(child_counts[parent_id]));
                        gbnfAcc.append("\n");
                    }
                    else {
                        gbnfAcc.append("# no edge \n");
                    }
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
                        vertices.emplace(idx1, jobject{.obj_type=JSON_STRING, .id=vertices.size(), .parent_id=parent_id, .syntax_id=syntax_id, .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(startOffset, cursor - startOffset), .depth=parentStack.size()});
                        // edges.push(std::tuple<size_t, size_t, int>{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id, 1});
                        EDGE_KIND edge_kind=EDGE_KIND_AST;
                        if(std::get<1>(vertices[parentStack.back()]).is_choice_node){
                            edge_kind=EDGE_KIND_AST_CHOICE;
                        }
                        raw_edges.push(RawEdge{parent_id, std::get<1>(vertices.back()).id, edge_kind});
                        child_counts[parent_id]++;
                    }
                    else if(!hitColon){
                        keyStart=startOffset;
                        keyEnd=cursor;
                    }
                    else{
                        size_t idx1 = vertices.size();
                        std::u8string_view current_key = (keyEnd > keyStart) ? jsonBuffer.substr(keyStart, keyEnd - keyStart) : u8"";
                        size_t parent_id=parentStack.empty() ? 0 : parentStack.back();
                        EDGE_KIND edge_kind=getSchemaSyntax(current_key);
                        size_t syntax_id=(edge_kind==EDGE_KIND_PROPERTIES) ? parent_id : 0;
                        if(hasParentalSyntax(vertices, parentStack)){
                            parent_id=parentStack[parentStack.size()-2];
                        }
                        vertices.emplace(idx1, jobject{.obj_type=PAIR_VALUE, .id=vertices.size(), .parent_id=parent_id, .syntax_id=syntax_id, .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(startOffset, cursor - startOffset), .depth=parentStack.size()});
                        // edges.push(std::tuple<size_t, size_t, int>{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id, 1});
                        if(vertices.size()>1 && edge_kind==EDGE_KIND_AST){
                            raw_edges.push(RawEdge{parent_id, std::get<1>(vertices.back()).id, edge_kind});
                            child_counts[parent_id]++;
                        }
                        hitColon=false;
                        hitComma=false;
                    }
                    cursor++;
                    
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
                        EDGE_KIND edge_kind=EDGE_KIND_AST;
                        raw_edges.push(RawEdge{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id, edge_kind});
                        child_counts[std::get<1>(vertices.back()).parent_id]++;
                    }
                    else{
                        size_t idx1 = vertices.size();
                        vertices.emplace(idx1, jobject{.obj_type=JSON_NUMBER, .id=idx1, .parent_id=parentStack.back(), .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(itStart, cursor - itStart), .depth=parentStack.size()});
                        raw_edges.push(RawEdge{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id});
                        child_counts[std::get<1>(vertices.back()).parent_id]++;
                    }
                    hitColon=false;
                    hitComma=false;
                    
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
            
            std::array<size_t, 4096> edge_offsets{};
            size_t running_sum = 0;

            for (size_t i = 0; i < vertices.size(); ++i) {
                edge_offsets[i] = running_sum;
                running_sum += child_counts[i];
                // gbnfAcc.append("# Node ");
                // gbnfAcc.append(std::get<1>(vertices[i]).key);
                // gbnfAcc.append(" ");
                // gbnfAcc.append(std::get<1>(vertices[i]).value);
                // gbnfAcc.append(" ");
                // gbnfAcc.append(std::to_string(std::get<1>(vertices[i]).id));
                // gbnfAcc.append(" ");
                // gbnfAcc.append(std::to_string(running_sum));
                // gbnfAcc.append("\n");
            }

            // Set the final edge size bounds explicitly
            edges.current_size = raw_edges.size();

            // Populate the final edges table in a single linear pass.
            // Because we use the pre-calculated offsets, all edges share the same source 
            // are automatically written into a perfectly contiguous block!
            for (size_t i = 0; i < raw_edges.size(); ++i) {
                size_t src = raw_edges[i].source;
                size_t dest = raw_edges[i].destination;
                
                size_t target_slot = edge_offsets[src]++;
                edges[target_slot] = std::tuple<size_t, size_t, int>{src, dest, raw_edges[i].edge_kind};
            }
            // 1. Storage bounds matching your structural limits
            std::array<size_t, 4096> edge_start_idx{};
            size_t sum = 0;
            for (size_t i = 0; i < vertices.size(); ++i) {
                edge_start_idx[i] = sum;
                sum += child_counts[i];
            }

            std::array<fixed_accumulator<256>, 4096> vertex_rules{};
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
            
            gbnfAcc.append(
                "ws ::= [ \\t\\n\\r]*\n"
                "string ::= \"\\\"\" ([^\"])* \"\\\"\"\n"
                "number ::= [0-9]+ (\".\" [0-9]+)?\n"
                "boolean ::= \"true\" | \"false\"\n"
                "null ::= \"null\"\n\n\0"
            );
            return sylvanmats::metaphrase::fixed_string<8193>(gbnfAcc.data(), gbnfAcc.size());
        }

    private:
        template<size_t VCapacity, size_t ECapacity>
        constexpr void traverse_gbnf_graph(
            const fixed_stack<std::tuple<size_t, jobject>, VCapacity>& vertices,
            const fixed_stack<std::tuple<size_t, size_t, int>, ECapacity>& edges,
            const std::array<size_t, VCapacity>& edge_start_idx,
            std::array<size_t, 4096>& child_counts,
            std::array<fixed_accumulator<256>, 4096>& vertex_rules
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

            std::array<size_t, 4096> printed_fields{};
            printed_fields.fill(0);
            
            while (!traversal_stack.empty()) {
                auto& state = traversal_stack.back();
                size_t u = state.vertex_id;
                const auto& u_obj = std::get<1>(vertices[u]);

                if(u_obj.key==u8"properties" || u_obj.key==u8"additionalProperties" || u_obj.key==u8"definitions" || u_obj.key==u8"required" || u_obj.is_choice_node){
                    vertex_rules[u].append("\n# ");
                    vertex_rules[u].append(u_obj.key);
                    vertex_rules[u].append(" should not be connected to anything\n");
                    if (!visited[u]) {
                    visited[u] = true;
                    }
                    traversal_stack.pop();
                    continue;
                }
                // 1. Declare the rule name on entry
                if (state.current_edge_idx == 0) {
                    vertex_rules[u].append(u_obj.key);
                    vertex_rules[u].append("_rule ::= ");
                    if (u_obj.obj_type == JSON_OBJECT) vertex_rules[u].append("\"{\" ws ");
                }

                // 2. Linear traversal over perfectly mapped edges
                if (state.current_edge_idx < child_counts[u]) {
                    size_t next_edge = edge_start_idx[u] + state.current_edge_idx++;
                    size_t v = std::get<1>(edges[next_edge]);
                    const auto& v_obj = std::get<1>(vertices[v]);

                    if (std::get<2>(edges[next_edge]) == EDGE_KIND_AST_CHOICE) {
                        if (state.current_edge_idx > 1) vertex_rules[u].append(" | ");
                        vertex_rules[u].append(v_obj.key);
                        vertex_rules[u].append("_rule");
                    } else {
                        if (state.current_edge_idx > 1) vertex_rules[u].append(" \",\" ws ");
                        // Emit normal sequential keys and reference links...
                        vertex_rules[u].append(v_obj.key);
                    }
                    
                    if (!visited[v]) {
                        visited[v] = true;
                        traversal_stack.push(DFSState{v, 0});
                    }
                } else {
                    // 3. Close rule on exit
                    if (u_obj.obj_type == JSON_OBJECT) vertex_rules[u].append("\"}\"");
                    vertex_rules[u].append("\n");
                    traversal_stack.pop();
                }
            }
        };

        // A simple constexpr flag helper to identify JSON Schema keywords
        constexpr bool is_schema_keyword(std::u8string_view key) {
            return key == u8"properties" || key == u8"oneOf" || key == u8"anyOf" || 
                key == u8"items"      || key == u8"required" || key == u8"additionalProperties" ||
                key == u8"$schema"    || key == u8"$id"       || key == u8"title" || key == u8"type";
        }    

        template<size_t VCapacity>
        constexpr bool hasParentalSyntax(const fixed_stack<std::tuple<size_t, jobject>, VCapacity>& vertices, fixed_stack<size_t, 64>& parentStack){
            if(parentStack.size()>=2 && (getSchemaSyntax(std::get<1>(vertices[parentStack.back()]).key)>EDGE_KIND_AST_CHOICE || getChoiceSyntax(std::get<1>(vertices[parentStack.back()]).key)>CHOICE_KIND_NONE))
                return true;
            else return false;
        }

        constexpr EDGE_KIND getSchemaSyntax(std::u8string_view key) {
            if (key == u8"properties") return EDGE_KIND_PROPERTIES;
            if (key == u8"additionalProperties") return EDGE_KIND_ADDITIONAL_PROPERTIES;
            if (key == u8"definitions") return EDGE_KIND_DEFINITIONS;
            if (key == u8"required") return EDGE_KIND_REQUIRED;
            if (getChoiceSyntax(key)!=CHOICE_KIND_NONE) return EDGE_KIND_AST_CHOICE;
            return EDGE_KIND_AST;
        }

        constexpr CHOICE_KIND getChoiceSyntax(std::u8string_view key) {
            if (key == u8"oneOf") return CHOICE_KIND_ONEOF;
            if (key == u8"anyOf") return CHOICE_KIND_ANYOF;
            if (key == u8"allOf") return CHOICE_KIND_ALLOF;
            if (key == u8"enum") return CHOICE_KIND_ENUM;
            if (key == u8"type") return CHOICE_KIND_TYPE;
            return CHOICE_KIND_NONE;
        }

        template<size_t VCapacity>
        constexpr size_t resolve_ref_pointer(
            std::u8string_view ref_path, 
            const fixed_stack<std::tuple<size_t, jobject>, VCapacity>& vertices
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