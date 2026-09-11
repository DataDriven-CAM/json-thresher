#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#define DOCTEST_CONFIG_USE_STD_HEADERS // Add this line
#include <doctest/doctest.h>

#include <cstdio>
#include <fstream>
#include <ranges>
#include <deque>
#include <chrono>
#include <meta>

#define protected public
#include "io/json/Binder.h"
#include "io/tikz/GraphPublisher.h"

#include "graph/container/compressed_graph.hpp"
#include "graph/views/incidence.hpp"
#include "graph/views/vertexlist.hpp"

TEST_SUITE("compile-time"){

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
  jsonBinder.display();
  sylvanmats::io::json::Path jp;
  jp["geometry"]["*"];
  std::vector<double> geometry;
  jsonBinder(jp, [&geometry](std::any& v){
    std::cout<<"type: "<<v.type().name()<<std::endl;
    geometry.push_back(std::any_cast<double>(v));
  });
  CHECK_EQ(geometry.size(), 3);
  if(geometry.size()==3){
    CHECK_EQ(geometry[0], 1.0);
    CHECK_EQ(geometry[1], 20.0);
    CHECK_EQ(geometry[2], -4.0);
  }
}

}
