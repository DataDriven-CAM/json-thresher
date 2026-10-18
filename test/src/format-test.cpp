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
    std::string number_of_pages{};
};

TEST_SUITE("format preparation by metaphrase"){

TEST_CASE("test generating formatting for citation") {
    sylvanmats::io::json::meta::Formatter<citation> formatter;
    citation cit;//{.id="knuth1997art", .type="book", .publisher="", .publisher_place="", .volume="", .edition="", .title="", .author={}, .issued={.date_parts="", .season="", .circa="", .literal="", .raw=""}, .journalAbbreviation="", .number_of_pages=""};
    std::string&& jobjectTemplate=formatter(cit);

    std::cout << "--- METAPROGRAMMED STRUCT FORMAT ---\n";
    std::cout << jobjectTemplate.size()<<" "<<jobjectTemplate<<std::endl;
    CHECK_EQ(jobjectTemplate.size(), 269);
}

}