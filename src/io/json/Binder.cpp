#include <cstdio>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <chrono>

#include "io/json/Binder.h"

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
            size_t insertionObjSize=(!dag.empty() && dag.size()>=2) ? dag.size()-2 : 1;
            size_t insertionOffset=/*(!dag.empty() && dag.size()>=2) ?*/ findInsertionOffset(dag[insertionObjSize].first.end);// : jsonContent.size() - 1;
            
//                std::cout<<dag.back().second.size()<<" "<<insertionOffset<<" indention: "<<dag.back().second.back().key<<" "<<dag.back().second.back().obj_type<<std::endl;
//                auto s=objects[objects.size()-2];
//                size_t insertionOffset=s.key_index+1;//findInsertionOffset(s.key_index);
                size_t indention=(!dag.empty() && dag.size()>=2) ?  depthList[insertionObjSize] : depthList[insertionObjSize]+1;
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
                if(dag[obj_size].first.obj_type==END_OBJ || dag[obj_size].first.obj_type==END_ARRAY){
                    indention=depthList[obj_size]+1;
                    if(dag[obj_size-1].first.obj_type==START_OBJ || dag[obj_size-1].first.obj_type==START_ARRAY){
                        insertionObjSize=obj_size-1;
                        insertionOffset=findInsertionOffset(dag[insertionObjSize].first.end);
                        wrap=true;
                    }
                    else{
                        insertionObjSize=obj_size-1;
                        insertionOffset=findInsertionOffset(dag[insertionObjSize].first.end);
                       comma=true;
                        wrap=true;
                    }
                }
                else if(dag[obj_size].first.obj_type==PAIR_VALUE){
//                    std::cout<<"label "<<jp.p.back().label<<" "<<obj_size<<" "<<dag.size()<<" "<<depthList.size()<<std::endl;
                    indention=depthList[obj_size];
                    insertionObjSize=obj_size;
                       insertionOffset=findInsertionOffset(dag[insertionObjSize].first.end);
                       comma=true;
                        wrap=true;
                }
                std::string&& kv=typeset(comma, wrap, indention, newKey, newValue);
