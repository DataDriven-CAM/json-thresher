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

#include "graph/views/dfs.hpp"
#include "graph/views/incidence.hpp"

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
        
    //add
    bool Binder::operator ()(Path& jp, std::string_view newKey, std::any newValue){
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
            size_t insertionOffset=findInsertionOffset(vertices[insertionObjSize].end);// : jsonContent.size() - 1;
            
//                std::cout<<dag.back().second.size()<<" "<<insertionOffset<<" indention: "<<dag.back().second.back().key<<" "<<dag.back().second.back().obj_type<<std::endl;
//                auto s=objects[objects.size()-2];
//                size_t insertionOffset=s.key_index+1;//findInsertionOffset(s.key_index);
                size_t indention=(graph::num_vertices(dagGraph)>=2) ?  vertices[insertionObjSize].depth : vertices[insertionObjSize].depth+1;
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
            auto start = std::chrono::high_resolution_clock::now();
            size_t count=0;
            singleMatch=true;
            bool hit=match(jp, true, [&](size_t id, std::string_view key, std::any& v)-> bool{
                auto end = std::chrono::high_resolution_clock::now();
                count++;
                matchTime=std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()*1.0e-9;
                size_t insertionOffset=0;
                size_t indention=0;
                bool comma=false;
                bool wrap=false;
                size_t insertionObjSize=id;
                if(vertices[id].obj_type==END_OBJ || vertices[id].obj_type==END_ARRAY){
                    indention=vertices[id].depth+1;
                    if(vertices[id-1].obj_type==START_OBJ || vertices[id-1].obj_type==START_ARRAY){
                        insertionObjSize=id-1;
                        insertionOffset=findInsertionOffset(vertices[insertionObjSize].end);
                        wrap=true;
                    }
                    else{
                        insertionObjSize=id-1;
                        insertionOffset=findInsertionOffset(vertices[insertionObjSize].end);
                       comma=true;
                        wrap=true;
                    }
                }
                else if(vertices[id].obj_type==PAIR_VALUE){
//                    std::cout<<"label "<<jp.p.back().label<<" "<<id<<" "<<dag.size()<<" "<<depthList.size()<<std::endl;
                    indention=vertices[id].depth;
                    insertionObjSize=id;
                       insertionOffset=findInsertionOffset(vertices[insertionObjSize].end);
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
    }
        
    void Binder::operator ()(Path& jp, std::function<std::tuple<bool, std::string_view, std::any>(void)> apply){
        singleMatch=true;
        size_t count=0;
        bool hit=match(jp, true, [&](size_t id, std::string_view key, std::any& v)-> bool{
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
                    indention=vertices[id].depth;
                    size_t insertionObjSize=id;
                    if(vertices[id].obj_type==END_OBJ || vertices[id].obj_type==END_ARRAY){
                        indention++;
                        insertionObjSize=id-1;
                     }
                    insertionOffset=findInsertionOffset(vertices[insertionObjSize].end);
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
            depthProfile.clear();
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
        bool hit=match(jp, false, [&](size_t id, std::string_view key, std::any& v)->bool{
            auto u=*find_vertex(dagGraph, id);
             for (auto&& oe : graph::adj_list::out_edges(dagGraph, u) | std::views::filter([&](auto i){auto id=graph::target_id(dagGraph, i);return substr_view(jsonContent, vertices[id].start, vertices[id].end).compare(removalKey)==0;})) {
                auto oid=graph::target_id(dagGraph, oe);
                if(vertices[oid+1].obj_type==START_OBJ || vertices[oid+1].obj_type==START_ARRAY){
                    std::string::size_type start=vertices[oid].start-1;
                    std::string::size_type offset=start;
                    auto u2=*find_vertex(dagGraph, oid+1);
                    for (auto&& oe2 : graph::adj_list::out_edges(dagGraph, u2)){
                        auto oid2=graph::target_id(dagGraph, oe2);
                        offset=vertices[oid2+1].end+1;
                    }
                    jsonContent.erase(start, offset-start);
//                    std::cout<<jsonContent<<std::endl;
            size_t count=0;
            //dagGraph.resize_vertices(count);
            //dagGraph.resize_edges(count);
            dagGraph=G{};
            depthProfile.clear();
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
        
    //get
    void Binder::operator ()(Path& p, std::function<void(std::any& v)> apply){
        bool hit=match(p, false, [&apply](size_t id, std::string_view key, std::any& v)->bool{
            apply(v);
            return true;
        });
    }
        
    //traverse
    void Binder::operator ()(Path& p, std::function<void(std::string_view& key, std::any& v)> apply){
        bool hit=match(p, false, [&](size_t id, std::string_view key, std::any& v)->bool{
            auto u=*find_vertex(dagGraph, id);
            for (auto&& oe : graph::adj_list::out_edges(dagGraph, u)) {
                auto oid=graph::target_id(dagGraph, oe);
                if(vertices[oid].obj_type==PAIR_KEY){
                    std::string_view key=substr_view(jsonContent, vertices[oid].start, vertices[oid].end);
                    apply(key, vertices[oid+1].value_index);
                }
            }
            return true;
        });
    }
        
    void Binder::display(){
        //std::string depthText=fmt::format("{}\n", depthList);
        //std::cout<<depthText;
        auto it = std::ranges::find_if(dagGraph.vertex_ids(),
                                 [&](auto vid) { return dagGraph.vertex_value(vid).id==0; });
        graph::vertex_id_t<G> vid=static_cast<graph::vertex_id_t<G>>(it - std::begin(dagGraph.vertex_ids()));
        auto dfs      = graph::views::vertices_dfs(dagGraph, vid);
        size_t depth=dfs.depth();
        size_t count=0;
        size_t count2=0;
         for (auto&& [u] : dfs) {
            auto uid=graph::vertex_id(dagGraph, u);
            size_t outCount=0;
            for (auto&& [tid, uv] : graph::views::incidence(dagGraph, u)) {
                auto targetid = graph::adj_list::target_id(dagGraph, uv);
                outCount++;
            }
          size_t currentDepth=graph::vertex_value(dagGraph, u).depth;
          for(size_t i=0;i<currentDepth;i++)std::cout<<" ";
          std::cout<<count<<" "<<currentDepth<<" "<<depth<<" "<<uid<<" "<<graph::vertex_value(dagGraph, u).id<<" "<<graph::vertex_value(dagGraph, u).obj_type<<" "<<graph::vertex_value(dagGraph, u).start<<" "<<graph::vertex_value(dagGraph, u).end<<" depth: "<<graph::vertex_value(dagGraph, u).depth<<" "<<outCount<<" "<<substr_view(jsonContent, vertices[uid].start, vertices[uid].end)<<std::endl;
          count++;
        }
        // for(std::vector<std::vector<size_t>>::iterator it=depthProfile.begin();it!=depthProfile.end();it++){
        //     std::cout<<(std::distance(depthProfile.begin(), it))<<std::endl;
        //     std::string depthProfile=std::format("{}\n", (*it));
        //     std::cout<<"\t"<<depthProfile;
        // }
    }
    
    void Binder::shortenDAG(std::string::size_type insertionOffset, std::string::size_type insertionObjSize){
            auto startReduction = std::chrono::high_resolution_clock::now();
            size_t currentDAGSize=graph::num_vertices(dagGraph);
            dagGraph=G{};
            vertices.resize(insertionObjSize);
            bool hit=false;
            size_t d=0;
            for(std::vector<std::tuple<graph::vertex_id_t<G>, graph::vertex_id_t<G>, int>>::reverse_iterator it=edges.rbegin();!hit && it!=edges.rend();it++){
                if(std::get<0>(*it)<vertices.size() && std::get<1>(*it)<vertices.size()){
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
                bind(insertionOffset, vertices.back().depth);
                objDiff=countObjects()-objDiffStart;
            auto endBind = std::chrono::high_resolution_clock::now();
                reductionTime=std::chrono::duration_cast<std::chrono::nanoseconds>(endReduction-startReduction).count()*1.0e-9;
                bindTime=std::chrono::duration_cast<std::chrono::nanoseconds>(endBind-endReduction).count()*1.0e-9;
    }
    
    void Binder::bind(std::string::size_type startOffset, size_t startDepth){
       //auto startTime = std::chrono::high_resolution_clock::now();
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
            for(int di=0;di<depth;di++)depthProfile.push_back(std::vector<size_t>{});
            while(it!=s.end()){
                std::span<char>::iterator currentIt=it;
                if(isNull(s, it)){
                    vertices.push_back(jobject{.obj_type=VALUE_NULL, .id=vertices.size(), .value_index=std::string_view(it, it+4), .start=offset, .end=offset+4, .depth=depth});
                    if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                    depthProfile[depth].push_back(vertices.back().id);
                    it+=4;
                    offset+=4;
                    hitColon=false;
                    hitComma=false;
                }
                if((*it)=='{'){
                    // if(firstObject){
                        vertices.push_back(jobject{.obj_type=START_OBJ, .id=vertices.size(), .key_index=offset, .value_index=object(), .start=offset, .end=offset+1, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().id);
                        depth++;
                    // }
                    // else{
                    //     // if(vertices.back().obj_type==PAIR_KEY || hitComma)depth++;
                    //     vertices.push_back(jobject{.obj_type=START_OBJ, .id=vertices.size(), .key_index=offset, .value_index=object(), .start=offset, .end=offset+1, .depth=depth});
                    //     if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                    //     depthProfile[depth].push_back(vertices.back().id);
                    //     depth++;
                    // }
                    hitColon=false;
                    hitComma=false;
                    firstObject=false;
                    objectCount++;
                }
                else if((*it)=='}'){
                    // if(vertices.back().obj_type!=END_ARRAY && vertices.back().obj_type!=END_OBJ){
                        if(depth>0)depth--;
                    // }
                    vertices.push_back(jobject{.obj_type=END_OBJ, .id=vertices.size(), .key_index=offset, .start=offset, .end=offset+1, .depth=depth});
                    //if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                    depthProfile[depth].push_back(vertices.back().id);
                    //if(depth==0)std::cout<<"EO "<<vertices.back().id<<std::endl;
                    // if(depth>0)depth--;
                    hitColon=false;
                    hitComma=false;
                }
                else if((*it)=='['){
                    // if(firstObject){
                        vertices.push_back(jobject{.obj_type=START_ARRAY, .id=vertices.size(), .key_index=offset, .value_index=object(), .start=offset, .end=offset+1, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().id);
                        depth++;
                    // }
                    // else{
                    //     // if(vertices.back().obj_type==PAIR_KEY || hitComma)depth++;
                    //     vertices.push_back(jobject{.obj_type=START_ARRAY, .id=vertices.size(), .key_index=offset, .value_index=object(), .start=offset, .end=offset+1, .depth=depth});
                    //     if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                    //     depthProfile[depth].push_back(vertices.back().id);
                    //     depth++;
                    // }
                    hitColon=false;
                    hitComma=false;
                    firstObject=false;
                }
                else if((*it)==']'){
                    // if(vertices.back().obj_type!=END_ARRAY && vertices.back().obj_type!=END_OBJ)if(depth>0)depth--;
                    if(depth>0)depth--;
                    vertices.push_back(jobject{.obj_type=END_ARRAY, .id=vertices.size(), .key_index=offset, .start=offset, .end=offset, .depth=depth});
                    if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                    depthProfile[depth].push_back(vertices.back().id);
                    // if(depth>0)depth--;
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
                    if(!hitColon){
                        // if(std::string_view(itStart, it).find("S-Polynomials")!=std::string_view::npos)
                        //    std::cout<<hitColon<<" PK "<<vertices.size()<<" "<<std::string_view(itStart, it)<<std::endl;
                        vertices.push_back(jobject{.obj_type=PAIR_KEY, .id=vertices.size(), .key_index=offset, .start=startOffset, .end=offset, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().id);
                    }
                    else{
                    // if(std::string_view(itStart, it).find("S-Polynomials")!=std::string_view::npos)
                        //    std::cout<<"\tPV "<<vertices.size()<<" "<<std::string_view(itStart, it)<<std::endl;
                        vertices.push_back(jobject{.obj_type=PAIR_VALUE, .id=vertices.size(), .key_index=offset, .value_index=std::string_view(itStart, it), .start=startOffset, .end=offset, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().id);
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
                    if(hitPeriod){
                        vertices.push_back(jobject{.obj_type=PAIR_VALUE, .id=vertices.size(), .key_index=offset, .value_index=std::strtod(v.c_str(), nullptr), .start=startOffset, .end=offset, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().id);
                    }
                    else{
                        vertices.push_back(jobject{.obj_type=PAIR_VALUE, .id=vertices.size(), .key_index=offset, .value_index=std::strtol(v.c_str(), nullptr, 10), .start=startOffset, .end=offset, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().id);
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
            /*for(std::vector<std::vector<size_t>>::iterator it=depthProfile.begin();it!=depthProfile.end();it++){
                std::cout<<(std::distance(depthProfile.begin(), it))<<std::endl;
                std::string depthProfile=fmt::format("{}\n", (*it));
                std::cout<<"\t"<<depthProfile;
            }*/
    //auto edgeTime = std::chrono::high_resolution_clock::now();
            for(std::vector<sylvanmats::io::json::jobject>::iterator itDag=vertices.begin()+dagOffset;itDag!=vertices.end();itDag++){
                size_t currentDepth=(*itDag).depth;
                if((*itDag).obj_type==START_OBJ || (*itDag).obj_type==START_ARRAY){
                    //if(currentDepth==0)
//                        std::cout<<"START_OBJ "<<(*itDag).id<<" "<<currentDepth<<" "<<depthProfile.size()<<std::endl;
                    if(currentDepth>0){
                        //std::cout<<"this depth size "<<depthProfile[currentDepth-1].size()<<std::endl;
                        OBECT_TYPE objType=((*itDag).obj_type==START_OBJ) ? START_OBJ : START_ARRAY;
                        bool hit=false;
                        size_t parentObjSize=bisect(currentDepth-1, (*itDag).id, hit);
                        if(hit)edges.push_back(std::make_tuple(vertices[parentObjSize].id, (*itDag).id, 1));
                    }
                }
                else if((*itDag).obj_type==END_OBJ || (*itDag).obj_type==END_ARRAY){
                    //if(currentDepth==0)
//                        std::cout<<"END_OBJ "<<(*itDag).id<<" "<<currentDepth<<" "<<depthProfile.size()<<std::endl;
                    if(currentDepth>=0){
                        OBECT_TYPE objType=((*itDag).obj_type==END_OBJ) ? END_OBJ : END_ARRAY;
                        bool hit=false;
                        size_t parentObjSize=bisect(currentDepth, objType, (*itDag).id, hit);
                        if(hit)edges.push_back(std::make_tuple(vertices[parentObjSize].id, (*itDag).id, 1));
                    }
                }
                else if((*itDag).obj_type==PAIR_VALUE || (*itDag).obj_type==VALUE_NULL){
                    edges.push_back(std::make_tuple((*itDag).id-1, (*itDag).id, 1));
                }
                else if((*itDag).obj_type==PAIR_KEY){
                    if(currentDepth>0){
                        bool hit=false;
                        size_t parentObjSize=bisect(currentDepth-1, (*itDag).id, hit);
                        if(hit)edges.push_back(std::make_tuple(vertices[parentObjSize].id, (*itDag).id, 1));
                    }
                }
            }
    //auto sortTime = std::chrono::high_resolution_clock::now();
            std::sort(edges.begin(), edges.end(), [](std::tuple<graph::vertex_id_t<G>, graph::vertex_id_t<G>, int>& a, std::tuple<graph::vertex_id_t<G>, graph::vertex_id_t<G>, int>& b){ if(std::get<0>(a)!=std::get<0>(b)){return std::get<0>(a)<std::get<0>(b);} return std::get<1>(a)<std::get<1>(b);});
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
            dagGraph.load_edges(edges, [](const auto& c) -> graph::copyable_edge_t<size_t, int> { return {std::get<0>(c), std::get<1>(c), std::get<2>(c)}; });
            dagGraph.load_vertices(vertices, [](const sylvanmats::io::json::jobject& u) -> graph::copyable_vertex_t<size_t, sylvanmats::io::json::jobject> { return {u.id, u}; });
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

    bool Binder::match(Path& jp, bool last, std::function<bool(size_t id, std::string_view key, std::any& v)> apply){
        bool hit=false;
        size_t pi=0;
        int countMap=0;
        if(jp.p.empty()){
            if(apply(0, substr_view(jsonContent, vertices[0].start, vertices[0].end), vertices[0].value_index))hit=true;
            return hit;
        }
        auto it = std::ranges::find_if(dagGraph.vertex_ids(),
                                 [&](auto vid) { return dagGraph.vertex_value(vid).id==0; });
        graph::vertex_id_t<G> vid=static_cast<graph::vertex_id_t<G>>(it - std::begin(dagGraph.vertex_ids()));
        auto dfs      = graph::views::vertices_dfs(dagGraph, vid);
        size_t depth=dfs.depth();
        size_t count=0;
        size_t count2=0;
        for (auto&& [u] : dfs) {
            auto uid=graph::vertex_id(dagGraph, u);
            // size_t inCount=0;
            // for (auto&& e : graph::views::in_incidence(dagGraph, u)) {
            //     auto source_id = graph::adj_list::source_id(dagGraph, e);
            //     inCount++;
            // }
            // std::cout<<uid<<" "<<inCount<<std::endl;
          size_t currentDepth=graph::vertex_value(dagGraph, u).depth;
          count++;
          if(currentDepth==0)continue;
          count2++;
          currentDepth--;
          if(currentDepth>=jp.p.size()){
                continue;
           }
          if(currentDepth<jp.p.size()){
            if(vertices[uid].obj_type==PAIR_KEY && (jp.p[currentDepth].label.compare("*")==0 || substr_view(jsonContent, vertices[uid].start, vertices[uid].end).compare(jp.p[currentDepth].label)==0)){
            //    if(jp.p[currentDepth].action==TEST)std::cout<<vid<<" test dfs: "<<uid<<" "<<graph::vertex_value(dagGraph, u).id<<" "<<currentDepth<<" "<<jp.p[currentDepth].label<<std::endl;
                if(currentDepth<jp.p.size() && jp.p[currentDepth].action==TEST){
                //    std::cout<<vid<<"\ttest: "<<substr_view(jsonContent, vertices[uid].start, vertices[uid].end)<<" "<<jp.p[currentDepth].label<<" "<<(vertices[uid+1].obj_type==PAIR_VALUE)<<" "<<test(substr_view(jsonContent, vertices[uid+1].start, vertices[uid+1].end), jp.p[currentDepth].value)<<std::endl;
                    if(substr_view(jsonContent, vertices[uid].start, vertices[uid].end).compare(jp.p[currentDepth].label)==0 && vertices[uid+1].obj_type==PAIR_VALUE && test(substr_view(jsonContent, vertices[uid+1].start, vertices[uid+1].end), jp.p[currentDepth].value)){
                        size_t parentObjSize=depthProfile[currentDepth].back();
                        bool ihit=parentObjSize<vertices[uid].id;
                        for(std::vector<size_t>::reverse_iterator it=depthProfile[currentDepth].rbegin();!ihit && it!=depthProfile[currentDepth].rend();it++){
                            if(parentObjSize>=vertices[uid].id)parentObjSize=vertices[(*it)].id;
                            if(parentObjSize<vertices[uid].id)ihit=true;
                        }
                    //graph::vertex_id_t<G> sid=u.source_id;
                    //std::cout<<sid<<" "<<uid<<" TEST "<<jp.p[currentDepth].value<<" "<<substr_view(jsonContent, vertices[uid].start, vertices[uid].end)<<" "<<substr_view(jsonContent, vertices[parentObjSize].start, vertices[parentObjSize].end)<<std::endl;
                    std::any a{};
                    if(apply(parentObjSize, substr_view(jsonContent, vertices[parentObjSize].start, vertices[parentObjSize].end), a))hit=true;
                    if(!singleMatch && hit && currentDepth>0 && jp.p[currentDepth-1].label.compare("*")!=0)dfs.cancel(graph::views::cancel_search::cancel_all);
                    }
                }
                else if(currentDepth<jp.p.size() && vertices[uid+1].obj_type==START_ARRAY){
                    // std::cout<<"START_ARRAY "<<substr_view(jsonContent, vertices[uid].start, vertices[uid].end)<<std::endl;
                }
                else if(currentDepth<jp.p.size()-1 && vertices[uid+1].obj_type==START_OBJ && jp.p[currentDepth].label.compare("*")==0){
                    // std::cout<<"START_OBJ "<<substr_view(jsonContent, vertices[uid].start, vertices[uid].end)<<std::endl;
                }
                else if(currentDepth==jp.p.size()-1 && vertices[uid+1].obj_type==START_OBJ){
//                    std::cout<<"PAIR_KEY -> START_OBJ "<<substr_view(jsonContent, vertices[uid+1].start, vertices[uid+1].end)<<std::endl;
                    std::any a{};
                    if(apply(vertices[uid+1].id, substr_view(jsonContent, vertices[uid].start, vertices[uid].end), a))hit=true;
                    //std::cout<<"hit "<<hit<<std::endl;
                    if(!singleMatch && hit && currentDepth>0 && jp.p[currentDepth-1].label.compare("*")!=0)dfs.cancel(graph::views::cancel_search::cancel_all);
                }
                else if(currentDepth==jp.p.size()-1 && vertices[uid+1].obj_type==PAIR_VALUE){
                    if(apply(vertices[uid].id, substr_view(jsonContent, vertices[uid].start, vertices[uid].end), vertices[uid+1].value_index))hit=true;
                    if(!singleMatch && hit && currentDepth>0 && jp.p[currentDepth-1].label.compare("*")!=0)dfs.cancel(graph::views::cancel_search::cancel_branch);
                }
            }
            else if(currentDepth==jp.p.size()-1 && (vertices[uid].obj_type==START_OBJ)){
                for (auto&& oe : graph::adj_list::out_edges(dagGraph, u)) {
                    auto oid=graph::target_id(dagGraph, oe);
                    //graph::edge_value(g, v);
                    if(apply(vertices[oid].id, substr_view(jsonContent, vertices[oid].start, vertices[oid].end), vertices[oid].value_index))hit=true;
                    if(!singleMatch && hit && currentDepth>0 && jp.p[currentDepth-1].label.compare("*")!=0)dfs.cancel(graph::views::cancel_search::cancel_branch);
                }
            }
           else if(currentDepth<jp.p.size() && jp.p[currentDepth].label.compare("*")==0){
            // std::cout<<currentDepth<<" at * match status "<<count<<" "<<count2<<" "<<hit<<std::endl;
           }
//            else {};
          }
          else if(currentDepth<jp.p.size() && jp.p[currentDepth].label.compare("*")==0){continue;}
          else if(!singleMatch)dfs.cancel(graph::views::cancel_search::cancel_branch);
        }
//        std::cout<<"match status "<<count<<" "<<count2<<" "<<hit<<std::endl;
        singleMatch=false;
        return hit;
    }
    
}