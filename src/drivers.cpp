#include "drivers.h"
#include "util.h"
#include "args.h"

#include <iostream>

using std::filesystem::path;

namespace drivers {
    namespace compiler {
        
        std::string include_check;
        bool filetime_nocache;
        
        driver::~driver(){
            
        }
        
        base::base(const std::string &cmp,const std::vector<std::string> &fs,const std::vector<std::string> &ds):compiler(cmp),flags(fs),defines(ds){
            
        }
        
        void base::calc_defines(){
            if(defines.size()>0&&defines_calc.size()==0)defines_calc=Util::map(defines,[](const std::string&s){return "-D"+s;});
        }
        
        bool base::needs_compile(const path &,const path &,const path &,const path &){
            return true;
        }
        
        bool base::compile(const path &working_path,const path &src_base,const path &file_in,const path &file_out,const std::vector<std::string> &extra_flags,Util::redirect_data * rd){
            calc_defines();
            static bool silent=!Args::has_flag("verbose");
            std::filesystem::create_directories(std::filesystem::path(file_out).remove_filename());
            if(silent) Util::print_sync(std::filesystem::relative(file_in).string()+"\n");
            return Util::run(compiler,Util::merge(std::vector<std::string>{file_in.string(),"-o",file_out.string()},flags,defines_calc,extra_flags),nullptr,silent,rd)==0;
        }
        
        bool generic::compile(const path &working_path,const path &src_base,const path &file_in,const path &file_out,const std::vector<std::string> &extra_flags,Util::redirect_data * rd){
            calc_defines();
            static bool silent=!Args::has_flag("verbose");
            std::filesystem::create_directories(std::filesystem::path(file_out).remove_filename());
            if(silent) Util::print_sync(std::filesystem::relative(file_in).string()+"\n");
            return Util::run(compiler,Util::merge(std::vector<std::string>{"-c",file_in.string(),"-o",file_out.string()},flags,defines_calc,extra_flags),&Util::alternate_cmdline_args_to_file_regular,silent,rd)==0;
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
        
        static std::map<path,std::filesystem::file_time_type> filetime_cache;
        
        static std::filesystem::file_time_type get_cached_file_write_time(const path& file){
            if(filetime_nocache){
                return std::filesystem::last_write_time(file);
            }else if(auto it=filetime_cache.find(file);it!=filetime_cache.end()){
                return it->second;
            }else{
                auto time=std::filesystem::last_write_time(file);
                filetime_cache.insert({file,time});
                return time;
            }
        }
        
        bool gnu::needs_compile(const path &working_path,const path &src_base,const path &file_in,const path &file_out) try {
            static bool rebuild=Args::has_flag("rebuild");
            path dfile=get_dpath(working_path,src_base,file_in);
            if(!std::filesystem::exists(dfile)||rebuild)return true;
            auto ctime=get_cached_file_write_time(file_out);
            
            if(get_cached_file_write_time(file_in)>ctime)return true;
            
            std::vector<path> files(({
                std::string data=Util::readfile(dfile.string());
                size_t s=data.find(':');
                if(s==std::string::npos){
                    return true;//MALFORMED
                }
                Util::map(Util::filter_if(Util::split(data.substr(s+1),{'\n','\r',' '}),[](const std::string &ss){return ss!="\\";}),[](const std::string &ss){return path(ss);});
            }));
            
            for(auto &p:files){
                if(get_cached_file_write_time(p)>ctime){
                    return true;
                }
            }
            return false;
        }catch(std::exception &e){
            return true;
        }
        
        bool gnu::compile(const path &working_path,const path &src_base,const path &file_in,const path &file_out,const std::vector<std::string> &extra_args,Util::redirect_data * rd){
            const path dpath=get_dpath(working_path,src_base,file_in);
            std::filesystem::create_directories(std::filesystem::path(dpath).remove_filename());
            return generic::compile(working_path,src_base,file_in,file_out,Util::merge(std::vector<std::string>{include_check,"-MF"+dpath.string()},extra_args),rd);
        }
        
        void gas::calc_defines(){
            if(defines.size()>0&&defines_calc.size()==0)defines_calc=Util::insert_interleaved_before(Util::map(defines,[](const std::string &s){return s.find('=')==std::string::npos?s+"=1":s;}),"--defsym");
        }
        
        bool gas::compile(const path &working_path,const path &src_base,const path &file_in,const path &file_out,const std::vector<std::string> &extra_flags,Util::redirect_data * rd){
            calc_defines();
            static bool silent=!Args::has_flag("verbose");
            std::filesystem::create_directories(std::filesystem::path(file_out).remove_filename());
            if(silent) Util::print_sync(std::filesystem::relative(file_in).string()+"\n");
            return Util::run(compiler,Util::merge(std::vector<std::string>{file_in.string(),"-o",file_out.string()},flags,defines_calc,extra_flags),&Util::alternate_cmdline_args_to_file_regular,silent,rd)==0;
        }
        
        bool nasm::compile(const path &working_path,const path &src_base,const path &file_in,const path &file_out,const std::vector<std::string> &extra_flags,Util::redirect_data * rd){
            calc_defines();
            static bool silent=!Args::has_flag("verbose");
            const path dpath=get_dpath(working_path,src_base,file_in);
            std::filesystem::create_directories(std::filesystem::path(dpath).remove_filename());
            std::filesystem::create_directories(std::filesystem::path(file_out).remove_filename());
            if(silent) Util::print_sync(std::filesystem::relative(file_in).string()+"\n");
            return Util::run(compiler,Util::merge(std::vector<std::string>{file_in.string(),"-o",file_out.string()},flags,defines_calc,std::vector<std::string>{include_check,"-MF",dpath.string()},extra_flags),&Util::alternate_cmdline_args_to_file_nasm,silent,rd)==0;
        }
        
    }
    
