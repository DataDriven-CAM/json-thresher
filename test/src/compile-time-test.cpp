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
#include <filesystem>
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>

#define protected public
#include "io/json/Binder.h"
#include "io/tikz/GraphPublisher.h"

#include "graph/container/dynamic_graph.hpp"
#include "graph/views/incidence.hpp"
#include "graph/views/vertexlist.hpp"

#include "metaphrase/GBackusNaurFormation.h"

    extern char** environ;

    void target_test_environment() {
    std::filesystem::path root = std::filesystem::current_path();
    
    // Resolve clean absolute paths
    std::filesystem::path bin_dir = root / "../cpp_modules/llama/dist/bin";
    std::filesystem::path lib_dir = root / "../cpp_modules/llama/dist/lib";

    // Append to PATH
    const char* old_path = std::getenv("PATH");
    std::string new_path = bin_dir.string() + ":" + (old_path ? old_path : "");
    setenv("PATH", new_path.c_str(), 1);

    // Append to LD_LIBRARY_PATH
    const char* old_ld = std::getenv("LD_LIBRARY_PATH");
    std::string new_ld = lib_dir.string() + ":" + (old_ld ? old_ld : "");
    setenv("LD_LIBRARY_PATH", new_ld.c_str(), 1);
}

bool validate_gbnf(std::string_view gbnfView, std::string_view jsonContent){
    std::filesystem::path tmpDir=std::filesystem::temp_directory_path();
    std::string grammarPath = (tmpDir / "schema_test.gbnf").string();
    std::string jsonPath = (tmpDir / "schema_test.json").string();
    std::ofstream g_file(grammarPath);
     g_file << gbnfView;
     g_file.close();
    std::ofstream j_file(jsonPath);
     j_file << jsonContent;
     j_file.close();
    std::array<const char*, 4> args = {
        "test-gbnf-validator", 
        grammarPath.data(), 
        jsonPath.data(), 
        nullptr
    };

    pid_t pid;
    // Spawns the executable using PATH resolution (the 'p' variant)
    int spawn_result = posix_spawnp(&pid, args[0], nullptr, nullptr, 
                                    const_cast<char* const*>(args.data()), environ);
    
    if (spawn_result != 0) {
        // Target binary was not found or failed to execute completely
        //FAIL("Failed to spawn grammar validator process");
        return false;
    }

    // Block synchronously until the specific validator process closes
    int status;
    waitpid(pid, &status, 0);
    std::cout<<"status "<<status<<" "<<WIFEXITED(status)<<" "<<WEXITSTATUS(status)<<std::endl;
    CHECK_EQ(WEXITSTATUS(status), 0);
    // Returns true if test-gbnf-validator exits cleanly with return code 0
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

TEST_SUITE("compile-time"){

TEST_CASE("test generating grammar components") {
    constexpr auto root_rule  = sylvanmats::metaphrase::make_gbnf_rule<"root", R"("{ \"graph\": " graphBody " }")">();
    constexpr auto graph_body = sylvanmats::metaphrase::make_gbnf_rule<"graphBody", R"("{ \"nodes\": " nodesArray " }")">();

    std::cout << "--- METAPROGRAMMED GBNF ---\n";
    std::cout << root_rule.view();
    std::cout << graph_body.view();

}

TEST_CASE("test simple primitives"){
  try{
    // Ensure static storage duration so the view points to persistent data
    static constexpr std::u8string_view sample_schema = u8R"({
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "id": { "type": "number" },
    "name": { "type": "string" }
  }
})";

    sylvanmats::metaphrase::GBackusNaurFormation gBackusNaurFormation;
    static constexpr auto gbnf=gBackusNaurFormation(sample_schema);
    constexpr std::string_view gbnfView=gbnf.view();
    std::cout << gbnfView<<std::endl;
    CHECK_EQ(gbnfView.size(), 178);
    CHECK_NE(gbnfView.find("id"), std::string_view::npos);
    CHECK_NE(gbnfView.find("name"), std::string_view::npos);

    target_test_environment();
    CHECK(validate_gbnf(gbnfView, R"({"id": 100, "name": "Alice"})"));
  }
  catch(std::out_of_range& e){
    std::cout << "out of range "<<e.what()<<std::endl;
  }
  catch(std::exception& e){
    std::cout << "exception "<<e.what()<<std::endl;
  }
}

