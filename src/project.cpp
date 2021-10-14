#include "project.h"
#include "drivers.h"

#include <iostream>
#include <stdexcept>
#include <thread>
#include <queue>

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
    compiler_binary_override_c(JSON::str_opt(project,"compiler_binary_override_c")),
    compiler_binary_override_cpp(JSON::str_opt(project,"compiler_binary_override_cpp")),
    compiler_binary_override_c_cpp(JSON::str_opt(project,"compiler_binary_override_c_cpp")),
    compiler_binary_override_asm(JSON::str_opt(project,"compiler_binary_override_asm")),
    compiler_binary_override_all(JSON::str_opt(project,"compiler_binary_override_all")),
    linker_binary_override_c(JSON::str_opt(project,"linker_binary_override_c")),
    linker_binary_override_cpp(JSON::str_opt(project,"linker_binary_override_cpp")),
    linker_binary_override_c_cpp(JSON::str_opt(project,"linker_binary_override_c_cpp")),
    linker_binary_override_other(JSON::str_opt(project,"linker_binary_override_other")),
    linker_binary_override_all(JSON::str_opt(project,"linker_binary_override_all")),
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
                warnings_out.push_back("Invalid "+
                                       std::string(invalid_targets.size()==1?"Target":"Targets")+
                                       " in 'targets_default': "+
                                       Util::join(Util::map(invalid_targets,&Util::quote_str_single),", "));
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
        "compiler_binary_override_c",
        "compiler_binary_override_cpp",
        "compiler_binary_override_c_cpp",
        "compiler_binary_override_asm",
        "compiler_binary_override_all",
        "linker_binary_override_c",
        "linker_binary_override_cpp",
        "linker_binary_override_c_cpp",
        "linker_binary_override_other",
        "linker_binary_override_all",
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
    using source_type=Targets::target::source_type;
    if(!std::filesystem::exists(root)) return;
    
    if(std::filesystem::is_directory(root)){
        for(const Targets::target::source_t &folder:folders){
            std::filesystem::path folder_root(root/folder.name);
            if(std::filesystem::is_directory(folder_root)){
                if(folder.type==source_type::WHITELIST){
                    gather_sources(out,folder_root,folder.whitelist);
                }else if(folder.type==source_type::FOLDER_WHITELIST_FILE_BLACKLIST){
                    for(const std::filesystem::directory_entry &entry:std::filesystem::directory_iterator(folder_root)){
                        if(std::filesystem::is_regular_file(entry)){
                            if(Util::contains(folder.blacklist,entry.path().filename().string()))continue;
                            out.push_back(entry);
                        }
                    }
                    gather_sources(out,folder_root,folder.whitelist);
                }else{
                    std::vector<std::filesystem::path> folder_folders;
                    for(const std::filesystem::directory_entry &entry:std::filesystem::directory_iterator(folder_root)){
                        if(Util::contains(folder.blacklist,entry.path().filename().string()))continue;
                        if(std::filesystem::is_regular_file(entry)){
                            out.push_back(entry);
                        }else if(std::filesystem::is_directory(entry)){
                            folder_folders.push_back(entry);
                        }
                    }
                    if(!folder_folders.empty())gather_sources(out,root,folder_folders);
                }
            }else if(std::filesystem::is_regular_file(folder_root)){
                out.push_back(folder_root);
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

//resolving link order is O(n*m) where n is number of objects to link and m is number of objects with specified link order
static ssize_t get_link_order(Targets::target &target,const std::filesystem::path &out_base,const std::filesystem::path &out){
    for(const Targets::target::link_order_t &lo_entry:target.linker_order){
        if(lo_entry.type==Targets::target::LINK_NORMAL){
            if(out.filename().string()==lo_entry.name){
                return lo_entry.weight;
            }
        }else if(lo_entry.type==Targets::target::LINK_NORMAL){
            if(std::filesystem::relative(out,out_base).string()==lo_entry.name){
                return lo_entry.weight;
            }
        }
    }
    return 0;
}

namespace {
    struct job_t {
        drivers::compiler::driver * driver;
        std::filesystem::path working_path;
        std::filesystem::path src_base;
        std::filesystem::path src;
        std::filesystem::path src_out;
        std::vector<std::string> extra_args;
        Util::redirect_data output;
        std::atomic<bool> finished;
        std::atomic<bool> success;
        static void run_job(job_t * data) try {
            data->success=data->driver->compile(data->working_path,data->src_base,data->src,data->src_out,data->extra_args,&data->output);
            data->output.stop();
            data->finished=true;
        } catch (std::exception &e) {
            try {
                data->output.stop();
            }catch (std::exception &e2){
                data->output.s_stderr+="\nUnexpected Exception while stopping output thread: "+Util::quote_str_single(e2.what())+"\n";
            }
            data->output.s_stderr+="\nUnexpected Exception while compiling: "+Util::quote_str_single(e.what())+"\n";
            data->success=false;
            data->finished=true;
        }
        static std::vector<std::unique_ptr<job_t>> run_jobs(std::queue<std::unique_ptr<job_t>> &jobs){
            bool ok=true;
            std::unique_ptr<job_t> running_jobs_data[num_jobs];
            std::thread running_jobs_thread[num_jobs];
            
            std::vector<std::unique_ptr<job_t>> finished_jobs;
            
            while(ok&&jobs.size()>0){
                bool vacant_jobs=false;
                int first_vacant=0;
                int vacant_count=0;
                for(int i=0;i<num_jobs;i++){
                    if(running_jobs_data[i]){
                        if(running_jobs_data[i]->finished){
                            running_jobs_thread[i].join();
                            if(!running_jobs_data[i]->success)ok=false;
                            finished_jobs.emplace_back(std::move(running_jobs_data[i]));
                            running_jobs_data[i]=nullptr;
                            if(!vacant_jobs){
                                first_vacant=i;
                                vacant_jobs=true;
                            }
                            vacant_count++;
                        }
                    }else{
                        if(!vacant_jobs){
                            first_vacant=i;
                            vacant_jobs=true;
                        }
                        vacant_count++;
                    }
                }
                if(vacant_jobs&&ok){
                    for(int i=first_vacant;i<num_jobs&&vacant_count>0&&jobs.size()>0;i++){
                        if(!running_jobs_data[i]){
                            running_jobs_data[i]=std::move(jobs.front());
                            jobs.pop();
                            running_jobs_thread[i]=std::thread(job_t::run_job,running_jobs_data[i].get());
                            vacant_count--;
                        }
                    }
                }
                std::this_thread::yield();
            }
            
            for(int i=0;i<num_jobs;i++){
                if(running_jobs_data[i]){
                    running_jobs_thread[i].join();
                    if(!running_jobs_data[i]->success)ok=false;
                    finished_jobs.emplace_back(std::move(running_jobs_data[i]));
                    running_jobs_data[i]=nullptr;
                }
            }
            return finished_jobs;
        }
    };
}

static const char * arch_folder(
#if defined(_WIN32)
    "win"
#elif defined(__linux__)
    "lin"
#elif defined(__unix__)
    "nix"
#else
    #warning unknown architecture
    "unknown"
#endif
);

static std::filesystem::path get_working_path(Targets::target &target,bool noarch,std::filesystem::path working_folder,std::string target_name){
    return (noarch?working_folder:(working_folder/arch_folder))/(target.target_folder_override?*target.target_folder_override:target_name);
}

bool Project::build_target(const std::string & target_name) try{
    using std::filesystem::path;
    
    auto &target=targets.targets.at(target_name);
    
    using namespace drivers;
    
    std::unique_ptr<compiler::driver> c_compiler_driver(drivers::get_compiler(target.compiler_driver_override_c?*target.compiler_driver_override_c
                                                                             :target.compiler_driver_override_c_cpp?*target.compiler_driver_override_c_cpp
                                                                             :target.compiler_driver_override_all?*target.compiler_driver_override_all
                                                                             :compiler_c?*compiler_c
                                                                             :compiler_c_cpp?*compiler_c_cpp
                                                                             :compiler_all?*compiler_all
                                                                             :"gcc"
                                                                             ,drivers::LANG_C
                                                                             ,Util::merge(target.flags_all,target.flags_c_cpp,target.flags_c)
                                                                             ,Util::merge(target.defines_all,target.defines_c_cpp,target.defines_c)
                                                                             ,target.compiler_binary_override_c?target.compiler_binary_override_c
                                                                             :target.compiler_binary_override_c_cpp?target.compiler_binary_override_c_cpp
                                                                             :target.compiler_binary_override_all?target.compiler_binary_override_all
                                                                             :compiler_binary_override_c?compiler_binary_override_c
                                                                             :compiler_binary_override_c_cpp?compiler_binary_override_c_cpp
                                                                             :compiler_binary_override_all?compiler_binary_override_all
                                                                             :std::nullopt
                                                                             ));
    
    std::unique_ptr<compiler::driver> cpp_compiler_driver(drivers::get_compiler(target.compiler_driver_override_cpp?*target.compiler_driver_override_cpp
                                                                               :target.compiler_driver_override_c_cpp?*target.compiler_driver_override_c_cpp
                                                                               :target.compiler_driver_override_all?*target.compiler_driver_override_all
                                                                               :compiler_cpp?*compiler_cpp
                                                                               :compiler_c_cpp?*compiler_c_cpp
                                                                               :compiler_all?*compiler_all
                                                                               :"gcc"
                                                                               ,drivers::LANG_CPP
                                                                               ,Util::merge(target.flags_all,target.flags_c_cpp,target.flags_cpp)
                                                                               ,Util::merge(target.defines_all,target.defines_c_cpp,target.defines_cpp)
                                                                               ,target.compiler_binary_override_cpp?target.compiler_binary_override_cpp
                                                                               :target.compiler_binary_override_c_cpp?target.compiler_binary_override_c_cpp
                                                                               :target.compiler_binary_override_all?target.compiler_binary_override_all
                                                                               :compiler_binary_override_cpp?compiler_binary_override_cpp
                                                                               :compiler_binary_override_c_cpp?compiler_binary_override_c_cpp
                                                                               :compiler_binary_override_all?compiler_binary_override_all
                                                                               :std::nullopt
                                                                               ));
    
    std::unique_ptr<compiler::driver> asm_compiler_driver(drivers::get_compiler(target.compiler_driver_override_asm?*target.compiler_driver_override_asm
                                                                               :target.compiler_driver_override_all?*target.compiler_driver_override_all
                                                                               :compiler_asm?*compiler_asm
                                                                               :compiler_all?*compiler_all
                                                                               :"gcc"
                                                                               ,drivers::LANG_ASM
                                                                               ,Util::merge(target.flags_all,target.flags_asm)
                                                                               ,Util::merge(target.defines_all,target.defines_asm)
                                                                               ,target.compiler_binary_override_asm?target.compiler_binary_override_asm
                                                                               :target.compiler_binary_override_all?target.compiler_binary_override_all
                                                                               :compiler_binary_override_asm?compiler_binary_override_asm
                                                                               :compiler_binary_override_all?compiler_binary_override_all
                                                                               :std::nullopt
                                                                               ));
    
    std::unique_ptr<linker::driver> linker_driver(drivers::get_linker(target.linker_driver_override?*target.linker_driver_override
                                                                     :linker?*linker
                                                                     :"gcc"
                                                                     ,target.linker_flags,target.linker_libs
                                                                     ,target.linker_binary_override_c?target.linker_binary_override_c
                                                                     :target.linker_binary_override_c_cpp?target.linker_binary_override_c_cpp
                                                                     :target.linker_binary_override_all?target.linker_binary_override_all
                                                                     :linker_binary_override_c?linker_binary_override_c
                                                                     :linker_binary_override_c_cpp?linker_binary_override_c_cpp
                                                                     :linker_binary_override_all?linker_binary_override_all
                                                                     :std::nullopt
                                                                     ,target.linker_binary_override_cpp?target.linker_binary_override_cpp
                                                                     :target.linker_binary_override_c_cpp?target.linker_binary_override_c_cpp
                                                                     :target.linker_binary_override_all?target.linker_binary_override_all
                                                                     :linker_binary_override_cpp?linker_binary_override_cpp
                                                                     :linker_binary_override_c_cpp?linker_binary_override_c_cpp
                                                                     :linker_binary_override_all?linker_binary_override_all
                                                                     :std::nullopt
                                                                     ,target.linker_binary_override_other?target.linker_binary_override_other
                                                                     :target.linker_binary_override_all?target.linker_binary_override_all
                                                                     :linker_binary_override_other?linker_binary_override_other
                                                                     :linker_binary_override_all?linker_binary_override_all
                                                                     :std::nullopt
                                                                     ));
    
    path src_base=src_path.empty()?std::filesystem::current_path():path(src_path);
    path working_path=get_working_path(target,noarch,working_folder,target_name);
    path out_base=working_path/"obj";
    std::vector<path> sources_all;
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
        std::vector<path> sources_c(FILTER_EXTENSIONS(c_extensions));
        std::vector<path> sources_cpp(FILTER_EXTENSIONS(cpp_extensions));
        std::vector<path> sources_asm(FILTER_EXTENSIONS(asm_extensions));
    #undef FILTER_EXTENSIONS
    
    for(const Targets::target::link_order_t &lo_entry:target.linker_order){
        if(lo_entry.type==Targets::target::LINK_EXTRA){
            linker_driver->add_file(lo_entry.weight,lo_entry.name);
        }
    }
    
    std::cout<<"\n";
    
    if(num_jobs>0){
        std::queue<std::unique_ptr<job_t>> jobs;
        
        #define COMPILE_JOB(lang)\
            for(const auto & src : PP_JOIN(sources_,lang) ){\
                path src_out (get_obj_path(working_path,src_base,src));\
                bool needs_compile=PP_JOIN(lang,_compiler_driver)->needs_compile(working_path,src_base,src,src_out);\
                if(needs_compile){\
                    jobs.push(std::make_unique<job_t>(\
                                        PP_JOIN(lang,_compiler_driver).get(),\
                                        working_path,\
                                        src_base,\
                                        src,\
                                        src_out,\
                                        std::vector<std::string>{},\
                                        Util::redirect_data{},\
                                        false,\
                                        false\
                                  ));\
                }\
                 \
                /*
                 * files need to be added early to the linking list
                 * to ensure linking order is the same between
                 * job builds and non-job builds
                 *
                 */ \
                    \
                linker_driver->add_file(get_link_order(target,out_base,src_out),src_out);\
            }
        
        COMPILE_JOB(c);
        COMPILE_JOB(cpp);
        COMPILE_JOB(asm);
        
        std::vector<std::unique_ptr<job_t>> finished_jobs(job_t::run_jobs(jobs));
        
        bool ok=true;
        
        std::vector<std::string> failed_files;
        
        for(auto &job:finished_jobs){
            std::string fname("'"+std::filesystem::relative(job->src,job->src_base).string()+"'");
            if(!(job->output.s_stdout.empty()&&job->output.s_stderr.empty())){
                std::cout<<"\n----------\n\n\nWhile compiling "<<fname<<":\n";
                std::cout<<job->output.s_stdout<<"\n";
                std::cerr<<job->output.s_stderr<<"\n";
            }
            if(!job->success){
                failed_files.push_back(fname);
                ok=false;
            }
        }
        
        if(!ok){
            throw std::runtime_error("Failed to compile "+Util::join(failed_files,", "));
        }
        
        #undef COMPILE_JOB
        
    }else{
        
        #define COMPILE_NOJOB(lang)\
            for(const auto & src : PP_JOIN(sources_,lang) ){\
                path src_out (get_obj_path(working_path,src_base,src));\
                if(!PP_JOIN(lang,_compiler_driver)->needs_compile(working_path,src_base,src,src_out)\
                  ||PP_JOIN(lang,_compiler_driver)->compile(working_path,src_base,src,src_out,{},nullptr)){\
                    linker_driver->add_file(get_link_order(target,out_base,src_out),src_out);\
                }else{\
                    throw std::runtime_error("Failed to compile "+Util::quote_str_single(std::filesystem::relative(src).string()));\
                }\
            }
        
        COMPILE_NOJOB(c);
        COMPILE_NOJOB(cpp);
        COMPILE_NOJOB(asm);
        
        #undef COMPILE_NOJOB
        
    }
    
    std::string out=(
                     (target.binary_folder_override?path(*target.binary_folder_override):(binary_folder_override?path(*binary_folder_override):(working_path/"bin")))
                     /
                     (
                      (target.project_binary_override?*target.project_binary_override:project_binary)
                      +
                      (project_ext?*project_ext:linker_driver->get_ext())
                     )
                    ).string();
    
    if(!linker_driver->link(working_path,out,{})){
        throw std::runtime_error("Failed to link");
    }
    return true;
}catch(std::out_of_range &e){
    throw std::runtime_error("Invalid target "+Util::quote_str_single(target_name));
}

void Project::clean_target(const std::string &target_name){
    using std::filesystem::path;
    
    auto &target=targets.targets.at(target_name);
    
    path working_path=get_working_path(target,noarch,working_folder,target_name);
    
    path obj_path(working_path/"obj");
    path tmp_path(working_path/"tmp");
    
    std::cout<<"Deleting "<<std::filesystem::relative(obj_path)<<"\n";
    std::filesystem::remove_all(obj_path);
    std::cout<<"Deleting "<<std::filesystem::relative(tmp_path)<<"\n";
    std::filesystem::remove_all(tmp_path);
}