    namespace linker {
        
        bool force_static_link;
        
        driver::~driver(){
            
        }
        
        base::base(const std::string &lnk,const std::vector<std::string> &fs,const std::vector<std::string> &ls):linker(lnk),flags(fs),libs(ls){
            if(force_static_link){
                flags.push_back("-static");
            }
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
            return Util::run(linker,Util::merge(std::vector<std::string>{"-o",file_out.string()},libs,flags,extra_flags,join_link_files()),nullptr,silent)==0;
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
            return Util::run(linker,Util::merge(std::vector<std::string>{"-o",file_out.string()},flags,extra_flags,join_link_files(),libs),&Util::alternate_cmdline_args_to_file_regular,silent)==0;
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
            return Util::run(linker,Util::merge(flags,std::vector<std::string>{file_out.string()},libs,extra_flags,join_link_files()),&Util::alternate_cmdline_args_to_file_regular,silent)==0;
        }
        
    }
    
    std::unique_ptr<compiler::driver> get_compiler(const std::string &name,compiler_lang lang,const std::vector<std::string> &flags,const std::vector<std::string> &defines,const std::optional<std::string> &compiler_binary_override){
        switch(lang){
        case LANG_C:
        case LANG_CPP:
            if(name=="gcc"){
                return std::make_unique<compiler::gnu>((lang==LANG_C?Args::namedArgOr("gcc_override",compiler_binary_override?*compiler_binary_override:"gcc"):Args::namedArgOr("gxx_override",compiler_binary_override?*compiler_binary_override:"g++")),flags,defines);
            }else if(name=="clang"){
                return std::make_unique<compiler::gnu>((lang==LANG_C?Args::namedArgOr("clang_override",compiler_binary_override?*compiler_binary_override:"clang"):Args::namedArgOr("clangxx_override",compiler_binary_override?*compiler_binary_override:"clang++")),flags,defines);
            }else if(name=="generic"&&compiler_binary_override){
                return std::make_unique<compiler::generic>(*compiler_binary_override,flags,defines);
            }else{
                throw std::runtime_error("unknown compiler "+Util::quote_str_single(name));
            }
        case LANG_ASM:
            if(name=="gcc"){
                return std::make_unique<compiler::gnu>(Args::namedArgOr("gcc_override",compiler_binary_override?*compiler_binary_override:"gcc"),flags,defines);
            }else if(name=="clang"){
                return std::make_unique<compiler::gnu>(Args::namedArgOr("clang_override",compiler_binary_override?*compiler_binary_override:"clang"),flags,defines);
            }else if(name=="generic"&&compiler_binary_override){
                return std::make_unique<compiler::generic>(*compiler_binary_override,flags,defines);
            }else if(name=="as"){
                return std::make_unique<compiler::gas>(compiler_binary_override?*compiler_binary_override:name,flags,defines);
            }else if(name=="nasm"){
                return std::make_unique<compiler::nasm>(compiler_binary_override?*compiler_binary_override:name,flags,defines);
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
    
    std::unique_ptr<linker::driver> get_linker(const std::string &name,const std::vector<std::string> &flags,const std::vector<std::string> &libs,const std::optional<std::string> &linker_binary_override_c,const std::optional<std::string> &linker_binary_override_cpp,const std::optional<std::string> &linker_binary_override_other){
        if(name=="gcc"){
            return std::make_unique<linker::gnu>(Args::namedArgOr("gcc_override",linker_binary_override_c?*linker_binary_override_c:"gcc"),Args::namedArgOr("gxx_override",linker_binary_override_cpp?*linker_binary_override_cpp:"g++"),flags,libs);
        }else if(name=="clang"){
            return std::make_unique<linker::gnu>(Args::namedArgOr("clang_override",linker_binary_override_c?*linker_binary_override_c:"clang"),Args::namedArgOr("clangxx_override",linker_binary_override_cpp?*linker_binary_override_cpp:"clang++"),flags,libs);
        }else if(name=="generic"&&linker_binary_override_other){
            return std::make_unique<linker::generic>(*linker_binary_override_other,flags,libs);
        }else if(Util::contains(std::vector<std::string>{"ld","ld.gold","ld.lld"},name)){
            return std::make_unique<linker::generic>(linker_binary_override_other?*linker_binary_override_other:name,flags,libs);
        }else if(Util::contains(std::vector<std::string>{"ar","llvm-ar"},name)){
            return std::make_unique<linker::ar>(linker_binary_override_other?*linker_binary_override_other:name,flags,libs);
        }else{
            throw std::runtime_error("unknown linker "+Util::quote_str_single(name));
        }
    }
    
}
