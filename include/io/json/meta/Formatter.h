#prama once
#include <cstdio>
#include <string>
#include <string_view>

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
        constexpr std::string_view operator ()(T value){
            return std::string_view(reinterpret_cast<const char*>(&value), sizeof(T));
        }
    };
}