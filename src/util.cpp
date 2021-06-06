#include "util.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <mutex>


#ifdef __unix__
    #include <unistd.h>
    #include <spawn.h>
    #include <sys/wait.h>
    #include <sys/ioctl.h>
    #include <fcntl.h>
    extern char **environ;
#elif defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <rpc.h>
    #include <shlobj.h>
    
    static std::string Win32ErrStr(DWORD err){
        char * tmp=nullptr;
        size_t n=FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,nullptr,err,0,reinterpret_cast<char*>(&tmp),0,nullptr);
        if(n>0){
            std::string tmpstr(tmp,n);
            LocalFree(tmp);
            return tmpstr;
        }
        return "Unknown Error";
    }
    
    static std::string build_cmdline(const std::string &path,const std::vector<std::string> &args_in){
        std::vector<std::string> args_v;
        args_v.reserve(args_in.size()+1);
        args_v.push_back(Util::quote_str_double(path));
        std::transform(std::begin(args_in),std::end(args_in),std::back_inserter(args_v),&Util::quote_str_double);
        return Util::join(args_v);
    }
    
    static std::string resolve_path(std::string path,bool &path_found){
        std::unique_ptr<char[]> cpath(new char[MAX_PATH]);
        char * p;
        if(SearchPathA(NULL,path.c_str(),".exe",MAX_PATH,cpath.get(),&p)>0){
            path_found=true;
        }else{
            cpath.get()[0]=0;
        }
        return std::move(std::string(cpath.get()));
    }
    #define CREATEPROCESS_CMD_MAX 32_K
#endif

namespace Util {
    namespace {
        constexpr char escape(char c){
            switch(c) {
            case '\a':
                return 'a';
            case '\b':
                return 'b';
            case '\e':
                return 'e';
            case '\f':
                return 'f';
            case '\n':
                return 'n';
            case '\r':
                return 'r';
            case '\t':
                return 't';
            case '\v':
                return 'v';
            case '\\':
                return '\\';
            case '"':
                return '"';
            default:
                return c;
            }
        }
    }
    
    std::string quote_str(const std::string &s,char quote_char){
        std::string str;
        str+=quote_char;
        for(char c:s){
            if(c=='\\'||c==quote_char||escape(c)!=c){
                str+='\\';
                str+=escape(c);
            }else{
                str+=c;
            }
        }
        str+=quote_char;
        return str;
    }
    
    std::string join(const std::vector<std::string> &v,const std::string &on){
        std::string o;
        bool first=true;
        for(auto &s:v){
            if(!first)o+=on;
            o+=s;
            first=false;
        }
        return o;
    }
    
    std::string join_or(const std::vector<std::string> &v,const std::string &sep_comma,const std::string &sep_or){
        std::string o;
        const size_t n=v.size();
        for(size_t i=0;i<n;i++){
            if(i==(n-1)){
                o+=sep_or;
            }else if(i>0){
                o+=sep_comma;
            }
            o+=v[i];
        }
        return o;
    }
    
    std::vector<std::string> split(const std::string &ss,char c,bool split_empty){
        //const size_t n=ss.size();
        std::vector<std::string> o;
        const char * s=ss.c_str();
        const char * cs=NULL;
        while((cs=strchr(s,c))){
            o.emplace_back(s,cs-s);
            s=cs+1;
            if(!split_empty) while(*s==c) ++s;
        }
        if(*s!='\0') o.emplace_back(s);
        return o;
    }
    
    std::vector<std::string> split(const std::string &ss,const std::vector<char> &cv,bool split_empty){
        size_t i=0,s=0;
        std::vector<std::string> o;
        const size_t n=ss.size();
        for(;i<n;i++){
            if(contains(cv,ss[i])){
                if(i==s&&!split_empty){
                    ++s;
                    continue;
                }
                o.emplace_back(ss.substr(s,i-s));
                s=i+1;
            }
        }
        if(i!=s) o.emplace_back(ss.substr(s));
        return o;
    }
    