//                std::cout<<"|"<<kv<<"|"<<std::endl;
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
                    indention=depthList[obj_size];
                    size_t insertionObjSize=obj_size;
                    if(dag[obj_size].first.obj_type==END_OBJ || dag[obj_size].first.obj_type==END_ARRAY){
                        indention++;
                        insertionObjSize=obj_size-1;
                     }
                    insertionOffset=findInsertionOffset(dag[insertionObjSize].first.end);
                    firstCall=false;
                }
                else comma=true;
                insertableBlock.append(typeset(comma, wrap, indention, newKey, newValue));

            notfinal=finialize;
            }while(!notfinal);
            jsonContent.insert(insertionOffset, insertableBlock);
            dag.clear();
            depthList.clear();
            bind(0);
            return true;
        });
    };
    //remove
    bool Binder::operator ()(Path& jp, std::string removalKey){
        bool ret=false;
        bool hit=match(jp, false, [&](size_t obj_size, std::string_view key, std::any& v)->bool{
            for(jobject& o : dag[obj_size].second | std::views::filter([&](jobject& i){return substr_view(jsonContent, i.start, i.end).compare(removalKey)==0;})){
                if(dag[o.obj_size+1].first.obj_type==START_OBJ || dag[o.obj_size+1].first.obj_type==START_ARRAY){
                    std::string::size_type start=dag[o.obj_size].first.start-1;
                    std::string::size_type offset=dag[dag[o.obj_size+1].second.back().obj_size+1].first.end+1;
                    jsonContent.erase(start, offset-start);
//                    std::cout<<jsonContent<<std::endl;
                    dag.clear();
                    depthList.clear();
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
            apply(key, v);
//            for(auto  vi=std::next(dag[obj_size].second.begin());vi!=dag[obj_size].second.end();++vi){
//                if((*vi).obj_type==PAIR_KEY && (*std::next(vi)).obj_type==PAIR_VALUE)
//                    apply((*vi).key, (*std::next(vi)).value_index);
//            }
            return false;
        });
    }
        
    void Binder::display(){
            std::string depthText=fmt::format("{}\n", depthList);
            std::cout<<depthText;
        for(std::vector<std::pair<sylvanmats::io::json::jobject, std::vector<sylvanmats::io::json::jobject>>>::iterator itDAG=dag.begin();itDAG!=dag.end();++itDAG){
            for(size_t ti=0;ti<depthList[(*itDAG).first.obj_size];ti++)std::cout<<"    ";
            std::cout<<jsonContent.substr((*itDAG).first.start, (*itDAG).first.end-(*itDAG).first.start)<<"\t\t"<<(*itDAG).second.size()<<std::endl;
        }
    }
    
    void Binder::shortenDAG(std::string::size_type insertionOffset, std::string::size_type insertionObjSize){
            auto startReduction = std::chrono::high_resolution_clock::now();
            size_t currentDAGSize=dag.size();
                dag.resize(insertionObjSize);
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
                }
//                std::cout<<currentDAGSize<<" resize "<<inObjSize<<" "<<dag.size()<<" indent "<<depthList.back()<<" insertionOffset "<<insertionOffset<<" total size: "<<jsonContent.size()<<" "<<insertionObjSize<<" "<<depthList.back()<<" start obj: "<<(dag.back().first.obj_type==START_OBJ)<<std::endl;
            auto endReduction = std::chrono::high_resolution_clock::now();
                int objDiffStart=countObjects();
                bind(insertionOffset, depthList.back());
                objDiff=countObjects()-objDiffStart;
            auto endBind = std::chrono::high_resolution_clock::now();
                reductionTime=std::chrono::duration_cast<std::chrono::nanoseconds>(endReduction-startReduction).count()*1.0e-9;
                bindTime=std::chrono::duration_cast<std::chrono::nanoseconds>(endBind-endReduction).count()*1.0e-9;
    }
    
    void Binder::bind(std::string::size_type startOffset, size_t startDepth){
        objectCount=0;
        size_t depth=startDepth;
        size_t dagOffset=(!dag.empty())? dag.size()-1 : 0;
        std::span s={jsonContent};
        {
            std::span<char>::iterator it=(startOffset>0) ? s.begin()+startOffset : s.begin();
            size_t offset=startOffset;
            bool firstObject=startOffset==0;
            bool hitColon=false;
            bool hitPeriod=false;
            std::vector<std::vector<size_t>> depthProfile;
            for(int di=0;di<depth;di++)depthProfile.push_back(std::vector<size_t>{});
            while(it!=s.end()){
                if(isNull(s, it)){
                    vertices.push_back(jobject{.obj_type=VALUE_NULL, .obj_size=vertices.size(), .value_index=std::string_view(it, it+4), .start=offset, .end=offset+4, .depth=depth});
                    if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                    depthProfile[depth].push_back(vertices.back().obj_size);
                    dag.push_back(std::make_pair(jobject{.obj_type=VALUE_NULL, .obj_size=dag.size(), .value_index=std::string_view(it, it+4), .start=offset, .end=offset+4}, std::vector<jobject>{}));
                    depthList.push_back(depth);
                    it+=4;
                    offset+=4;
                    hitColon=false;
                }
                if((*it)=='{'){
                    if(firstObject){
                        vertices.push_back(jobject{.obj_type=START_OBJ, .obj_size=vertices.size(), .value_index=object(), .start=offset, .end=offset+1, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().obj_size);
                        dag.push_back(std::make_pair(jobject{.obj_type=START_OBJ, .obj_size=dag.size(), .value_index=object(), .start=offset, .end=offset+1}, std::vector<jobject>{}));
                    }
                    else{
//                        if(dag.empty())key="/";
                       vertices.push_back(jobject{.obj_type=START_OBJ, .obj_size=vertices.size(), .key_index=offset, .value_index=object(), .start=offset, .end=offset+1, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().obj_size);
                       dag.push_back(std::make_pair(jobject{.obj_type=START_OBJ, .obj_size=dag.size(), .key_index=offset, .value_index=object(), .start=offset, .end=offset+1}, std::vector<jobject>{}));                        
                    }
                    depthList.push_back(depth);
                    depth++;
                    hitColon=false;
                    firstObject=false;
                    objectCount++;
                }
                else if((*it)=='}'){
                    vertices.push_back(jobject{.obj_type=END_OBJ, .obj_size=vertices.size(), .key_index=offset, .start=offset, .end=offset+1, .depth=depth});
                    if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                    depthProfile[depth].push_back(vertices.back().obj_size);
                    dag.push_back(std::make_pair(jobject{.obj_type=END_OBJ, .obj_size=dag.size(), .key_index=offset, .start=offset, .end=offset+1}, std::vector<jobject>{}));
                    if(depth>0)depth--;
                    depthList.push_back(depth);
                    hitColon=false;
                }
                else if((*it)=='['){
                    if(firstObject){
                        vertices.push_back(jobject{.obj_type=START_ARRAY, .obj_size=vertices.size(), .value_index=object(), .start=offset, .end=offset+1, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().obj_size);
                        dag.push_back(std::make_pair(jobject{.obj_type=START_ARRAY, .obj_size=dag.size(), .value_index=object(), .start=offset, .end=offset+1}, std::vector<jobject>{}));
                    }
                    else{
//                        if(dag.empty())key="/";
                       vertices.push_back(jobject{.obj_type=START_ARRAY, .obj_size=vertices.size(), .key_index=offset, .value_index=object(), .start=offset, .end=offset+1, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().obj_size);
                       dag.push_back(std::make_pair(jobject{.obj_type=START_ARRAY, .obj_size=dag.size(), .key_index=offset, .value_index=object(), .start=offset, .end=offset+1}, std::vector<jobject>{}));                        
                    }
                    depthList.push_back(depth);
                    depth++;
                    hitColon=false;
                    firstObject=false;
                }
                else if((*it)==']'){
                    vertices.push_back(jobject{.obj_type=END_ARRAY, .obj_size=vertices.size(), .key_index=offset, .start=offset, .end=offset, .depth=depth});
                    if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                    depthProfile[depth].push_back(vertices.back().obj_size);
                    dag.push_back(std::make_pair(jobject{.obj_type=END_ARRAY, .obj_size=dag.size(), .key_index=offset, .start=offset, .end=offset}, std::vector<jobject>{}));
                    if(depth>0)depth--;
                    depthList.push_back(depth);
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
//                        if(std::string_view(itStart, it).compare("8DR")==0)
//                            std::cout<<"PK "<<dag.size()<<" |"<<std::string_view(itStart, it)<<"|"<<startOffset<<" "<<(offset-startOffset)<<std::endl;
                        vertices.push_back(jobject{.obj_type=PAIR_KEY, .obj_size=vertices.size(), .key_index=offset, .start=startOffset, .end=offset, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().obj_size);
                        dag.push_back(std::make_pair(jobject{.obj_type=PAIR_KEY, .obj_size=dag.size(), .key_index=offset, .start=startOffset, .end=offset}, std::vector<jobject>{}));                        
                    }
                    else{
//                        if(std::string_view(itStart, it).compare("H")==0)std::cout<<"PV "<<dag.size()<<" "<<std::string_view(itStart, it)<<std::endl;
                        vertices.push_back(jobject{.obj_type=PAIR_VALUE, .obj_size=vertices.size(), .key_index=offset, .value_index=std::string_view(itStart, it), .start=startOffset, .end=offset, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().obj_size);
                        dag.push_back(std::make_pair(jobject{.obj_type=PAIR_VALUE, .obj_size=dag.size(), .key_index=offset, .value_index=std::string_view(itStart, it), .start=startOffset, .end=offset}, std::vector<jobject>{}));                        
                        hitColon=false;
                    }
                    depthList.push_back(depth);
                    
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
                        dag.push_back(std::make_pair(jobject{.obj_type=PAIR_VALUE, .obj_size=dag.size(), .key_index=offset, .value_index=std::strtod(v.c_str(), nullptr), .start=startOffset, .end=offset}, std::vector<jobject>{}));                        
                    }
                    else{
                        vertices.push_back(jobject{.obj_type=PAIR_VALUE, .obj_size=dag.size(), .key_index=offset, .value_index=std::strtol(v.c_str(), nullptr, 10), .start=startOffset, .end=offset, .depth=depth});
                        if(depth>=depthProfile.size())depthProfile.push_back(std::vector<size_t>{});
                        depthProfile[depth].push_back(vertices.back().obj_size);
                        dag.push_back(std::make_pair(jobject{.obj_type=PAIR_VALUE, .obj_size=vertices.size(), .key_index=offset, .value_index=std::strtol(v.c_str(), nullptr, 10), .start=startOffset, .end=offset}, std::vector<jobject>{}));                        
                    }
                    depthList.push_back(depth);
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
//            std::string depthText=fmt::format("{}\n", depthList);
//            std::cout<<depthText;
            /*for(std::vector<std::vector<size_t>>::iterator it=depthProfile.begin();it!=depthProfile.end();it++){
                std::cout<<(std::distance(depthProfile.begin(), it))<<std::endl;
                std::string depthProfile=fmt::format("{}\n", (*it));
                std::cout<<"\t"<<depthProfile;
            }*/
            for(std::vector<sylvanmats::io::json::jobject>::iterator itDag=vertices.begin()+dagOffset;itDag!=vertices.end();itDag++){
                size_t currentDepth=(*itDag).depth;
                if((*itDag).obj_type==START_OBJ || (*itDag).obj_type==START_ARRAY){
                    if(currentDepth>0){
                        size_t parentObjSize=depthProfile[currentDepth-1].back();
                        bool hit=false;
                        for(std::vector<size_t>::reverse_iterator it=depthProfile[currentDepth-1].rbegin();!hit && it!=depthProfile[currentDepth-1].rend();it++)
                            if(parentObjSize>=(*itDag).obj_size)parentObjSize=vertices[(*it)].obj_size;
                            else hit=true;
                        if(hit)edges.push_back(std::make_tuple((*itDag).obj_size, vertices[parentObjSize].obj_size, 1));
                    }
                }
                else if((*itDag).obj_type==END_OBJ || (*itDag).obj_type==END_ARRAY){
                    if(currentDepth>0){
                        OBECT_TYPE objType=((*itDag).obj_type==END_OBJ) ? START_OBJ : START_ARRAY;
                        size_t parentObjSize=depthProfile[currentDepth].back();
                        bool hit=false;
                        for(std::vector<size_t>::reverse_iterator it=depthProfile[currentDepth].rbegin();!hit && it!=depthProfile[currentDepth].rend();it++)
                            if(parentObjSize>=(*itDag).obj_size || vertices[parentObjSize].obj_type!=objType)parentObjSize=vertices[(*it)].obj_size;
                            else hit=true;
                        edges.push_back(std::make_tuple((*itDag).obj_size, vertices[parentObjSize].obj_size, 1));
                    }
                }
                else if((*itDag).obj_type==VALUE_PAIR || (*itDag).obj_type==PAIR_KEY || (*itDag).obj_type==PAIR_VALUE){
                    if(currentDepth>0){
                        size_t parentObjSize=depthProfile[currentDepth-1].back();
                        bool hit=false;
                        for(std::vector<size_t>::reverse_iterator it=depthProfile[currentDepth-1].rbegin();!hit && it!=depthProfile[currentDepth-1].rend();it++)
                        if(parentObjSize>=(*itDag).obj_size)parentObjSize=vertices[(*it)].obj_size;
                        else hit=true;
//                        std::cout<<"VP "<<hit<<" "<<parentObjSize<<" "<<(*itDag).obj_size<<std::endl;
                        if(hit)edges.push_back(std::make_tuple((*itDag).obj_size, vertices[parentObjSize].obj_size, 1));
                    }
                }
            }
            depth=startDepth;
            std::vector<std::pair<jobject, std::vector<jobject>>>::iterator previousNode=dag.end();
            if(dagOffset>0){
                previousNode=dag.begin()+dagOffset;
                if((*previousNode).first.obj_type!=START_OBJ && (*previousNode).first.obj_type!=START_ARRAY){
                    previousNode--;
                }
            }
            for(std::vector<std::pair<jobject, std::vector<jobject>>>::iterator itDag=dag.begin()+dagOffset;itDag!=dag.end();itDag++){
                size_t currentDepth=depth;
                if((*itDag).first.obj_type==START_OBJ || (*itDag).first.obj_type==START_ARRAY){
                    if(previousNode!=dag.end()){
                        (*previousNode).second.push_back((*itDag).first);
                        (*itDag).second.push_back((*previousNode).first);
                    }
                    previousNode=itDag;
                    depth++;
                }
                else if((*itDag).first.obj_type==END_OBJ || (*itDag).first.obj_type==END_ARRAY){
                    if(previousNode!=dag.end()){
                        size_t currentOffset=(*itDag).first.obj_size;
                        size_t vOffset=(currentOffset>0) ? currentOffset-1: 0;
                        int currentDepth=(depthList[currentOffset]>0 && currentOffset<depthList.size()-1 && depthList[currentOffset+1]==depthList[currentOffset]) ? depthList[currentOffset]-1: depthList[currentOffset];
                        while(vOffset>0 && currentDepth<depthList[vOffset]){
                            vOffset--;
                        }
//                        if(vOffset>0 && currentOffset<depthList.size()-1 && depthList[currentOffset+1]<depthList[currentOffset])vOffset--;
//                        std::cout<<depthList[currentOffset]<<" "<<currentOffset<<" "<<vOffset<<" "<<depthList[vOffset]<<std::endl;
                        previousNode=std::next(dag.begin(), vOffset);
                        (*previousNode).second.push_back((*itDag).first);
                        (*itDag).second.push_back((*previousNode).first);
                    }
                }
                else if((*itDag).first.obj_type==VALUE_PAIR || (*itDag).first.obj_type==PAIR_KEY || (*itDag).first.obj_type==PAIR_VALUE){
                    if(previousNode!=dag.end()){
                        (*previousNode).second.push_back((*itDag).first);
                        (*itDag).second.push_back((*previousNode).first);
                    }
                }
            }
            using value = std::ranges::range_value_t<decltype(edges)>;
            graph::vertex_id_t<G> N = static_cast<graph::vertex_id_t<G>>(size(graph::vertices(dagGraph)));
            using edge_desc  = graph::edge_descriptor<graph::vertex_id_t<G>, true, void, int>;
            //dagGraph.reserve(3);
            dagGraph.load_vertices(vertices, [&](sylvanmats::io::json::jobject& nm) {
                auto uid = static_cast<graph::vertex_id_t<G>>(&nm - vertices.data());
                return graph::copyable_vertex_t<graph::vertex_id_t<G>, sylvanmats::io::json::jobject>{uid, nm};
              });
            dagGraph.load_edges(edges, [](const value& val) -> edge_desc {
                    //std::cout<<"edge "<<std::get<0>(val)<<" "<<std::get<1>(val)<<" "<<std::get<2>(val)<<std::endl;
                return edge_desc{std::get<0>(val), std::get<1>(val), std::get<2>(val)};
              }, N);
            
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
            if(apply(0, substr_view(jsonContent, dag[0].first.start, dag[0].first.end), dag[0].first.value_index))hit=true;
        }
        else
        while(!hit && pi<jp.p.size()){
        for(node_iterator<size_t> ni(dag);!hit && ni!=ni.end();++ni){
            if(depthList[(*ni)]==0)matchMap[(*ni)]=true;
            else if(pi>0 && !dag[(*ni)].second.empty() && depthList[dag[(*ni)].second.front().obj_size]==pi){
                matchMap[(*ni)]=true;
                countMap++;
            }
            if(depthList[(*ni)]==pi && matchMap.count((*ni)) && matchMap[(*ni)]){
            for(out_edge_iterator<size_t> oei(dag, ni); !hit && oei!=oei.end(); ++oei){
//                if(depthList[(*oei)]==pi+1 && dag[(*oei)].first.key.compare(jp.p[pi].label)==0)
//                        std::cout<<" "<<jp.p[pi].label<<" "<<(*oei)<<" "<<depthList[(*oei)]<<" ? "<<(pi+1)<<" "<<dag[(*oei)].first.obj_type<<" "<<substr_view(jsonContent, dag[(*oei)].first.start, dag[(*oei)].first.end)<<" "<<dag[(*oei)].first.end<<std::endl;
                if(depthList[(*oei)]==pi+1)
                    if(dag[(*oei)].first.obj_type==PAIR_KEY && (jp.p[pi].label.compare("*")==0 || substr_view(jsonContent, dag[(*oei)].first.start, dag[(*oei)].first.end).compare(jp.p[pi].label)==0)){
//                        auto nx = std::next(itS);
                        Node<size_t> nextNode((*oei)+1);
//                        std::cout<<" "<<jp.p[pi].label<<" "<<depthList[(*nextNode)]<<" "<<(pi+1)<<std::endl;
                        if(pi+1>=jp.p.size() && jp.p[pi].action==TEST){
//                            std::cout<<depthList[(*nextNode)]<<" "<<(pi+1)<<" "<<dag[(*nextNode)].first.obj_type<<" in test "<<jp.p[pi].value<<" "<<dag[(*nextNode)].first.key<<" "<<dag[(*nextNode)].first.value_index.has_value()<<" "<<std::type_index(typeid(dag[(*nextNode)].first.value_index)).name()<<" "<<std::type_index(typeid(std::string_view)).name()<<" "<<type_names[std::type_index(dag[(*nextNode)].first.value_index.type())]<<" "<<std::any_cast<std::string_view>(dag[(*nextNode)].first.value_index)<<std::endl;
                            if((*nextNode)<dag.size() && depthList[(*nextNode)]==pi+1 && dag[(*nextNode)].first.obj_type==PAIR_VALUE && dag[(*nextNode)].first.value_index.has_value() && jp.p[pi].value.compare(std::any_cast<std::string_view>(dag[(*nextNode)].first.value_index))==0){
//                                std::cout<<pi<<" TEST "<<jp.p[pi].label<<" "<<jp.p[pi].value<<std::endl;
                                Node<size_t> previousNode(dag[(*oei)].second.front().obj_size);
                                for(out_edge_iterator<size_t> oeiA(dag, previousNode); !hit && oeiA!=oeiA.end(); ++oeiA){
//                                    if(dag[(*oeiA)].first.obj_type==PAIR_KEY)std::cout<<" "<<dag[(*oeiA)].first.key;
                                    if(dag[(*oeiA)].first.obj_type==PAIR_KEY && dag[(*oeiA)+1].first.obj_type==PAIR_VALUE){
//                                    std::cout<<dag[(*nextNode)].second.size()<<" "<<dag[(*oeiA)].first.key<<" how many "<<(*oeiA)<<std::endl;
                                        if(apply(dag[(*oeiA)].first.obj_size, substr_view(jsonContent, dag[(*oeiA)].first.start, dag[(*oeiA)].first.end), dag[(*oeiA)+1].first.value_index))hit=true;
                                    }
                                }
                                
                            }
                        }
                        else if((*nextNode)<dag.size() && depthList[(*nextNode)]==pi+1 && dag[(*nextNode)].first.obj_type==PAIR_VALUE){
                            if(pi+1>=jp.p.size()){
                                if(apply((*oei), substr_view(jsonContent, dag[(*oei)].first.start, dag[(*oei)].first.end), dag[(*nextNode)].first.value_index))hit=true;
                                if(jp.p[pi].label.compare("*")!=0)hit=true;
                            }
                            else if(jp.p[pi].label.compare("*")==0){
                                matchMap[(*oei)]=true;
                                matchMap[(*oei)+1]=true;
                            }
//                            else{
//                                currentChildren=dag[(*itS).obj_size].second;
//                                pi++;
//                            }
//                            matched=true;
                        }
                        else if((*nextNode)<dag.size() && (dag[(*nextNode)].first.obj_type==START_OBJ || dag[(*nextNode)].first.obj_type==START_ARRAY)){
                            if(pi+1>=jp.p.size()){
//                                std::cout<<"obj or array "<<dag[(*nextNode)].first.obj_type<<" "<<dag[(*nextNode)].second.size()<<std::endl;
                                if(dag[(*nextNode)].first.obj_type==START_OBJ){
                                    if(jp.p[pi].label.compare("*")==0){
                                        if(apply(dag[(*oei)].first.obj_size, substr_view(jsonContent, dag[(*oei)].first.start, dag[(*oei)].first.end), dag[(*oei)].first.value_index))hit=true;
                                    }
                                    else if(last){
                                        if(apply(dag[(*nextNode)].second.back().obj_size, substr_view(jsonContent, dag[(*nextNode)].second.back().start, dag[(*nextNode)].second.back().end), dag[(*nextNode)].second.back().value_index))hit=true;
                                    }
                                    else
                                    for(out_edge_iterator<size_t> oeiA(dag, nextNode); !hit && oeiA!=oeiA.end(); ++oeiA){
//                                        if(dag[(*oeiA)].first.obj_type==PAIR_KEY)std::cout<<" "<<dag[(*oeiA)].first.key;
                                        if(dag[(*oeiA)].first.obj_type==PAIR_KEY && dag[(*oeiA)+1].first.obj_type==PAIR_VALUE){
//                                        std::cout<<dag[(*nextNode)].second.size()<<" "<<dag[(*oeiA)].first.key<<" how many "<<(*oeiA)<<std::endl;
                                            if(apply(dag[(*oeiA)].first.obj_size, substr_view(jsonContent, dag[(*oeiA)].first.start, dag[(*oeiA)].first.end), dag[(*oeiA)+1].first.value_index))hit=true;
                                        }
                                    }
                                    if(jp.p[pi].label.compare("*")!=0)hit=true;
                                }
                                else if(dag[(*nextNode)].first.obj_type==START_ARRAY){
    //                            std::cout<<pi<<" "<<jp.p[pi].label<<" inner obj "<<dag[(*nextNode)].first.obj_size<<" "<<dag[(*nextNode)].first.key;
                                    for(out_edge_iterator<size_t> oeiA(dag, nextNode); !hit && oeiA!=oeiA.end(); ++oeiA){
    //                                    if(dag[(*oeiA)].first.obj_type==PAIR_KEY)std::cout<<" "<<dag[(*oeiA)].first.key;
                                        if(dag[(*oeiA)].first.obj_type==PAIR_KEY)
                                        if(apply(dag[(*oeiA)].first.obj_size, substr_view(jsonContent, dag[(*oeiA)].first.start, dag[(*oeiA)].first.end), dag[(*oeiA)].first.value_index))hit=true;
                                    }
                                    if(jp.p[pi].label.compare("*")!=0)hit=true;
    //                            std::cout<<std::endl;
                                }
                           }
                            else if(jp.p[pi].label.compare("*")==0){
                                matchMap[(*oei)]=true;
                                matchMap[(*nextNode)]=true;
                            }
                            else {
                                matchMap[(*oei)]=true;
                                matchMap[(*nextNode)]=true;
//                            std::cout<<jp.p[pi].label<<"cont inner obj "<<std::endl;
//                                currentChildren=dag[(*nx).obj_size].second;
//                                pi++;
                            }
//                            matched=true;
                        }
//                        else{
//                            std::cout<<" else "<<(*nx).obj_size<<" "<<(*nx).obj_type<<" "<<std::endl;
//                        }
                    }
                    else if(jp.p[pi].label.compare("*")==0){
//                        std::cout<<pi<<" any "<<jp.p[pi].label<<std::endl;
                        matchMap[(*oei)]=true;
//                        matchMap[(*nextNode)]=true;
                    }
                    
            }
//            if(count>=2)std::cout<<"ni "<<(*ni)<<" "<<count<<" "<<countObj<<" "<<countFinal<<std::endl;
            }
        }
        pi++;
        }
        return hit;
    }
    
}