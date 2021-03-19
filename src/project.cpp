#include "project.h"
#include "drivers.h"

#include <iostream>
#include <stdexcept>

Project::Project(const JSON::object_t &project,std::vector<std::string> &warnings_out) :
    targets(project.at("targets").get_obj(),warnings_out),
    compiler_all(JSON::str_opt(project,"compiler_all")),
    compiler_c_cpp(JSON::str_opt(project,"compiler_c_cpp")),
    compiler_c(JSON::str_opt(project,"compiler_c")),
    compiler_cpp(JSON::str_opt(project,"compiler_cpp")),
    compiler_asm(JSON::str_opt(project,"compiler_asm")),
    linker(JSON::str_opt(project,"linker")),
    src_path(JSON::str_opt(project,"src_folder","")),
    working_folder(JSON::str_nonopt(project,"working_folder")),
    name(JSON::str_opt(project,"project_name")),
    project_binary(JSON::str_nonopt(project,"project_binary")),
    project_ext(JSON::str_opt(project,"project_ext")),
    binary_folder_override(JSON::str_opt(project,"binary_folder_override")),
    noarch(JSON::bool_opt(project,"noarch",false))
{
    if(auto it=project.find("targets_default");it!=project.end()){
        if(it->second.is_str()){
            auto &s=it->second.get_str();
            if(s=="all"){
                default_targets=Util::keys(targets.targets);
            }else if(targets.targets.contains(s)){
                default_targets={s};
            }else{
                warnings_out.push_back("Invalid Target in 'targets_default': "+Util::quote_str_single(s));
            }
        }else if(it->second.is_arr()){
            default_targets=JSON::mkstrlist(it->second.get_arr());
            std::vector<std::string> invalid_targets;
            std::tie(default_targets,invalid_targets)=Util::filter_inout(default_targets,Util::keys(targets.targets));
            if(invalid_targets.size()>0){
                warnings_out.push_back("Invalid "+std::string(invalid_targets.size()==1?"Target":"Targets")+" in 'targets_default': "+Util::join(Util::map(invalid_targets,&Util::quote_str_single),", "));
            }
        }else{
            throw JSON::JSON_Exception("In 'targets_default': ",std::vector<std::string>{"String","Array"},it->second.type_name());
        }
    }else{
        default_targets=Util::keys(targets.targets);
    }
    static const std::string valid_keys[]{
        "targets",
        "src_folder",
        "working_folder",
        "project_name",
        "project_binary",
        "project_ext",
        "targets_default",
        "compiler_all",
        "compiler_c_cpp",
        "compiler_c",
        "compiler_cpp",
        "compiler_asm",
        "linker",
        "noarch",
        "binary_folder_override",
    };
    for(auto &e:Util::filter_exclude(Util::keys(project),Util::CArrayIteratorAdaptor(valid_keys))){
        warnings_out.push_back("Ignored Unknown Element "+Util::quote_str_single(e));
    }
    
}

static void gather_sources(std::vector<std::filesystem::path> & out,const std::filesystem::path &root,const std::vector<std::filesystem::path> &folders){
    if(!std::filesystem::exists(root)) return;
    
    if(std::filesystem::is_directory(root)){
        for(const std::filesystem::path &folder:folders){
            std::filesystem::path folder_root(root/folder);
            std::vector<std::filesystem::path> folder_folders;
            for(const std::filesystem::directory_entry &entry:std::filesystem::directory_iterator(folder_root)){
                if(std::filesystem::is_regular_file(entry)){
                    out.push_back(entry);
                }else if(std::filesystem::is_directory(entry)){
                    folder_folders.push_back(entry);
                }
            }
            if(!folder_folders.empty())gather_sources(out,root,folder_folders);
        }
    }else if(std::filesystem::is_regular_file(root)){
        out.push_back(root);
    }
}

static void gather_sources(std::vector<std::filesystem::path> & out,const std::filesystem::path &root,const std::vector<Targets::target::source_t> &folders){
    if(!std::filesystem::exists(root)) return;
    
    if(std::filesystem::is_directory(root)){
        for(const Targets::target::source_t &folder:folders){
            std::filesystem::path folder_root(root/folder.name);
            if(folder.exclude_all){
                gather_sources(out,folder_root,folder.include_exclude_list);
            }else{
                std::vector<std::filesystem::path> folder_folders;
                std::vector<std::string> exclude(Util::map(folder.include_exclude_list,[](const auto &s)->std::string{return s.name;}));
                for(const std::filesystem::directory_entry &entry:std::filesystem::directory_iterator(folder_root)){
                    if(Util::contains(exclude,entry.path().filename().string()))continue;
                    if(std::filesystem::is_regular_file(entry)){
                        out.push_back(entry);
                    }else if(std::filesystem::is_directory(entry)){
                        folder_folders.push_back(entry);
                    }
                }
                if(!folder_folders.empty())gather_sources(out,root,folder_folders);
            }
        }
    }else if(std::filesystem::is_regular_file(root)){
        out.push_back(root);
    }
}

static std::filesystem::path get_obj_path(const std::filesystem::path &working_path,const std::filesystem::path &src_base,const std::filesystem::path &src_file){
    if(Util::is_subpath(src_base,src_file)){
        return working_path/"obj"/(std::filesystem::relative(src_file,src_base).string()+".o");
    }else{
        throw std::runtime_error("source files outside src base directory not supported");
    }
}

static drivers::linker::bucket get_linker_bucket(Targets::target &target,std::filesystem::path out){
    using namespace drivers::linker;
    //std::cout<<"filename="<<out.filename()<<"\n";
    if(Util::contains(target.linker_link_order_before,out.filename().string())){
        return bucket::BEFORE;
    }else if(Util::contains(target.linker_link_order_after,out.filename().string())){
        return bucket::AFTER;
    }else{
        return bucket::NORMAL;
    }
}