    std::vector<std::string> split_str(const std::string &ss,const std::string &s,bool split_empty){
        std::vector<std::string> o;
        size_t offset=0;
        const size_t len=s.size();
        while(true){
            size_t start=ss.find(s,offset);
            if(start==std::string::npos)break;
            if(offset==start&&!split_empty){
                offset+=len;
                continue;
            }
            o.emplace_back(ss.substr(offset,start-offset));
            offset=start+len;
        }
        o.emplace_back(ss.substr(offset));
        return o;
    }
    
    static std::pair<size_t,size_t> first_of_vec(const std::string &str,size_t offset,const std::vector<std::string> &vec){
        size_t start=std::string::npos;
        size_t len=std::string::npos;
        for(auto &elem:vec){
            if(auto i=str.find(elem,offset);i!=std::string::npos){
                if(start==std::string::npos||i<start){
                    start=i;
                    len=elem.size();
                }
            }
        }
        return {start,len};
    }
    
    std::vector<std::string> split_str(const std::string &ss,const std::vector<std::string> &sv,bool split_empty){
        std::vector<std::string> o;
        size_t offset=0;
        while(true){
            auto [start,len]=first_of_vec(ss,offset,sv);
            if(start==std::string::npos)break;
            if(offset==start&&!split_empty){
                offset+=len;
                continue;
            }
            o.emplace_back(ss.substr(offset,start-offset));
            offset=start+len;
        }
        o.emplace_back(ss.substr(offset));
        return o;
    }
    
    std::string readfile(const std::string &filename) try {
        std::ostringstream ss;
        std::ifstream f(filename);
        if(!f)throw std::runtime_error(strerror(errno));
        ss<<f.rdbuf();
        return ss.str();
    }catch(std::exception &e){
        throw std::runtime_error("Failed to read "+Util::quote_str_single(filename)+" : "+e.what());
    }
    
    void writefile(const std::string &filename,const std::string &data) try {
        std::ofstream f(filename);
        f<<data;
    }catch(std::exception &e){
        throw std::runtime_error("Failed to write to "+Util::quote_str_single(filename)+" : "+e.what());
    }
    
    int run_noexcept(const std::string &program,const std::vector<std::string> &args_in,std::string (*alternate_cmdline)(const std::string&,const std::vector<std::string>&),bool silent,redirect_data * redir_data) noexcept {
        try{
            return run(program,args_in,alternate_cmdline,silent,redir_data);
        }catch(std::exception &e){
            std::cerr<<"Run "+Util::quote_str_single(program)+" Failed: "<<e.what()<<"\n";
            return -1;
        }
    }
    
    std::mutex print_mutex;
    
    void print_sync(std::string s){
        std::lock_guard g(print_mutex);
        std::cout<<s;
    }
    
    int run(std::string program,const std::vector<std::string> &args_in,std::string (*alternate_cmdline)(const std::string&,const std::vector<std::string>&),bool silent,redirect_data * redir_data){
        #if defined(__unix__)
            std::vector<std::string> args_v;
            args_v.reserve(std::size(args_in)+1);
            args_v.push_back(program);
            std::copy(std::begin(args_in),std::end(args_in),std::back_inserter(args_v));
            std::vector<const char *> args(get_cstrs(args_v));
            args.push_back(nullptr);
            pid_t pid;
            if(!silent)print_sync(program+" "+join(map(args_in,&quote_str_double))+"\n");
            if(redir_data){
                redir_data->start();
            }
            if(int err=posix_spawnp(&pid,program.c_str(),redir_data?&redir_data->f_acts:nullptr,nullptr,const_cast<char*const*>(args.data()),environ);err==0){
                int status;
                waitpid(pid,&status,0);
                return WIFEXITED(status)?WEXITSTATUS(status):-1;
            }else{
                throw std::runtime_error("posix_spawnp: "+std::string(strerror(err)));
            }
        #elif defined(_WIN32)
            if(!program.ends_with(".exe"))program+=".exe";
            
            bool path_found=false;
            std::string path=resolve_path(program,path_found);
            if(path_found){
                STARTUPINFOA si={};
                PROCESS_INFORMATION pi={};
                if(redir_data){
                    si.dwFlags|=STARTF_USESTDHANDLES;
                    si.hStdInput=redir_data->hStdIn;
                    si.hStdOutput=redir_data->hStdOut;
                    si.hStdError=redir_data->hStdErr;
                }
                std::string args=build_cmdline(path,args_in);
                if(args.size()>=CREATEPROCESS_CMD_MAX){
                    if(alternate_cmdline){
                        args=alternate_cmdline(path,args_in);
                        if(args.size()>=CREATEPROCESS_CMD_MAX){
                            throw std::runtime_error("Running '"+program+"': Command line too long, is "+std::to_string(args.size())+", max "+std::to_string(CREATEPROCESS_CMD_MAX));
                        }
                    }else{
                        throw std::runtime_error("Running '"+program+"': Command line too long, is "+std::to_string(args.size())+", max "+std::to_string(CREATEPROCESS_CMD_MAX));
                    }
                }
                if(!silent)print_sync(program+" "+join(map(args_in,&quote_str_double))+"\n");
                if(redir_data){
                    redir_data->start();
                }
                if(!CreateProcessA(nullptr,args.data(),nullptr,nullptr,redir_data!=nullptr,0,nullptr,nullptr,&si,&pi)){
                    throw std::runtime_error("Running '"+program+"': CreateProcessA: "+Win32ErrStr(GetLastError()));
                }
                WaitForSingleObject(pi.hProcess,INFINITE);
                DWORD ret;
                GetExitCodeProcess(pi.hProcess,&ret);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                return ret;
            }else{
                throw std::runtime_error("Running '"+program+"': Could not find executable");
            }
        #endif
    }
    
