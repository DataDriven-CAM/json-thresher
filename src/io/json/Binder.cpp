#include <cstdio>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <vector>
#include <chrono>
#include <typeinfo>
#include <ranges>
#include <format>
#include <functional>

#include "io/json/Binder.h"

namespace sylvanmats::io::json{
    
    void Binder::operator ()(std::istream& is){
        this->jsonContent=std::string((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
        clear();
        bind(0);
    }
    
    void Binder::operator ()(std::string& jsonContent){
        this->jsonContent=jsonContent;
        clear();
        bind(0);
    }
        
        
    void Binder::operator ()(Path& jp, std::function<std::tuple<bool, std::string_view, JsonValue>(void)> apply){
        singleMatch=true;
        size_t count=0;
        bool hit=match(jp, true, [&](size_t id, std::string_view key, const JsonValue& v)-> bool{
            bool firstCall=true;
            bool notfinal=false;
            bool comma=false;
            bool wrap=true;
            size_t insertionOffset=0;
            size_t indention=0;
            std::string insertableBlock{};
            do{
            auto&& [finialize, newKey, newValue] = apply();
                if(firstCall){
                    indention=vertices[id].value.depth;
                    size_t insertionObjSize=id;
                    if(id<vertices.size()-1 && (vertices[id+1].value.obj_type==JSON_OBJECT || vertices[id+1].value.obj_type==JSON_ARRAY)){
                        indention++;
                        insertionObjSize=id-1;
                     }
                    insertionOffset=findInsertionOffset(vertices[insertionObjSize].value.end);
                    firstCall=false;
                }
                else comma=true;
                insertableBlock.append(typeset(comma, wrap, indention, newKey, newValue));

            notfinal=finialize;
            }while(!notfinal);
            jsonContent.insert(insertionOffset, insertableBlock);
            size_t count=0;
            //dagGraph.resize_vertices(count);
            //dagGraph.resize_edges(count);
            return true;
        });
        if(hit){
            dagGraph=G{};
            parentStack.clear();
            vertices.clear();
            edges.clear();
            //dag.clear();
            //depthList.clear();
            bind(0);

        }
    }

    //remove
    bool Binder::operator ()(Path& jp, std::string removalKey){
        singleMatch=true;
        bool ret=false;
        bool hit=match(jp, false, [&](size_t id, std::string_view key, const JsonValue& v)->bool{
            auto u=*find_vertex(dagGraph, id);
             for (auto&& oe : graph::adj_list::out_edges(dagGraph, u) | std::views::filter([&](auto i){auto id=graph::target_id(dagGraph, i);return substr_view(jsonContent, vertices[id].value.start, vertices[id].value.end).compare(removalKey)==0;})) {
                auto oid=graph::target_id(dagGraph, oe);
                if(vertices[oid+1].value.obj_type==JSON_OBJECT || vertices[oid+1].value.obj_type==JSON_ARRAY){
                    std::string::size_type start=vertices[oid].value.start-1;
                    std::string::size_type offset=start;
                    auto u2=*find_vertex(dagGraph, oid+1);
                    for (auto&& oe2 : graph::adj_list::out_edges(dagGraph, u2)){
                        auto oid2=graph::target_id(dagGraph, oe2);
                        offset=vertices[oid2+1].value.end+1;
                    }
                    jsonContent.erase(start, offset-start);
//                    std::cout<<jsonContent<<std::endl;
            size_t count=0;
            //dagGraph.resize_vertices(count);
            //dagGraph.resize_edges(count);
            dagGraph=G{};
            parentStack.clear();
            vertices.clear();
            edges.clear();
                    //dag.clear();
                    //depthList.clear();
                    bind(0);
                    ret=true;
                    break;
                }
            }
            return true;
            });
        return ret;
    }
        
    void Binder::display(){
        auto it = std::ranges::find_if(graph::vertices(dagGraph),
                                 [&](const std::ranges::range_value_t<decltype(graph::vertices(dagGraph))>& v) { return vertices[graph::vertex_id(dagGraph, v)].value.id==0; });
        auto vid=graph::vertex_id(dagGraph, *it);
        auto dfs      = graph::views::vertices_dfs(dagGraph, vid);
        size_t depth=dfs.depth();
        size_t count=0;
        size_t count2=0;
         for (auto&& [u] : dfs) {
            auto uid=graph::vertex_id(dagGraph, u);
            size_t outCount=0;
            for (auto&& [tid, uv] : graph::views::incidence(dagGraph, u)) {
                auto targetid = graph::adj_list::target_id(dagGraph, uv);
                for(size_t i=0;i<vertices[targetid].value.depth;i++)std::cout<<" ";
                std::cout<<substr_view(jsonContent, vertices[uid].value.start, vertices[uid].value.end)<<" -> "<<vertices[targetid].value.key<<"="<<substr_view(jsonContent, vertices[targetid].value.start, vertices[targetid].value.end)<<std::endl;
                outCount++;
            }
          count++;
        }
    }
    
    void Binder::shortenDAG(std::string::size_type insertionOffset, std::string::size_type insertionObjSize){
            auto startReduction = std::chrono::high_resolution_clock::now();
            size_t currentDAGSize=graph::num_vertices(dagGraph);
            dagGraph=G{};
            vertices.resize(insertionObjSize);
            bool hit=false;
            size_t d=0;
            for(std::vector<graph::copyable_edge_t<size_t, int>>::reverse_iterator it=edges.rbegin();!hit && it!=edges.rend();it++){
                if((*it).source_id<vertices.size() && (*it).target_id<vertices.size()){
                    d=vertices.size()-std::distance(edges.rbegin(), it);
                    hit=true;
                }
            }
            if(hit && d<edges.size())edges.resize(d);
                /*dag.resize(insertionObjSize);
                depthList.resize(insertionObjSize);
                size_t inObjSize=insertionObjSize;
                if(inObjSize<dag.size())
                while(inObjSize>0 && !dag[inObjSize].second.empty()){
                    if(!dag[inObjSize].second.empty()){
                        bool hit=false;
                        size_t secondSize=dag[inObjSize].second.size()-1;
                        for(std::vector<jobject>::reverse_iterator it=dag[inObjSize].second.rbegin();!hit && it!=dag[inObjSize].second.rend();++it){
                            if(inObjSize>=(*it).id){
                                hit=true;
                            }
                            else
                                secondSize--;
                        }
                        if(hit && secondSize>1)dag[inObjSize].second.resize(secondSize-1);
                        inObjSize=dag[inObjSize].second.front().id;
                        
                    }
                    else
                        inObjSize--;
                }*/
//                std::cout<<currentDAGSize<<" resize "<<inObjSize<<" "<<dag.size()<<" indent "<<depthList.back()<<" insertionOffset "<<insertionOffset<<" total size: "<<jsonContent.size()<<" "<<insertionObjSize<<" "<<depthList.back()<<" start obj: "<<(dag.back().first.obj_type==START_OBJ)<<std::endl;
            auto endReduction = std::chrono::high_resolution_clock::now();
                int objDiffStart=countObjects();
                bind(insertionOffset, vertices.back().value.depth);
                objDiff=countObjects()-objDiffStart;
            auto endBind = std::chrono::high_resolution_clock::now();
                reductionTime=std::chrono::duration_cast<std::chrono::nanoseconds>(endReduction-startReduction).count()*1.0e-9;
                bindTime=std::chrono::duration_cast<std::chrono::nanoseconds>(endBind-endReduction).count()*1.0e-9;
    }
    
    void Binder::bind(std::string::size_type startOffset, size_t startDepth){
       //auto startTime = std::chrono::high_resolution_clock::now();
        vertices.reserve(1024); // Choose a number safely higher than your expected element count
        edges.reserve(2048);
        objectCount=0;
        size_t depth=startDepth;
        size_t dagOffset=(!vertices.empty())? vertices.size()-1 : 0;
        std::span s={jsonContent};
            std::span<char>::iterator it=(startOffset>0) ? s.begin()+startOffset : s.begin();
            size_t offset=startOffset;
            bool firstObject=startOffset==0;
            bool hitColon=false;
            bool hitComma=false;
            bool hitPeriod=false;
            size_t keyStart=0;
            size_t keyEnd=0;
            while(it!=s.end()){
                std::span<char>::iterator currentIt=it;
                if(isNull(s, it)){
                    vertices.push_back(graph::copyable_vertex_t<size_t, sylvanmats::io::json::jobject>{vertices.size(), jobject{.obj_type=VALUE_NULL, .id=vertices.size(), .parent_id=parentStack.back(), .value_index=std::string_view(it, it+4), .start=offset, .end=offset+4, .depth=parentStack.size(), .key=substr_view(jsonContent, keyStart, keyEnd)}});
                    edges.push_back(graph::copyable_edge_t<size_t, int>{vertices.back().value.parent_id, vertices.back().value.id, 1});
                    // parentStack.push_back(vertices.back().id);
                    // if(maxStackSize<parentStack.size())maxStackSize=parentStack.size();
                    it+=4;
                    offset+=4;
                    hitColon=false;
                    hitComma=false;
                }
                if((*it)=='{'){
                    if(firstObject){
                        vertices.push_back(graph::copyable_vertex_t<size_t, sylvanmats::io::json::jobject>{vertices.size(), jobject{.obj_type=JSON_OBJECT, .id=vertices.size(), .parent_id=0, .key_index=offset, .value_index={}, .start=offset, .end=offset+1, .depth=parentStack.size(), .key=std::string_view{}}});
                        firstObject=false;
                    }
                    else{
                        vertices.push_back(graph::copyable_vertex_t<size_t, sylvanmats::io::json::jobject>{vertices.size(), jobject{.obj_type=JSON_OBJECT, .id=vertices.size(), .parent_id=parentStack.back(), .key_index=offset, .value_index={}, .start=offset, .end=offset+1, .depth=parentStack.size(), .key=substr_view(jsonContent, keyStart, keyEnd)}});
                        edges.push_back(graph::copyable_edge_t<size_t, int>{vertices.back().value.parent_id, vertices.back().value.id, 1});
                    }
                        parentStack.push_back(vertices.back().id);
                        if(maxStackSize<parentStack.size())maxStackSize=parentStack.size();
                    hitColon=false;
                    hitComma=false;
                    objectCount++;
                }
                else if((*it)=='}'){
                    parentStack.pop_back();
                    hitColon=false;
                    hitComma=false;
                }
                else if((*it)=='['){
                        vertices.push_back(graph::copyable_vertex_t<size_t, sylvanmats::io::json::jobject>{vertices.size(), jobject{.obj_type=JSON_ARRAY, .id=vertices.size(), .parent_id=parentStack.back(), .key_index=offset, .value_index={}, .start=offset, .end=offset+1, .depth=parentStack.size(), .key=substr_view(jsonContent, keyStart, keyEnd)}});
                        edges.push_back(graph::copyable_edge_t<size_t, int>{vertices.back().value.parent_id, vertices.back().value.id, 1});
                        parentStack.push_back(vertices.back().id);
                        if(maxStackSize<parentStack.size())maxStackSize=parentStack.size();
                    hitColon=false;
                    hitComma=false;
                    firstObject=false;
                }
                else if((*it)==']'){
                    parentStack.pop_back();
                    hitColon=false;
                    hitComma=false;
                }
                else if((*it)=='"'){
                    ++it;
                    offset++;
                    size_t startOffset=offset;
                    std::span<char>::iterator itStart=it;
                    int c=0;
                    while((*it)!='"'){if((*it)=='\\'){++it;offset++;};++it;offset++;c++;};
                    if(vertices[parentStack.back()].value.obj_type==JSON_ARRAY){
                        vertices.push_back(graph::copyable_vertex_t<size_t, sylvanmats::io::json::jobject>{vertices.size(), jobject{.obj_type=JSON_STRING, .id=vertices.size(), .parent_id=parentStack.back(), .key_index=offset, .value_index=std::string_view(std::to_address(itStart), std::to_address(it)), .start=startOffset, .end=offset, .depth=parentStack.size()}});
                        edges.push_back(graph::copyable_edge_t<size_t, int>{vertices.back().value.parent_id, vertices.back().value.id, 1});
                    }
                    else if(!hitColon){
                        keyStart=startOffset;
                        keyEnd=offset;
                    }
                    else{
                        vertices.push_back(graph::copyable_vertex_t<size_t, sylvanmats::io::json::jobject>{vertices.size(), jobject{.obj_type=PAIR_VALUE, .id=vertices.size(), .parent_id=parentStack.back(), .key_index=offset, .value_index=std::string_view(std::to_address(itStart), std::to_address(it)), .start=startOffset, .end=offset, .depth=parentStack.size(), .key=substr_view(jsonContent, keyStart, keyEnd)}});
                        edges.push_back(graph::copyable_edge_t<size_t, int>{vertices.back().value.parent_id, vertices.back().value.id, 1});
                        vertices.back().value.value_index=std::string_view(std::to_address(itStart), std::to_address(it));
                        hitColon=false;
                        hitComma=false;
                    }
                    ++it;
                    offset++;
                    
                }
                else if((*it)=='-' || (*it)=='.' || ((*it)>='0' && (*it)<='9')){
                    std::span<char>::iterator itStart=it;
                    ++it;
                    size_t startOffset=offset;
                    offset++;
                    bool hitPeriod=((*it)=='.') ? true : false;
                    int c=0;
                    while(((*it)>='0' && (*it)<='9') || (*it)=='.'){if(!hitPeriod && (*it)=='.')hitPeriod=true;++it;offset++;c++;};
                    std::string v(itStart, it);
                    // std::cout<<"num PV "<<vertices.size()<<" "<<v<<" depth: "<<depth<<std::endl;
                    if(hitPeriod){
                        vertices.push_back(graph::copyable_vertex_t<size_t, sylvanmats::io::json::jobject>{vertices.size(), jobject{.obj_type=VALUE, .id=vertices.size(), .parent_id=parentStack.back(), .key_index=offset, .value_index=std::strtod(v.c_str(), nullptr), .start=startOffset, .end=offset, .depth=parentStack.size(), .key=substr_view(jsonContent, keyStart, keyEnd)}});
                        edges.push_back(graph::copyable_edge_t<size_t, int>{vertices.back().value.parent_id, vertices.back().value.id, 1});
                        // parentStack.push_back(vertices.back().id);
                        // if(maxStackSize<parentStack.size())maxStackSize=parentStack.size();
                    }
                    else{
                        vertices.push_back(graph::copyable_vertex_t<size_t, sylvanmats::io::json::jobject>{vertices.size(), jobject{.obj_type=VALUE, .id=vertices.size(), .parent_id=parentStack.back(), .key_index=offset, .value_index=std::strtol(v.c_str(), nullptr, 10), .start=startOffset, .end=offset, .depth=parentStack.size(), .key=substr_view(jsonContent, keyStart, keyEnd)}});
                        edges.push_back(graph::copyable_edge_t<size_t, int>{vertices.back().value.parent_id, vertices.back().value.id, 1});
                        // parentStack.push_back(vertices.back().id);
                        // if(maxStackSize<parentStack.size())maxStackSize=parentStack.size();
                    }
                    hitColon=false;
                    hitComma=false;
                    
                }
                else if((*it)==':'){
                    hitColon=true;
                    hitComma=false;
                }
                else if((*it)==','){
                    hitComma=true;
                    hitColon=false;
                }
                else if((*it)=='\\'){
                    ++it;
                    offset++;
                    ++it;
                    offset++;
                }
                if(std::distance(currentIt, it)==0){
                    ++it;
                    offset++;
                }
            }
    //auto sortTime = std::chrono::high_resolution_clock::now();
            std::sort(edges.begin(), edges.end(), [](graph::copyable_edge_t<size_t, int>& a, graph::copyable_edge_t<size_t, int>& b){ if(a.source_id!=b.source_id){return a.source_id<b.source_id;} return a.source_id<b.source_id;});
            /*std::cout<<"{";
            for(std::vector<std::tuple<graph::vertex_id_t<G>, graph::vertex_id_t<G>, int>>::iterator it=edges.begin();it!=edges.end();it++){
                std::cout<<"{"<<std::get<0>((*it))<<","<<std::get<1>((*it))<<","<<std::get<2>((*it))<<"}, ";
            }
            std::cout<<"}"<<std::endl;*/
    //auto graphTime = std::chrono::high_resolution_clock::now();
            depth=startDepth;
            using value = std::ranges::range_value_t<decltype(edges)>;
            graph::vertex_id_t<G> N = static_cast<graph::vertex_id_t<G>>(graph::vertices(dagGraph).size());
            //dagGraph.reserve_vertices(vertices.size());
            //dagGraph.reserve_edges(edges.size());

            dagGraph.reserve_vertices(vertices.size());
            dagGraph.load_vertices(vertices, std::identity{});//, [&](const auto& nm) -> graph::copyable_vertex_t<size_t, sylvanmats::io::json::jobject> {
                //     auto uid = static_cast<graph::vertex_id_t<G>>(&nm - vertices.data());
                //     //std::cout<<"uid "<<uid<<std::endl;
                //             return {uid, nm};
                // });
            dagGraph.reserve_edges(edges.size());
            dagGraph.load_edges(edges, std::identity{});//[](const auto& c) -> graph::copyable_edge_t<graph::vertex_id_t<G>, int> { return {std::get<0>(c), std::get<1>(c), std::get<2>(c)}; });
    //auto endTime = std::chrono::high_resolution_clock::now();
    //std::cout << "scan time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(edgeTime-startTime).count()*1.0e-9 << "s\n";
    //std::cout << "edge time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(sortTime-edgeTime).count()*1.0e-9 << "s\n";
    //std::cout << "edge sort time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(graphTime-sortTime).count()*1.0e-9 << "s\n";
    //std::cout << "graph construct time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(endTime-graphTime).count()*1.0e-9 << "s\n";
    //std::cout << "bind time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(endTime-startTime).count()*1.0e-9 << "s\n";
    }

    bool Binder::isNull(std::span<char>& s, std::span<char>::iterator& it){
        std::span<char>::iterator itStart=it;
        itStart!=s.end();
        if(*itStart!='n')return false;
        itStart++;
        itStart!=s.end();
        if(*itStart!='u')return false;
        itStart++;
        itStart!=s.end();
        if(*itStart!='l')return false;
        itStart++;
        itStart!=s.end();
        if(*itStart!='l')return false;
        return true;
    }

//     bool Binder::match(Path& jp, bool last, std::function<bool(size_t id, std::string_view key, std::any& v)> apply){
//         bool hit=false;
//         size_t pi=0;
//         int countMap=0;
//         if(jp.p.empty()){
//             if(apply(0, substr_view(jsonContent, vertices[0].value.start, vertices[0].value.end), vertices[0].value.value_index))hit=true;
//             return hit;
//         }
//         auto prunedGraph = graph::adaptors::filtered_graph(dagGraph, [&](auto vertexDescriptor) -> bool { // <-- VERTEX PREDICATE SLOT
//                 // 1. Get the vertex ID
//                 // auto vid = graph::vertex_id(dagGraph, vertexDescriptor);
//                 auto&& targetNode = vertices[vertexDescriptor].value;
//                 size_t currentDepth = targetNode.depth;

//                 std::cout << "prune vertex depth: " << currentDepth << " query size: " << jp.p.size() << std::endl;

//                 // Safety: If the JSON depth is deeper than our XPath query path, prune it
//                 if (currentDepth >= jp.p.size()) return false;

//                 auto&& expected_segment = jp.p[currentDepth];
//                 if (expected_segment.label == "*") return true;

//                 // If the token key doesn't match our active path level, return false 
//                 // to completely block the DFS from stepping onto this node or its children!
//                 return (targetNode.key == expected_segment.label);
//             }, [&](const auto& edge) -> bool {
//                         auto&& targetNode = vertices[graph::target(dagGraph, edge)].value;
//                 size_t currentDepth = targetNode.depth;

//                 std::cout<<"prune "<<currentDepth<<" "<<jp.p.size()<<std::endl;
//                 // Safety: If the JSON file nesting is deeper than our XPath query length, prune it
//                 if (currentDepth >= jp.p.size()) return false;

//                 // Fetch the corresponding query segment requirement for this depth
//                 auto&& expectedSegment = jp.p[currentDepth]; 

//                 // PRUNING RULES:
//                 // 1. If it's a wildcard segment (*), let all branches through
//                 // 2. If it's a named key segment, only allow edges matching the literal key string
//                 if (expectedSegment.label == "*") {
//                     return true;
//                 }
//                 std::cout<<"match "<<targetNode.key<<" "<<expectedSegment.label<<std::endl;
//                 return (targetNode.key == expectedSegment.label);
//             });
//         auto rootVertex = graph::vertex_id_t<G>(0);
//         auto dfs_range = graph::views::vertices_dfs(dagGraph, rootVertex);

//         size_t max_valid_depth = 0;
//         std::vector<typename graph::vertex_id_t<G>> matched_ids;

//         for (auto&& vertexDescriptor : dfs_range) {
//             auto vid = graph::vertex_id(dagGraph, vertexDescriptor.vertex);
//             // auto&& node = vertices[vid].value;
            
//             if (vertices[vid].value.depth > max_valid_depth) {
//                 continue;
//             }

//             if (vertices[vid].value.depth >= jp.p.size()) {
//                 continue;
//             }

//             auto&& expected_segment = jp.p[vertices[vid].value.depth];

//             if (vertices[vid].value.depth == 0 || expected_segment.label == "*" || vertices[vid].value.key == expected_segment.label) {
//                 // Step forward: allow the DFS to explore children at the next depth level
//                 max_valid_depth = vertices[vid].value.depth + 1;
//                     // If we reach a leaf node or the exact target depth of the path, we have a successful query match!
//                     if (vertices[vid].value.depth == jp.p.size() - 1) {
//                         std::cout << "Valid Query Match Node Found: " << vertices[vid].value.key << std::endl;
//                         if((vertices[vid].value.obj_type==PAIR_VALUE || vertices[vid].value.obj_type==JSON_STRING || vertices[vid].value.obj_type==VALUE)){
//                             std::cout<<"\t"<<vertices[vid].value.obj_type<<" simple value "<<vertices[vid].value.value_index.type().name()<<std::endl;
//                             if(apply(vertices[vid].value.id, vertices[vid].value.key, vertices[vid].value.value_index))hit=true;
//                         }
//                     }
//             } else {
//                 // Mismatch! We do not increment max_valid_depth. 
//                 // This locks out all downstream children of this branch on subsequent loop iterations.
//                 //std::cout << "Pruning branch at key: " << node.key << std::endl;
//             }
//         }

//         // auto it = std::ranges::find_if(graph::vertices(dagGraph),
//         //                          [&](const std::ranges::range_value_t<decltype(graph::vertices(dagGraph))>& v) { return vertices[graph::vertex_id(dagGraph, v)].id==0; });
//         // auto vid=graph::vertex_id(dagGraph, *it);
//         // auto dfs      = graph::views::vertices_dfs(dagGraph, vid);
//         // size_t depth=dfs.depth();
//         // size_t count=0;
//         // size_t count2=0;
//         // for (auto&& [u] : dfs) {
//         //   auto uid=graph::vertex_id(dagGraph, u);
//         //   size_t currentDepth=graph::vertex_value(dagGraph, u).depth;
//         //     // std::cout<<"\tat "<<uid<<" "<<vertices[uid].depth<<" |"<<substr_view(jsonContent, vertices[uid].start, vertices[uid].end)<<"| "<<currentDepth<<" "<<jp.p.size()<<" "<<std::endl;
//         //   count++;
//         // //   if(currentDepth==0)continue;
//         //   count2++;
//         // //   if(vertices[uid].obj_type==PAIR_KEY || vertices[uid].obj_type==PAIR_VALUE)
//         //     // currentDepth--;
//         //   if(currentDepth>=jp.p.size()){
//         //         continue;
//         //    }
//         //   if(currentDepth<jp.p.size()){
//         //     if((jp.p[currentDepth].label.compare("*")==0 || vertices[uid].key.compare(jp.p[currentDepth].label)==0)){
//         //         if(vertices[uid].obj_type==PAIR_VALUE && jp.p[currentDepth].action==TEST){
//         //             if(vertices[uid].key.compare(jp.p[currentDepth].label)==0 && test(substr_view(jsonContent, vertices[uid].start, vertices[uid].end), jp.p[currentDepth].value)){
//         //                 // std::cout<<"PAIR_VALUE "<<vertices[uid].key<<" "<<jp.p[currentDepth].label<<" "<<substr_view(jsonContent, vertices[uid].start, vertices[uid].end)<<" "<<std::any_cast<std::string_view>(jp.p[currentDepth].value)<<std::endl;
//         //                        std::any a{};
//         //                 if(apply(vertices[uid].parent_id, vertices[uid].key, a))hit=true;
//         //                 // if(!singleMatch && hit && currentDepth>0 && jp.p[currentDepth-1].label.compare("*")!=0)dfs.cancel(graph::views::cancel_search::cancel_all);
//         //             }
//         //         }
//         //         else if(vertices[uid].obj_type==JSON_ARRAY && vertices[uid].key.compare(jp.p[currentDepth].label)==0){
//         //             // std::cout<<"START_ARRAY "<<vertices[uid].id<<" "<<substr_view(jsonContent, vertices[uid].start, vertices[uid].end)<<" "<<graph::views::out_incidence(dagGraph, u).size()<<std::endl;
//         //             for (auto&& oe : graph::adj_list::out_edges(dagGraph, u)) {
//         //                 auto oid=graph::target_id(dagGraph, oe);
//         //                 std::cout<<"\t"<<oid<<" "<<vertices[oid].key<<" "<<substr_view(jsonContent, vertices[oid].start, vertices[oid].end)<<std::endl;
//         //                 std::any a=substr_view(jsonContent, vertices[oid].start, vertices[oid].end);
//         //                 if(apply(vertices[oid].id, vertices[oid].key, a))hit=true;
//         //                 // if(!singleMatch && hit && currentDepth>0 && jp.p[currentDepth-1].label.compare("*")!=0)dfs.cancel(graph::views::cancel_search::cancel_branch);
//         //             }
//         //             // std::cout<<"START_ARRAY end "<<std::endl;
//         //         }
//         //         else if(vertices[uid].obj_type==JSON_OBJECT && jp.p[currentDepth].label.compare("*")==0){
//         //             // std::cout<<"JSON_OBJECT * "<<vertices[uid].key<<" "<<substr_view(jsonContent, vertices[uid].start, vertices[uid].end)<<std::endl;
//         //         }
//         //         else if(vertices[uid].obj_type==JSON_OBJECT && vertices[uid].key.compare(jp.p[currentDepth].label)==0){
//         //         //    std::cout<<"JSON_OBJECT key matched "<<vertices[uid].key<<" "<<substr_view(jsonContent, vertices[uid].start, vertices[uid].end)<<std::endl;
//         //             std::any a{};
//         //             if(apply(vertices[uid].id, vertices[uid].key, a))hit=true;
//         //             //std::cout<<"hit "<<hit<<std::endl;
//         //             // if(!singleMatch && hit && currentDepth>0 && jp.p[currentDepth-1].label.compare("*")!=0)dfs.cancel(graph::views::cancel_search::cancel_all);
//         //         }
//         //         else if((vertices[uid].obj_type==PAIR_VALUE || vertices[uid].obj_type==JSON_STRING || vertices[uid].obj_type==VALUE) && (vertices[uid].key.compare(jp.p[currentDepth].label)==0)){
//         //             if(apply(vertices[uid].id, vertices[uid].key, vertices[uid].value_index))hit=true;
//         //             // if(!singleMatch && hit && currentDepth>0 && jp.p[currentDepth-1].label.compare("*")!=0)dfs.cancel(graph::views::cancel_search::cancel_branch);
//         //         }
//         //     }
//         //     else if((vertices[uid].obj_type==JSON_OBJECT && vertices[uid].key.compare(jp.p[currentDepth].label)==0)){
//         //         for (auto&& oe : graph::adj_list::out_edges(dagGraph, u)) {
//         //             auto oid=graph::target_id(dagGraph, oe);
//         //             //graph::edge_value(g, v);
//         //             if(apply(vertices[oid].id, vertices[oid].key, vertices[oid].value_index))hit=true;
//         //             // if(!singleMatch && hit && currentDepth>0 && jp.p[currentDepth-1].label.compare("*")!=0)dfs.cancel(graph::views::cancel_search::cancel_branch);
//         //         }
//         //     }
//         //    }
//         //   else if(currentDepth<jp.p.size() && jp.p[currentDepth].label.compare("*")==0){continue;}
//         // //   else if(!singleMatch)dfs.cancel(graph::views::cancel_search::cancel_branch);
//         // }
// //        std::cout<<"match status "<<count<<" "<<count2<<" "<<hit<<std::endl;
//         singleMatch=false;
//         return hit;
//     }
    
}