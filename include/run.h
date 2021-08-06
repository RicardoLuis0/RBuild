#pragma once

#include <vector>
#include <atomic>
#include <thread>

#if defined(__unix__)
    #include <unistd.h>
    #include <spawn.h>
#endif // defined

namespace Util {
    
    class redirect_data { // should be saved on long-living memory (heap/global)
    private:
        std::atomic<bool> running;
        std::atomic<bool> except;
        std::exception_ptr e;
        static void thread_entry(redirect_data*);
        void thread_main();
    public:
        redirect_data();
        redirect_data(redirect_data&& other);
        ~redirect_data();
        void start();
        void stop();
        
        std::string s_stdout;
        std::string s_stderr;
        std::thread t;
        #if defined(__unix__)
            bool close_fds;
            bool initialized;
            posix_spawn_file_actions_t f_acts;
            int p_stdin[2];
            int p_stdout[2];
            int p_stderr[2];
        #elif defined(_WIN32)
            std::string sUUID;
            void * hStdInPipe;
            void * hStdOutPipe;
            void * hStdErrPipe;
            void * hStdIn;
            void * hStdOut;
            void * hStdErr;
        #endif
    };
    
    int run_noexcept(const std::string &program,const std::vector<std::string> &args_in,std::string (*alternate_cmdline)(const std::string&,const std::vector<std::string>&)=nullptr,bool silent=false,redirect_data * redir_data=nullptr) noexcept;
    
    int run(std::string program,const std::vector<std::string> &args_in,std::string (*alternate_cmdline)(const std::string&,const std::vector<std::string>&)=nullptr,bool silent=false,redirect_data * redir_data=nullptr);
    
    std::string alternate_cmdline_args_to_file(const std::string&,const std::vector<std::string>&,const std::string &file_arg);
    inline std::string alternate_cmdline_args_to_file_regular(const std::string&p,const std::vector<std::string>&v){
        return alternate_cmdline_args_to_file(p,v," @");
    }
    
    inline std::string alternate_cmdline_args_to_file_nasm(const std::string&p,const std::vector<std::string>&v){
        return alternate_cmdline_args_to_file(p,v," -@ ");
    }
    
}