    bool tmpfile_init=false;
    std::string tmpfile_name="tmp.args";
    std::mutex tmpfile_mutex;
    
    std::string alternate_cmdline_args_to_file(const std::string &program,const std::vector<std::string> &args_in,const std::string &file_arg){
        std::lock_guard _(tmpfile_mutex);
        tmpfile_init=true;
        {
            std::ofstream f(tmpfile_name);
            f<<join(map(args_in,&quote_str_double),"\n");
        }
        return program+file_arg+tmpfile_name;
    }
    
    static void remove_tmpfile() __attribute__((destructor));
    
    void remove_tmpfile(){
        if(tmpfile_init){
            remove(tmpfile_name.c_str());
        }
    }

    redirect_data::redirect_data():running(false),close_fds(false){
        #if defined(__unix__)
            
            if(pipe2(p_stdin,O_NONBLOCK)!=0){
                throw std::runtime_error("stdin pipe creation failed: "+std::string(strerror(errno)));
            }
            if(pipe2(p_stdout,O_NONBLOCK)!=0){
                close(p_stdin[0]);
                close(p_stdin[1]);
                throw std::runtime_error("stdout pipe creation failed: "+std::string(strerror(errno)));
            }
            if(pipe2(p_stderr,O_NONBLOCK)!=0){
                close(p_stdin[0]);
                close(p_stdin[1]);
                close(p_stdout[0]);
                close(p_stdout[1]);
                throw std::runtime_error("stdout pipe creation failed: "+std::string(strerror(errno)));
            }
            
            posix_spawn_file_actions_init(&f_acts);
            
            posix_spawn_file_actions_addclose(&f_acts,p_stdin[1]);
            posix_spawn_file_actions_addclose(&f_acts,p_stdout[0]);
            posix_spawn_file_actions_addclose(&f_acts,p_stderr[0]);
            posix_spawn_file_actions_adddup2(&f_acts,p_stdin[0],STDIN_FILENO);
            posix_spawn_file_actions_adddup2(&f_acts,p_stdout[1],STDOUT_FILENO);
            posix_spawn_file_actions_adddup2(&f_acts,p_stderr[1],STDERR_FILENO);
            posix_spawn_file_actions_addclose(&f_acts,p_stdin[0]);
            posix_spawn_file_actions_addclose(&f_acts,p_stdout[1]);
            posix_spawn_file_actions_addclose(&f_acts,p_stderr[1]);
            
            close_fds=true;
            
        #elif defined(_WIN32)
            {
                UUID uuid;
                RPC_STATUS st=UuidCreate(&uuid);
                if(!(st==RPC_S_OK||st==RPC_S_UUID_LOCAL_ONLY)){//not OK
                    throw std::runtime_error("redirect_data: UuidCreate failed");
                }
                RPC_CSTR uuid_rcs;
                UuidToStringA(&uuid,&uuid_rcs);
                sUUID=std::string(reinterpret_cast<const char *>(uuid_rcs));
                RpcStringFreeA(&uuid_rcs);
            }
            
            SECURITY_ATTRIBUTES sa={};
            
            sa.nLength=sizeof(sa);
            sa.bInheritHandle=true;
            
            std::string common_name("\\\\.\\pipe\\");
            std::string stdin_name(common_name+"stdin."+sUUID);
            std::string stdout_name(common_name+"stdout."+sUUID);
            std::string stderr_name(common_name+"stderr."+sUUID);
            if((hStdInPipe=CreateNamedPipeA(stdin_name.c_str(),PIPE_ACCESS_DUPLEX,PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_NOWAIT|PIPE_REJECT_REMOTE_CLIENTS,PIPE_UNLIMITED_INSTANCES,256,256,0,&sa))==INVALID_HANDLE_VALUE){
                throw std::runtime_error("redirect_data: CreateNamedPipeA hStdInPipe failed: "+Win32ErrStr(GetLastError()));
            }
            if((hStdOutPipe=CreateNamedPipeA(stdout_name.c_str(),PIPE_ACCESS_DUPLEX,PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_NOWAIT|PIPE_REJECT_REMOTE_CLIENTS,PIPE_UNLIMITED_INSTANCES,256,256,0,&sa))==INVALID_HANDLE_VALUE){
                CloseHandle(hStdInPipe);
                throw std::runtime_error("redirect_data: CreateNamedPipeA hStdOutPipe failed: "+Win32ErrStr(GetLastError()));
            }
            if((hStdErrPipe=CreateNamedPipeA(stderr_name.c_str(),PIPE_ACCESS_DUPLEX,PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_NOWAIT|PIPE_REJECT_REMOTE_CLIENTS,PIPE_UNLIMITED_INSTANCES,256,256,0,&sa))==INVALID_HANDLE_VALUE){
                CloseHandle(hStdInPipe);
                CloseHandle(hStdOutPipe);
                throw std::runtime_error("redirect_data: CreateNamedPipeA hStdErrPipe failed: "+Win32ErrStr(GetLastError()));
            }
            if((hStdIn=CreateFile(stdin_name.c_str(),GENERIC_READ|GENERIC_WRITE,0,&sa,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr))==INVALID_HANDLE_VALUE){
                CloseHandle(hStdInPipe);
                CloseHandle(hStdOutPipe);
                CloseHandle(hStdErrPipe);
                throw std::runtime_error("redirect_data: CreateFile hStdIn failed: "+Win32ErrStr(GetLastError()));
            }
            if((hStdOut=CreateFile(stdout_name.c_str(),GENERIC_READ|GENERIC_WRITE,0,&sa,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr))==INVALID_HANDLE_VALUE){
                CloseHandle(hStdInPipe);
                CloseHandle(hStdOutPipe);
                CloseHandle(hStdErrPipe);
                CloseHandle(hStdIn);
                throw std::runtime_error("redirect_data: CreateFile hStdIn failed: "+Win32ErrStr(GetLastError()));
            }
            if((hStdErr=CreateFile(stderr_name.c_str(),GENERIC_READ|GENERIC_WRITE,0,&sa,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr))==INVALID_HANDLE_VALUE){
                CloseHandle(hStdInPipe);
                CloseHandle(hStdOutPipe);
                CloseHandle(hStdErrPipe);
                CloseHandle(hStdIn);
                CloseHandle(hStdOut);
                throw std::runtime_error("redirect_data: CreateFile hStdIn failed: "+Win32ErrStr(GetLastError()));
            }
            try {
                //throw std::runtime_error("redirect_data::redirect_data unimplemented");
            } catch(...){
                CloseHandle(hStdInPipe);
                CloseHandle(hStdOutPipe);
                CloseHandle(hStdErrPipe);
                CloseHandle(hStdIn);
                CloseHandle(hStdOut);
                CloseHandle(hStdErr);
                throw;
            }
        #endif
    }
    
