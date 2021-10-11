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
    
    inline std::string namedArg(std::string arg){
        if(auto it=named.find(arg);it!=named.end()){
            if(it->second.type==NamedArgType::VALUE){
                return it->second.value;
            }else{
                throw std::runtime_error("Argument "+Util::quote_str_single(arg)+" missing value");
            }
        }else{
            throw std::runtime_error("Argument "+Util::quote_str_single(arg)+" missing");
        }
    }
    
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
    
    inline int namedIntArgOr(std::string arg,int defval,bool allow_negative=true){
        if(auto it=named.find(arg);it!=named.end()){
            if(it->second.type==NamedArgType::VALUE){
                std::string val=it->second.value;
                if(Util::count_if(val,[](char c){return (c<'0'||c>'9')&&c!='-';})>0){
                    throw std::runtime_error("Argument "+Util::quote_str_single(arg)+" must be an integer");
                }else{
                    int ival;
                    try {
                        ival=std::stoi(val);
                    } catch(std::exception &e){
                        throw std::runtime_error("Unkonwn error while converting Argument "+Util::quote_str_single(arg)+" to an integer ( "+e.what()+" )");
                    }
                    if(!allow_negative&&ival<0){
                        throw std::runtime_error("Argument "+Util::quote_str_single(arg)+" must not be negative");
                    }
                    return ival;
                }
            }else{
                throw std::runtime_error("Argument "+Util::quote_str_single(arg)+" missing value");
            }
        }else{
            return defval;
        }
    }
    
    inline std::optional<int> namedIntArgOrMatchStr(std::string arg,int defval,std::string match,bool allow_negative=true){//null opt is returned when match is ok, else number or throw
        if(auto it=named.find(arg);it!=named.end()){
            if(it->second.type==NamedArgType::VALUE){
                std::string val=it->second.value;
                if(val==match){
                    return std::nullopt;
                }
                if(Util::count_if(val,[](char c){return (c<'0'||c>'9')&&c!='-';})>0){
                    throw std::runtime_error("Argument "+Util::quote_str_single(arg)+" must be an integer");
                }else{
                    int ival;
                    try {
                        ival=std::stoi(val);
                    } catch(std::exception &e){
                        throw std::runtime_error("Unkonwn error while converting Argument "+Util::quote_str_single(arg)+" to an integer ( "+e.what()+" )");
                    }
                    if(!allow_negative&&ival<0){
                        throw std::runtime_error("Argument "+Util::quote_str_single(arg)+" must not be negative");
                    }
                    return ival;
                }
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

