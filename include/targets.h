#pragma once

#include <vector>
#include <string>
#include <map>

#include "json.h"

class Targets {
    void process_includes(std::vector<std::string> &warnings_out);
public:
    struct target {
        
        enum link_order_type_t {
            LINK_NORMAL,
            LINK_FULL_PATH,
            LINK_EXTRA,
        };
        
        struct source_t {
            source_t()=default;
            inline source_t(const std::string &s):name(s){
            }
            inline source_t(const std::string &s,const std::vector<source_t> &sv,bool b):name(s),include_exclude_list(sv),exclude_all(b){
            }
            inline source_t(const std::string &s,std::vector<source_t> && sv,bool b):name(s),include_exclude_list(std::move(sv)),exclude_all(b){
            }
            std::string name;
            std::vector<source_t> include_exclude_list;
            bool exclude_all=false;
        };
        
        struct link_order_t {
            std::string name;
            ssize_t weight;
            link_order_type_t type;
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
        
        std::vector<link_order_t> linker_order;
        
        std::optional<std::string> binary_folder_override;
        std::optional<std::string> project_binary_override;
        
        bool include_only;
        
    };
    
    std::map<std::string,target> targets;
    
    Targets(const JSON::object_t &targets,std::vector<std::string> &warnings_out);
    
};
