
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "io/json/Binder.h"

TEST_SUITE("main"){


TEST_CASE("test create json") {
    sylvanmats::io::json::Binder jsonBinder;
    sylvanmats::io::json::Path jp;
    jsonBinder(jp, "8DR", sylvanmats::io::json::object());
    CHECK_EQ(jsonBinder.countObjects(), 4);
    std::cout<<jsonBinder<<std::endl;
    jp["8DR"];
    jsonBinder(jp, "start", 100);
    std::cout<<jsonBinder<<std::endl;
    jsonBinder(jp, "end", 200);
    sylvanmats::io::json::Path jp2;
    jsonBinder(jp2, "CGU", sylvanmats::io::json::object());
    jp2["CGU"];
    jsonBinder(jp2, "start", 300);
    jsonBinder(jp2, "end", 400);
    //jsonBinder.display();
    std::cout<<jsonBinder<<std::endl;
    CHECK_EQ(jsonBinder.countObjects(), 10);
    sylvanmats::io::json::Path jp3=Root();
//    jp3["8DR"];
    std::cout<<"jp3 "<<jp3<<std::endl;
    jsonBinder(jp3, "8DR");
    std::cout<<jsonBinder<<std::endl;
    CHECK_EQ(jsonBinder.countObjects(), 6);
}

}
