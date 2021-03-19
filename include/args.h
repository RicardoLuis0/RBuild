#pragma once

#include <map>
#include <vector>
#include <string>
#include <stdexcept>
#include "util.h"

namespace Args{
    
    enum class NamedArgType {
        FLAG,
        VALUE,
    };
    
    struct NamedArg {
        NamedArgType type;
        std::string value;
    };
    
    extern std::map<std::string,NamedArg> named;
    extern std::vector<std::string> unnamed;
    void init(int argc,char ** argv);
    
    inline std::string namedArgOr(std::string arg,std::string defval){
        if(auto it=named.find(arg);it!=named.end()){
            if(it->second.type==NamedArgType::VALUE){
                return it->second.value;
            }else{
                throw std::runtime_error("Argument "+Util::quote_str_single(arg)+" missing value");
            }
        }else{
            return defval;
        }
    }
    
    inline bool has_flag(const std::string &flag){
        if(auto it=named.find(flag);it!=named.end()){
            if(it->second.type==NamedArgType::FLAG){
                return true;
            }else{
                throw std::runtime_error("Expected flag for argument "+Util::quote_str_single(flag)+" but got value");
            }
        }
        return false;
    }
    
}

