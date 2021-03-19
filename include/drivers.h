#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace drivers {
    
    namespace compiler {
        
        class driver {
        public:
            virtual ~driver()=0;
            virtual bool needs_compile(const std::filesystem::path &working_path,const std::filesystem::path &src_base,const std::filesystem::path &file_in,const std::filesystem::path &file_out)=0;
            virtual bool compile(const std::filesystem::path &working_path,const std::filesystem::path &src_base,const std::filesystem::path &file_in,const std::filesystem::path &file_out,const std::vector<std::string> &extra_flags)=0;
        };
        
        class base : public driver {
        protected:
            std::string compiler;
            std::vector<std::string> flags;
            std::vector<std::string> defines;
            std::vector<std::string> defines_calc;
            virtual void calc_defines();
        public:
            base(const std::string &compiler,const std::vector<std::string> &flags,const std::vector<std::string> &defines);
            virtual bool needs_compile(const std::filesystem::path &working_path,const std::filesystem::path &src_base,const std::filesystem::path &file_in,const std::filesystem::path &file_out) override;
            virtual bool compile(const std::filesystem::path &working_path,const std::filesystem::path &src_base,const std::filesystem::path &file_in,const std::filesystem::path &file_out,const std::vector<std::string> &extra_flags) override;
            
        };
        
        class generic : public base {
        public:
            using base::base;
            virtual bool compile(const std::filesystem::path &working_path,const std::filesystem::path &src_base,const std::filesystem::path &file_in,const std::filesystem::path &file_out,const std::vector<std::string> &extra_flags) override;
        };
        
        class gas final : public generic {
        protected:
            virtual void calc_defines() override;
        public:
            using generic::generic;
            virtual bool compile(const std::filesystem::path &working_path,const std::filesystem::path &src_base,const std::filesystem::path &file_in,const std::filesystem::path &file_out,const std::vector<std::string> &extra_flags) override;
        };
        
        class gnu : public generic {
            public:
                std::filesystem::path get_dpath(const std::filesystem::path &working_path,const std::filesystem::path &src_base,const std::filesystem::path &src_file);
                std::filesystem::path get_out(const std::filesystem::path &working_path,const std::filesystem::path &src_base,const std::filesystem::path &src_file);
                using generic::generic;
                virtual bool needs_compile(const std::filesystem::path &working_path,const std::filesystem::path &src_base,const std::filesystem::path &file_in,const std::filesystem::path &file_out) override;
                virtual bool compile(const std::filesystem::path &working_path,const std::filesystem::path &src_base,const std::filesystem::path &file_in,const std::filesystem::path &file_out,const std::vector<std::string> &extra_args) override;
        };
        
        class nasm final : public gnu {
        public:
            using gnu::gnu;
            virtual bool compile(const std::filesystem::path &working_path,const std::filesystem::path &src_base,const std::filesystem::path &file_in,const std::filesystem::path &file_out,const std::vector<std::string> &extra_flags) override;
        };
        
    }
    
    namespace linker {
        
        enum class bucket {
            BEFORE,
            NORMAL,
            AFTER,
        };
        
        class driver {
        public:
            virtual ~driver()=0;
            virtual void add_file(bucket,const std::filesystem::path &file)=0;
            virtual void clear()=0;
            virtual bool link(const std::filesystem::path &working_path,const std::filesystem::path &file_out,const std::vector<std::string> &extra_flags)=0;
            virtual std::string get_ext()=0;
        };
        
        class base : public driver {
        protected:
            std::string linker;
            std::vector<std::string> flags;
            std::vector<std::string> libs;
            std::vector<std::filesystem::path> link_before,link_extra_before,link_normal,link_extra_after,link_after;
        public:
            base(const std::string &linker,const std::vector<std::string> &flags,const std::vector<std::string> &libs,const std::vector<std::filesystem::path> &link_extra_before,const std::vector<std::filesystem::path> &link_extra_after);
            virtual void add_file(bucket,const std::filesystem::path &file) override;
            virtual void clear() override;
            virtual bool link(const std::filesystem::path &working_path,const std::filesystem::path &file_out,const std::vector<std::string> &extra_flags) override;
            virtual std::string get_ext() override;
        };
        
        class generic : public base {
        public:
            using base::base;
            virtual bool link(const std::filesystem::path &working_path,const std::filesystem::path &file_out,const std::vector<std::string> &extra_flags) override;
        };
        
        class gnu final : public generic {
                bool cpp=false;
                std::string linker_cpp;
            public:
                gnu(const std::string &linker_c,const std::string &linker_cpp,const std::vector<std::string> &flags,const std::vector<std::string> &libs,const std::vector<std::filesystem::path> &link_extra_before,const std::vector<std::filesystem::path> &link_extra_after);
                virtual void add_file(bucket,const std::filesystem::path &file) override;
        };
        
        class ar final : public base {
            using base::base;
            virtual std::string get_ext() override;
            virtual bool link(const std::filesystem::path &working_path,const std::filesystem::path &file_out,const std::vector<std::string> &extra_flags) override;
        };
        
    }
    
    enum compiler_lang {
        LANG_C,
        LANG_CPP,
        LANG_ASM,
    };
    
    std::unique_ptr<compiler::driver> get_compiler(const std::string &name,compiler_lang lang,const std::vector<std::string> &flags,const std::vector<std::string> &defines);
    
    std::unique_ptr<linker::driver> get_linker(const std::string &name,const std::vector<std::string> &flags,const std::vector<std::string> &libs,const std::vector<std::filesystem::path> &link_extra_before,const std::vector<std::filesystem::path> &link_extra_after);
    
}
