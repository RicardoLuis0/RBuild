#include "drivers.h"
#include "util.h"
#include "args.h"

#include <iostream>

using std::filesystem::path;

namespace drivers {
    namespace compiler {
        
        driver::~driver(){
            
        }
        
        base::base(const std::string &cmp,const std::vector<std::string> &fs,const std::vector<std::string> &ds):compiler(cmp),flags(fs){
            
        }
        
        void base::calc_defines(){
            if(defines.size()>0&&defines_calc.size()==0)defines_calc=Util::map(defines,[](const std::string&s){return "-D"+s;});
        }
        
        bool base::needs_compile(const path &,const path &,const path &,const path &){
            return true;
        }
        
        bool base::compile(const path &working_path,const path &src_base,const path &file_in,const path &file_out,const std::vector<std::string> &extra_flags){
            calc_defines();
            static bool silent=!Args::has_flag("verbose");
            std::filesystem::create_directories(std::filesystem::path(file_out).remove_filename());
            if(silent)std::cout<<std::filesystem::relative(file_in).string()<<"\n";
            return Util::run(compiler,Util::cat(std::vector<std::string>{file_in.string(),"-o",file_out.string()},flags,defines_calc,extra_flags),nullptr,silent)==0;
        }
        
        bool generic::compile(const path &working_path,const path &src_base,const path &file_in,const path &file_out,const std::vector<std::string> &extra_flags){
            calc_defines();
            static bool silent=!Args::has_flag("verbose");
            std::filesystem::create_directories(std::filesystem::path(file_out).remove_filename());
            if(silent)std::cout<<std::filesystem::relative(file_in).string()<<"\n";
            return Util::run(compiler,Util::cat(std::vector<std::string>{"-c",file_in.string(),"-o",file_out.string()},flags,defines_calc,extra_flags),&Util::alternate_cmdline_args_to_file_regular,silent)==0;
        }
        
        path gnu::get_dpath(const path &working_path,const path &src_base,const path &src_file){
            if(Util::is_subpath(src_base,src_file)){
                return working_path/"tmp"/(std::filesystem::relative(src_file,src_base).string()+".d");
            }else{
                throw std::runtime_error("source files outside src base directory not supported");
            }
        }
        
        path gnu::get_out(const path &working_path,const path &src_base,const path &src_file){
            if(Util::is_subpath(src_base,src_file)){
                return (working_path/"obj"/std::filesystem::relative(src_file,src_base)).string()+".o";
            }else{
                throw std::runtime_error("source files outside src base directory not supported");
            }
        }
        
        bool gnu::needs_compile(const path &working_path,const path &src_base,const path &file_in,const path &file_out) try {
            static bool rebuild=Args::has_flag("rebuild");
            path dfile=get_dpath(working_path,src_base,file_in);
            if(!std::filesystem::exists(dfile)||rebuild)return true;
            auto ctime=std::filesystem::last_write_time(file_out);
            
            if(std::filesystem::last_write_time(file_in)>ctime)return true;
            
            std::vector<path> files(({
                std::string data=Util::readfile(dfile.string());
                size_t s=data.find(':');
                if(s==std::string::npos){
                    return true;//MALFORMED
                }
                Util::map(Util::filter_if(Util::split(data.substr(s+1),{'\n','\r',' '}),[](const std::string &ss){return ss!="\\";}),[](const std::string &ss){return path(ss);});
            }));
            
            for(auto &p:files){
                if(std::filesystem::last_write_time(p)>ctime){
                    return true;
                }
            }
            return false;
        }catch(std::exception &e){
            return true;
        }
        
        bool gnu::compile(const path &working_path,const path &src_base,const path &file_in,const path &file_out,const std::vector<std::string> &extra_args){
            const path dpath=get_dpath(working_path,src_base,file_in);
            std::filesystem::create_directories(std::filesystem::path(dpath).remove_filename());
            return generic::compile(working_path,src_base,file_in,file_out,Util::cat(std::vector<std::string>{"-MD","-MF",dpath.string()},extra_args));
        }
        
        void gas::calc_defines(){
            if(defines.size()>0&&defines_calc.size()==0)defines_calc=Util::insert_interleaved_before(Util::map(defines,[](const std::string &s){return s.find('=')==std::string::npos?s+"=1":s;}),"--defsym");
        }
        
        bool gas::compile(const path &working_path,const path &src_base,const path &file_in,const path &file_out,const std::vector<std::string> &extra_flags){
            calc_defines();
            static bool silent=!Args::has_flag("verbose");
            std::filesystem::create_directories(std::filesystem::path(file_out).remove_filename());
            if(silent)std::cout<<std::filesystem::relative(file_in).string()<<"\n";
            return Util::run(compiler,Util::cat(std::vector<std::string>{file_in.string(),"-o",file_out.string()},flags,defines_calc,extra_flags),&Util::alternate_cmdline_args_to_file_regular,silent)==0;
        }
        
        bool nasm::compile(const path &working_path,const path &src_base,const path &file_in,const path &file_out,const std::vector<std::string> &extra_flags){
            calc_defines();
            static bool silent=!Args::has_flag("verbose");
            const path dpath=get_dpath(working_path,src_base,file_in);
            std::filesystem::create_directories(std::filesystem::path(dpath).remove_filename());
            std::filesystem::create_directories(std::filesystem::path(file_out).remove_filename());
            if(silent)std::cout<<std::filesystem::relative(file_in).string()<<"\n";
            return Util::run(compiler,Util::cat(std::vector<std::string>{file_in.string(),"-o",file_out.string()},flags,defines_calc,std::vector<std::string>{"-MD","-MF",dpath.string()},extra_flags),&Util::alternate_cmdline_args_to_file_nasm,silent)==0;
        }
        
    }
    
