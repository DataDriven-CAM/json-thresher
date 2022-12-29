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
    bool Binder::operator ()(Path& jp, std::string_view key, std::any value){
        if(jsonContent.empty()){
            jsonContent.append("{\n");
            jsonContent.append(typeset(false, 1, key, value));
            jsonContent.append("}\n");
//            std::cout<<(*this)<<std::endl;
            bind(0);
        }
        else if(jp.p.empty()){
                auto s=objects[objects.size()-2];
                size_t insertionOffset=s.key_index+1;//findInsertionOffset(s.key_index);
                size_t indention=findIndention(insertionOffset-1)+1;
                if(!s.children.empty()){
//                    std::cout<<objects.size()<<" ch # "<<s.children.back()<<" "<<objects[s.children.back()].key<<std::endl;
                    insertionOffset=findInsertionOffset(objects[s.children.back()].value_end);
                }
                jsonContent.insert(insertionOffset, typeset(true, indention, key, value));
            objects.clear();
            bind(0);
            
        }
        else stroll(jp, [&](size_t objIndex, std::string_view& k, std::any& v){
                auto s=objects[objIndex];
//                std::cout<<objIndex<<" "<<s.key<<" add to "<<s.children.size()<<std::endl;
//                std::cout<<"s.key_index "<<s.key_index<<" "<<s.obj_size<<std::endl;
                size_t insertionOffset=findInsertionOffset(s.key_index);
                size_t indention=findIndention(insertionOffset-1);
//                std::cout<<"insertionOffset "<<insertionOffset<<" "<<indention<<std::endl;
                if(!s.children.empty()){
//                    std::cout<<objects.size()<<" ch # "<<s.children.back()<<" "<<objects[s.children.back()].key<<std::endl;
                    insertionOffset=findInsertionOffset(objects[s.children.back()].value_end);
                }
                    jsonContent.insert(insertionOffset, typeset(!s.children.empty(), indention, key, value));
//            std::cout<<(*this)<<std::endl;
            objects.clear();
            bind(0);
                    //bind(s.key_index+1, s.obj_size);
                    //display();
//            for(auto vpIt=objects.begin();vpIt!=objects.end();vpIt++){
//                std::cout<<"vps2 "<<std::get<OBJ_INDEX>(*vpIt)<<" "<<std::get<PAIR_KEY>(*vpIt)<<std::endl;
//            }
            });
        return true;
    }
        
    //remove
    bool Binder::operator ()(Path& jp, std::string key){
        bool ret=false;
        stroll(jp, [&](size_t objIndex, std::string_view& k, std::any& v){
//            std::cout<<"remove "<<objIndex<<" "<<objects[objIndex].children.size()<<std::endl;
            for(size_t i : objects[objIndex].children | std::views::filter([&](size_t i){return objects[i].key.compare(key)==0;})){
                std::string::size_type start=objects[i].key_start-1;
                std::string::size_type offset=objects[objects[i].children.back()+2].key_start-1;
//                std::cout<<"i "<<i<<" "<<key<<" "<<objects[i].children.size()<<std::endl;
                jsonContent.erase(start, offset-start);
//                std::cout<<jsonContent<<std::endl;
                objects.clear();
                bind(0);//previousnl, p.parent_index);
                break;
            }
        });
//        for(auto& d : jp.p){
//            for(auto s : objects | std::views::filter([&d](jobject& s){return s.key.compare(d.label)==0;})){
//                for(auto& p : objects | std::views::filter([&key,&s](jobject& p){return p.parent_index==s.obj_size && p.key.compare(key)==0;})){
//                    auto it=std::find_if(objects.begin(),objects.end(), [&p](jobject& vp){std::cout<<vp.key<<" "<<p.key<<std::endl;return vp.parent_index==p.parent_index && vp.key.compare(p.key)==0;});
//                    auto itS=std::find_if(objects.begin(),objects.end(), [&s](jobject& s2){return s2.obj_type==s.obj_type && s2.obj_size==s.obj_size && s2.parent_index==s.parent_index && s2.key.compare(s.key)==0;});
//                    if(itS!=objects.end())itS++;
//                    std::string::size_type distance=(it!=objects.end()) ? std::distance(objects.begin(), it): objects.size();
//                    std::string::size_type distance2=(itS!=objects.end()) ? std::distance(objects.begin(), itS): objects.size();
//                    std::string::size_type start=(std::string::size_type)std::to_address(jsonContent.data());
//                    std::string::size_type offset=(std::string::size_type)std::to_address(p.key.data())-start;
//                    std::string::size_type previousnl=jsonContent.find_last_of(",\n{", offset);
//                    std::string::size_type nl=jsonContent.find_first_of(",\n}", offset);
//                    if(nl==std::string::npos)nl=previousnl;
//                    jsonContent.erase(previousnl, nl-previousnl);
//                    if(distance2<objects.size())objects.resize(distance2);
//                    bind(0);//previousnl, p.parent_index);
//                    return true;
//                }
//            }
//        }
        return ret;
    }
        
    //get
    void Binder::operator ()(Path& p, std::function<void(std::any& v)> apply){
        stroll(p, [&apply](size_t objIndex, std::string_view& key, std::any& v){apply(v);}, 0, 1);
    }
        
    //traverse
    void Binder::operator ()(Path& p, std::function<void(std::string_view& key, std::any& v)> apply){
        stroll(p, [&](size_t objIndex, std::string_view& key, std::any& v){
            for(size_t  vi: objects[objIndex].children)
                apply(objects[vi].key, objects[vi].value_index);
        }, 0, 0);
    }
        
    void Binder::display(){
        for(auto s : objects){
            for(size_t ti=0;ti<s.parent_index;ti++)std::cout<<"\t";
            //std::cout<<"obj index "<<s.obj_size<<" "<<s.obj_size<<" "<<s.parent_index<<" "<<s.key;
                for(auto p : objects | std::views::filter([&s](jobject& p){return p.parent_index==s.obj_size;})){
                    for(size_t ti=0;ti<s.parent_index+1;ti++)std::cout<<"\t";
                    std::cout<<"\tobj "<<p.parent_index<<" "<<p.obj_size<<" "<<p.key<<" "<<type_names[std::type_index(p.value_index.type())]<<std::endl;
                    if(type_names[std::type_index(p.value_index.type())].compare("const char*")==0){
                        std::cout<<"\""<<std::any_cast<const char*>(p.value_index)<<"\"";
                    }
                    else if(type_names[std::type_index(p.value_index.type())].compare("int")==0){
                        std::cout<<" "<<std::to_string(std::any_cast<int>(p.value_index));
                    }
                    else if(type_names[std::type_index(p.value_index.type())].compare("object")==0){
                        std::cout<<" "<<"{}";
                    }
                    std::cout<<std::endl;
                }
            std::cout<<std::endl;

        }
    }
    
    void Binder::bind(std::string::size_type offset, size_t objParent){
        std::span s={jsonContent};
        std::span<char>::iterator it=s.begin();
                if(offset>0)std::advance(it, offset);
        size_t key_start=0ull;
        size_t key_end=0ull;
        std::string_view key;
        size_t value_start=0ull;
        size_t value_end=0ull;
        std::any value;
        bool hitColon=false;
        bool hitPeriod=false;
        std::vector<size_t> arrayObjects{objParent};
        bool hit=objParent==0;
        if(objParent<objects.size())objects.resize(objParent+1);
        while(!hit && objParent>=0 && objParent<objects.size()){
//            std::cout<<" "<<objParent;
            arrayObjects.insert(arrayObjects.begin(), objParent);
            if(objParent==0)hit=true;
            else objParent=objects[objParent].parent_index;
//            std::cout<<" "<<objParent<<std::endl;
        }
        while(it!=s.end()){
            if(isNull(s, it)){
                value_start=std::distance(s.begin(), it);
                value_end=std::distance(s.begin(), it+4);
                value=std::string_view(it, it+4);
                objects.push_back(jobject{.obj_type=VALUE_PAIR, .obj_size=objects.size(), .parent_index=objParent, .key=key, .value_index=value, .key_start=key_start, .key_end=key_end, .value_start=value_start, .value_end=value_end});
                objects[objParent].children.push_back(objects.size()-1);
                hitColon=false;
                it+=4;
            }
            switch(*it){
                case '{':
                    if(!key.empty()){
                        objects.emplace_back(jobject{.obj_type=START_OBJ, .obj_size=objects.size(), .parent_index=(arrayObjects.size()>0) ? arrayObjects.back() : 0ull, .key=key, .key_index=std::distance(s.begin(), it), .value_index=object(), .key_start=key_start, .key_end=key_end, .value_start=value_start, .value_end=value_end});
                        if(arrayObjects.size()>0)objects[arrayObjects.back()].children.push_back(objects.size()-1);
                    }
                    else{
                        if(objects.empty())key="/";
                        objects.emplace_back(jobject{.obj_type=START_OBJ, .obj_size=objects.size(), .parent_index=(arrayObjects.size()>0) ? arrayObjects.back() : 0ull, .key=key, .key_index=std::distance(s.begin(), it), .value_index=object()});
                        if(arrayObjects.size()>0)objects[arrayObjects.back()].children.push_back(objects.size()-1);
                    }
                    arrayObjects.push_back(objects.size()-1);    
//                    std::cout<<arrayObjects.size()<<" { "<<key<<" "<<arrayObjects.back()<<" "<<objParent<<std::endl;
                    key="";
                    hitColon=false;
                it++;
                    break;
                case '}':
                    arrayObjects.pop_back();    
                    objects.emplace_back(jobject{.obj_type=END_OBJ, .obj_size=objects.size(), .parent_index=(arrayObjects.size()>0) ? arrayObjects.back() : 0ull, .key=key, .key_index=std::distance(s.begin(), it)});
//                    std::cout<<arrayObjects.size()<<" } "<<key<<" "<<arrayObjects.back()<<" "<<objParent<<std::endl;
                    key="";
                    hitColon=false;
                it++;
                    break;
                case '\\':
                it++;
                it++;
                    break;
                case '"':
                {
                it++;
                    std::span<char>::iterator itStart=it;
                    int c=0;
                    while((*it)!='"'){if((*it)=='\\')it++;it++;c++;};
                    if(hitColon || objects[arrayObjects.back()].obj_type==START_ARRAY){
                        value_start=std::distance(s.begin(), itStart);
                        value_end=std::distance(s.begin(), it-2);
                        value=std::string_view(itStart, it);
//                        if(key.compare("name")==0)// && std::any_cast<std::string_view>(value).compare("C")==0)
//                            std::cout<<arrayObjects.size()<<" hitColon "<<key<<" "<<std::any_cast<std::string_view>(value)<<" "<<objects.size()<<" "<<arrayObjects.back()<<std::endl;
                        objects.emplace_back(jobject{.obj_type=VALUE_PAIR, .obj_size=objects.size(), .parent_index=(arrayObjects.size()>0) ? arrayObjects.back() : 0ull, .key=key, .value_index=value, .key_start=key_start, .key_end=key_end, .value_start=value_start, .value_end=value_end});
                        objects[arrayObjects.back()].children.push_back(objects.size()-1);
                        hitColon=false;
                        key="";
                    }
                    else{
                        key_start=std::distance(s.begin(), itStart);
                        key_end=std::distance(s.begin(), it);
                        key=std::string_view(itStart, it);
                    }
                it++;
                }
                    break;
                case '.':
                    hitPeriod=true;
                case '-':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                {
                    std::span<char>::iterator itStart=it;
                    if(hitColon || objects[arrayObjects.back()].obj_type==START_ARRAY){
                        int c=0;
                        while(((*it)>='0' && (*it)<='9') || (*it)=='.'){if((*it)=='.')hitPeriod=true;it++;c++;};
                        if(objects[arrayObjects.back()].obj_type==START_ARRAY)
                            key=std::string_view(itStart, it);
                        value_start=std::distance(s.begin(), itStart);
                        value_end=std::distance(s.begin(), it-2);
                        std::string v(itStart, it);
                        if(hitPeriod)
                            value=std::strtod(v.c_str(), nullptr);
                        else
                            value=std::strtol(v.c_str(), nullptr, 10);
                        //std::cout<<objects.size()<<" #num "<<key<<" "<<v<<" "<<objects.size()<<std::endl;
                        objects.emplace_back(jobject{.obj_type=VALUE_PAIR, .obj_size=objects.size(), .parent_index=(arrayObjects.size()>0) ? arrayObjects.back() : 0ull, .key=key, .value_index=value, .key_start=key_start, .key_end=key_end, .value_start=value_start, .value_end=value_end});
                        objects[arrayObjects.back()].children.push_back(objects.size()-1);
                        hitColon=false;
                        hitPeriod=false;
                        key="";
                    }
                    it++;
                }
                    break;
                case ':':
                    hitColon=true;
                it++;
                    break;
                case ',':
                    key="";
                    hitColon=false;
                it++;
                    break;
                case '[':
                    objects.emplace_back(jobject{.obj_type=START_ARRAY, .obj_size=objects.size(), .parent_index=(arrayObjects.size()>0) ? arrayObjects.back() : 0ull, .key="0", .key_index=std::distance(s.begin(), it)});
                    if(arrayObjects.size()>0)objects[arrayObjects.back()].children.push_back(objects.size()-1ull);
                    arrayObjects.push_back(objects.size()-1);
                    if(!key.empty()){
                        objects.emplace_back(jobject{.obj_type=VALUE_PAIR, .obj_size=objects.size(), .parent_index=(arrayObjects.size()>0) ? arrayObjects.back() : 0ull, .key=key, .value_index=array(), .key_start=key_start, .key_end=key_end, .value_start=value_start, .value_end=value_end});
                        objects[objects.size()-1].children.push_back(objects.size()-1);
                    }
                    hitColon=false;
                    key="";
                it++;
                    break;
                case ']':
                    arrayObjects.pop_back();
                    objects.emplace_back(jobject{.obj_type=END_ARRAY, .obj_size=objects.size(), .parent_index=(arrayObjects.size()>0) ? arrayObjects.back() : 0ull, .key=key, .key_index=std::distance(s.begin(), it)});
                    key="";
                it++;
                    break;
                default:
                it++;
                    break;
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

    void Binder::stroll(Path& jp, std::function<void(size_t objIndex, std::string_view& key, std::any& v)> apply, size_t i, unsigned int objParent){
        if(i<jp.p.size()){
            element d=jp.p[i];
            size_t objSize=objects.size();
//            if(objParent<objects.size() && d.label.compare("name")==0)std::cout<<i<<" d.label "<<d.label<<" "<<i<<" "<<objParent<<" "<<objects[objParent].children.size()<<" "<<objects[objParent].obj_type<<std::endl;
            if(objParent<objects.size() && objects[objParent].children.size()>0 && objects[objParent].obj_type==START_ARRAY){
                jobject p=objects[objParent];
                //if(objParent==0 && objects[objParent].key.empty() && objects[objParent].obj_type==START_OBJ)std::cout<<"array "<<(objParent)<<" "<<d.label<<std::endl;
                for(std::vector<size_t>::iterator itArray=p.children.begin();itArray!=p.children.end();itArray++){
//                    std::cout<<"child "<<(objParent)<<" "<<(*itArray)<<std::endl;
                    stroll(jp, apply, i, (*itArray));
                    //if((*itArray).key.compare(d.label)==0)apply(p.parent_index, p.key, p.value_index);
                }
            }
            for(auto s : objects | std::views::filter([&](jobject& s){return (s.parent_index==objParent || (s.obj_size==objParent && objects[objParent].obj_type==VALUE_PAIR)) && (s.key.compare(d.label)==0 || (s.key.empty() && s.obj_type==START_OBJ));})){
                    if(i==jp.p.size()-1 && jp.p.back().action==TEST){
                        //size_t vpsSize=objects.size();
                        //std::cout<<"test end of jp "<<jp.p.back().label<<" "<<jp.p.back().value<<" "<<objects.size()<<std::endl;
                        for(auto p : objects | std::views::filter([&](jobject& p){return objects[p.parent_index].obj_size==s.parent_index && p.key.compare(jp.p.back().label)==0 && jp.p.back().value.compare(std::any_cast<std::string_view>(p.value_index))==0;})){
                            //for(size_t ti=0;ti<s.obj_size+1;ti++)std::cout<<"\t";
                            apply(p.parent_index, p.key, p.value_index);
                        }

                    }
                    else if(i==jp.p.size()-1){
//                        std::cout<<"end of jp "<<objects.size()<<std::endl;
                        size_t vpsSize=objects.size();
                        for(auto p : objects | std::views::filter([&](jobject& p){return p.parent_index==s.parent_index && p.key.compare(jp.p.back().label)==0;})){
                            //for(size_t ti=0;ti<s.obj_size+1;ti++)std::cout<<"\t";
                            //std::cout<<"\t"<<std::get<PAIR_DEPTH>(p)<<" "<<std::get<PAIR_KEY>(p)<<" "<<std::any_cast<std::string_view>(std::get<PAIR_INDEX>(p))<<std::endl;
                            apply(p.obj_size, p.key, p.value_index);
                            if(vpsSize!=objects.size())break;
                        }

                    }
                    else if(i<jp.p.size()){
                        stroll(jp, apply, i+1, s.parent_index);
                    }
                    if(objSize!=objects.size())break;
            }
        }
        else{
            
        }
    }
        
}