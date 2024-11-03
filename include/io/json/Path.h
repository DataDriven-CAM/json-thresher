#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>
#include <memory>

namespace sylvanmats::io::json{
    template <typename T>
    struct iterator_traits {
        using __secret_am_i_the_primary_alias = iterator_traits;
    };

    template <typename T>concept is_iterator_primary = std::same_as<typename iterator_traits<T>::__secret_am_i_the_primary_alias, iterator_traits<T>>;

    enum ACTION{
        NOP,
        TEST
    };
    
    struct element{
        using size_type =  std::size_t;
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
        std::allocator<element> allocations;
        std::string::size_type n=0;
        std::string::size_type reserveSize=0;
        element* buffer=nullptr;
        template<typename I> requires std::same_as<I, element>// && std::input_or_output_iterator<I>
        struct iterator {
            template<typename D>
            using iter_difference_t = typename std::conditional_t<is_iterator_primary<D>, std::incrementable_traits<D>, iterator_traits<D>>::difference_type;
            I* buffer=nullptr;
            iterator() = default;
            iterator(I* buffer){this->buffer=buffer;};
            iterator(const iterator<I>& orig) = default;
            iterator(iterator<I>&& other) = default;
            virtual ~iterator() = default;
            iterator& operator=(const iterator& other) noexcept = default;
            iterator& operator=(iterator&& other) noexcept = default;
            
            bool operator==(const iterator<I>& other){ return this->buffer==other.buffer;};
            bool operator!=(const iterator<I>& other){ return this->buffer!=other.buffer;};
            iterator<I>& operator++(){buffer++; return *this;};
            iterator<I>& operator++(int){iterator<I>& old=*this;operator++(); return old;};
            iterator<I>& operator--(){buffer--; return *this;};
            iterator<I>& operator--(int){iterator<I>& old=*this;operator--(); return old;};
            I& operator*() {return (I&)(*buffer);};
            I* operator->() { return buffer; };
            
        };
        friend bool operator==(const iterator<element>& orig, const iterator<element>& other){ return orig.buffer==other.buffer;};
        friend bool operator!=(const iterator<element>& orig, const iterator<element>& other){ return orig.buffer!=other.buffer;};
    public:
        Path() = default;
        Path(const char* c);
        
//        Path(std::vector<element>& p){
//            this->p.insert(this->p.end(), p.begin(), p.end());
//        };
        
        Path(struct Root&) {} // converting constructor
        
        Path(const Path& orig) = default;
        Path(Path&& orig) = default;
        virtual ~Path(){
            if(n>0){
            std::destroy_n(buffer, n);
            allocations.deallocate(buffer, n);
            }
        };
        Path& operator=(const Path& other) noexcept = default;
        Path& operator=(Path&& other) noexcept = default;
        
        void reserve(std::string::size_type reserveSize){
            buffer=allocations.allocate(reserveSize);
            this->reserveSize=reserveSize;
        };
        
        void push_back(const element& value){
            if(n>=reserveSize){
                
            }
            std::construct_at(buffer+n, value);
            n++;
            
        };
        
        iterator<element> begin(){ return iterator<element>(buffer);};

        iterator<element> end(){ return iterator<element>(buffer+n);};
        
        const iterator<element> cbegin() const { return iterator<element>(buffer);};

        const iterator<element> cend() const { return iterator<element>(buffer+n);};
        
        element::size_type size(){return n;};
        
        element::size_type max_size(){return std::numeric_limits<element::size_type>::max() / sizeof(element);};//std::allocator_traits<std::allocator<element>>::max_size;};
        
        bool empty(){return n==0;};
        std::string string(){
            std::string ret{};
            for(auto e : p){
                if(e.action==TEST){
                    ret.append("/").append(e.label).append(" == ").append(e.value);
                }
                else{
                    ret.append("/").append(e.label);
                    }
            }
            return ret;
        };
        
//        Path& operator=(const Path& p){
//            return *this = Path(p);
//        }
        
//        Path& operator=(Path&& p) noexcept{
//            std::swap(this->p, p.p);
//            return *this;
//        }
        
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
        
        Path& operator==(std::string s){
            this->s.emplace_back(s);
            this->p.back().action=TEST;
            this->p.back().value=std::string_view(this->s.back());
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

        friend Path& operator/(Path& p, const char* c){
            p.s.push_back(std::string(c));
//            std::cout<<"in this divide / "<<p.p.size()<<" "<<p.s.back()<<std::endl;
            p.p.push_back({.label=std::string_view(p.s.back()), .action=NOP});
            return p;
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
        for(auto p : p.p){
            if(p.action==TEST){
                s <<"/"<< p.label<<" == "<<p.value;
            }
            else
                s <<"/"<< p.label;
        }
          return s;
        }
    };
    
}

    struct Root
    {
        operator sylvanmats::io::json::Path(){return sylvanmats::io::json::Path("/");} // conversion function
    };
    
    sylvanmats::io::json::Path operator"" _jp(const char* c, size_t s);
    
