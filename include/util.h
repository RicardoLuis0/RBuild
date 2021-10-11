#pragma once

#include <ranges>
#include <map>
#include <vector>
#include <algorithm>
#include <filesystem>

#define __PP_JOIN(a,b) a##b
#define PP_JOIN(a,b) __PP_JOIN(a,b)

#if defined(__linux__)
    #define CUR_PLATFORM "linux"
#elif defined(_WIN32)
    #define CUR_PLATFORM "windows"
#endif

#define RBUILD_VERSION "0.0.0c"

inline std::string operator"" _s(const char * s,size_t n){
    return {s,n};
}

constexpr unsigned long long int operator"" _K(unsigned long long int n){
    return n*1024;
}

constexpr unsigned long long int operator"" _M(unsigned long long int n){
    return n*1024_K;
}

constexpr unsigned long long int operator"" _G(unsigned long long int n){
    return n*1024_M;
}

namespace Util {
    
    std::string readfile(const std::string &filename);
    void writefile(const std::string &filename,const std::string &data);
    
    std::string quote_str(const std::string &s,char quote_char);
    
    inline std::string quote_str_double(const std::string &s){
        return quote_str(s,'"');
    }
    
    inline std::string quote_str_single(const std::string &s){
        return quote_str(s,'\'');
    }
    
    template<typename K,typename V>
    std::vector<K> keys(const std::map<K,V> &m){
        std::vector<K> k;
        k.reserve(m.size());
        for(const auto &kv:m){
            k.push_back(kv.first);
        }
        return k;
    }
    
    template<typename K,typename V>
    std::vector<V> values(const std::map<K,V> &m){
        std::vector<V> v;
        v.reserve(m.size());
        for(const auto &kv:m){
            v.push_back(kv.second);
        }
        return v;
    }
    
    inline bool is_subpath(const std::filesystem::path &base,const std::filesystem::path &sub){
        return std::filesystem::canonical(sub).string().starts_with(std::filesystem::canonical(base).string());
    }
    
    std::string join(const std::vector<std::string> &v,const std::string &on=" ");
    
    std::string join_or(const std::vector<std::string> &v,const std::string &sep_comma=", ",const std::string &sep_or=", or ");
    
    std::vector<std::string> split(const std::string &str,char split_on,bool split_empty=false);
    std::vector<std::string> split(const std::string &str,const std::vector<char> &split_on,bool split_empty=false);
    std::vector<std::string> split_str(const std::string &str,const std::string &split_on,bool split_empty=false);
    std::vector<std::string> split_str(const std::string &str,const std::vector<std::string> &split_on,bool split_empty=false);
    
    template<std::ranges::input_range R>
    bool contains(const R &r,const typename R::value_type &v) requires std::equality_comparable<typename R::value_type>{ //O(n)
        for(const auto &e:r){
            if(e==v)return true;
        }
        return false;
    }
    
    template<std::ranges::input_range Ra,std::ranges::input_range Rb>
    bool contains(const Ra &ra,const Rb &rb) requires std::equality_comparable_with<typename Ra::value_type,typename Rb::value_type>{//O(n*m)
        for(const auto &ea:ra){
            for(auto &eb:rb){
                if(ea==eb)return true;
            }
        }
        return false;
    }
    
    template<std::ranges::input_range R,std::invocable<typename R::value_type> Fn_T>
    std::vector<typename std::invoke_result<Fn_T,typename R::value_type>::type> map(const R &r,const Fn_T &f) {
        std::vector<typename std::invoke_result<Fn_T,typename R::value_type>::type> v;
        v.reserve(std::size(r));
        for(const auto &e:r){
            v.emplace_back(std::invoke(f,e));
        }
        return v;
    }
    
    template<std::ranges::input_range R,std::invocable<typename R::value_type> Fn_T>
    std::vector<typename std::invoke_result<Fn_T,typename R::value_type>::type> map_copy(const R &r,const Fn_T &f) {
        std::vector<typename std::invoke_result<Fn_T,typename R::value_type>::type> v;
        v.reserve(std::size(r));
        for(const auto &e:r){
            v.push_back(std::invoke(f,e));
        }
        return v;
    }
    
    template<typename T,std::ranges::input_range R>
    std::vector<T> map_construct(const R &r) requires std::constructible_from<T,typename R::value_type> {
        std::vector<T> v;
        v.reserve(std::size(r));
        for(const auto &e:r){
            v.emplace_back(e);
        }
        return v;
    }
    