    namespace linker {
        
        driver::~driver(){
            
        }
        
        base::base(const std::string &lnk,const std::vector<std::string> &fs,const std::vector<std::string> &ls):linker(lnk),flags(fs),libs(ls){
            
        }
        
        void base::add_file(ssize_t link_order,const path &file){
            link_files[link_order].push_back(file);
        }
        
        void base::clear() {
            link_files.clear();
        }
        std::vector<std::string> base::join_link_files(){
            std::vector<std::string> out;
            for(auto &vp:link_files){
                out.reserve(out.size()+vp.second.size());
                for(auto &e:vp.second){
                    out.push_back(e.string());
                }
            }
            return out;
        }
        
        bool base::link(const path &working_path,const path &file_out,const std::vector<std::string> &extra_flags){
            static bool silent=!Args::has_flag("verbose");
            std::filesystem::create_directories(std::filesystem::path(file_out).remove_filename());
            if(silent)std::cout<<"linking\n";
            return Util::run(linker,Util::cat(std::vector<std::string>{"-o",file_out.string()},libs,flags,extra_flags,join_link_files()),nullptr,silent)==0;
        }
        
        std::string base::get_ext(){
            #if defined(_WIN32)
                return ".exe";
            #elif defined(__linux__)
                return "";
            #else
                #warning possibly platform
                return "";
            #endif // _WIN32
        }
        
        bool generic::link(const path &working_path,const path &file_out,const std::vector<std::string> &extra_flags){
            static bool silent=!Args::has_flag("verbose");
            std::filesystem::create_directories(std::filesystem::path(file_out).remove_filename());
            if(silent)std::cout<<"linking\n";
            return Util::run(linker,Util::cat(std::vector<std::string>{"-o",file_out.string()},libs,flags,extra_flags,join_link_files()),&Util::alternate_cmdline_args_to_file_regular,silent)==0;
        }
        
        gnu::gnu(const std::string &lnk,const std::string &lnk_cpp,const std::vector<std::string> &fs,const std::vector<std::string> &ls) : generic(lnk,fs,ls),linker_cpp(lnk_cpp) {
            
        }
        
        void gnu::add_file(ssize_t link_order,const path &f){
            if(!cpp&&f.filename().string().ends_with(".cpp.o")){
                cpp=true;
                linker=linker_cpp;
            }
            generic::add_file(link_order,f);
        }
        
        std::string ar::get_ext(){
            return ".o";
        }
        
        bool ar::link(const path &working_path,const path &file_out,const std::vector<std::string> &extra_flags) {
            static bool silent=!Args::has_flag("verbose");
            std::filesystem::create_directories(std::filesystem::path(file_out).remove_filename());
            if(silent)std::cout<<"linking\n";
            return Util::run(linker,Util::cat(flags,std::vector<std::string>{file_out.string()},libs,extra_flags,join_link_files()),&Util::alternate_cmdline_args_to_file_regular,silent)==0;
        }
        
    }
    
    std::unique_ptr<compiler::driver> get_compiler(const std::string &name,compiler_lang lang,const std::vector<std::string> &flags,const std::vector<std::string> &defines){
        switch(lang){
        case LANG_C:
        case LANG_CPP:
            if(name=="gcc"){
                return std::make_unique<compiler::gnu>((lang==LANG_C?"gcc":"g++"),flags,defines);
            }else if(name=="clang"){
                return std::make_unique<compiler::gnu>((lang==LANG_C?"clang":"clang++"),flags,defines);
            }else{
                throw std::runtime_error("unknown compiler "+Util::quote_str_single(name));
            }
        case LANG_ASM:
            if(name=="gcc"||name=="clang"){
                return std::make_unique<compiler::gnu>(name,flags,defines);
            }else if(name=="as"){
                return std::make_unique<compiler::gas>(name,flags,defines);
            }else if(name=="nasm"){
                return std::make_unique<compiler::nasm>(name,flags,defines);
            }else{
                throw std::runtime_error("unknown compiler "+Util::quote_str_single(name));
            }
        default:
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wterminate"
                []()noexcept{throw std::runtime_error("invalid internal state: enum compiler_lang has illegal value");}();
            #pragma GCC diagnostic pop
        }
        __builtin_unreachable();
    }
    
    std::unique_ptr<linker::driver> get_linker(const std::string &name,const std::vector<std::string> &flags,const std::vector<std::string> &libs){
        if(name=="gcc"){
            return std::make_unique<linker::gnu>("gcc","g++",flags,libs);
        }else if(name=="clang"){
            return std::make_unique<linker::gnu>("clang","clang++",flags,libs);
        }else if(Util::contains(std::vector<std::string>{"ld","ld.gold","ld.lld"},name)){
            return std::make_unique<linker::generic>(name,flags,libs);
        }else if(Util::contains(std::vector<std::string>{"ar","llvm-ar"},name)){
            return std::make_unique<linker::ar>(name,flags,libs);
        }else{
            throw std::runtime_error("unknown linker "+Util::quote_str_single(name));
        }
    }
    
}
