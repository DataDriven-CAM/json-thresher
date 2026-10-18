#pragma once
#include <cstdio>
#include <string>
#include <string_view>
#include <sstream>

namespace sylvanmats::io::json::meta{
    
    template<typename T>
    class Formatter{
        public:
        Formatter() = default;
        Formatter(const Formatter& orig) = delete;
        Formatter(Formatter&& orig) = delete;
        Formatter& operator=(const Formatter& orig) = delete;
        Formatter& operator=(Formatter&& orig) = delete;
        ~Formatter() = default;
        constexpr std::string operator ()(T value, int depth = 0){
            std::string jobjectTemplate;
            std::string indent(depth * 4, ' ');
            std::stringstream ss;
            if constexpr (std::meta::is_type(^^T) && std::is_class_v<typename [: ^^T :]>) {
                ss<<"{\n";
                template for (constexpr auto m : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))) {
                    ss<<indent <<"\""<< std::meta::identifier_of(m) << "\": {"<<std::meta::identifier_of(m)<<"}";
                    //std::cout <<indent<< "Field: " << std::meta::identifier_of(m) << " = ";
                    ss<<"\n";
                }
                ss<<"}\n";
            }

            jobjectTemplate=ss.str();
            return jobjectTemplate;
        }
    };
}