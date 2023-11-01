
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

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
//    sylvanmats::io::json::Path jp2;
//    jsonBinder(jp2, "CGU", sylvanmats::io::json::object());
//    jp2["CGU"];
//    jsonBinder(jp2, "start", 300);
//    jsonBinder(jp2, "end", 400);
//    //jsonBinder.display();
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
            else if(count==1){
                CHECK_EQ(key, "json-graph-specification");
                CHECK_EQ(std::any_cast<std::string_view>(v), "jsongraph/json-graph-specification");
            }
            count++;
        });
        CHECK_EQ(count, 2);
}

}