    redirect_data::redirect_data(redirect_data&& other):redirect_data(){
        if(other.running){
            throw std::runtime_error("Cannot move active 'redirect_data'");
        }
        s_stdout=other.s_stdout;
        s_stderr=other.s_stderr;
        #if defined(__unix__)
            f_acts=other.f_acts;
            p_stdin[0]=other.p_stdin[0];
            p_stdin[1]=other.p_stdin[1];
            p_stdout[0]=other.p_stdout[0];
            p_stdout[1]=other.p_stdout[1];
            p_stderr[0]=other.p_stderr[0];
            p_stderr[1]=other.p_stderr[1];
            other.close_fds=false;
        #elif defined(_WIN32)
            hStdInPipe=other.hStdInPipe;
            other.hStdInPipe=nullptr;
            hStdOutPipe=other.hStdOutPipe;
            other.hStdOutPipe=nullptr;
            hStdErrPipe=other.hStdErrPipe;
            other.hStdErrPipe=nullptr;
            hStdIn=other.hStdIn;
            other.hStdIn=nullptr;
            hStdOut=other.hStdOut;
            other.hStdOut=nullptr;
            hStdErr=other.hStdErr;
            other.hStdErr=nullptr;
        #endif
    }
    
    redirect_data::~redirect_data(){
        #if defined(__unix__)
            if(close_fds){
                close(p_stdin[0]);
                close(p_stdin[1]);
                close(p_stdout[0]);
                close(p_stdout[1]);
                close(p_stderr[0]);
                close(p_stderr[1]);
            }
        #elif defined(_WIN32)
            if(hStdIn){
                CloseHandle(hStdIn);
            }
            if(hStdOut){
                CloseHandle(hStdOut);
            }
            if(hStdErr){
                CloseHandle(hStdErr);
            }
            if(hStdInPipe){
                CloseHandle(hStdInPipe);
            }
            if(hStdOutPipe){
                CloseHandle(hStdOutPipe);
            }
            if(hStdErrPipe){
                CloseHandle(hStdErrPipe);
            }
        #endif
    }
    
