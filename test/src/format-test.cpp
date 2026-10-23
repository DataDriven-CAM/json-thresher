#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#define DOCTEST_CONFIG_USE_STD_HEADERS
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
#include <atomic>

#define protected public


#include "io/json/Binder.h"
#include "io/tikz/GraphPublisher.h"
#include "io/json/meta/Formatter.h"

struct citation{
    std::string id{};
    std::string type{};
    std::string publisher{};
    std::string publisher_place{};
    std::string volume{};
    std::string edition{};
    std::string title{};
    std::vector<std::string> author{};
    struct {
        std::string date_parts{};
        std::string season{};
        std::string circa{};
        std::string literal{};
        std::string raw{};
    } issued{};
    std::string journalAbbreviation{};
    std::variant<int, std::string> number_of_pages{};
};

    // template<typename T>
    // T render(std::basic_string_view<typename T::value_type> formatSyntax, std::format_args&& args){
    //     return std::vformat(formatSyntax, args);
    // }

TEST_SUITE("format preparation by metaphrase"){

TEST_CASE("test generating formatting for citation") {
    sylvanmats::io::json::meta::Formatter<citation> formatter;
    static constexpr citation cit{};//{.id="knuth1997art", .type="book", .publisher="", .publisher_place="", .volume="", .edition="", .title="", .author={}, .issued={.date_parts="", .season="", .circa="", .literal="", .raw=""}, .journalAbbreviation="", .number_of_pages=""};
    std::string_view formatSyntax=formatter(cit).view();

    std::cout << "--- METAPROGRAMMED STRUCT FORMAT ---\n";
    std::cout << formatSyntax.size()<<" "<<formatSyntax<<std::endl;
    CHECK_EQ(formatSyntax.size(), 269);

    // auto idArg=std::arg("id", "knuth1997art");
    // std::string_view jobjectText=render<std::string_view>(formatSyntax, fmt::make_format_args(idArg));
    // std::cout<<"json object text: "<<jobjectText<<std::endl;
    // CHECK_EQ(jobjectText.size(), 269);
}

TEST_CASE("test generating formatting for citation") {
    citation my_citation;
    my_citation.id = "REF-001";
    my_citation.title = "Modern C++26 Reflection Pipelines";
    my_citation.author = {"John Doe", "Jane Smith"};
    
    // Partially initialize nested sub-struct
    my_citation.issued.date_parts = "2026"; 
    
    // Assign a pure number to your dual-typed layout field
    my_citation.number_of_pages = 412; 

    sylvanmats::io::json::meta::Formatter<citation> formatter;
    // Execute the compiled buffer mapping sequence
    std::string json_result = formatter.emit_json_buffer(my_citation);
    
    std::cout<<std::format("{}", json_result)<<std::endl;
    CHECK_EQ(json_result.size(), 269);
}

TEST_CASE("test inverse deserialization parsing back into citation struct") {
    citation parsed_citation;

    // Simulate standard streaming or token pairs passing through from your binder paths
    std::string_view incoming_key = "number__of__pages";
    std::string_view incoming_value = "412";

    sylvanmats::io::json::meta::Formatter<citation> formatter;
    // Instantly map the field over
    bool success = formatter.deserialize_field(parsed_citation, incoming_key, incoming_value);

    // Verify type recovery matches the dual variant type exactly
    CHECK(success == true);
    CHECK(std::get<int>(parsed_citation.number_of_pages) == 412);
}

}