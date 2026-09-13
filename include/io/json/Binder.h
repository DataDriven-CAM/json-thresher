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

#include "io/json/Path.h"

#include "graph/container/dynamic_graph.hpp"
#include <graph/container/traits/vov_graph_traits.hpp>
#include "graph/views/dfs.hpp"
#include "graph/adaptors/filtered_graph.hpp" // Adaptor to prune branches
#include "graph/views/incidence.hpp"
#include "graph/views/vertexlist.hpp"
#include "graph/views/edgelist.hpp"


inline const std::string_view substr_view(const std::string& source, size_t offset = 0,
                std::string_view::size_type count = 
                std::numeric_limits<std::string_view::size_type>::max()) {
    if (offset < source.size()&& count>0) 
        return std::string_view(source.data() + offset, count - offset);
    return {};
}

namespace sylvanmats::io::json{

    enum OBJECT_TYPE{
        JSON_OBJECT,
        JSON_ARRAY,
        JSON_STRING,
        VALUE,
        PAIR_VALUE,
        VALUE_NULL
    };

using JsonValue = std::variant<std::monostate, std::string_view, const char*, double, int, unsigned int, long, unsigned long, long long, bool>;

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };

template <typename... Args>
concept IsJsonKeyValuePair = requires {
    // 1. Enforce that exactly 2 arguments are provided
    requires sizeof...(Args) == 2;
    
    // 2. C++26 Pack Indexing: Check individual types explicitly
    requires std::same_as<Args...[0], std::string_view>;
    requires std::same_as<Args...[1], JsonValue>;
};

    struct jobject{
        OBJECT_TYPE obj_type;
        size_t id=0;
        size_t parent_id=0;
        size_t key_index=0;
        mutable std::variant<std::monostate, std::string_view, const char*, double, int, unsigned int, long, unsigned long, long long, bool> value_index{std::monostate{}};
        size_t start=0;
        size_t end=0;
        size_t depth=0;
        std::string_view key{};
    };

    using G = graph::container::dynamic_adjacency_graph<graph::container::vov_graph_traits<int, sylvanmats::io::json::jobject>>;

    /**Binder
    *   @brief Binder class to bind a json to a graph
    */
    class Binder{
    private:
        std::unordered_map<std::type_index, std::string> type_names{{std::type_index(typeid(const char*)), "const char*"},
                                                                    {std::type_index(typeid(std::string_view)), "std::string_view"},
                                                                    {std::type_index(typeid(int)), "int"},
                                                                    {std::type_index(typeid(unsigned int)), "unsigned int"},
                                                                    {std::type_index(typeid(long)), "long"},
                                                                    {std::type_index(typeid(unsigned long)), "unsigned long"},
                                                                    {std::type_index(typeid(size_t)), "size_t"},
                                                                    {std::type_index(typeid(double)), "double"},
                                                                    {std::type_index(typeid(object)), "object"},
                                                                    {std::type_index(typeid(array)), "array"}};
        protected:
        std::string jsonContent="";
        G dagGraph;
        std::vector<graph::copyable_vertex_t<size_t, sylvanmats::io::json::jobject>> vertices;
        std::vector<graph::copyable_edge_t<size_t, int>> edges;
        std::vector<size_t> parentStack;
        size_t maxStackSize=0;

        size_t objectCount=0;
        
        double  matchTime=0.0;
        double  reductionTime=0.0;
        double  bindTime=0.0;
        int objDiff=0;
        size_t bindObjSize=0;
        bool singleMatch=false;
    public:
        Binder()=default;
        Binder(const Binder& orig) =  delete;
        virtual ~Binder()= default;
        
        void clear(){
            if(dagGraph.size()>0)dagGraph.clear();
            if(vertices.size()>0)vertices.clear();
            if(edges.size()>0)edges.clear();
            if(matchTime>0)matchTime=0;
            if(reductionTime>0)reductionTime=0;
            if(bindTime>0)bindTime=0;
            if(objDiff!=0)objDiff=0;
            if(bindObjSize>0)bindObjSize=0;
        }
        //Populate 
        void operator ()(std::istream& is);
        
        void operator ()(std::string& jsonContent);
        
        //add
        bool operator ()(Path& jp, std::string_view newKey, JsonValue newValue){
        if(!newKey.empty() && newKey.at(0)=='/')newKey=newKey.substr(1);
        if(jsonContent.empty()){
            jsonContent.append("{\n");
            jsonContent.append(typeset(false, false, 1, newKey, newValue));
            jsonContent.append("}\n");
//            std::cout<<(*this)<<std::endl;
            bind(0);
        }
        else if(jp.p.empty()){
            singleMatch=true;
            size_t insertionObjSize=(graph::num_vertices(dagGraph)>=2) ? graph::num_vertices(dagGraph)-2 : 1;
            size_t insertionOffset=findInsertionOffset(vertices[insertionObjSize].value.end);// : jsonContent.size() - 1;
            
//                std::cout<<dag.back().second.size()<<" "<<insertionOffset<<" indention: "<<dag.back().second.back().key<<" "<<dag.back().second.back().obj_type<<std::endl;
//                auto s=objects[objects.size()-2];
//                size_t insertionOffset=s.key_index+1;//findInsertionOffset(s.key_index);
                size_t indention=(graph::num_vertices(dagGraph)>=2) ?  vertices[insertionObjSize].value.depth : vertices[insertionObjSize].value.depth+1;
//                std::cout<<" "<<insertionOffset<<" indention2: "<<indention<<" "<<jsonContent.size()<<std::endl;
                std::string&& kv=typeset(true, true, indention, newKey, newValue);
//                std::cout<<"jp empty|"<<kv<<"|"<<std::endl;
                jsonContent.insert(insertionOffset, kv);
//                dag.resize(insertionObjSize+1);
//                depthList.resize(insertionObjSize+1);
//            bind(insertionOffset, depthList.back());
//                std::cout<<" indent "<<indention<<" insertionOffset "<<insertionOffset<<" total size: "<<jsonContent.size()<<" insertionObjSize "<<insertionObjSize<<" "<<dag.size()<<" "<<depthList.back()<<" start obj: "<<(dag.back().first.obj_type==START_OBJ)<<std::endl;
            shortenDAG(insertionOffset-1, insertionObjSize);
//                dag.clear();
//                depthList.clear();
//            bind(0);
            
        }
        else {
            // auto start = std::chrono::high_resolution_clock::now();
            size_t count=0;
            singleMatch=true;
            bool hit=match(jp, true, [&](size_t id, std::string_view key, const JsonValue& v)-> bool{
                // auto end = std::chrono::high_resolution_clock::now();
                count++;
                // matchTime=std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()*1.0e-9;
                size_t insertionOffset=0;
                size_t indention=0;
                bool comma=false;
                bool wrap=false;
                size_t insertionObjSize=id;
                if(id<vertices.size()-1 && (vertices[id+1].value.obj_type==JSON_OBJECT || vertices[id+1].value.obj_type==JSON_ARRAY)){
                    indention=vertices[id+1].value.depth+1;
                    if(vertices[id-1].value.obj_type==JSON_OBJECT || vertices[id-1].value.obj_type==JSON_ARRAY){
                        insertionObjSize=id-1;
                        insertionOffset=findInsertionOffset(vertices[insertionObjSize].value.end);
                        wrap=true;
                    }
                    else{
                        insertionObjSize=id-1;
                        insertionOffset=findInsertionOffset(vertices[insertionObjSize].value.end);
                       comma=true;
                        wrap=true;
                    }
                }
                else if(vertices[id].value.obj_type==PAIR_VALUE){
//                    std::cout<<"label "<<jp.p.back().label<<" "<<id<<" "<<dag.size()<<" "<<depthList.size()<<std::endl;
                    indention=vertices[id].value.depth;
                    insertionObjSize=id;
                       insertionOffset=findInsertionOffset(vertices[insertionObjSize].value.end);
                       comma=true;
                        wrap=true;
                }
                std::string&& kv=typeset(comma, wrap, indention, newKey, newValue);
                std::cout<<"|"<<kv<<"|"<<std::endl;
                jsonContent.insert(insertionOffset, kv);
            bindObjSize=insertionObjSize;
            shortenDAG(insertionOffset-1, insertionObjSize);
//                dag.clear();
//                depthList.clear();
//            bind(0);
                return true;
            });
            std::cout<<"match count "<<count<<std::endl;
        }
        return true;
    };
        void operator ()(Path& p, std::function<std::tuple<bool, std::string_view, JsonValue>(void)> apply);
        
        //remove
        bool operator ()(Path& jp, std::string keyToRemove);
        
        /**get
        *   @brief get a value from the json
        *   @code
        *   double value=0.0;
        *   sylvanmats::io::json::Path jp="geometry/*"_jp;
        *   jsonBinder(jp, [&value](std::any& v){
        *       value=std::any_cast<double>(v);
        *   });
        *   @param p the path to the value
        *   @param apply the callback function to apply to the value
        */
        template <typename Callback>
        requires std::invocable<Callback, const JsonValue&>
        void operator ()(json::Path& query_path, Callback&& cb){
            match(query_path, false, [&](size_t, std::string_view key, const JsonValue& val) {
                    cb(val); // Matches your single-argument get logic perfectly
                    return true;
                });
        };
        
        /**traverse
        *   @brief traverse the json
        *   @code
        *   sylvanmats::io::json::Path jp;
        *   jp["deDependencies"]["*"]["name"];
        *   jsonBinder(jp, [&](std::string_view& key, std::any& v){
        *       std::cout<<key<<" "<<std::any_cast<const char*>(v)<<std::endl;
        *   });
        *   @param p the path to the value
        *   @param apply the callback function to apply to the value
        */
        template <typename Callback>
        requires std::invocable<Callback, std::string_view, const JsonValue&>
        void operator ()(Path& p, Callback&& cb){
        bool hit=match(p, false, [&](size_t id, std::string_view key, const JsonValue& v)->bool{
            auto u=*find_vertex(dagGraph, id);
            for (auto&& oe : graph::adj_list::out_edges(dagGraph, u)) {
                auto oid=graph::target_id(dagGraph, oe);
                // std::cout<<vertices[oid].value.obj_type<<" "<<PAIR_VALUE<<" PAIR_VALUE "<<vertices[oid].value.key<<" "<<substr_view(jsonContent, vertices[oid].value.start, vertices[oid].value.end)<<" "<<std::endl;
                if(vertices[oid].value.obj_type==PAIR_VALUE || vertices[oid].value.obj_type==JSON_STRING || vertices[oid].value.obj_type==VALUE){
                    // std::string_view key=substr_view(jsonContent, vertices[oid].start, vertices[oid].end);
                    cb(vertices[oid].value.key, vertices[oid].value.value_index);
                }
            }
            return true;
            });
        };
        
        //traverse sibling
        template <typename Callback>
        requires std::invocable<Callback, std::string_view, JsonValue>
        void operator ()(Path& p, std::string_view sibling, Callback&& cb){
//            for(auto d : p.p){
//                for(auto s : objects | std::views::filter([&sibling](jobject& s){return s.key.compare(sibling)==0;})){
////                    std::cout<<std::get<OBJ_SIZE>(s)<<" here "<<s.id<<" "<<s.key<<std::endl;
//                    for(auto p : objects | std::views::filter([&s](jobject& p){return p.parent_index==s.id;})){
//                        //std::cout<<"\t"<<std::get<PAIR_DEPTH>(p)<<" "<<std::get<PAIR_KEY>(p)<<std::endl;
//                        apply(p.key, p.value_index);
//                    }
//                }
//            }
        }
        
        size_t countObjects(){
            objectCount=0;
            for(auto& n:  vertices)
                if(n.value.obj_type==JSON_OBJECT)objectCount++;
            return objectCount;
        };
        
        void display();
        
    protected:
        void shortenDAG(std::string::size_type insertionOffset, std::string::size_type offset);
        void bind(std::string::size_type offset, size_t depth=0);
        
        bool isNull(std::span<char>& s, std::span<char>::iterator& it);

        inline bool test(std::string_view pairValue, std::any& value){
            if(type_names[std::type_index(value.type())].compare("long")==0)
                return pairValue.compare(std::to_string(std::any_cast<long>(value)))==0;
            else
                return pairValue.compare(std::any_cast<std::string_view>(value))==0;
        };
        
        template <typename Callback>
        requires std::predicate<Callback, size_t, std::string_view, const JsonValue&>
        bool match(Path& jp, bool last, Callback&& cb){
        bool hit=false;
        size_t pi=0;
        int countMap=0;
        if(jp.p.empty()){
            std::string_view key=substr_view(jsonContent, vertices[0].value.start, vertices[0].value.end);
            if(cb(0ul, key, vertices[0].value.value_index))hit=true;
            return hit;
        }
        auto prunedGraph = graph::adaptors::filtered_graph(dagGraph, [&](auto vertexDescriptor) -> bool { // <-- VERTEX PREDICATE SLOT
                // 1. Get the vertex ID
                // auto vid = graph::vertex_id(dagGraph, vertexDescriptor);
                auto&& targetNode = vertices[vertexDescriptor].value;
                size_t currentDepth = targetNode.depth;

                std::cout << "prune vertex depth: " << currentDepth << " query size: " << jp.p.size() << std::endl;

                // Safety: If the JSON depth is deeper than our XPath query path, prune it
                if (currentDepth >= jp.p.size()) return false;

                auto&& expected_segment = jp.p[currentDepth];
                if (expected_segment.label == "*") return true;

                // If the token key doesn't match our active path level, return false 
                // to completely block the DFS from stepping onto this node or its children!
                return (targetNode.key == expected_segment.label);
            }, [&](const auto& edge) -> bool {
                        auto&& targetNode = vertices[graph::target(dagGraph, edge)].value;
                size_t currentDepth = targetNode.depth;

                std::cout<<"prune "<<currentDepth<<" "<<jp.p.size()<<std::endl;
                // Safety: If the JSON file nesting is deeper than our XPath query length, prune it
                if (currentDepth >= jp.p.size()) return false;

                // Fetch the corresponding query segment requirement for this depth
                auto&& expectedSegment = jp.p[currentDepth]; 

                // PRUNING RULES:
                // 1. If it's a wildcard segment (*), let all branches through
                // 2. If it's a named key segment, only allow edges matching the literal key string
                if (expectedSegment.label == "*") {
                    return true;
                }
                std::cout<<"match "<<targetNode.key<<" "<<expectedSegment.label<<std::endl;
                return (targetNode.key == expectedSegment.label);
            });
        auto rootVertex = graph::vertex_id_t<G>(0);
        auto dfs_range = graph::views::vertices_dfs(dagGraph, rootVertex);

        size_t max_valid_depth = 0;
        std::unordered_set<typename graph::vertex_id_t<G>> matched_ids;

        for (auto&& vertexDescriptor : dfs_range) {
            auto vid = graph::vertex_id(dagGraph, vertexDescriptor.vertex);
            // auto&& node = vertices[vid].value;
            
            if (vertices[vid].value.depth > max_valid_depth) {
                continue;
            }

            if (vertices[vid].value.depth >= jp.p.size()) {
                continue;
            }

            if (vertices[vid].value.depth > 0 && !matched_ids.contains(vertices[vid].value.parent_id)) {
                continue; 
            }

            auto&& expected_segment = jp.p[vertices[vid].value.depth];

            if (vertices[vid].value.depth == 0 || expected_segment.label == "*" || vertices[vid].value.key == expected_segment.label) {
                // Step forward: allow the DFS to explore children at the next depth level
                max_valid_depth = vertices[vid].value.depth + 1;
                    matched_ids.insert(vid);
                    // If we reach a leaf node or the exact target depth of the path, we have a successful query match!
                    if (vertices[vid].value.depth == jp.p.size() - 1) {
                        // std::cout << "Valid Query Match Node Found: " << vertices[vid].value.key<<" "<<vertices[vid].value.obj_type << std::endl;
                        if(vertices[vid].value.obj_type==PAIR_VALUE && expected_segment.action==TEST){
                            if(vertices[vid].value.key.compare(expected_segment.label)==0 && test(substr_view(jsonContent, vertices[vid].value.start, vertices[vid].value.end), expected_segment.value)){
                                // std::cout<<"PAIR_VALUE "<<vertices[vid].value.key<<" "<<expected_segment.label<<" "<<substr_view(jsonContent, vertices[vid].valuestart, vertices[vid].valueend)<<" "<<std::any_cast<std::string_view>(jp.p[currentDepth].value)<<std::endl;
                                    JsonValue a{};
                                if(cb(vertices[vid].value.parent_id, vertices[vid].value.key, a))hit=true;
                                // if(!singleMatch && hit && currentDepth>0 && jp.p[currentDepth-1].label.compare("*")!=0)dfs.cancel(graph::views::cancel_search::cancel_all);
                            }
                        }
                        else if(vertices[vid].value.obj_type==JSON_ARRAY){
                            // std::cout<<"START_ARRAY "<<vertices[vid].value.id<<" "<<substr_view(jsonContent, vertices[vid].value.start, vertices[vid].value.end)<<" "<<graph::views::out_incidence(dagGraph, u).size()<<std::endl;
                            for (auto&& oe : graph::adj_list::out_edges(dagGraph, vertexDescriptor.vertex)) {
                                auto oid=graph::target_id(dagGraph, oe);
                                std::cout<<"\t"<<oid<<" "<<vertices[oid].value.key<<" "<<substr_view(jsonContent, vertices[oid].value.start, vertices[oid].value.end)<<std::endl;
                                JsonValue a=substr_view(jsonContent, vertices[oid].value.start, vertices[oid].value.end);
                                if(cb(vertices[oid].value.id, vertices[oid].value.key, a))hit=true;
                                // if(!singleMatch && hit && currentDepth>0 && jp.p[currentDepth-1].label.compare("*")!=0)dfs.cancel(graph::views::cancel_search::cancel_branch);
                            }
                            // std::cout<<"START_ARRAY end "<<std::endl;
                        }
                        else if(vertices[vid].value.obj_type==JSON_OBJECT && expected_segment.label.compare("*")==0){
                            // std::cout<<"JSON_OBJECT * "<<vertices[vid].valuekey<<" "<<substr_view(jsonContent, vertices[vid].valuestart, vertices[vid].valueend)<<std::endl;
                        }
                        if((vertices[vid].value.obj_type==PAIR_VALUE || vertices[vid].value.obj_type==JSON_STRING || vertices[vid].value.obj_type==VALUE)){
                            // std::cout<<"\t"<<vertices[vid].value.obj_type<<" simple value "<<vertices[vid].value.value_index<<std::endl;
                            if(cb(vertices[vid].value.id, vertices[vid].value.key, vertices[vid].value.value_index))hit=true;
                        }
                        else if(vertices[vid].value.obj_type==JSON_OBJECT){
                            // std::cout<<"JSON_OBJECT key matched "<<vertices[vid].value.key<<" "<<substr_view(jsonContent, vertices[vid].value.start, vertices[vid].value.end)<<std::endl;
                            JsonValue jv{};
                            if(cb(vertices[vid].value.id, vertices[vid].value.key, jv))hit=true;
                            //std::cout<<"hit "<<hit<<std::endl;
                            // if(!singleMatch && hit && currentDepth>0 && jp.p[currentDepth-1].label.compare("*")!=0)dfs.cancel(graph::views::cancel_search::cancel_all);
                        }
                    }
            } else {
                // Mismatch! We do not increment max_valid_depth. 
                // This locks out all downstream children of this branch on subsequent loop iterations.
                //std::cout << "Pruning branch at key: " << node.key << std::endl;
            }
        }

        // auto it = std::ranges::find_if(graph::vertices(dagGraph),
        //                          [&](const std::ranges::range_value_t<decltype(graph::vertices(dagGraph))>& v) { return vertices[graph::vertex_id(dagGraph, v)].id==0; });
        // auto vid=graph::vertex_id(dagGraph, *it);
        // auto dfs      = graph::views::vertices_dfs(dagGraph, vid);
        // size_t depth=dfs.depth();
        // size_t count=0;
        // size_t count2=0;
        // for (auto&& [u] : dfs) {
        //   auto uid=graph::vertex_id(dagGraph, u);
        //   size_t currentDepth=graph::vertex_value(dagGraph, u).depth;
        //     // std::cout<<"\tat "<<uid<<" "<<vertices[uid].depth<<" |"<<substr_view(jsonContent, vertices[uid].start, vertices[uid].end)<<"| "<<currentDepth<<" "<<jp.p.size()<<" "<<std::endl;
        //   count++;
        // //   if(currentDepth==0)continue;
        //   count2++;
        // //   if(vertices[uid].obj_type==PAIR_KEY || vertices[uid].obj_type==PAIR_VALUE)
        //     // currentDepth--;
        //   if(currentDepth>=jp.p.size()){
        //         continue;
        //    }
        //   if(currentDepth<jp.p.size()){
        //     if((jp.p[currentDepth].label.compare("*")==0 || vertices[uid].key.compare(jp.p[currentDepth].label)==0)){
        //         if(vertices[uid].obj_type==PAIR_VALUE && jp.p[currentDepth].action==TEST){
        //             if(vertices[uid].key.compare(jp.p[currentDepth].label)==0 && test(substr_view(jsonContent, vertices[uid].start, vertices[uid].end), jp.p[currentDepth].value)){
        //                 // std::cout<<"PAIR_VALUE "<<vertices[uid].key<<" "<<jp.p[currentDepth].label<<" "<<substr_view(jsonContent, vertices[uid].start, vertices[uid].end)<<" "<<std::any_cast<std::string_view>(jp.p[currentDepth].value)<<std::endl;
        //                        std::any a{};
        //                 if(apply(vertices[uid].parent_id, vertices[uid].key, a))hit=true;
        //                 // if(!singleMatch && hit && currentDepth>0 && jp.p[currentDepth-1].label.compare("*")!=0)dfs.cancel(graph::views::cancel_search::cancel_all);
        //             }
        //         }
        //         else if(vertices[uid].obj_type==JSON_ARRAY && vertices[uid].key.compare(jp.p[currentDepth].label)==0){
        //             // std::cout<<"START_ARRAY "<<vertices[uid].id<<" "<<substr_view(jsonContent, vertices[uid].start, vertices[uid].end)<<" "<<graph::views::out_incidence(dagGraph, u).size()<<std::endl;
        //             for (auto&& oe : graph::adj_list::out_edges(dagGraph, u)) {
        //                 auto oid=graph::target_id(dagGraph, oe);
        //                 std::cout<<"\t"<<oid<<" "<<vertices[oid].key<<" "<<substr_view(jsonContent, vertices[oid].start, vertices[oid].end)<<std::endl;
        //                 std::any a=substr_view(jsonContent, vertices[oid].start, vertices[oid].end);
        //                 if(apply(vertices[oid].id, vertices[oid].key, a))hit=true;
        //                 // if(!singleMatch && hit && currentDepth>0 && jp.p[currentDepth-1].label.compare("*")!=0)dfs.cancel(graph::views::cancel_search::cancel_branch);
        //             }
        //             // std::cout<<"START_ARRAY end "<<std::endl;
        //         }
        //         else if(vertices[uid].obj_type==JSON_OBJECT && jp.p[currentDepth].label.compare("*")==0){
        //             // std::cout<<"JSON_OBJECT * "<<vertices[uid].key<<" "<<substr_view(jsonContent, vertices[uid].start, vertices[uid].end)<<std::endl;
        //         }
        //         else if(vertices[uid].obj_type==JSON_OBJECT && vertices[uid].key.compare(jp.p[currentDepth].label)==0){
        //         //    std::cout<<"JSON_OBJECT key matched "<<vertices[uid].key<<" "<<substr_view(jsonContent, vertices[uid].start, vertices[uid].end)<<std::endl;
        //             std::any a{};
        //             if(apply(vertices[uid].id, vertices[uid].key, a))hit=true;
        //             //std::cout<<"hit "<<hit<<std::endl;
        //             // if(!singleMatch && hit && currentDepth>0 && jp.p[currentDepth-1].label.compare("*")!=0)dfs.cancel(graph::views::cancel_search::cancel_all);
        //         }
        //         else if((vertices[uid].obj_type==PAIR_VALUE || vertices[uid].obj_type==JSON_STRING || vertices[uid].obj_type==VALUE) && (vertices[uid].key.compare(jp.p[currentDepth].label)==0)){
        //             if(apply(vertices[uid].id, vertices[uid].key, vertices[uid].value_index))hit=true;
        //             // if(!singleMatch && hit && currentDepth>0 && jp.p[currentDepth-1].label.compare("*")!=0)dfs.cancel(graph::views::cancel_search::cancel_branch);
        //         }
        //     }
        //     else if((vertices[uid].obj_type==JSON_OBJECT && vertices[uid].key.compare(jp.p[currentDepth].label)==0)){
        //         for (auto&& oe : graph::adj_list::out_edges(dagGraph, u)) {
        //             auto oid=graph::target_id(dagGraph, oe);
        //             //graph::edge_value(g, v);
        //             if(apply(vertices[oid].id, vertices[oid].key, vertices[oid].value_index))hit=true;
        //             // if(!singleMatch && hit && currentDepth>0 && jp.p[currentDepth-1].label.compare("*")!=0)dfs.cancel(graph::views::cancel_search::cancel_branch);
        //         }
        //     }
        //    }
        //   else if(currentDepth<jp.p.size() && jp.p[currentDepth].label.compare("*")==0){continue;}
        // //   else if(!singleMatch)dfs.cancel(graph::views::cancel_search::cancel_branch);
        // }
//        std::cout<<"match status "<<count<<" "<<count2<<" "<<hit<<std::endl;
        singleMatch=false;
        return hit;
    };
        
//        void stroll(Path& jp, std::function<void(size_t objIndex, std::string_view& key, std::any& v)> apply, size_t i=0, unsigned int objParent=0);
        
        inline std::string typeset(bool comma, bool wrap, size_t indention, std::string_view& key, JsonValue& value){
            std::string kv{};
            if(comma)kv.append(",");
            if(wrap)kv.append("\n");
            for(size_t ti=0;ti<indention;ti++)kv.append("    ");
            kv.append("\"");
            kv.append(key);
            kv.append("\": ");
            std::visit(overloaded {
                [&kv](std::monostate)       { kv.append("\""); kv.append("[empty]"); kv.append("\""); },
                [&kv](std::string_view val) { kv.append("\""); kv.append(val); kv.append("\""); },
                [&kv](const char* val)      { kv.append("\""); kv.append(val); kv.append("\""); },
                [&kv](double val)           { kv.append(std::to_string(val)); },
                [&kv](int val)              { kv.append(std::to_string(val)); },
                [&kv](unsigned int val)     { kv.append(std::to_string(val)); },
                [&kv](long val)             { kv.append(std::to_string(val)); },
                [&kv](unsigned long val)    { kv.append(std::to_string(val)); },
                [&kv](long long val)        { kv.append(std::to_string(val)); },
                [&kv](bool val)             { (val)? kv.append("true") : kv.append("false"); },
            }, value);
            return std::move(kv);
        }

        inline size_t findInsertionOffset(size_t index){
            size_t offset=index;
            if(offset<jsonContent.size() && (jsonContent.at(offset)=='\n' ||  jsonContent.at(offset)>='0'))return offset;
            while(++offset<jsonContent.size()-1 && jsonContent.at(offset)!='\n' &&  jsonContent.at(offset)<'0'){
                
            }
//            if(offset<jsonContent.size()-1)++offset;
            return offset;
        }
        
        inline size_t findIndention(size_t index){
            size_t offset=index;
            
            if(offset>0)
            while((--offset)>0 && jsonContent.at(offset)!='\n' && jsonContent.at(offset)!=',' && jsonContent.at(offset)!='}' && jsonContent.at(offset)!=']'){
//            std::cout<<offset<<" findIndention "<<index<<" "<<jsonContent.at(offset)<<" "<<jsonContent.length()<<std::endl;
                
            }
//            if(offset<jsonContent.size()-1)++offset;
            std::cout<<"offset "<<offset<<" "<<index<<" "<<4<<std::endl;
            if(index>offset)return (index-offset)/4;
            return 0;
        }
        
    public:
        friend std::ostream& operator<<(std::ostream& s, Binder& jb) {
          s << jb.jsonContent;
          return s;
        }
        
    };
    
}
