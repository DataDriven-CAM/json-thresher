#include <cstdio>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <vector>
#include <chrono>
#include <typeinfo>

#include "io/json/Binder.h"

#include "graph/views/depth_first_search.hpp"

namespace sylvanmats::io::json{
    
    void Binder::operator ()(std::istream& is){
        this->jsonContent=std::string((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
        bind(0);
    }
    
    void Binder::operator ()(std::string& jsonContent){
        this->jsonContent=jsonContent;
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
            bool hit=match(jp, true, [&](size_t obj_size, std::string_view key, std::any& v)-> bool{
                auto end = std::chrono::high_resolution_clock::now();
                matchTime=std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()*1.0e-9;
                size_t insertionOffset=0;
                size_t indention=0;
                bool comma=false;
                bool wrap=false;
                size_t insertionObjSize=obj_size;
                if(vertices[obj_size].obj_type==END_OBJ || vertices[obj_size].obj_type==END_ARRAY){
                    indention=vertices[obj_size].depth+1;
                    if(vertices[obj_size-1].obj_type==START_OBJ || vertices[obj_size-1].obj_type==START_ARRAY){
                        insertionObjSize=obj_size-1;
                        insertionOffset=findInsertionOffset(vertices[insertionObjSize].end);
                        wrap=true;
                    }
                    else{
                        insertionObjSize=obj_size-1;
                        insertionOffset=findInsertionOffset(vertices[insertionObjSize].end);
                       comma=true;
                        wrap=true;
                    }
                }
                else if(vertices[obj_size].obj_type==PAIR_VALUE){
//                    std::cout<<"label "<<jp.p.back().label<<" "<<obj_size<<" "<<dag.size()<<" "<<depthList.size()<<std::endl;
                    indention=vertices[obj_size].depth;
                    insertionObjSize=obj_size;
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
        }
        return true;
    }
        
    void Binder::operator ()(Path& jp, std::function<std::tuple<bool, std::string_view, std::any>(void)> apply){
        bool hit=match(jp, true, [&](size_t obj_size, std::string_view key, std::any& v)-> bool{
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
                    indention=vertices[obj_size].depth;
                    size_t insertionObjSize=obj_size;
                    if(vertices[obj_size].obj_type==END_OBJ || vertices[obj_size].obj_type==END_ARRAY){
                        indention++;
                        insertionObjSize=obj_size-1;
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
            dagGraph=G{};
            depthProfile.clear();
            vertices.clear();
            edges.clear();
            //dag.clear();
            //depthList.clear();
            bind(0);
            return true;
        });
    }

    //remove
    bool Binder::operator ()(Path& jp, std::string removalKey){
        bool ret=false;
        bool hit=match(jp, false, [&](size_t obj_size, std::string_view key, std::any& v)->bool{
            auto& u=dagGraph[obj_size];
             for (auto&& oe : graph::edges(dagGraph, u) | std::views::filter([&](auto& i){auto id=graph::target_id(dagGraph, i);return substr_view(jsonContent, vertices[id].start, vertices[id].end).compare(removalKey)==0;})) {
                auto oid=graph::target_id(dagGraph, oe);
                if(vertices[oid+1].obj_type==START_OBJ || vertices[oid+1].obj_type==START_ARRAY){
                    std::string::size_type start=vertices[oid].start-1;
                    std::string::size_type offset=start;
                    auto& u2=dagGraph[oid+1];
                    for (auto&& oe2 : graph::edges(dagGraph, u2)){
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
        bool hit=match(p, false, [&apply](size_t obj_size, std::string_view key, std::any& v)->bool{apply(v);return true;});
    }
        
    //traverse
    void Binder::operator ()(Path& p, std::function<void(std::string_view& key, std::any& v)> apply){
        bool hit=match(p, false, [&](size_t obj_size, std::string_view key, std::any& v)->bool{
            auto& u=dagGraph[obj_size];
             for (auto&& oe : graph::edges(dagGraph, u)) {
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
        for(std::vector<std::vector<size_t>>::iterator it=depthProfile.begin();it!=depthProfile.end();it++){
            std::cout<<(std::distance(depthProfile.begin(), it))<<std::endl;
            std::string depthProfile=fmt::format("{}\n", (*it));
            std::cout<<"\t"<<depthProfile;
        }
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
            std::cout<<" "<<hit<<" d="<<d<<" "<<edges.size()<<std::endl;
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
                            if(inObjSize>=(*it).obj_size){
                                hit=true;
                            }
                            else
                                secondSize--;
                        }
                        if(hit && secondSize>1)dag[inObjSize].second.resize(secondSize-1);
                        inObjSize=dag[inObjSize].second.front().obj_size;
                        
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
        objectCount=0;
        size_t depth=startDepth;
        size_t dagOffset=(!vertices.empty())? vertices.size()-1 : 0;
        std::span s={jsonContent};
        {
            std::span<char>::iterator it=(startOffset>0) ? s.begin()+startOffset : s.begin();
            size_t offset=startOffset;
            bool firstObject=startOffset==0;
            bool hitColon=false;
            bool hitPeriod=false;
            for(int di=0;di<depth;di++)depthProfile.push_back(std::vector<size_t>{});
            while(it!=s.end()){
                if(isNull(s, it)){
                    vertices.push_back(jobject{.obj_type=VALUE_NULL, .obj_size=vertices.size(), .value_index=std::string_view(it, it+4), .start=offset, .end=offset+4, .depth=depth});
                    if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                    depthProfile[depth].push_back(vertices.back().obj_size);
                    it+=4;
                    offset+=4;
                    hitColon=false;
                }
                if((*it)=='{'){
                    if(firstObject){
                        vertices.push_back(jobject{.obj_type=START_OBJ, .obj_size=vertices.size(), .value_index=object(), .start=offset, .end=offset+1, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().obj_size);
                        depth++;
                    }
                    else{
                        if(vertices.back().obj_type==PAIR_KEY)depth++;
                        vertices.push_back(jobject{.obj_type=START_OBJ, .obj_size=vertices.size(), .key_index=offset, .value_index=object(), .start=offset, .end=offset+1, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().obj_size);
                        depth++;
                    }
                    hitColon=false;
                    firstObject=false;
                    objectCount++;
                }
                else if((*it)=='}'){
                    if(vertices.back().obj_type!=END_ARRAY && vertices.back().obj_type!=END_OBJ)if(depth>0)depth--;
                    vertices.push_back(jobject{.obj_type=END_OBJ, .obj_size=vertices.size(), .key_index=offset, .start=offset, .end=offset+1, .depth=depth});
                    //if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                    depthProfile[depth].push_back(vertices.back().obj_size);
                    //if(depth==0)std::cout<<"EO "<<vertices.back().obj_size<<std::endl;
                    if(depth>0)depth--;
                    hitColon=false;
                }
                else if((*it)=='['){
                    if(firstObject){
                        vertices.push_back(jobject{.obj_type=START_ARRAY, .obj_size=vertices.size(), .value_index=object(), .start=offset, .end=offset+1, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().obj_size);
                        depth++;
                    }
                    else{
                        if(vertices.back().obj_type==PAIR_KEY)depth++;
                        vertices.push_back(jobject{.obj_type=START_ARRAY, .obj_size=vertices.size(), .key_index=offset, .value_index=object(), .start=offset, .end=offset+1, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().obj_size);
                        depth++;
                    }
                    hitColon=false;
                    firstObject=false;
                }
                else if((*it)==']'){
                    if(vertices.back().obj_type!=END_ARRAY && vertices.back().obj_type!=END_OBJ)if(depth>0)depth--;
                    vertices.push_back(jobject{.obj_type=END_ARRAY, .obj_size=vertices.size(), .key_index=offset, .start=offset, .end=offset, .depth=depth});
                    //if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                    depthProfile[depth].push_back(vertices.back().obj_size);
                    if(depth>0)depth--;
                    hitColon=false;
                }
                else if((*it)=='"'){
                    ++it;
                    offset++;
                    size_t startOffset=offset;
                    std::span<char>::iterator itStart=it;
                    int c=0;
                    while((*it)!='"'){if((*it)=='\\'){++it;offset++;};++it;offset++;c++;};
                    if(!hitColon){
                        vertices.push_back(jobject{.obj_type=PAIR_KEY, .obj_size=vertices.size(), .key_index=offset, .start=startOffset, .end=offset, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().obj_size);
                    }
                    else{
//                        if(std::string_view(itStart, it).compare("H")==0)
//                            std::cout<<"PV "<<dag.size()<<" "<<std::string_view(itStart, it)<<std::endl;
                        vertices.push_back(jobject{.obj_type=PAIR_VALUE, .obj_size=vertices.size(), .key_index=offset, .value_index=std::string_view(itStart, it), .start=startOffset, .end=offset, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().obj_size);
                        hitColon=false;
                    }
                    
                }
                else if((*it)=='-' || (*it)=='.' || ((*it)>='0' && (*it)<='9')){
                    std::span<char>::iterator itStart=it;
                    ++it;
                    offset++;
                    size_t startOffset=offset;
                    bool hitPeriod=((*it)=='.') ? true : false;
                    int c=0;
                    while(((*it)>='0' && (*it)<='9') || (*it)=='.'){if(!hitPeriod && (*it)=='.')hitPeriod=true;++it;offset++;c++;};
                    std::string v(itStart, it);
                    if(hitPeriod){
                        vertices.push_back(jobject{.obj_type=PAIR_VALUE, .obj_size=vertices.size(), .key_index=offset, .value_index=std::strtod(v.c_str(), nullptr), .start=startOffset, .end=offset, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().obj_size);
                    }
                    else{
                        vertices.push_back(jobject{.obj_type=PAIR_VALUE, .obj_size=vertices.size(), .key_index=offset, .value_index=std::strtol(v.c_str(), nullptr, 10), .start=startOffset, .end=offset, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().obj_size);
                    }
                    hitColon=false;
                    
                }
                else if((*it)==':'){
                    hitColon=true;
                }
                else if((*it)==','){
                    hitColon=false;
                }
                else if((*it)=='\\'){
                    ++it;
                    offset++;
                }
                ++it;
                offset++;
            }
            /*for(std::vector<std::vector<size_t>>::iterator it=depthProfile.begin();it!=depthProfile.end();it++){
                std::cout<<(std::distance(depthProfile.begin(), it))<<std::endl;
                std::string depthProfile=fmt::format("{}\n", (*it));
                std::cout<<"\t"<<depthProfile;
            }*/
            for(std::vector<sylvanmats::io::json::jobject>::iterator itDag=vertices.begin()+dagOffset;itDag!=vertices.end();itDag++){
                size_t currentDepth=(*itDag).depth;
                if((*itDag).obj_type==START_OBJ || (*itDag).obj_type==START_ARRAY){
                    //if(currentDepth==0)
//                        std::cout<<"START_OBJ "<<(*itDag).obj_size<<" "<<currentDepth<<" "<<depthProfile.size()<<std::endl;
                    if(currentDepth>0){
                        //std::cout<<"this depth size "<<depthProfile[currentDepth-1].size()<<std::endl;
                        size_t parentObjSize=depthProfile[currentDepth-1].back();
                        bool hit=parentObjSize<(*itDag).obj_size;
                        for(std::vector<size_t>::reverse_iterator it=depthProfile[currentDepth-1].rbegin();!hit && it!=depthProfile[currentDepth-1].rend();it++){
                            if(parentObjSize>=(*itDag).obj_size)parentObjSize=vertices[(*it)].obj_size;
                            if(parentObjSize<(*itDag).obj_size)hit=true;
                        }
                        //std::cout<<hit<<" Edge SO SA "<<(*itDag).obj_size<<" "<<parentObjSize<<std::endl;
                        if(hit)edges.push_back(std::make_tuple(vertices[parentObjSize].obj_size, (*itDag).obj_size, 1));
                    }
                }
                else if((*itDag).obj_type==END_OBJ || (*itDag).obj_type==END_ARRAY){
                    //if(currentDepth==0)
//                        std::cout<<"END_OBJ "<<(*itDag).obj_size<<" "<<std::endl;
                    if(currentDepth>=0){
                        OBECT_TYPE objType=((*itDag).obj_type==END_OBJ) ? START_OBJ : START_ARRAY;
                        size_t parentObjSize=depthProfile[currentDepth].back();
                        bool hit=false;
                        for(std::vector<size_t>::reverse_iterator it=depthProfile[currentDepth].rbegin();!hit && it!=depthProfile[currentDepth].rend();it++){
                            if(parentObjSize>=(*itDag).obj_size || vertices[parentObjSize].obj_type!=objType)parentObjSize=vertices[(*it)].obj_size;
                            if(parentObjSize<(*itDag).obj_size) hit=true;
                        }
                        //std::cout<<hit<<" Edge EO EA "<<(*itDag).obj_size<<" "<<parentObjSize<<std::endl;
                        if(hit)edges.push_back(std::make_tuple(vertices[parentObjSize].obj_size, (*itDag).obj_size, 1));
                    }
                }
                else if((*itDag).obj_type==PAIR_KEY || (*itDag).obj_type==PAIR_VALUE || (*itDag).obj_type==VALUE_NULL){
                    if(currentDepth>0){
                        size_t parentObjSize=depthProfile[currentDepth-1].back();
                        bool hit=false;
                        size_t offset=((*itDag).obj_type==PAIR_KEY) ? 1: 0;
                        for(std::vector<size_t>::reverse_iterator it=depthProfile[currentDepth-offset].rbegin();!hit && it!=depthProfile[currentDepth-offset].rend();it++)
                        if(parentObjSize>=(*itDag).obj_size)parentObjSize=vertices[(*it)].obj_size;
                        else hit=true;
//                        std::cout<<"VP "<<hit<<" "<<offset<<" "<<(currentDepth-offset)<<" "<<parentObjSize<<" "<<vertices[parentObjSize].obj_size<<" "<<(*itDag).obj_size<<std::endl;
                        //if(hit)
                            edges.push_back(std::make_tuple(vertices[parentObjSize].obj_size, (*itDag).obj_size, 1));
                    }
                }
            }
            std::sort(edges.begin(), edges.end(), [](std::tuple<graph::vertex_id_t<G>, graph::vertex_id_t<G>, int>& a, std::tuple<graph::vertex_id_t<G>, graph::vertex_id_t<G>, int>& b){return std::get<0>(a)<std::get<0>(b) || std::get<1>(a)<std::get<1>(b);});
            /*std::cout<<"{";
            for(std::vector<std::tuple<graph::vertex_id_t<G>, graph::vertex_id_t<G>, int>>::iterator it=edges.begin();it!=edges.end();it++){
                std::cout<<"{"<<std::get<0>((*it))<<","<<std::get<1>((*it))<<","<<std::get<2>((*it))<<"}, ";
            }
            std::cout<<"}"<<std::endl;*/
            depth=startDepth;
            using value = std::ranges::range_value_t<decltype(edges)>;
            graph::vertex_id_t<G> N = static_cast<graph::vertex_id_t<G>>(size(graph::vertices(dagGraph)));
            using edge_desc  = graph::edge_info<graph::vertex_id_t<G>, true, void, int>;
            dagGraph.reserve_vertices(vertices.size());
            dagGraph.reserve_edges(edges.size());
            dagGraph.load_edges(edges, [](const value& val) -> edge_desc {
//                    std::cout<<"edge "<<std::get<0>(val)<<" "<<std::get<1>(val)<<" "<<std::get<2>(val)<<std::endl;
                return edge_desc{std::get<0>(val), std::get<1>(val), std::get<2>(val)};
              }, N);
            dagGraph.load_vertices(vertices, [&](sylvanmats::io::json::jobject& nm) {
                auto uid = static_cast<graph::vertex_id_t<G>>(&nm - vertices.data());
//                std::cout<<"vertex "<<uid<<std::endl;
                return graph::copyable_vertex_t<graph::vertex_id_t<G>, sylvanmats::io::json::jobject>{uid, nm};
              });
            
        }
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

    bool Binder::match(Path& jp, bool last, std::function<bool(size_t obj_size, std::string_view key, std::any& v)> apply){
        bool hit=false;
        size_t pi=0;
        int countMap=0;
        std::unordered_map<size_t, bool> matchMap;
        if(jp.p.empty()){
            if(apply(0, substr_view(jsonContent, vertices[0].start, vertices[0].end), vertices[0].value_index))hit=true;
            return hit;
        }
        auto it = std::ranges::find_if(graph::vertices(dagGraph),
                                 [&](auto& u) { return graph::vertex_value(dagGraph, u).obj_size == 0; });
        graph::vertex_id_t<G> vid=static_cast<graph::vertex_id_t<G>>(it - begin(graph::vertices(dagGraph)));
        auto dfs      = graph::views::vertices_depth_first_search(dagGraph, vid);
        size_t depth=dfs.depth();
        std::vector<bool> branchQuality(jp.p.size(), false);
        graph::vertex_id_t<G> sid=vid;
        for ( auto v=dfs.begin();!hit&&v!=dfs.end();++v) {
          auto&& [uid, u] = *v;
          size_t currentDepth=graph::vertex_value(dagGraph, u).depth;
          if(currentDepth==0)continue;
          currentDepth--;
          bool good=(currentDepth<jp.p.size()) ? std::all_of(branchQuality.begin(), branchQuality.begin()+currentDepth, [](bool a){return a;}) : currentDepth==0;
            //std::string bqText=fmt::format("{}\n", branchQuality);
            //if(branchQuality[0] || jp.p[0].label.compare("elements")==0)std::cout<<good<<" "<<currentDepth<<" "<<jp.p[std::min(currentDepth, jp.p.size()-1)].label<<" "<<bqText;
          if(good && currentDepth<jp.p.size()){
            if(vertices[uid].obj_type==PAIR_KEY && (jp.p[currentDepth].label.compare("*")==0 || substr_view(jsonContent, vertices[uid].start, vertices[uid].end).compare(jp.p[currentDepth].label)==0)){
                if(jp.p[currentDepth].action==TEST)std::cout<<vid<<" test dfs: "<<uid<<" "<<graph::vertex_value(dagGraph, u).obj_size<<" "<<currentDepth<<" good: "<<good<<" "<<jp.p[currentDepth].label<<" "<<std::all_of(branchQuality.begin(), branchQuality.begin()+currentDepth, [](bool a){return a;})<<std::endl;
                if(currentDepth==jp.p.size()-1 && jp.p[currentDepth].action==TEST){
                    if(substr_view(jsonContent, vertices[uid].start, vertices[uid].end).compare(jp.p[currentDepth].label)==0 && vertices[uid+1].obj_type==PAIR_VALUE && substr_view(jsonContent, vertices[uid+1].start, vertices[uid+1].end).compare(jp.p[currentDepth].value)==0){
                        size_t parentObjSize=depthProfile[currentDepth].back();
                        bool hit=parentObjSize<vertices[uid].obj_size;
                        for(std::vector<size_t>::reverse_iterator it=depthProfile[currentDepth].rbegin();!hit && it!=depthProfile[currentDepth].rend();it++){
                            if(parentObjSize>=vertices[uid].obj_size)parentObjSize=vertices[(*it)].obj_size;
                            if(parentObjSize<vertices[uid].obj_size)hit=true;
                        }
                    //graph::vertex_id_t<G> sid=u.source_id;
                    //std::cout<<sid<<" "<<uid<<" TEST "<<jp.p[currentDepth].value<<" "<<substr_view(jsonContent, vertices[uid].start, vertices[uid].end)<<" "<<substr_view(jsonContent, vertices[parentObjSize].start, vertices[parentObjSize].end)<<std::endl;
                    std::any a{};
                    if(apply(parentObjSize, substr_view(jsonContent, vertices[parentObjSize].start, vertices[parentObjSize].end), a))hit=true;
                    }
                }
                else if(currentDepth==jp.p.size()-1 && vertices[uid+1].obj_type==START_OBJ){
//                    std::cout<<"PAIR_KEY -> START_OBJ "<<substr_view(jsonContent, vertices[uid+1].start, vertices[uid+1].end)<<std::endl;
                    std::any a{};
                    if(apply(vertices[uid+1].obj_size, substr_view(jsonContent, vertices[uid].start, vertices[uid].end), a))hit=true;
                    //std::cout<<"hit "<<hit<<std::endl;
                    if(hit)break;
                }
                else if(currentDepth==jp.p.size()-1 && vertices[uid+1].obj_type==PAIR_VALUE)
                    if(apply(vertices[uid].obj_size, substr_view(jsonContent, vertices[uid].start, vertices[uid].end), vertices[uid+1].value_index))hit=true;
                branchQuality[currentDepth]=true;
            }
            else if(currentDepth==jp.p.size() && (vertices[uid].obj_type==START_OBJ)){
                for (auto&& oe : graph::edges(dagGraph, u)) {
                    auto oid=graph::target_id(dagGraph, oe);
                    //graph::edge_value(g, v);
                    if(apply(vertices[oid].obj_size, substr_view(jsonContent, vertices[oid].start, vertices[oid].end), vertices[oid].value_index))hit=true;
                }
                branchQuality[currentDepth]=true;
            }
            else if(jp.p[currentDepth].label.compare("*")==0)branchQuality[currentDepth]=true;
            else branchQuality[currentDepth]=false;
          }
          /*else if(good && currentDepth==jp.p.size() && vertices[uid].obj_type==START_OBJ){
                for (auto&& oe : graph::edges(dagGraph, u)) {
                    auto oid=graph::target_id(dagGraph, oe);
                    //graph::edge_value(g, v);
                    if(apply(vertices[oid].obj_size, substr_view(jsonContent, vertices[oid].start, vertices[oid].end), vertices[oid].value_index))hit=true;
                }
          }*/
          else if(currentDepth<jp.p.size() && jp.p[currentDepth].label.compare("*")==0)branchQuality[currentDepth]=true;
          depth=(depth<currentDepth) ? currentDepth : 0;
          sid=uid;
        }

        return hit;
    }
    
}