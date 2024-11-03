
#include <cstdio>
#include <ranges>
#include <cmath>

#include "io/json/Path.h"

#include <iostream>

namespace sylvanmats::io::json{
    Path::Path(const char* c){
        s.push_back(std::string(c));
//        this->p.push_back({.label=std::string_view(s.back()), .action=NOP});
        std::string pathRespresentation(c);
        constexpr std::string_view delim{"/"};
        std::ranges::split_view splitting(pathRespresentation, delim);
        reserveSize=std::distance(splitting.cbegin(), splitting.cend());
        reserve(reserveSize);
        for (const auto wordRange : splitting){
            std::string word(wordRange.begin(), wordRange.end());
            s.push_back(word);
            if(!word.empty())this->p.push_back({.label=std::string_view(s.back()), .action=NOP});
            if(!word.empty())push_back({.label=std::string_view(s.back()), .action=NOP});
        }
    };
}

sylvanmats::io::json::Path operator"" _jp(const char* c, size_t s){
    return sylvanmats::io::json::Path(c);
}