    void redirect_data::thread_main() try {
        while(running){
            #if defined(__unix__)
                int n;
                if(ioctl(p_stdout[0],FIONREAD,&n)==0&&n>0){
                    char buf[n+1];
                    int r=read(p_stdout[0],buf,n);
                    buf[r]=0;
                    s_stdout+=std::string(buf);
                }
                if(ioctl(p_stderr[0],FIONREAD,&n)==0&&n>0){
                    char buf[n+1];
                    int r=read(p_stderr[0],buf,n);
                    buf[r]=0;
                    s_stderr+=std::string(buf);
                }
            #elif defined(_WIN32)
                DWORD num;
                if(!PeekNamedPipe(hStdOutPipe,nullptr,0,nullptr,&num,nullptr)){
                    throw std::runtime_error("redirect_data::thread_main: PeekNamedPipe hStdInR failed: "+Win32ErrStr(GetLastError()));
                }
                if(num>0){
                    char buf[num+1];
                    DWORD numread;
                    ReadFile(hStdOutPipe,buf,num,&numread,nullptr);
                    buf[numread]=0;
                    s_stdout+=std::string(buf);
                }
                if(!PeekNamedPipe(hStdErrPipe,nullptr,0,nullptr,&num,nullptr)){
                    throw std::runtime_error("redirect_data::thread_main: PeekNamedPipe hStdInR failed: "+Win32ErrStr(GetLastError()));
                }
                if(num>0){
                    char buf[num+1];
                    DWORD numread;
                    ReadFile(hStdErrPipe,buf,num,&numread,nullptr);
                    buf[numread]=0;
                    s_stderr+=std::string(buf);
                }
            #endif
            std::this_thread::yield();
        }
    } catch(...){
        e=std::current_exception();
        except=true;
    }
    
    void redirect_data::thread_entry(redirect_data * d){
        d->thread_main();
    }
    
    void redirect_data::start(){
        if(running) stop();
        running=true;
        t=std::thread(thread_entry,this);
    }
    
    void redirect_data::stop(){
        if(running){
            running=false;
            t.join();
            if(except){
                std::rethrow_exception(e);
            }
        }
    }
    
}

