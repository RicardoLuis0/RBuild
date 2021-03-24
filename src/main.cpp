#include <iostream>
#include "json.h"
#include "targets.h"
#include "project.h"
#include "util.h"
#include "drivers.h"

#include "args.h"

#include <cstdlib>

static bool show_warnings(const std::vector<std::string> &warnings){
    if(warnings.size()>0){
        for(auto &warning:warnings){
            std::cout<<"Warning: "<<warning<<"\n";
        }
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
    }
    return true;
}

int main(int argc,char ** argv) try {
    Args::init(argc,argv);
    
    std::vector<std::string> warnings;
    std::string project_file=Args::namedArgOr("file",std::filesystem::current_path().filename().string()+".json");
    
    auto project_json=JSON::parse(Util::readfile(project_file));
    Project project(project_json.get_obj(),warnings);
    
    {
        std::vector<std::string> invalid_args=Util::filter_exclude(Util::keys(Args::named),std::vector<std::string>{"rebuild","file","verbose"});
        if(invalid_args.size()>0){
            warnings.push_back("Invalid commandline "+((invalid_args.size()==1?"parameter ":"parameters: ")+Util::join(Util::map(invalid_args,&Util::quote_str_single),", ")));
        }
    }
    
    std::vector<std::string> possible_targets=Util::keys(project.targets.targets);
    
    std::vector<std::string> valid_targets;
    
    if(Args::unnamed.size()==1&&Args::unnamed[0]=="list"){
        if(!show_warnings(warnings)){
            return EXIT_FAILURE;
        }
        std::cout<<"targets: "<<Util::join(Util::map(possible_targets,&Util::quote_str_single),", ")<<"\n";
        return EXIT_SUCCESS;
    }else if(Args::unnamed.size()==1&&Args::unnamed[0]=="all"){
        if(!show_warnings(warnings)){
            return EXIT_FAILURE;
        }
        valid_targets=possible_targets;
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
    bool fail=false;
    if(valid_targets.size()==0){
        std::cout<<"----------------\nNo Targets\n----------------\n";
    }else{
        for(auto &target : valid_targets){
            std::cout<<"----------------\nBuilding target "<<Util::quote_str_single(target)<<(project.name?(" in "+Util::quote_str_single(*project.name)):"")<<"\n----------------\n";
            try{
                if(project.build_target(target)){
                    std::cout<<"\n\nBuilt target "<<Util::quote_str_single(target)<<" successfully!\n\n\n";
                }else{
                    std::cout<<"\n\nBuilding target "<<Util::quote_str_single(target)<<" failed!\n\n\n";
                    fail=true;
                }
            }catch(std::exception &e){
                std::cout<<"\n\nBuilding target "<<Util::quote_str_single(target)<<" failed: "<<e.what()<<"!\n\n\n";
            }
        }
    }
    return fail?EXIT_SUCCESS:EXIT_FAILURE;
} catch(std::exception &e) {
    std::cerr<<"Unexpected Exception: "<<e.what()<<"\n";
    return EXIT_FAILURE;
}

