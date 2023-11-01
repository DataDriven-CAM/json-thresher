#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>

namespace sylvanmats::io::json{

    enum ACTION{
        NOP,
        TEST
    };
    
    struct element{
        std::string_view label;
        ACTION action = NOP;
        std::string_view value;
    };
    
    struct Root;
    
    class Path{
        friend class Binder;
    protected:
        std::vector<element> p;
        std::vector<std::string> s;
        std::allocator<std::string> allocations;
    public:
        Path() = default;
        Path(const char* c){
            s.push_back(std::string(c));
            this->p.push_back({.label=std::string_view(s.back()), .action=NOP});
        };
        
        Path(std::vector<element>& p){
            this->p.insert(this->p.end(), p.begin(), p.end());
        };
        
        Path(struct Root&) {} // converting constructor
        
        Path(const Path& orig) = default;
        Path(Path&& orig) = default;
        virtual ~Path() = default;
        
        Path& operator=(const Path& p){
            return *this = Path(p);
        }
        
        Path& operator=(Path&& p) noexcept{
            std::swap(this->p, p.p);
            return *this;
        }
        
        Path& operator=(const char* c){
            //if(!this->p.empty())this->p.clear();
            this->p.push_back({.label=std::string_view(c), .action=NOP});
            return *this;
        }
        
        const Path& operator=(const std::string& s){
            //if(!this->p.empty())this->p.clear();
            this->p.push_back({.label=std::string_view(s), .action=NOP});
            return *this;
        }
        
        const Path& operator=(const std::string_view& sv){
            //if(!this->p.empty())this->p.clear();
            this->p.push_back({.label=std::string_view(sv), .action=NOP});
            return *this;
        }
        
        Path& operator/(const Path& p1){
//            std::cout<<"in this divide "<<p1.p.size()<<" "<<this->p.size()<<std::endl;
            this->p.insert(this->p.end(), p1.p.begin(), p1.p.end());
            return *this;
        }
        
        Path& operator[](std::string s){
//            std::string s(c);
            size_t offset=s.find("=");
            if(s.at(0)=='@' && offset!=std::string::npos){
                this->s.emplace_back(std::string(s.substr(1, offset-1)));
                this->s.emplace_back(std::string(s.substr(offset+1)));
                this->p.push_back({.label=std::string_view(this->s[this->s.size()-2]), .action=TEST, .value=std::string_view(this->s.back())});
            }
            else{
                this->s.emplace_back(s);
                this->p.push_back({.label=std::string_view(this->s.back()), .action=NOP});            
            }
            return *this;
        }
        
        Path& operator[](std::vector<element> p[]){
            //this->p.insert(this->p.end(), p.begin(), p.end());
            return *this;
        }
        
        
        Path& operator[](size_t i){
            std::string s="["+std::to_string(i)+"]";
            this->p.push_back({.label=std::string_view(s), .action=NOP});
            return *this;
        }
        
        Path& operator !(){
            std::string s="/";
            this->p.push_back({.label=std::string_view(s), .action=NOP});
            return *this;
        }

        friend Path& operator !(const Path& p){
            std::string s="/";
            ((std::vector<element>)p.p).push_back({.label=std::string_view(s), .action=NOP});
            return (Path&)p;
        }

        friend Path& operator /=(const Path& p, Path& p2){
            std::string s="/";
            ((std::vector<element>)p.p).insert(((std::vector<element>)p.p).end(), p2.p.begin(), p2.p.end());
            return (Path&)p;
        }

        friend Path& operator/(const Path& p, const char* s){
            ((std::vector<element>)p.p).push_back({.label=std::string_view(s), .action=NOP});
            return (Path&)p;
        }

        friend Path& operator/(const Path& p, const std::string_view& sv){
            ((std::vector<element>)p.p).push_back({.label=sv, .action=NOP});
            return (Path&)p;
        }

        friend Path& operator/=(const Path& p, const char* s){
            ((Path&)p).p.push_back({.label=std::string_view(s), .action=NOP});
            return (Path&)p;
        }

        friend Path& operator/=(const Path& p, const std::string_view& sv){
            ((std::vector<element>)p.p).push_back({.label=sv, .action=NOP});
            return (Path&)p;
        }

        friend std::ostream& operator<<(std::ostream& s, Path& p) {
        for(auto p : p.p)
          s <<"/"<< p.label;
          return s;
        }
    };
    
}

    struct Root
    {
        operator sylvanmats::io::json::Path(){return sylvanmats::io::json::Path("/");} // conversion function
    };
    
    sylvanmats::io::json::Path operator"" _jp(const char* c, size_t s){
        return sylvanmats::io::json::Path(c);
    }
    