bool Project::build_target(const std::string & target_name) try{
    
    auto &target=targets.targets.at(target_name);
    
    using namespace drivers;
    
    std::unique_ptr<compiler::driver> c_compiler_driver(drivers::get_compiler(compiler_c?*compiler_c
                                                                             :compiler_c_cpp?*compiler_c_cpp
                                                                             :compiler_all?*compiler_all
                                                                             :"gcc"
                                                                             ,drivers::LANG_C,Util::cat(target.flags_all,target.flags_c_cpp,target.flags_c),Util::cat(target.defines_all,target.defines_c,target.defines_c)));
    
    std::unique_ptr<compiler::driver> cpp_compiler_driver(drivers::get_compiler(compiler_cpp?*compiler_cpp
                                                                               :compiler_c_cpp?*compiler_c_cpp
                                                                               :compiler_all?*compiler_all
                                                                               :"gcc"
                                                                               ,drivers::LANG_CPP,Util::cat(target.flags_all,target.flags_c_cpp,target.flags_cpp),Util::cat(target.defines_all,target.defines_c,target.defines_cpp)));
    
    std::unique_ptr<compiler::driver> asm_compiler_driver(drivers::get_compiler(compiler_asm?*compiler_asm
                                                                               :compiler_all?*compiler_all
                                                                               :"gcc"
                                                                               ,drivers::LANG_ASM,Util::cat(target.flags_all,target.flags_asm),Util::cat(target.defines_all,target.defines_asm)));
    
    std::unique_ptr<linker::driver> linker_driver(drivers::get_linker(linker?*linker
                                                 :"gcc"
                                                 ,target.linker_flags,target.linker_libs,Util::map_construct<std::filesystem::path>(target.linker_link_extra_before),Util::map_construct<std::filesystem::path>(target.linker_link_extra_after)));
    
    
    std::string arch_folder(noarch?"":
    #if defined(_WIN32)
        "win"
    #elif defined(__linux__)
        "lin"
    #else
        #warning unknown architecture
        "unknown"
    #endif // defined
    );
    
    std::filesystem::path src_base=src_path.empty()?std::filesystem::current_path():std::filesystem::path(src_path);
    std::filesystem::path wf_path=std::filesystem::path(working_folder)/arch_folder;
    std::vector<std::filesystem::path> sources_all;
    gather_sources(sources_all,src_base,target.sources);
    
    static const std::string c_extensions[]{
        ".c",
    };
    
    static const std::string cpp_extensions[]{
        ".cpp",
        ".c++",
        ".cxx",
        ".cc",
        #if defined(__linux__)
        ".C",
        #endif // defined
    };
    
    static const std::string asm_extensions[]{
        ".asm",
        ".s",
        ".S",
    };
    
    #define FILTER_EXTENSIONS(exts) Util::filter_if(sources_all,[](const auto &p){return Util::contains(Util::CArrayIteratorAdaptor(exts),p.extension().string());})
        std::vector<std::filesystem::path> sources_c(FILTER_EXTENSIONS(c_extensions));
        std::vector<std::filesystem::path> sources_cpp(FILTER_EXTENSIONS(cpp_extensions));
        std::vector<std::filesystem::path> sources_asm(FILTER_EXTENSIONS(asm_extensions));
    #undef FILTER_EXTENSIONS
    
    std::filesystem::path working_path(wf_path/target_name);
    
    for(const auto &c_src:sources_c){
        std::filesystem::path c_src_out(get_obj_path(working_path,src_base,c_src));
        if(!cpp_compiler_driver->needs_compile(working_path,src_base,c_src,c_src_out)||c_compiler_driver->compile(working_path,src_base,c_src,c_src_out,{})){
            linker_driver->add_file(get_linker_bucket(target,c_src_out),c_src_out);
        }else{
            throw std::runtime_error("Failed to compile "+Util::quote_str_single(std::filesystem::relative(c_src).string()));
        }
    }
    
    for(const auto &cpp_src:sources_cpp){
        std::filesystem::path cpp_src_out(get_obj_path(working_path,src_base,cpp_src));
        if(!cpp_compiler_driver->needs_compile(working_path,src_base,cpp_src,cpp_src_out)||cpp_compiler_driver->compile(working_path,src_base,cpp_src,cpp_src_out,{})){
            linker_driver->add_file(get_linker_bucket(target,cpp_src_out),cpp_src_out);
        }else{
            throw std::runtime_error("Failed to compile "+Util::quote_str_single(std::filesystem::relative(cpp_src).string()));
        }
    }
    
    for(const auto &asm_src:sources_asm){
        std::filesystem::path asm_src_out(get_obj_path(working_path,src_base,asm_src));
        if(!asm_compiler_driver->needs_compile(working_path,src_base,asm_src,asm_src_out)||asm_compiler_driver->compile(working_path,src_base,asm_src,asm_src_out,{})){
            linker_driver->add_file(get_linker_bucket(target,asm_src_out),asm_src_out);
        }else{
            throw std::runtime_error("Failed to compile "+Util::quote_str_single(std::filesystem::relative(asm_src).string()));
        }
    }
    
    std::string out=((binary_folder_override?std::filesystem::path(*binary_folder_override):working_path/"bin")/(project_binary+(project_ext?*project_ext:linker_driver->get_ext()))).string();
    
    if(!linker_driver->link(working_path,out,{})){
        throw std::runtime_error("Failed to link");
    }
    return true;
}catch(std::out_of_range &e){
    throw std::runtime_error("Invalid target "+Util::quote_str_single(target_name));
}