    template<std::ranges::input_range R,std::invocable<typename R::value_type> Fn_T>
    R& inplace_map(R &r,const Fn_T &f) requires std::is_assignable_v<std::add_lvalue_reference_t<typename R::value_type>,typename std::invoke_result<Fn_T,typename R::value_type>::type> {
        for(auto &e:r){
            e=std::invoke(f,e);
        }
        return r;
    }
    
    template<std::ranges::input_range Ra,std::ranges::input_range Rb>
    std::vector<typename Ra::value_type> filter_exclude(const Ra &ra,const Rb &rb) requires std::equality_comparable_with<typename Ra::value_type,typename Rb::value_type> { // O(n*m)
        std::vector<typename Ra::value_type> v;
        v.reserve(std::size(ra));
        for(const auto &ea:ra){
            bool match=false;
            for(const auto&eb:rb){
                if(ea==eb){
                    match=true;
                }
            }
            if(!match)v.push_back(ea);
        }
        v.shrink_to_fit();
        return v;
    }
    
    template<std::ranges::input_range R>
    std::vector<typename R::value_type> insert_interleaved_before(const R& r,const typename R::value_type &v){
        std::vector<typename R::value_type> out;
        out.reserve(std::size(r)*2);
        for(auto &e:r){
            out.push_back(v);
            out.push_back(e);
        }
        return out;
    }
    
    template<std::ranges::input_range R>
    std::vector<typename R::value_type> insert_interleaved_after(const R& r,const typename R::value_type &v){
        std::vector<typename R::value_type> out;
        out.reserve(std::size(r)*2);
        for(auto &e:r){
            out.push_back(e);
            out.push_back(v);
        }
        return out;
    }
    
    template<std::ranges::input_range Ra,std::ranges::input_range Rb>
    std::pair</* IN */ std::vector<typename Ra::value_type>,/* OUT */ std::vector<typename Ra::value_type>> filter_inout(const Ra &ra,const Rb &rb) requires std::equality_comparable_with<typename Ra::value_type,typename Rb::value_type> { // O(n*m)
        std::vector<typename Ra::value_type> in;
        std::vector<typename Ra::value_type> out;
        in.reserve(std::size(ra));
        out.reserve(std::size(ra));
        for(const auto &ea:ra){
            bool match=false;
            for(const auto&eb:rb){
                if(ea==eb){
                    match=true;
                }
            }
            (match?in:out).push_back(ea);
        }
        in.shrink_to_fit();
        out.shrink_to_fit();
        return {in,out};
    }
    
    template<std::ranges::input_range R,std::invocable<typename R::value_type> Fn_T>
    std::vector<typename R::value_type> filter_if(const R &r,const Fn_T &f) requires std::convertible_to<typename std::invoke_result<Fn_T,typename R::value_type>::type,bool> {
        std::vector<typename R::value_type> v;
        v.reserve(std::size(r));
        for(const auto &e:r){
            if(std::invoke(f,e)){
                v.push_back(e);
            }
        }
        v.shrink_to_fit();
        return v;
    }
    
    template<std::ranges::input_range R,std::invocable<typename R::value_type> Fn_T>
    size_t count_if(const R &r,const Fn_T &f) requires std::convertible_to<typename std::invoke_result<Fn_T,typename R::value_type>::type,bool> {
        size_t acc=0;
        for(const auto &e:r){
            if(std::invoke(f,e)){
                acc++;
            }
        }
        return acc;
    }
    
    inline std::vector<const char *> get_cstrs(const std::vector<std::string> &v){
        return Util::map(v,&std::string::c_str);
    }
    
    void print_sync(std::string s);
    
    inline void extract_warnings(std::vector<std::string> && warnings,std::vector<std::string> &warnings_out){
        warnings_out.reserve(warnings_out.size()+warnings.size());
        std::move(warnings.begin(),warnings.end(),std::back_inserter(warnings_out));
    }
    
    template<typename T>
    T extract_warnings(std::pair<std::vector<std::string>,T> && warnings_pair,std::vector<std::string> &warnings_out){
        extract_warnings(std::move(warnings_pair.first),warnings_out);
        return std::move(warnings_pair.second);
    }
    
    template<std::ranges::input_range Ra,std::ranges::input_range ... Rb>
    std::vector<typename Ra::value_type> merge(const Ra &ra,const Rb & ... rb) requires (std::convertible_to<typename Ra::value_type,typename Rb::value_type>&&...) {
        std::vector<typename Ra::value_type> o(std::begin(ra),std::end(ra));
        o.reserve(std::size(ra)+(std::size(rb)+...));
        (std::copy(std::begin(rb),std::end(rb),std::back_inserter(o)),...);
        return o;
    }
    
