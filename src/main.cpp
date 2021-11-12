#include <iostream>
#include "json.h"
#include "targets.h"
#include "project.h"
#include "util.h"
#include "drivers.h"

#include "args.h"

#include <cstdlib>

#define RBUILD_VERSION "0.0.0g"

static bool show_warnings(std::vector<std::string> &warnings){
    bool show_prompt=true;
    try {
        show_prompt=!Args::has_flag("ignore_warnings");
    } catch (std::exception &e){
        warnings.push_back(e.what());
    }
    if(warnings.size()>0){
        for(auto &warning:warnings){
            std::cout<<"Warning: "<<warning<<"\n";
        }
        if(show_prompt){
            while(true){
                std::cout<<"Continue(Yes/No) ? ";
                std::string in;
                std::getline(std::cin,in);
                if(in=="yes"||in=="Yes"||in=="y"||in=="Y"){
                    std::cout<<"Continuing...\n";
                    return true;
                }else if(in=="no"||in=="No"||in=="n"||in=="N"){
                    std::cout<<"Cancelling...\n";
                    return false;
                }
            }
        }else{
            std::cout<<"Warning: Warning Prompt Skipped ('ignore_warnings' flag)\n";
        }
    }
    return true;
}

int num_jobs=0;

const char * valid_args[] {
    "rebuild",
    "file",
    "verbose",
    "gcc_override",
    "gxx_override",
    "clang_override",
    "clangxx_override",
    "failexit",
    "num_jobs",
    "ignore_warnings",
    "filetime_nocache",
    "incremental_build_exclude_system",
    "MMD",
    "static",
    "clean",
};

int main(int argc,char ** argv) try {
    Args::init(argc,argv);
    
    if(Args::has_flag("version")){
        std::cout<<"RBuild Version " RBUILD_VERSION "\n";
        return 0;
    }
    
    if(Args::has_flag("help")){
        std::cout<<"See https://github.com/RicardoLuis0/RBuild/blob/main/README.md\n";
        return 0;
    }
    
    if(Args::unnamed.size()>0&&std::filesystem::exists(Args::unnamed[0])&&std::filesystem::is_regular_file(Args::unnamed[0])){
        if(!Args::named.contains("file")){
            Args::named.insert({"file",{Args::NamedArgType::VALUE,Args::unnamed[0]}});
        }
        Args::unnamed.erase(Args::unnamed.begin());
    }
    
    if(Args::has_flag("filetime_nocache")){
        drivers::compiler::filetime_nocache=true;
    }
    
    if(Args::has_flag("static")){
        drivers::linker::force_static_link=true;
    }
    
    if(Args::has_flag("incremental_build_exclude_system")||Args::has_flag("MMD")){
        drivers::compiler::include_check="-MMD";
    }else{
        drivers::compiler::include_check="-MD";
    }
    
    std::vector<std::string> warnings;
    
    try{
        std::optional<int> njopt=Args::namedIntArgOrMatchStr("num_jobs",0,"auto",false);
        num_jobs=njopt?*njopt:Util::numCPUs();
    }catch(std::exception &e){
        warnings.push_back(std::string(e.what())+", Argument Ignored");
    }
    
    std::string project_file=Args::namedArgOr("file",std::filesystem::current_path().filename().string()+".json");
    
    auto project_json=JSON::parse(Util::readfile(project_file));
    Project project(project_json.get_obj(),warnings);
    
    {
        std::vector<std::string> invalid_args=Util::filter_exclude(Util::keys(Args::named),Util::CArrayIteratorAdaptor(valid_args));
        if(invalid_args.size()>0){
            warnings.push_back("Invalid commandline "+((invalid_args.size()==1?"parameter ":"parameters: ")+Util::join(Util::map(invalid_args,&Util::quote_str_single),", ")));
        }
    }
    
    std::vector<std::string> possible_targets=Util::merge(Util::keys(project.targets.targets),Util::keys(project.targets.target_groups));
    
    bool target_group_all_exists=!project.targets.target_groups.insert_or_assign("all",Util::keys(project.targets.targets)).second;
    
    if(target_group_all_exists||project.targets.targets.contains("all")){
        warnings.push_back(std::string(target_group_all_exists?"Target group":"Target")+" 'all' was defined by project, and overwritten (project must not define 'all' target)");
        if(!target_group_all_exists){
            project.targets.targets.erase("all");
        }
    }else{
        possible_targets.push_back("all");
    }
    
    std::vector<std::string> valid_targets;
    
    if(Args::unnamed.size()==1&&Args::unnamed[0]=="list"){
        if(!show_warnings(warnings)){
            return EXIT_FAILURE;
        }
        std::cout<<"targets: "<<Util::join(Util::map(possible_targets,&Util::quote_str_single),", ")<<"\n";
        return EXIT_SUCCESS;
    }else if(Args::unnamed.size()==0){
        if(!show_warnings(warnings)){
            return EXIT_FAILURE;
        }
        valid_targets=project.default_targets;
    }else{
        std::vector<std::string> invalid_targets;
        std::tie(valid_targets,invalid_targets)=Util::filter_inout(Args::unnamed,possible_targets);
        
        if(invalid_targets.size()>0){
            warnings.push_back("Asking for Invalid "+std::string(invalid_targets.size()==1?"Target ":"Targets: ")+Util::join(Util::map(invalid_targets,&Util::quote_str_single),", "));
        }
        if(!show_warnings(warnings)){
            return EXIT_FAILURE;
        }
    }
    
    const bool do_clean=Args::has_flag("clean");
    const bool failexit=Args::has_flag("failexit");
    bool ok=true;
    if(valid_targets.size()==0){
        std::cout<<"----------------\nNo Targets\n----------------\n";
    }else if(do_clean){
        ok=project.clean_targets(valid_targets,failexit);
    }else{
        ok=project.build_targets(valid_targets,failexit);
    }
    return ok?EXIT_SUCCESS:EXIT_FAILURE;
} catch(std::exception &e) {
    std::cerr<<"Unexpected Exception: "<<e.what()<<"\n";
    return EXIT_FAILURE;
}