TEST_CASE("test homogeneous vector"){
  try{
    // Ensure static storage duration so the view points to persistent data
    static constexpr std::u8string_view sample_schema = u8R"({
  "$schema": "https://json-schema.org",
  "type": "object",
  "properties": {
    "tags": {
      "type": "array",
      "items": { "type": "string" }
    }
  },
  "required": ["tags"],
  "additionalProperties": false
})";

    sylvanmats::metaphrase::GBackusNaurFormation gBackusNaurFormation;
    static constexpr auto gbnf=gBackusNaurFormation(sample_schema);
    constexpr std::string_view gbnfView=gbnf.view();
    std::cout << gbnfView<<std::endl;
    CHECK_EQ(gbnfView.size(), 178);
    CHECK_NE(gbnfView.find("tags"), std::string_view::npos);
    CHECK_NE(gbnfView.find("type"), std::string_view::npos);
    CHECK_NE(gbnfView.find("items"), std::string_view::npos);
    target_test_environment();
    std::string_view gbnfView2=R"(root   ::= "{" ws "\"tags\"" ws ":" ws string-array "}" ws
string-array ::= "[" ws ( string ( ws "," ws string )* )? ws "]" ws

string ::= "\"" [^"\\]* "\"" ws
ws  ::= [ \t\n\r]*
)";
    CHECK(validate_gbnf(gbnfView, R"({"tags": ["Alice", "Matilda"]})"));
   }
  catch(std::out_of_range& e){
    std::cout << "out of range "<<e.what()<<std::endl;
  }
  catch(std::exception& e){
    std::cout << "exception "<<e.what()<<std::endl;
  }
}

TEST_CASE("test fixed array / tuple"){
  try{
    // Ensure static storage duration so the view points to persistent data
    static constexpr std::u8string_view sample_schema = u8R"({
  "$schema": "https://json-schema.org",
  "type": "object",
  "properties": {
    "point_2d": {
      "type": "array",
      "prefixItems": [
        { "type": "number" },
        { "type": "number" }
      ],
      "items": false
    }
  },
  "required": ["point_2d"],
  "additionalProperties": false
})";

    sylvanmats::metaphrase::GBackusNaurFormation gBackusNaurFormation;
    static constexpr auto gbnf=gBackusNaurFormation(sample_schema);
    constexpr std::string_view gbnfView=gbnf.view();
    std::cout << gbnfView<<std::endl;
    CHECK_EQ(gbnfView.size(), 178);
    CHECK_NE(gbnfView.find("oint_2d"), std::string_view::npos);
    // CHECK_NE(gbnfView.find("name"), std::string_view::npos);
    target_test_environment();
    CHECK(validate_gbnf(gbnfView, R"({"point_2d": [{0.0, 0.0}, {1.0, 1.0}]})"));
  }
  catch(std::out_of_range& e){
    std::cout << "out of range "<<e.what()<<std::endl;
  }
  catch(std::exception& e){
    std::cout << "exception "<<e.what()<<std::endl;
  }
}

