#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#define DOCTEST_CONFIG_USE_STD_HEADERS // Add this line
#include <doctest/doctest.h>

#include <cstdio>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <ranges>
#include <deque>
#include <chrono>
#include <meta>
#include <string_view>
#include <span>
#include <array>
#include <format>
#include <concepts>

#define protected public
#include "io/json/Binder.h"
#include "io/tikz/GraphPublisher.h"

#include "graph/container/dynamic_graph.hpp"
#include "graph/views/incidence.hpp"
#include "graph/views/vertexlist.hpp"

#include "metaphrase/GBackusNaurFormation.h"


TEST_SUITE("compile-time"){

TEST_CASE("test generating grammar components") {
    constexpr auto root_rule  = sylvanmats::metaphrase::make_gbnf_rule<"root", R"("{ \"graph\": " graphBody " }")">();
    constexpr auto graph_body = sylvanmats::metaphrase::make_gbnf_rule<"graphBody", R"("{ \"nodes\": " nodesArray " }")">();

    std::cout << "--- METAPROGRAMMED GBNF ---\n";
    std::cout << root_rule.view();
    std::cout << graph_body.view();

}

TEST_CASE("test dfs"){
  try{
    // Ensure static storage duration so the view points to persistent data
    static constexpr std::u8string_view sample_schema = u8R"({
        "type": "object",
        "properties": {
            "id": "number",
            "name": "string"
        }
    })";

        sylvanmats::metaphrase::GBackusNaurFormation gbnf_parser{};
    constexpr auto generated_rules = gbnf_parser(sample_schema);

    std::cout << generated_rules.view();
  }
  catch(std::out_of_range& e){
    std::cout << "out of range "<<e.what()<<std::endl;
  }
  catch(std::exception& e){
    std::cout << "exception "<<e.what()<<std::endl;
  }
}

TEST_CASE("test jgf 2.0 binding"){
    constexpr char8_t jsonBuffer[] ={
#embed "json-graph-schema_v2.json"
    };
    sylvanmats::metaphrase::GBackusNaurFormation gBackusNaurFormation;
    static constexpr auto gbnf=gBackusNaurFormation(std::u8string_view(jsonBuffer, sizeof(jsonBuffer)));
    constexpr std::string_view gbnfView=gbnf.view();
    std::cout << gbnfView<<std::endl;
    CHECK_EQ(gbnfView.size(), 178);
    CHECK_NE(gbnfView.find("node"), std::string_view::npos);
    CHECK_NE(gbnfView.find("edge"), std::string_view::npos);
    CHECK_NE(gbnfView.find("graph"), std::string_view::npos);
    CHECK_NE(gbnfView.find("nodes"), std::string_view::npos);
    CHECK_NE(gbnfView.find("edges"), std::string_view::npos);
    CHECK_NE(gbnfView.find("root"), std::string_view::npos);


}

TEST_CASE("test meta of typing") {
  std::string jsonContent=R"({
  "name": "winnow-mr",
  "repository": "https://github.com/DataDriven-CAM/winnow-mr.git",
  "private": null,
  "geometry": [1.0, 20.0, -4.0],
  "scripts": {
    "compilecore": "make -C packages/winnow",
    "devui": "bun --filter winnow-ui dev",
    "buildui": "bun --filter winnow-ui build",
    "testall": "bun test packages/**/*"
  }
})";
  sylvanmats::io::json::Binder jsonBinder;
  jsonBinder(jsonContent);
  CHECK_EQ(graph::num_vertices(jsonBinder.dagGraph), 13);
  CHECK_EQ(graph::num_edges(jsonBinder.dagGraph), 12);
  // jsonBinder.display();
  sylvanmats::io::json::Path jp;
  jp["geometry"]["*"];
  std::vector<double> geometry;
  jsonBinder(jp, [&geometry](const sylvanmats::io::json::JsonValue& v){
    if (auto pVal = std::get_if<double>(&v)) {
        geometry.push_back(*pVal);
    }
  });
  CHECK_EQ(geometry.size(), 3);
  if(geometry.size()==3){
    CHECK_EQ(geometry[0], 1.0);
    CHECK_EQ(geometry[1], 20.0);
    CHECK_EQ(geometry[2], -4.0);
  }
}

}
