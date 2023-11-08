#include <algorithm>
#include <iomanip>
#include <vector>

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
        if(jsonContent.empty()){
            jsonContent.append("{\n");
            jsonContent.append(typeset(false, false, 1, newKey, newValue));
            jsonContent.append("}\n");
//            std::cout<<(*this)<<std::endl;
            bind(0);
        }
        else if(jp.p.empty()){
            size_t insertionOffset=(!dag.empty() && dag.back().second.size()<=2) ? findInsertionOffset(dag.back().second.back().value_end) : jsonContent.size() - 1;
            
                std::cout<<dag.back().second.size()<<" "<<insertionOffset<<" indention: "<<dag.back().second.back().key<<" "<<dag.back().second.back().obj_type<<std::endl;
//                auto s=objects[objects.size()-2];
//                size_t insertionOffset=s.key_index+1;//findInsertionOffset(s.key_index);
                size_t indention=findIndention((insertionOffset>0)? insertionOffset-1 : 0)+1;
                std::cout<<" "<<insertionOffset<<" indention2: "<<indention<<" "<<jsonContent.size()<<std::endl;
                jsonContent.insert(insertionOffset, typeset(true, true, indention, newKey, newValue));
                dag.clear();
                depthList.clear();
            bind(0);
            
        }
        else {
            bool hit=match(jp, [&](size_t obj_size, std::string_view& key, std::any& v){
                size_t insertionOffset=0;
                bool comma=false;
                bool wrap=false;
                if(dag[obj_size].first.obj_type==PAIR_KEY && dag[obj_size+1].first.obj_type==START_OBJ){
                std::cout<<dag[obj_size].second.back().obj_type<<" add match pair key and child obj size:"<<" "<<dag[obj_size].second.size()<<" "<<dag[obj_size].first.key_end<<" "<<dag[obj_size].first.key<<std::endl;
                       insertionOffset=findInsertionOffset(dag[obj_size+1].first.key_end);
                }
                else if(dag[obj_size].first.obj_type==START_OBJ && dag[obj_size].second.size()==2){
                std::cout<<dag[obj_size].second.back().obj_type<<" add match obj and child obj size:"<<" "<<dag[obj_size].second.size()<<" "<<dag[obj_size].first.key_end<<" "<<dag[obj_size].second.front().key<<" "<<dag[obj_size].first.key<<" "<<dag[obj_size].second.back().key<<std::endl;
                       insertionOffset=findInsertionOffset(dag[obj_size].first.key_end);
                    wrap=true;
                }
                else if(dag[obj_size].second.size()>1 || (obj_size==0 && dag[obj_size].second.size()>0)){
                std::cout<<dag[obj_size].second.back().obj_type<<" add match sec size:"<<" "<<dag[obj_size].second.size()<<" "<<dag[obj_size].second.back().key_end<<" "<<dag[obj_size].second.back().value_end<<std::endl;
                    if(dag[obj_size].second.back().obj_type==PAIR_VALUE){
                        insertionOffset=findInsertionOffset(dag[obj_size].second.back().value_end);
                    comma=true;
                    } 
                    else{
                 std::cout<<newKey<<" add second ret"<<" "<<obj_size<<" " << dag.size()<<" "<<dag[obj_size].second.size()<<" ke:"<<dag[obj_size].second.back().key_end<<std::endl;
                       insertionOffset=findInsertionOffset(dag[obj_size].first.key_end);
                    }
                    wrap=true;
                }
                else{
                std::cout<<"add match first:"<<" "<<dag[obj_size].first.obj_type<<std::endl;
                    if(dag[obj_size].first.obj_type==PAIR_VALUE)
                        insertionOffset=dag[obj_size].first.value_end - 1;
                    else
                        insertionOffset=dag[obj_size].first.key_end;
                }
                size_t indention=findIndention((insertionOffset>0)? insertionOffset : 0)+1;
                jsonContent.insert(insertionOffset, typeset(comma, wrap, indention, newKey, newValue));
                dag.clear();
                depthList.clear();
                bind(0);
            });
        }
        return true;
    }
        
    //remove
    bool Binder::operator ()(Path& jp, std::string key){
        bool ret=false;
        bool hit=match(jp, [&](size_t obj_size, std::string_view& key, std::any& v){
            for(jobject& i : dag[obj_size].second | std::views::filter([&](jobject& i){return i.key.compare(key)==0;})){
                std::string::size_type start=i.key_start-1;
                std::string::size_type offset=dag[i.obj_size+2].first.key_start-1;
//                std::cout<<"i "<<i<<" "<<key<<" "<<objects[i].children.size()<<std::endl;
                jsonContent.erase(start, offset-start);
                std::cout<<jsonContent<<std::endl;
                dag.clear();
                depthList.clear();
                bind(0);
                break;
            }
            });
        return ret;
    }
        
    //get
    void Binder::operator ()(Path& p, std::function<void(std::any& v)> apply){
        bool hit=match(p, [&apply](size_t obj_size, std::string_view& key, std::any& v){apply(v);});
    }
        
    //traverse
    void Binder::operator ()(Path& p, std::function<void(std::string_view& key, std::any& v)> apply){
        bool hit=match(p, [&](size_t obj_size, std::string_view& key, std::any& v){
            apply(key, v);
//            for(auto  vi=std::next(dag[obj_size].second.begin());vi!=dag[obj_size].second.end();++vi){
//                if((*vi).obj_type==PAIR_KEY && (*std::next(vi)).obj_type==PAIR_VALUE)
//                    apply((*vi).key, (*std::next(vi)).value_index);
//            }
        });
    }
        
    void Binder::display(){
            std::string depthText=fmt::format("{}\n", depthList);
            std::cout<<depthText;
        for(std::vector<std::pair<sylvanmats::io::json::jobject, std::vector<sylvanmats::io::json::jobject>>>::iterator itDAG=dag.begin();itDAG!=dag.end();++itDAG){
            for(size_t ti=0;ti<depthList[(*itDAG).first.obj_size];ti++)std::cout<<"    ";
            std::cout<<(*itDAG).first.key<<std::endl;
        }
    }
    
    void Binder::bind(std::string::size_type offset, size_t objParent){
        std::span s={jsonContent};
        {
            std::span<char>::iterator it=s.begin();
            bool firstObject=true;
            bool hitColon=false;
            bool hitPeriod=false;
            size_t depth=0;
            while(it!=s.end()){
                if(isNull(s, it)){
                    dag.push_back(std::make_pair(jobject{.obj_type=VALUE_NULL, .obj_size=dag.size(), .value_index=std::string_view(it, it+4), .value_start=std::distance(s.begin(), it), .value_end=std::distance(s.begin(), it+4)}, std::vector<jobject>{}));
                    depthList.push_back(depth);
                    it+=4;
                }
                if((*it)=='{'){
                    if(firstObject){
                        dag.push_back(std::make_pair(jobject{.obj_type=START_OBJ, .obj_size=dag.size(), .key=std::string_view(it, std::next(it)), .value_index=object(), .key_start=std::distance(s.begin(), it), .key_end=std::distance(s.begin(), std::next(it)), .value_start=std::distance(s.begin(), it), .value_end=std::distance(s.begin(), std::next(it))}, std::vector<jobject>{}));
                    }
                    else{
//                        if(dag.empty())key="/";
                       dag.push_back(std::make_pair(jobject{.obj_type=START_OBJ, .obj_size=dag.size(), .key=std::string_view(it, std::next(it)), .key_index=std::distance(s.begin(), it), .value_index=object(), .key_start=std::distance(s.begin(), it), .key_end=std::distance(s.begin(), std::next(it)), .value_start=std::distance(s.begin(), it), .value_end=std::distance(s.begin(), std::next(it))}, std::vector<jobject>{}));                        
                    }
                    depthList.push_back(depth);
                    depth++;
                    hitColon=false;
                    firstObject=false;
                }
                else if((*it)=='}'){
                    dag.push_back(std::make_pair(jobject{.obj_type=END_OBJ, .obj_size=dag.size(), .key=std::string_view(it, std::next(it)), .key_index=std::distance(s.begin(), it), .key_start=std::distance(s.begin(), it), .key_end=std::distance(s.begin(), std::next(it)), .value_start=std::distance(s.begin(), it), .value_end=std::distance(s.begin(), std::next(it))}, std::vector<jobject>{}));
                    if(depth>0)depth--;
                    depthList.push_back(depth);
                    hitColon=false;
                }
                else if((*it)=='['){
                    if(firstObject){
                        dag.push_back(std::make_pair(jobject{.obj_type=START_ARRAY, .obj_size=dag.size(), .key=std::string_view(it, std::next(it)), .value_index=object(), .key_start=std::distance(s.begin(), it), .key_end=std::distance(s.begin(), it)+1, .value_start=std::distance(s.begin(), it), .value_end=std::distance(s.begin(), it)+1}, std::vector<jobject>{}));
                    }
                    else{
//                        if(dag.empty())key="/";
                       dag.push_back(std::make_pair(jobject{.obj_type=START_ARRAY, .obj_size=dag.size(), .key=std::string_view(it, std::next(it)), .key_index=std::distance(s.begin(), it), .value_index=object()}, std::vector<jobject>{}));                        
                    }
                    depthList.push_back(depth);
                    depth++;
                    hitColon=false;
                    firstObject=false;
                }
                else if((*it)==']'){
                    dag.push_back(std::make_pair(jobject{.obj_type=END_ARRAY, .obj_size=dag.size(), .key=std::string_view(it, std::next(it)), .key_index=std::distance(s.begin(), it)}, std::vector<jobject>{}));
                    depthList.push_back(depth);
                    if(depth>0)depth--;
                    hitColon=false;
                }
                else if((*it)=='"'){
                    it++;
                    std::span<char>::iterator itStart=it;
                    int c=0;
                    while((*it)!='"'){if((*it)=='\\')it++;it++;c++;};
                    if(!hitColon){
                        dag.push_back(std::make_pair(jobject{.obj_type=PAIR_KEY, .obj_size=dag.size(), .key=std::string_view(itStart, it), .key_index=std::distance(s.begin(), it), .key_start=std::distance(s.begin(), itStart), .key_end=std::distance(s.begin(), it)+1}, std::vector<jobject>{}));                        
                    }
                    else{
//                        std::cout<<"PV "<<dag.size()<<" "<<std::string_view(itStart, it)<<std::endl;
                        dag.push_back(std::make_pair(jobject{.obj_type=PAIR_VALUE, .obj_size=dag.size(), .key_index=std::distance(s.begin(), it), .value_index=std::any{std::string_view(itStart, it)}, .value_start=std::distance(s.begin(), itStart), .value_end=std::distance(s.begin(), it)+1}, std::vector<jobject>{}));                        
                        hitColon=false;
                    }
                    depthList.push_back(depth);
                    
                }
                else if((*it)=='-' || (*it)=='.' || ((*it)>='0' && (*it)<='9')){
                    ++it;
                    bool hitPeriod=((*it)=='.') ? true : false;
                    std::span<char>::iterator itStart=it;
                    int c=0;
                    while(((*it)>='0' && (*it)<='9') || (*it)=='.'){if(!hitPeriod && (*it)=='.')hitPeriod=true;it++;c++;};
                    std::string v(itStart, it);
                    if(hitPeriod)
                        dag.push_back(std::make_pair(jobject{.obj_type=PAIR_VALUE, .obj_size=dag.size(), .key_index=std::distance(s.begin(), it), .value_index=std::any{std::strtod(v.c_str(), nullptr)}, .value_start=std::distance(s.begin(), itStart), .value_end=std::distance(s.begin(), it)}, std::vector<jobject>{}));                        
                    else
                        dag.push_back(std::make_pair(jobject{.obj_type=PAIR_VALUE, .obj_size=dag.size(), .key_index=std::distance(s.begin(), it), .value_index=std::any{std::strtol(v.c_str(), nullptr, 10)}, .value_start=std::distance(s.begin(), itStart), .value_end=std::distance(s.begin(), it)}, std::vector<jobject>{}));                        
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
                }
                ++it;
            }
//            std::string depthText=fmt::format("{}\n", depthList);
//            std::cout<<depthText;
            depth=0;
            std::vector<std::pair<jobject, std::vector<jobject>>>::iterator previousNode=dag.end();
            for(std::vector<std::pair<jobject, std::vector<jobject>>>::iterator itDag=dag.begin();itDag!=dag.end();itDag++){
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

    bool Binder::match(Path& jp, std::function<void(size_t obj_size, std::string_view& key, std::any& v)> apply){
        bool hit=false;
        size_t pi=0;
        int countMap=0;
        std::unordered_map<size_t, bool> matchMap;
        do{
        for(node_iterator<size_t> ni(dag);!hit && ni!=ni.end();++ni){
            if(depthList[(*ni)]==0)matchMap[(*ni)]=true;
            else if(pi>0 && !dag[(*ni)].second.empty() && depthList[dag[(*ni)].second.front().obj_size]==pi){
                matchMap[(*ni)]=true;
                countMap++;
            }
            if(depthList[(*ni)]==pi && matchMap.count((*ni)) && matchMap[(*ni)]){
            size_t count=0;
            size_t countObj=0;
            size_t countFinal=0;
            for(out_edge_iterator<size_t> oei(dag, ni); !hit && oei!=oei.end(); ++oei){
//                if(depthList[(*oei)]==pi+1 && dag[(*oei)].first.key.compare(jp.p[pi].label)==0)
//                        std::cout<<" "<<jp.p[pi].label<<" "<<(*oei)<<" "<<(pi+1)<<" "<<dag[(*oei)].first.obj_type<<" "<<dag[(*oei)].first.key<<std::endl;
                if(depthList[(*oei)]==pi+1)
                    if(dag[(*oei)].first.obj_type==PAIR_KEY && (jp.p[pi].label.compare("*")==0 || dag[(*oei)].first.key.compare(jp.p[pi].label)==0)){
//                        auto nx = std::next(itS);
                        Node<size_t> nextNode((*oei)+1);
//                        std::cout<<" "<<jp.p[pi].label<<" "<<depthList[(*nextNode)]<<(pi+1)<<std::endl;
                        if((*nextNode)<dag.size() && depthList[(*nextNode)]==pi+1 && dag[(*nextNode)].first.obj_type==PAIR_VALUE){
                            if(pi+1>=jp.p.size()){
                                apply((*oei), dag[(*oei)].first.key, dag[(*nextNode)].first.value_index);
                                if(jp.p[pi].label.compare("*")!=0)hit=true;
                                countFinal++;
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
                            if(dag[(*nextNode)].first.obj_type==START_OBJ){
                                if(jp.p[pi].label.compare("*")==0){
                                    apply(dag[(*oei)].first.obj_size, dag[(*oei)].first.key, dag[(*oei)].first.value_index);
                                }
                                else
                                for(out_edge_iterator<size_t> oeiA(dag, nextNode); oeiA!=oeiA.end(); ++oeiA){
//                                    if(dag[(*oeiA)].first.obj_type==PAIR_KEY)std::cout<<" "<<dag[(*oeiA)].first.key;
                                    if(dag[(*oeiA)].first.obj_type==PAIR_KEY && dag[(*oeiA)+1].first.obj_type==PAIR_VALUE){
//                                    std::cout<<dag[(*nextNode)].second.size()<<" "<<dag[(*oeiA)].first.key<<" how many "<<(*oeiA)<<std::endl;
                                        apply(dag[(*oeiA)].first.obj_size, dag[(*oeiA)].first.key, dag[(*oeiA)+1].first.value_index);
                                    }
                                }
                                if(jp.p[pi].label.compare("*")!=0)hit=true;
                            }
                            else if(dag[(*nextNode)].first.obj_type==START_ARRAY){
//                            std::cout<<pi<<" "<<jp.p[pi].label<<" inner obj "<<dag[(*nextNode)].first.obj_size<<" "<<dag[(*nextNode)].first.key;
                                for(out_edge_iterator<size_t> oeiA(dag, nextNode); oeiA!=oeiA.end(); ++oeiA){
//                                    if(dag[(*oeiA)].first.obj_type==PAIR_KEY)std::cout<<" "<<dag[(*oeiA)].first.key;
                                    if(dag[(*oeiA)].first.obj_type==PAIR_KEY)
                                    apply(dag[(*oeiA)].first.obj_size, dag[(*oeiA)].first.key, dag[(*oeiA)].first.value_index);
                                }
                                if(jp.p[pi].label.compare("*")!=0)hit=true;
//                            std::cout<<std::endl;
                            }
                                countFinal++;
                            }
                            else if(jp.p[pi].label.compare("*")==0){
                                matchMap[(*oei)]=true;
                                matchMap[(*nextNode)]=true;
                            countObj++;
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
                        count++; 
                    }
                    else if(jp.p[pi].label.compare("*")==0){
                        matchMap[(*oei)]=true;
//                        matchMap[(*nextNode)]=true;
                        count++; 
                    }
                    
            }
//            if(count>=2)std::cout<<"ni "<<(*ni)<<" "<<count<<" "<<countObj<<" "<<countFinal<<std::endl;
            }
        }
        pi++;
        }
        while(!hit && pi<jp.p.size());
//        std::cout<<"countMap "<<countMap<<std::endl;
//        std::vector<std::pair<sylvanmats::io::json::jobject, std::vector<sylvanmats::io::json::jobject>>>::iterator itDAG=dag.begin();
//        int counter=0;
//        if(itDAG!=dag.end() && (*itDAG).first.obj_type==START_OBJ && !jp.p.empty()){
//            std::vector<jobject>& currentChildren=(*itDAG).second;
//            pi=0;
//                if(jp.p[pi].label.compare("CGU")==0)std::cout<<pi<<" CGU "<<currentChildren.size()<<std::endl;
//                bool matched=false;
//                for(std::vector<jobject>::iterator itS=currentChildren.begin(); pi<jp.p.size() && !matched && !hit && itS!=currentChildren.end();itS++){
//                    if(depthList[(*itS).obj_size]==pi+1)
//                    if((*itS).obj_type==PAIR_KEY && (jp.p[pi].label.compare("*")==0 || (*itS).key.compare(jp.p[pi].label)==0)){
//                        auto nx = std::next(itS);
//                        if(nx!=currentChildren.end() && (*nx).obj_type==PAIR_VALUE){
//                            if(pi+1>=jp.p.size()){
//                                apply((*itS).obj_size, (*itS).key, (*nx).value_index);
//                                hit=true;
//                            }
//                            else{
//                                currentChildren=dag[(*itS).obj_size].second;
//                                pi++;
//                            }
//                            matched=true;
//                        }
//                        else if(nx!=currentChildren.end() && ((*nx).obj_type==START_OBJ || (*nx).obj_type==START_ARRAY)){
//                            std::cout<<jp.p[pi].label<<" inner obj "<<(*nx).obj_size<<" "<<(*nx).key<<std::endl;
//                            if(pi+1>=jp.p.size())apply((*nx).obj_size, (*nx).key, (*nx).value_index);
//                            else {
//                            std::cout<<jp.p[pi].label<<"cont inner obj "<<std::endl;
//                                currentChildren=dag[(*nx).obj_size].second;
//                                pi++;
//                            }
//                            matched=true;
//                        }
//                        else{
//                            std::cout<<" else "<<(*nx).obj_size<<" "<<(*nx).obj_type<<" "<<std::endl;
//                        }
//                    }
//                    else if(((*itS).obj_type==START_OBJ || (*itS).obj_type==START_ARRAY) && pi+1>=jp.p.size()){
//                        counter++;
//                        matched=true;
//                    }
////                    if(depthList[(*itS).obj_size]==pi+1 && (*itS).obj_type==PAIR_KEY && pi==0)counter++;
//                }
////                std::cout<<"matched? "<<matched<<" "<<currentChildren.size()<<std::endl;
////            }
//        }
//        std::cout<<"counter: "<<counter<<std::endl;
        return hit;
    }
    
}