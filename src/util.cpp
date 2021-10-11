#include "util.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <cerrno>
#include <cstring>
#include <mutex>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(__unix__)
    #include <unistd.h>
#endif

namespace Util {
    namespace {
        constexpr char escape(char c){
            switch(c) {
            case '\a':
                return 'a';
            case '\b':
                return 'b';
            case '\e':
                return 'e';
            case '\f':
                return 'f';
            case '\n':
                return 'n';
            case '\r':
                return 'r';
            case '\t':
                return 't';
            case '\v':
                return 'v';
            case '\\':
                return '\\';
            case '"':
                return '"';
            default:
                return c;
            }
        }
    }
    
    int numCPUs(){
        #ifdef _WIN32
            SYSTEM_INFO si={};
            GetSystemInfo(&si);
            return si.dwNumberOfProcessors;
        #elif defined(_SC_NPROCESSORS_ONLN)
            return sysconf(_SC_NPROCESSORS_ONLN);
        #else
            return 4;//fallback
        #endif // _WIN32
    }
    
    std::string quote_str(const std::string &s,char quote_char){
        std::string str;
        str+=quote_char;
        for(char c:s){
            if(c=='\\'||c==quote_char||escape(c)!=c){
                str+='\\';
                str+=escape(c);
            }else{
                str+=c;
            }
        }
        str+=quote_char;
        return str;
    }
    
    std::string join(const std::vector<std::string> &v,const std::string &on){
        std::string o;
        bool first=true;
        for(auto &s:v){
            if(!first)o+=on;
            o+=s;
            first=false;
        }
        return o;
    }
    
    std::string join_or(const std::vector<std::string> &v,const std::string &sep_comma,const std::string &sep_or){
        std::string o;
        const size_t n=v.size();
        for(size_t i=0;i<n;i++){
            if(i==(n-1)){
                o+=sep_or;
            }else if(i>0){
                o+=sep_comma;
            }
            o+=v[i];
        }
        return o;
    }
    
    std::vector<std::string> split(const std::string &ss,char c,bool split_empty){
        //const size_t n=ss.size();
        std::vector<std::string> o;
        const char * s=ss.c_str();
        const char * cs=NULL;
        while((cs=strchr(s,c))){
            o.emplace_back(s,cs-s);
            s=cs+1;
            if(!split_empty) while(*s==c) ++s;
        }
        if(*s!='\0') o.emplace_back(s);
        return o;
    }
    
    std::vector<std::string> split(const std::string &ss,const std::vector<char> &cv,bool split_empty){
        size_t i=0,s=0;
        std::vector<std::string> o;
        const size_t n=ss.size();
        for(;i<n;i++){
            if(contains(cv,ss[i])){
                if(i==s&&!split_empty){
                    ++s;
                    continue;
                }
                o.emplace_back(ss.substr(s,i-s));
                s=i+1;
            }
        }
        if(i!=s) o.emplace_back(ss.substr(s));
        return o;
    }
    
    std::vector<std::string> split_str(const std::string &ss,const std::string &s,bool split_empty){
        std::vector<std::string> o;
        size_t offset=0;
        const size_t len=s.size();
        while(true){
            size_t start=ss.find(s,offset);
            if(start==std::string::npos)break;
            if(offset==start&&!split_empty){
                offset+=len;
                continue;
            }
            o.emplace_back(ss.substr(offset,start-offset));
            offset=start+len;
        }
        o.emplace_back(ss.substr(offset));
        return o;
    }
    
    static std::pair<size_t,size_t> first_of_vec(const std::string &str,size_t offset,const std::vector<std::string> &vec){
        size_t start=std::string::npos;
        size_t len=std::string::npos;
        for(auto &elem:vec){
            if(auto i=str.find(elem,offset);i!=std::string::npos){
                if(start==std::string::npos||i<start){
                    start=i;
                    len=elem.size();
                }
            }
        }
        return {start,len};
    }
    
    std::vector<std::string> split_str(const std::string &ss,const std::vector<std::string> &sv,bool split_empty){
        std::vector<std::string> o;
        size_t offset=0;
        while(true){
            auto [start,len]=first_of_vec(ss,offset,sv);
            if(start==std::string::npos)break;
            if(offset==start&&!split_empty){
                offset+=len;
                continue;
            }
            o.emplace_back(ss.substr(offset,start-offset));
            offset=start+len;
        }
        o.emplace_back(ss.substr(offset));
        return o;
    }
    
    std::string readfile(const std::string &filename) try {
        std::ostringstream ss;
        std::ifstream f(filename);
        if(!f)throw std::runtime_error(strerror(errno));
        ss<<f.rdbuf();
        return ss.str();
    }catch(std::exception &e){
        throw std::runtime_error("Failed to read "+Util::quote_str_single(filename)+" : "+e.what());
    }
    
    void writefile(const std::string &filename,const std::string &data) try {
        std::ofstream f(filename);
        f<<data;
    }catch(std::exception &e){
        throw std::runtime_error("Failed to write to "+Util::quote_str_single(filename)+" : "+e.what());
    }
    
    std::mutex print_mutex;
    
    void print_sync(std::string s){
        std::lock_guard g(print_mutex);
        std::cout<<s;
    }
    
}