TEST_CASE("test optional fields"){
  try{
    // Ensure static storage duration so the view points to persistent data
    static constexpr std::u8string_view sample_schema = u8R"({
  "$schema": "https://json-schema.org",
  "type": "object",
  "properties": {
    "name": { "type": "string" },
    "age": { "type": "integer" }
  },
  "required": ["name"],
  "additionalProperties": false
})";

    sylvanmats::metaphrase::GBackusNaurFormation gBackusNaurFormation;
    static constexpr auto gbnf=gBackusNaurFormation(sample_schema);
    constexpr std::string_view gbnfView=gbnf.view();
    std::cout << gbnfView<<std::endl;
    CHECK_EQ(gbnfView.size(), 178);
    CHECK_NE(gbnfView.find("name"), std::string_view::npos);
    CHECK_NE(gbnfView.find("age"), std::string_view::npos);
    target_test_environment();
    CHECK(validate_gbnf(gbnfView, R"({"name": "Alice", "age": 100})"));
    }
  catch(std::out_of_range& e){
    std::cout << "out of range "<<e.what()<<std::endl;
  }
  catch(std::exception& e){
    std::cout << "exception "<<e.what()<<std::endl;
  }
}

TEST_CASE("test nested structural dependency"){
  try{
    // Ensure static storage duration so the view points to persistent data
    static constexpr std::u8string_view sample_schema = u8R"({
  "$schema": "https://json-schema.org",
  "type": "object",
  "properties": {
    "user": { "$ref": "#/$defs/SimpleUser" },
    "tags": {
      "type": "array",
      "items": { "type": "string" }
    }
  },
  "required": ["user", "tags"],
  "additionalProperties": false,
  "$defs": {
    "SimpleUser": {
      "type": "object",
      "properties": {
        "id": { "type": "integer" },
        "active": { "type": "boolean" }
      },
      "required": ["id", "active"],
      "additionalProperties": false
    }
  }
})";

    sylvanmats::metaphrase::GBackusNaurFormation gBackusNaurFormation;
    static constexpr auto gbnf=gBackusNaurFormation(sample_schema);
    constexpr std::string_view gbnfView=gbnf.view();
    std::cout << gbnfView<<std::endl;
    CHECK_EQ(gbnfView.size(), 178);
    CHECK_NE(gbnfView.find("id"), std::string_view::npos);
    CHECK_NE(gbnfView.find("active"), std::string_view::npos);
    target_test_environment();
    CHECK(validate_gbnf(gbnfView, R"({"user": "Alice", "tags": ["Alice", "Matilda"]})"));
  }
  catch(std::out_of_range& e){
    std::cout << "out of range "<<e.what()<<std::endl;
  }
  catch(std::exception& e){
    std::cout << "exception "<<e.what()<<std::endl;
  }
}

TEST_CASE("test polymorphism"){
  try{
    // Ensure static storage duration so the view points to persistent data
    static constexpr std::u8string_view sample_schema = u8R"({
  "$schema": "https://json-schema.org",
  "type": "object",
  "properties": {
    "response": {
      "oneOf": [
        { "$ref": "#/$defs/SuccessPayload" },
        { "$ref": "#/$defs/ErrorPayload" }
      ]
    }
  },
  "required": ["response"],
  "additionalProperties": false,
  "$defs": {
    "SuccessPayload": {
      "type": "object",
      "properties": {
        "status": { "type": "string", "const": "success" },
        "data": { "type": "string" }
      },
      "required": ["status", "data"],
      "additionalProperties": false
    },
    "ErrorPayload": {
      "type": "object",
      "properties": {
        "status": { "type": "string", "const": "error" },
        "code": { "type": "integer" }
      },
      "required": ["status", "code"],
      "additionalProperties": false
    }
  }
})";

    sylvanmats::metaphrase::GBackusNaurFormation gBackusNaurFormation;
    static constexpr auto gbnf=gBackusNaurFormation(sample_schema);
    constexpr std::string_view gbnfView=gbnf.view();
    std::cout << gbnfView<<std::endl;
    CHECK_EQ(gbnfView.size(), 178);
    CHECK_NE(gbnfView.find("status"), std::string_view::npos);
    CHECK_NE(gbnfView.find("code"), std::string_view::npos);
    target_test_environment();
    CHECK(validate_gbnf(gbnfView, R"({"user": "Alice", "tags": ["Alice", "Matilda"]})"));
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
    target_test_environment();
    CHECK(validate_gbnf(gbnfView, R"({"user": "Alice", "tags": ["Alice", "Matilda"]})"));


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
