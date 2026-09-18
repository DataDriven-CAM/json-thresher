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
        std::u8string_view key{};
        std::u8string_view value{};
        size_t depth=0;
        bool is_choice_branch = false; // Flag to indicate if this node is an alternation sibling
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

    struct RawEdge {
         size_t source;
         size_t destination;
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
                    parentStack.push(std::get<1>(vertices.back()).id);
                    hitColon=false;
                    hitComma=false;
                    cursor+=4;
                }
                else if(jsonBuffer[cursor]=='t' && cursor<jsonBuffer.size()-4 && jsonBuffer.substr(cursor, 4)==u8"true"){
                    size_t idx1 = vertices.size();
                    vertices.emplace(idx1, jobject{.obj_type=JSON_BOOLEAN, .id=idx1, .parent_id=parentStack.back(), .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(cursor, 4), .depth=parentStack.size()});
                    // edges.push(std::tuple<size_t, size_t, int>{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id, 1});
                    raw_edges.push(RawEdge{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id});
                    child_counts[std::get<1>(vertices.back()).parent_id]++;
                    parentStack.push(std::get<1>(vertices.back()).id);
                    hitColon=false;
                    hitComma=false;
                    cursor+=4;
                }
                else if(jsonBuffer[cursor]=='f' && cursor<jsonBuffer.size()-5 && jsonBuffer.substr(cursor, 5)==u8"false"){
                    size_t idx1 = vertices.size();
                    vertices.emplace(idx1, jobject{.obj_type=JSON_BOOLEAN, .id=idx1, .parent_id=parentStack.back(), .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(cursor, 5), .depth=parentStack.size()});
                    // edges.push(std::tuple<size_t, size_t, int>{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id, 1});
                    raw_edges.push(RawEdge{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id});
                    child_counts[std::get<1>(vertices.back()).parent_id]++;
                    parentStack.push(std::get<1>(vertices.back()).id);
                    hitColon=false;
                    hitComma=false;
                    cursor+=5;
                }
                else if(jsonBuffer[cursor]=='{'){
                    size_t idx1 = vertices.size();
                    vertices.emplace(idx1, jobject{.obj_type=JSON_OBJECT, .id=idx1, .parent_id=parentStack.empty() ? 0 : parentStack.back(), .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(cursor, 1), .depth=parentStack.size()});
                    if(vertices.size()>1){
                        // edges.push(std::tuple<size_t, size_t, int>{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id, 1});
                        raw_edges.push(RawEdge{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id});
                        child_counts[std::get<1>(vertices.back()).parent_id]++;
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
                    vertices.emplace(idx1, jobject{.obj_type=JSON_ARRAY, .id=idx1, .parent_id=parentStack.empty() ? 0 : parentStack.back(), .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(cursor, 1), .depth=parentStack.size()});
                    if(vertices.size()>1){
                        // edges.push(std::tuple<size_t, size_t, int>{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id, 1});
                        raw_edges.push(RawEdge{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id});
                        child_counts[std::get<1>(vertices.back()).parent_id]++;
                    }
                    parentStack.push(std::get<1>(vertices.back()).id);
                    std::u8string_view& key=std::get<1>(vertices[std::get<1>(vertices.back()).parent_id]).key;
                    if(key==u8"oneOf" || key==u8"anyOf" || key==u8"enum" || key==u8"type")
                        std::get<1>(vertices[std::get<1>(vertices.back()).parent_id]).is_choice_branch=true;
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
                        vertices.emplace(idx1, jobject{.obj_type=JSON_STRING, .id=vertices.size(), .parent_id=parentStack.back(), .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(startOffset, cursor - startOffset), .depth=parentStack.size()});
                        // edges.push(std::tuple<size_t, size_t, int>{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id, 1});
                        raw_edges.push(RawEdge{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id});
                        child_counts[std::get<1>(vertices.back()).parent_id]++;
                    }
                    else if(!hitColon){
                        keyStart=startOffset;
                        keyEnd=cursor;
                    }
                    else{
                        size_t idx1 = vertices.size();
                        vertices.emplace(idx1, jobject{.obj_type=PAIR_VALUE, .id=vertices.size(), .parent_id=parentStack.back(), .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(startOffset, cursor - startOffset), .depth=parentStack.size()});
                        // edges.push(std::tuple<size_t, size_t, int>{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id, 1});
                        raw_edges.push(RawEdge{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id});
                        child_counts[std::get<1>(vertices.back()).parent_id]++;
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
                        // edges.push(std::tuple<size_t, size_t, int>{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id, 1});
                        raw_edges.push(RawEdge{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id});
                        child_counts[std::get<1>(vertices.back()).parent_id]++;
                        // parentStack.push_back(vertices.back().id);
                        // if(maxStackSize<parentStack.size())maxStackSize=parentStack.size();
                    }
                    else{
                        size_t idx1 = vertices.size();
                        vertices.emplace(idx1, jobject{.obj_type=JSON_NUMBER, .id=idx1, .parent_id=parentStack.back(), .key=jsonBuffer.substr(keyStart, keyEnd - keyStart), .value=jsonBuffer.substr(itStart, cursor - itStart), .depth=parentStack.size()});
                        // edges.push(std::tuple<size_t, size_t, int>{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id, 1});
                        raw_edges.push(RawEdge{std::get<1>(vertices.back()).parent_id, std::get<1>(vertices.back()).id});
                        child_counts[std::get<1>(vertices.back()).parent_id]++;
                        // parentStack.push_back(vertices.back().id);
                        // if(maxStackSize<parentStack.size())maxStackSize=parentStack.size();
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
                edges[target_slot] = std::tuple<size_t, size_t, int>{src, dest, 1};
            }
            // 1. Storage bounds matching your structural limits
            std::array<size_t, 4096> edge_start_idx{};
            size_t sum = 0;
            for (size_t i = 0; i < vertices.size(); ++i) {
                edge_start_idx[i] = sum;
                sum += child_counts[i];
            }

            fixed_accumulator<8193> gbnfAcc{};
            gbnfAcc.append("# --- METAPHRASED LLAMA COMPATIBLE GBNF GRAMMAR ---\n");
            gbnfAcc.append("# Generated automatically from source JSON Schema definitions\n\n");
            traverse_gbnf_graph(vertices, edges, edge_start_idx, child_counts, gbnfAcc);
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
            fixed_accumulator<8193>& gbnfAcc
        ) {
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
                size_t start = edge_start_idx[u];
                size_t total_children = child_counts[u];

                const auto& u_obj = std::get<1>(vertices[u]);

                if (state.current_edge_idx == 0 && !u_obj.is_choice_branch) {
                    if (!is_schema_keyword(u_obj.key)) {
                        // Emit rule name declaration
                        if (u == 0 || u_obj.parent_id == 0) gbnfAcc.append("root ::= ");
                        else { gbnfAcc.append(u_obj.key); gbnfAcc.append("_rule ::= "); }

                        // Rule Enclosures open with flexible spacing rules
                        if (u_obj.obj_type == JSON_OBJECT) {
                            gbnfAcc.append("\"{\" ws ");
                        } else if (u_obj.obj_type == JSON_ARRAY) {
                            gbnfAcc.append("\"[\" ws ");
                        }
                    }
                }
               // Find the next outgoing edge from vertex 'u' starting from state.current_edge_idx
                bool edge_found = false;
                size_t target_v = 0;

                if (state.current_edge_idx < total_children) {
                    size_t actual_edge_pos = start + state.current_edge_idx;
                    
                    target_v = std::get<1>(edges[actual_edge_pos]);
                    if (u_obj.is_choice_branch) {
                        // If this is not the first array item, separate them with a clean GBNF pipe
                        if (state.current_edge_idx > 0) {
                            gbnfAcc.append(" | ");
                        }
                    }
                    state.current_edge_idx++; // Move to next sibling for backtracking
                    edge_found = true;
                }

                if (edge_found) {
                    const auto& v_obj = std::get<1>(vertices[target_v]);
                    state.current_edge_idx++; // Move to next sibling

                    // 1. If the parent is a "oneOf" router container node, handle choice alternation branches
                    if (u_obj.key == u8"oneOf" || u_obj.key == u8"anyOf") {
                        if (state.current_edge_idx > 1) {
                            gbnfAcc.append(" | "); // Alternation separator
                        }
                        // Link directly to the target vertex branch rule name
                        gbnfAcc.append("branch_");
                        emit_size_t_as_string(gbnfAcc, target_v);
                        
                        // Push child to stack to explore its inner properties
                        if (!visited[target_v]) {
                            visited[target_v] = true;
                            traversal_stack.push(DFSState{target_v, 0});
                        }
                        continue;
                    }

                    // 2. Ignore structural keywords when evaluating sequence fields
                    if (is_schema_keyword(v_obj.key)) {
                        // Transparent routing node! Step straight down into its child graph values
                        if (!visited[target_v]) {
                            visited[target_v] = true;
                            traversal_stack.push(DFSState{target_v, 0});
                        }
                        continue;
                    }

                    // 3. Normal Data Fields: Handle comma spacing for sequential object keys
                    if (printed_fields[u] > 0 && !is_schema_keyword(u_obj.key)) {
                        gbnfAcc.append("\",\" ws ");
                    }
                    printed_fields[u]++;

                    // Format raw structural tokens for user object keys
                    if (u_obj.obj_type == JSON_OBJECT) {
                        gbnfAcc.append("\"\\\"");
                        gbnfAcc.append(v_obj.key);
                        gbnfAcc.append("\\\"\" ws \":\" ws ");
                    }

                    // Map data values to global rules
                    if (v_obj.obj_type == JSON_STRING) gbnfAcc.append("string ws ");
                    else if (v_obj.obj_type == JSON_NUMBER) gbnfAcc.append("number ws ");
                    else if (v_obj.obj_type == JSON_BOOLEAN) gbnfAcc.append("boolean ws ");
                    else {
                        // Nested sub-object target rule call lookup link
                        gbnfAcc.append(v_obj.key);
                        gbnfAcc.append("_rule ws ");
                        
                        if (!visited[target_v]) {
                            visited[target_v] = true;
                            traversal_stack.push(DFSState{target_v, 0});
                        }
                    }
                } 
                // --- FIX C: EXITING A NODE ---
                else {
                    if (!is_schema_keyword(u_obj.key)) {
                        if (u_obj.obj_type == JSON_OBJECT) gbnfAcc.append("\"}\"");
                        else if (u_obj.obj_type == JSON_ARRAY) gbnfAcc.append("\"]\"");
                        gbnfAcc.append("\n");
                    }
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