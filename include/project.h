#pragma once

#include "json.h"
#include "targets.h"


class Project {
public:
    Targets targets;
    std::vector<std::string> default_targets;
    
    std::optional<std::string> compiler_all;
    std::optional<std::string> compiler_c_cpp;
    std::optional<std::string> compiler_c;
    std::optional<std::string> compiler_cpp;
    std::optional<std::string> compiler_asm;
    std::optional<std::string> linker;
    
    std::string src_path;
    std::string working_folder;
    std::optional<std::string> name;
    std::string project_binary;
    std::optional<std::string> project_ext;
    std::optional<std::string> binary_folder_override;
    
    bool noarch;
    
    Project(const JSON::object_t &project,std::vector<std::string> &warnings_out);
    bool build_target(const std::string &);
protected:

private:
};
