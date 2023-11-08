
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

#include <fstream>

#define protected public
#include "io/json/Binder.h"

TEST_SUITE("main"){


TEST_CASE("test create json") {
    sylvanmats::io::json::Binder jsonBinder;
    sylvanmats::io::json::Path jp;
    jsonBinder(jp, "8DR", sylvanmats::io::json::object());
    CHECK_EQ(jsonBinder.countObjects(), 5);
    std::cout<<jsonBinder<<std::endl;
    jp["8DR"];
    jsonBinder(jp, "start", 100);
    std::cout<<jsonBinder<<std::endl;
    jsonBinder(jp, "end", 200);
    std::cout<<jsonBinder<<std::endl;
    sylvanmats::io::json::Path jp2;
    jsonBinder(jp2, "CGU", sylvanmats::io::json::object());
    jp2["CGU"];
    jsonBinder(jp2, "start", 300);
    jsonBinder.display();
    std::cout<<"check: "<<jsonBinder<<std::endl;
//    jsonBinder(jp2, "end", 400);
////    //jsonBinder.display();
//    std::cout<<jsonBinder<<std::endl;
//    CHECK_EQ(jsonBinder.countObjects(), 10);
//    sylvanmats::io::json::Path jp3=Root();
////    jp3["8DR"];
////    std::cout<<"jp3 "<<jp3<<std::endl;
//    jsonBinder(jp3, "8DR");
//    std::cout<<jsonBinder<<std::endl;
//    CHECK_EQ(jsonBinder.countObjects(), 6);
}

TEST_CASE("test reading package.json") {
    sylvanmats::io::json::Binder jsonBinder;
            std::ifstream is("../package.json");
            std::string jsonContent((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
            jsonBinder(jsonContent);
            std::string depthText=fmt::format("{}\n", jsonBinder.depthList);
            std::cout<<depthText;
        sylvanmats::io::json::Path jpName;
        jpName["name"];
        std::string_view currentPackageName;
        jsonBinder(jpName, [&currentPackageName](std::any& v){
            currentPackageName=std::any_cast<std::string_view>(v);
        });
        CHECK_EQ(currentPackageName, "json-thresher");
        sylvanmats::io::json::Path type;
        type["devDependencies"];
        size_t count=0;
        jsonBinder(type, [&count](std::string_view& key, std::any& v){
            if(count==0){
                CHECK_EQ(key, "doctest");
                CHECK_EQ(std::any_cast<std::string_view>(v), "onqtam/doctest");
            }
            else if(count==2){
                CHECK_EQ(key, "json-graph-specification");
                CHECK_EQ(std::any_cast<std::string_view>(v), "jsongraph/json-graph-specification");
            }
            count++;
        });
        CHECK_EQ(count, 3);
}

TEST_CASE("test reading mimes db.json") {
    sylvanmats::io::json::Binder jsonBinder;
        std::ifstream is("../cpp_modules/mime-db/db.json");
        std::string jsonContent((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
        jsonBinder(jsonContent);
//            std::string depthText=fmt::format("{}\n", jsonBinder.depthList);
//            std::cout<<depthText;
            CHECK_EQ(jsonBinder.dag.size(), 15140);
            CHECK_EQ(jsonBinder.depthList.size(), 15140);
//            std::ofstream ofs("check.txt");
//            ofs<<depthText;
//            ofs.close();
        sylvanmats::io::json::Path jp="*"_jp;
        CHECK_EQ(jp.p.size(), 1);
//        std::cout<<"jp "<<jp<<std::endl;
        std::vector<std::string> mimeNames;
        jsonBinder(jp, [&](std::string_view& key, std::any& v){
//            std::cout<<key<<std::endl;
            mimeNames.push_back(std::string(key.begin(), key.end()));
        });
        for(std::vector<std::string>::iterator it=mimeNames.begin();it!=mimeNames.end();++it){
            std::string jp2Str=(*it);
//            jp2Str.append("/extensions");
            sylvanmats::io::json::Path jp2;
            jp2/jp2Str.c_str()/"extensions";
//            std::cout<<"jp2 "<<jp2<<std::endl;
            jsonBinder(jp2, [&](std::string_view& key, std::any& v){
//                std::cout<<(*it)<<" "<<key<<std::endl;
            });
        }
//        sylvanmats::io::json::Path jp="*/extensions"_jp;
//        CHECK_EQ(jp.p.size(), 2);
//        std::cout<<"jp "<<jp<<std::endl;
//        jsonBinder(jp, [&](std::string_view& key, std::any& v){
//            std::cout<<key<<" "<<std::any_cast<const char*>(v)<<std::endl;
//        });
        is.close();
}

}
