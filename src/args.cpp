#include "args.h"

#include <cstring>

namespace Args{
    
    std::map<std::string,NamedArg> named;
    std::vector<std::string> unnamed;
    
    void init(int argc,char ** argv){
        for(int i=1;i<argc;i++){
            if(argv[i][0]=='-'){
                int start=(argv[i][1]=='-'?2:1);
                if(char * cs=strchr(argv[i],'=')){
                    named.insert_or_assign(std::string(argv[i]+start,cs-argv[i]-start),NamedArg {.type=NamedArgType::VALUE,.value=std::string(cs+1)});
                }else{
                    named.insert_or_assign(std::string(argv[i]+start),NamedArg {.type=NamedArgType::FLAG,.value=""});
                }
            }else{
                unnamed.emplace_back(argv[i]);
            }
        }
    }
    
}