    inline std::string str_tolower(std::string s){
        return inplace_map(s,&tolower);
    }
    
    template<typename T,size_t N>
    struct CArrayIteratorAdaptor {
        using value_type=T;
        using size_type=size_t;
        using difference_type=std::ptrdiff_t;
        
        using reference=T&;
        using pointer=T*;
        using iterator=T*;
        using reverse_iterator=std::reverse_iterator<T*>;
        
        using const_reference=const T&;
        using const_pointer=const T*;
        using const_iterator=const T*;
        using const_reverse_iterator=std::reverse_iterator<const T*>;
        T * const _data;
        
        CArrayIteratorAdaptor(T (&a)[N]) : _data(a) {
        }
        
        static constexpr size_type size() noexcept {
            return N;
        }
        
        static constexpr size_type max_size() noexcept {
            return (std::numeric_limits<size_type>::max()/sizeof(value_type))-1;
        }
        
        static constexpr size_type empty() noexcept {
            return N==0;
        }
        
        constexpr const_iterator begin() const noexcept {
            return _data;
        }
        
        constexpr const_iterator end() const noexcept {
            return _data+N;
        }
        
        constexpr iterator begin() noexcept {
            return _data;
        }
        
        constexpr iterator end() noexcept {
            return _data+N;
        }
        
        constexpr const_iterator cbegin() const noexcept {
            return _data;
        }
        
        constexpr const_iterator cend() const noexcept {
            return _data+N;
        }
        
        constexpr reverse_iterator rbegin() noexcept {
            return std::make_reverse_iterator(end());
        }
        
        constexpr reverse_iterator rend() noexcept {
            return std::make_reverse_iterator(begin());
        }
        
        
        constexpr const_reverse_iterator rbegin() const noexcept {
            return std::make_reverse_iterator(end());
        }
        
        constexpr const_reverse_iterator rend() const noexcept {
            return std::make_reverse_iterator(begin());
        }
        
        constexpr const_reverse_iterator crbegin() const noexcept {
            return std::make_reverse_iterator(end());
        }
        
        constexpr const_reverse_iterator crend() const noexcept {
            return std::make_reverse_iterator(begin());
        }
        
        constexpr const_pointer data() const noexcept {
            return _data;
        }
        
        constexpr pointer data() noexcept {
            return _data;
        }
        
        constexpr reference front() noexcept {
            return _data[0];
        }
        
        constexpr reference back() noexcept {
            return _data[N-1];
        }
        
        constexpr reference operator[](size_t i) noexcept {
            return _data[i];
        }
        
        constexpr const_reference front() const noexcept {
            return _data[0];
        }
        
        constexpr const_reference back() const noexcept {
            return _data[N-1];
        }
        
        constexpr const_reference operator[](size_t i) const noexcept {
            return _data[i];
        }
        
    };
    
    constexpr size_t conststr_len(const char * s){
        size_t i=0;
        for(;s[i];i++);
        return i;
    }
    
    constexpr bool conststr_eq(const char * a,const char * b){
        for(size_t i=0;a[i]&&b[i];i++){
            if(a[i]!=b[i])return false;
        }
        return true;
    }
    
    constexpr int conststr_find_first(const char * str,char c,size_t n){
        for(size_t i=0;i<n&&str[i];i++){
            if(str[i]==c)return i;
        }
        return -1;
    }
    
    constexpr int conststr_find_first(const char * str,char c){
        for(size_t i=0;str[i];i++){
            if(str[i]==c)return i;
        }
        return -1;
    }
    
    constexpr int conststr_to_int(const char * str,size_t n){
        int v=0;
        bool neg=str[0]=='-';
        size_t i=(str[0]=='-'||str[0]=='+')?1:0;
        for(;i<n&&str[i]>='0'&&str[i]<='9';i++){
            v*=10;
            v+=str[i]-'0';
        }
        return neg?-v:v;
    }
    
    constexpr int conststr_to_int(const char * str){
        int v=0;
        bool neg=str[0]=='-';
        size_t i=(str[0]=='-'||str[0]=='+')?1:0;
        for(;str[i]>='0'&&str[i]<='9';i++){
            v*=10;
            v+=str[i]-'0';
        }
        return neg?-v:v;
    }
    
    template<size_t N,typename T>
    constexpr size_t arr_len(T (&)[N]){
        return N;
    }
}

