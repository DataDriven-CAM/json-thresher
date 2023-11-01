#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <span>
#include <tuple>
#include <any>
#include <iostream>
#include <functional>
#include <ranges>
#include <vector>
#include <map>
#include <typeindex>
#include <cmath>

#include "io/json/Path.h"

#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "fmt/ranges.h"

namespace sylvanmats::io::json{

    enum OBECT_TYPE{
        END_OBJ,
        START_OBJ,
        END_ARRAY,
        START_ARRAY,
        VALUE_PAIR,
        PAIR_KEY,
        PAIR_VALUE,
        VALUE_NULL
    };
    
    struct jobject{
        OBECT_TYPE obj_type;
        size_t obj_size;
        std::string_view key;
        size_t key_index=0;
        std::any value_index;
        size_t key_start=0;
        size_t key_end=0;
        size_t value_start=0;
        size_t value_end=0;
    };

    struct object{};
    struct array{};
    
    size_t tabSize=std::string("    ").size();
    
    class Binder{
    private:
        std::unordered_map<std::type_index, std::string> type_names{{std::type_index(typeid(const char*)), "const char*"},
                                                                    {std::type_index(typeid(int)), "int"},
                                                                    {std::type_index(typeid(double)), "double"},
                                                                    {std::type_index(typeid(object)), "object"}};
        std::string jsonContent="";
//        std::vector<jobject> objects;
        std::vector<std::pair<jobject, std::vector<jobject>>> dag;
        std::vector<int> depthList;
        
    public:
        Binder()=default;
        Binder(const Binder& orig) =  delete;
        virtual ~Binder()= default;
        
        //Populate 
        void operator ()(std::istream& is);
        
        void operator ()(std::string& jsonContent);
        
        //add
        bool operator ()(Path& p, std::string_view key, std::any value);
        
        //remove
        bool operator ()(Path& jp, std::string key);
        
        //get
        void operator ()(Path& p, std::function<void(std::any& v)> apply);
        
        //traverse
        void operator ()(Path& p, std::function<void(std::string_view& key, std::any& v)> apply);
        
        //traverse sibling
        void operator ()(Path& p, std::string_view sibling, std::function<void(std::string_view& key, std::any& v)> apply){
//            for(auto d : p.p){
//                for(auto s : objects | std::views::filter([&sibling](jobject& s){return s.key.compare(sibling)==0;})){
////                    std::cout<<std::get<OBJ_SIZE>(s)<<" here "<<s.obj_size<<" "<<s.key<<std::endl;
//                    for(auto p : objects | std::views::filter([&s](jobject& p){return p.parent_index==s.obj_size;})){
//                        //std::cout<<"\t"<<std::get<PAIR_DEPTH>(p)<<" "<<std::get<PAIR_KEY>(p)<<std::endl;
//                        apply(p.key, p.value_index);
//                    }
//                }
//            }
        }
        
        size_t countObjects(){return dag.size();}
        
        void display();
        
    protected:
        void bind(std::string::size_type offset, size_t objParent=0);
        
        bool isNull(std::span<char>& s, std::span<char>::iterator& it);
        
        bool match(Path& jp, std::function<void(size_t obj_size, std::string_view& key, std::any& v)> apply);
        
//        void stroll(Path& jp, std::function<void(size_t objIndex, std::string_view& key, std::any& v)> apply, size_t i=0, unsigned int objParent=0);
        
        inline std::string typeset(bool comma, size_t indention, std::string_view& key, std::any& value){
            std::string kv{};
            if(comma)kv.append(",\n");
            for(size_t ti=0;ti<indention;ti++)kv.append("    ");
            kv.append("\"");
            kv.append(key);
            kv.append("\": ");
            if(type_names[std::type_index(value.type())].compare("const char*")==0){
                kv.append("\"");
                kv.append(std::any_cast<const char*>(value));
                kv.append("\"\n");
            }
            else if(type_names[std::type_index(value.type())].compare("int")==0){
                kv.append(std::to_string(std::any_cast<int>(value)));
                kv.append("\n");
            }
            else if(type_names[std::type_index(value.type())].compare("object")==0){
                kv.append("{\n");
                kv.append("\t}\n");
            }
            return std::move(kv);
        }
        
        inline size_t findInsertionOffset(size_t index){
            size_t offset=index;
            if(offset<jsonContent.size() && jsonContent.at(offset)=='\n' ||  jsonContent.at(offset)=='0')return offset;
            while(++offset<jsonContent.size() && jsonContent.at(offset)!='\n' &&  jsonContent.at(offset)<'0'){
                
            }
            if(offset<jsonContent.size()-1)++offset;
            return offset;
        }
        
        inline size_t findIndention(size_t index){
            size_t offset=index;
//            std::cout<<"findIndention "<<index<<" "<<jsonContent.length()<<std::endl;
            while((--offset)>0 && jsonContent.at(offset)!='\n' && jsonContent.at(offset)!=',' && jsonContent.at(offset)!='}' && jsonContent.at(offset)!=']'){
                
            }
//            if(offset<jsonContent.size()-1)++offset;
//            std::cout<<"offset "<<offset<<" "<<index<<" "<<tabSize<<std::endl;
            if(index>offset)return (index-offset)/tabSize;
            return 0;
        }
        
    public:
        friend std::ostream& operator<<(std::ostream& s, Binder& jb) {
          s << jb.jsonContent;
          return s;
        }
        
    };
    
}
