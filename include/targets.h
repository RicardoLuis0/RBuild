#pragma once

#include <vector>
#include <string>
#include <map>

#include "json.h"

class Targets {
    void process_includes(std::vector<std::string> &warnings_out);
public:
    struct target {
        
        struct source_t {
            std::string name;
            std::vector<source_t> include_exclude_list;
            bool exclude_all;
        };
        
        target(const JSON::object_t &target,std::string name,std::vector<std::string> &warnings_out);
        
        target& operator +=(const target& other);
        
        std::vector<source_t> sources;
        
        std::vector<std::string> includes;
        
        std::vector<std::string> defines_all;
        std::vector<std::string> defines_asm;
        std::vector<std::string> defines_c_cpp;
        std::vector<std::string> defines_c;
        std::vector<std::string> defines_cpp;
        
        std::vector<std::string> flags_all;
        std::vector<std::string> flags_asm;
        std::vector<std::string> flags_c_cpp;
        std::vector<std::string> flags_c;
        std::vector<std::string> flags_cpp;
        
        std::vector<std::string> linker_flags;
        std::vector<std::string> linker_libs;
        std::vector<std::string> linker_link_order_before;
        std::vector<std::string> linker_link_extra_before;
        std::vector<std::string> linker_link_extra_after;
        std::vector<std::string> linker_link_order_after;
        std::vector<std::string> linker_nolink;
        
        bool nocompile;
        
    };
    
    std::map<std::string,target> targets;
    
    Targets(const JSON::object_t &targets,std::vector<std::string> &warnings_out);
    
};
