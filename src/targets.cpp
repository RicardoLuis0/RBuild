#include "targets.h"
#include "util.h"
#include <functional>
#include <stdexcept>

using source_t=Targets::target::source_t;
using link_order_t=Targets::target::link_order_t;

static std::vector<source_t> mksrclist(const JSON::array_t &arr,const std::string &name,std::vector<std::string> &warnings_out);

[[maybe_unused]]
static std::vector<source_t> srclist_nonopt(const JSON::object_t &obj,const std::string &name,std::vector<std::string> &warnings_out) try {
    auto &elem=obj.at(name);
    if(!elem.is_arr()){
        throw JSON::JSON_Exception("In Source Array "+Util::quote_str_single(name)+": ","Array",elem.type_name());
    }
    return mksrclist(elem.get_arr(),name,warnings_out);
} catch(std::out_of_range &e) {
    throw JSON::JSON_Exception("Missing Source Array Element '"+Util::quote_str_single(name)+"'");
}

[[maybe_unused]]
static std::vector<source_t> srclist_opt(const JSON::object_t &obj,const std::string &name,std::vector<std::string> &warnings_out) {
    auto it=obj.find(name);
    if(it!=obj.end()) {
        if(!it->second.is_arr()){
            throw JSON::JSON_Exception("In Source Array "+Util::quote_str_single(name)+": ","Array",it->second.type_name());
        }
        return mksrclist(it->second.get_arr(),name,warnings_out);
    } else {
        return {};
    }
}

[[maybe_unused]]
static std::vector<source_t> srclist_multiopt(const JSON::object_t &obj,const std::vector<std::string> &names,std::vector<std::string> &warnings_out) {
    auto it=obj.end();
    std::string name;
    bool err=false;
    std::string err_str;
    for(auto name2:names){
        auto it2=obj.find(name2);
        if(it2!=obj.end()) {
            if(!it2->second.is_arr()){
                err=true;
                if(!err_str.empty()) err_str+=", ";
                err_str+=JSON::json_except_format("In Source Array "+Util::quote_str_single(name2)+": ","Array",it->second.type_name());
                continue;
            }
            if(it!=obj.end()){
                err=true;
                if(!err_str.empty()) err_str+=", ";
                err_str+="In Source Array "+Util::quote_str_single(name)+": Duplicate Entry "+Util::quote_str_single(name2);
                continue;
            }
            it=it2;
            name=name2;
        }
    }
    if(err){
        throw JSON::JSON_Exception(err_str);
    }
    if(it!=obj.end()){
        return mksrclist(it->second.get_arr(),name,warnings_out);
    }else{
        return {};
    }
}

static source_t mksrc(const JSON::Element &elem,const std::string &name,std::vector<std::string> &warnings_out,size_t i) try {
    using source_type=Targets::target::source_type;
    if(elem.is_str()){
        return source_t(elem.get_str());
    }else if(elem.is_obj()){
        std::vector<std::string> valid_keys {
            "name",
            "type",
            "include_list",
            "exclude_list",
        };
        auto &eobj=elem.get_obj();
        source_type type=JSON::enum_opt(eobj,"type",
                                        {
                                            {"exclude",source_type::BLACKLIST},
                                            {"blacklist",source_type::BLACKLIST},
                                            {"include",source_type::WHITELIST},
                                            {"whitelist",source_type::WHITELIST},
                                            {"include_folders_exclude_files",source_type::FOLDER_WHITELIST_FILE_BLACKLIST},
                                            {"exclude_files_include_folders",source_type::FOLDER_WHITELIST_FILE_BLACKLIST},
                                            {"folder_whitelist_file_blacklist",source_type::FOLDER_WHITELIST_FILE_BLACKLIST},
                                            {"file_blacklist_folder_whitelist",source_type::FOLDER_WHITELIST_FILE_BLACKLIST}
                                        },source_type::NONE);
        switch(type){
        default:
           throw std::runtime_error("unknown source type enum value");
        case source_type::NONE:
            break;
        case source_type::BLACKLIST:
            valid_keys.emplace_back("exclude_list");
            valid_keys.emplace_back("blacklist");
            break;
        case source_type::WHITELIST:
            valid_keys.emplace_back("include_list");
            valid_keys.emplace_back("whitelist");
            break;
        case source_type::FOLDER_WHITELIST_FILE_BLACKLIST:
            valid_keys.emplace_back("exclude_list");
            valid_keys.emplace_back("blacklist");
            valid_keys.emplace_back("include_list");
            valid_keys.emplace_back("whitelist");
            break;
        }
        for(auto &e:Util::filter_exclude(Util::keys(eobj),valid_keys)){
            warnings_out.push_back("In Source Array "+Util::quote_str_single(name)+": In Index #"+std::to_string(i)+": Ignored Unknown Element "+Util::quote_str_single(e));
        }
        return {
            .name=JSON::str_nonopt(eobj,"name"),
            .blacklist=((type==source_type::BLACKLIST||type==source_type::FOLDER_WHITELIST_FILE_BLACKLIST)?
                           JSON::strlist_multiopt(eobj,{"exclude_list","blacklist"})
                           :std::vector<std::string>{}),
            .whitelist=((type==source_type::WHITELIST||type==source_type::FOLDER_WHITELIST_FILE_BLACKLIST)?
                           srclist_multiopt(eobj,{"include_list","whitelist"},warnings_out)
                           :std::vector<source_t>{}),
            .type=type,
        };
    }else{
        throw JSON::JSON_Exception(std::vector<std::string>{"String","Object"},elem.type_name());
    }
} catch(JSON::JSON_Exception &e){
    throw JSON::JSON_Exception("In Source Array "+Util::quote_str_single(name)+": In Index #"+std::to_string(i)+": "+e.msg_top);
}

static std::optional<std::string> mkinc(const JSON::Element &elem,const std::string &name,std::vector<std::string> &warnings_out,size_t i) try {
    if(elem.is_str()){
        return elem.get_str();
    }else if(elem.is_obj()){
        std::optional<std::string> out;
        std::vector<std::string> valid_keys {
            "type",
        };
        enum types {
            SWITCH,
        };
        auto &eobj=elem.get_obj();
        types type=JSON::enum_nonopt<types>(eobj,"type", { {"switch",SWITCH}, });
        switch(type){
        case SWITCH:{
                enum conditions{
                    PLATFORM,
                };
                valid_keys.push_back("condition");
                valid_keys.push_back("cases");
                conditions cond=JSON::enum_nonopt<conditions>(eobj,"condition", { {"platform",PLATFORM}, });
                JSON::object_t cases=JSON::obj_nonopt(eobj,"cases");
                switch(cond){
                case PLATFORM:
                    out=JSON::str_opt(cases,CUR_PLATFORM);
                    break;
                default:
                    throw std::runtime_error("invalid include target condition");
                }
            }
            break;
        default:
            throw std::runtime_error("invalid include target type");
        }
        for(auto &e:Util::filter_exclude(Util::keys(eobj),valid_keys)){
            warnings_out.push_back("In Target Array "+Util::quote_str_single(name)+": In Index #"+std::to_string(i)+": Ignored Unknown Element "+Util::quote_str_single(e));
        }
        return out;
    }else{
        throw JSON::JSON_Exception(std::vector<std::string>{"String","Object"},elem.type_name());
    }
} catch(JSON::JSON_Exception &e){
    throw JSON::JSON_Exception("In Target Array "+Util::quote_str_single(name)+": In Index #"+std::to_string(i)+": "+e.msg_top);
}

static std::vector<std::string> mkinclist(const JSON::array_t &arr,const std::string &name,std::vector<std::string> &warnings_out){
    std::vector<std::string> slist;
    slist.reserve(arr.size());
    size_t n=0;
    for(const auto &elem:arr){
        auto s=mkinc(elem,name,warnings_out,n);
        if(s){
            slist.push_back(*s);
        }
        n++;
    }
    return slist;
}

static std::vector<std::string> inclist_opt(const JSON::object_t &obj,const std::string &name,std::vector<std::string> &warnings_out){
    auto it=obj.find(name);
    if(it!=obj.end()) {
        if(!it->second.is_arr()){
            throw JSON::JSON_Exception("In Include Array "+Util::quote_str_single(name)+": ","Array",it->second.type_name());
        }
        return mkinclist(it->second.get_arr(),name,warnings_out);
    } else {
        return {};
    }
}

static link_order_t mklinkorder(const JSON::object_t &obj,const std::string &name,std::vector<std::string> &warnings_out,size_t i) try {
    
    static const std::string valid_keys[]{
        "name",
        "type",
        "weight",
    };
    
    for(auto &e:Util::filter_exclude(Util::keys(obj),Util::CArrayIteratorAdaptor(valid_keys))){
        warnings_out.push_back("In In Link Order Array "+Util::quote_str_single(name)+": In Index #"+std::to_string(i)+": Ignored Unknown Element "+Util::quote_str_single(e));
    }
    if(obj.contains("weight")&&obj.at("weight").is_double()){
        warnings_out.push_back("In In Link Order Array "+Util::quote_str_single(name)+": In Index #"+std::to_string(i)+": Converting 'weight' from Double to Integer");
    }
    return {
        .name=JSON::str_nonopt(obj,"name"),
        .weight=JSON::number_int_nonopt(obj,"weight"),
        .type=enum_opt(obj,"type",
                       {
                           {"normal",Targets::target::LINK_NORMAL},
                           {"extra",Targets::target::LINK_EXTRA},
                           {"full_path",Targets::target::LINK_FULL_PATH}
                       },Targets::target::LINK_NORMAL)
    };
} catch(JSON::JSON_Exception &e){
    throw JSON::JSON_Exception("In Link Order Array "+Util::quote_str_single(name)+": In Index #"+std::to_string(i)+": "+e.msg_top);
}

static std::vector<link_order_t> mklinkorderlist(const JSON::array_t &arr,const std::string &name,std::vector<std::string> &warnings_out){
    std::vector<link_order_t> lolist;
    lolist.reserve(arr.size());
    size_t n=0;
    for(const auto &elem:arr){
        if(!elem.is_obj()){
            throw JSON::JSON_Exception("In Link Order Array ","Object",elem.type_name());
        }
        lolist.push_back(mklinkorder(elem.get_obj(),name,warnings_out,n));
        n++;
    }
    return lolist;
}


[[maybe_unused]]
static std::vector<link_order_t> linkorderlist_nonopt(const JSON::object_t &obj,const std::string &name,std::vector<std::string> &warnings_out) try {
    auto &elem=obj.at(name);
    if(!elem.is_arr()){
        throw JSON::JSON_Exception("In Link Order Array "+Util::quote_str_single(name)+": ","Array",elem.type_name());
    }
    return mklinkorderlist(elem.get_arr(),name,warnings_out);
} catch(std::out_of_range &e) {
    throw JSON::JSON_Exception("Missing Link Order Array Element '"+Util::quote_str_single(name)+"'");
}

[[maybe_unused]]
static std::vector<link_order_t> linkorderlist_opt(const JSON::object_t &obj,const std::string &name,std::vector<std::string> &warnings_out) {
    auto it=obj.find(name);
    if(it!=obj.end()) {
        if(!it->second.is_arr()){
            throw JSON::JSON_Exception("In Link Order Array "+Util::quote_str_single(name)+": ","Array",it->second.type_name());
        }
        return mklinkorderlist(it->second.get_arr(),name,warnings_out);
    } else {
        return {};
    }
}

static std::vector<source_t> mksrclist(const JSON::array_t &arr,const std::string &name,std::vector<std::string> &warnings_out) {
    std::vector<source_t> slist;
    slist.reserve(arr.size());
    size_t n=0;
    for(const auto &elem:arr){
        slist.push_back(mksrc(elem,name,warnings_out,n));
        n++;
    }
    return slist;
}

Targets::target::target(const JSON::object_t &tg,std::string name,std::vector<std::string> &warnings_out) :
sources(srclist_opt(tg,"sources",warnings_out)),
includes(inclist_opt(tg,"include",warnings_out)),
defines_all(JSON::strlist_opt(tg,"defines_all")),
defines_asm(JSON::strlist_opt(tg,"defines_asm")),
defines_c_cpp(JSON::strlist_opt(tg,"defines_c_cpp")),
defines_c(JSON::strlist_opt(tg,"defines_c")),
defines_cpp(JSON::strlist_opt(tg,"defines_cpp")),
flags_all(JSON::strlist_opt(tg,"flags_all")),
flags_asm(JSON::strlist_opt(tg,"flags_asm")),
flags_c_cpp(JSON::strlist_opt(tg,"flags_c_cpp")),
flags_c(JSON::strlist_opt(tg,"flags_c")),
flags_cpp(JSON::strlist_opt(tg,"flags_cpp")),
linker_flags(JSON::strlist_opt(tg,"linker_flags")),
linker_libs(JSON::strlist_opt(tg,"linker_libs")),
linker_order(linkorderlist_opt(tg,"linker_order",warnings_out)),
binary_folder_override(JSON::str_opt(tg,"binary_folder_override")),
project_binary_override(JSON::str_opt(tg,"project_binary_override")),
target_folder_override(JSON::str_opt(tg,"target_folder_override")),
compiler_driver_override_c(JSON::str_opt(tg,"compiler_driver_override_c")),
compiler_driver_override_cpp(JSON::str_opt(tg,"compiler_driver_override_cpp")),
compiler_driver_override_c_cpp(JSON::str_opt(tg,"compiler_driver_override_c_cpp")),
compiler_driver_override_asm(JSON::str_opt(tg,"compiler_driver_override_asm")),
compiler_driver_override_all(JSON::str_opt(tg,"compiler_driver_override_all")),
linker_driver_override(JSON::str_opt(tg,"linker_driver_override")),
compiler_binary_override_c(JSON::str_opt(tg,"compiler_binary_override_c")),
compiler_binary_override_cpp(JSON::str_opt(tg,"compiler_binary_override_cpp")),
compiler_binary_override_c_cpp(JSON::str_opt(tg,"compiler_binary_override_c_cpp")),
compiler_binary_override_asm(JSON::str_opt(tg,"compiler_binary_override_asm")),
compiler_binary_override_all(JSON::str_opt(tg,"compiler_binary_override_all")),
linker_binary_override_c(JSON::str_opt(tg,"linker_binary_override_c")),
linker_binary_override_cpp(JSON::str_opt(tg,"linker_binary_override_cpp")),
linker_binary_override_c_cpp(JSON::str_opt(tg,"linker_binary_override_c_cpp")),
linker_binary_override_other(JSON::str_opt(tg,"linker_binary_override_other")),
linker_binary_override_all(JSON::str_opt(tg,"linker_binary_override_all")),
include_only(JSON::bool_opt(tg,"include_only",false)) {
    static const std::string valid_keys[]{
        "sources",
        "include",
        "defines_all",
        "defines_asm",
        "defines_c_cpp",
        "defines_c",
        "defines_cpp",
        "flags_all",
        "flags_asm",
        "flags_c_cpp",
        "flags_c",
        "flags_cpp",
        "linker_flags",
        "linker_libs",
        "linker_order",
        "binary_folder_override",
        "project_binary_override",
        "target_folder_override",
        "compiler_driver_override_c",
        "compiler_driver_override_cpp",
        "compiler_driver_override_c_cpp",
        "compiler_driver_override_asm",
        "compiler_driver_override_all",
        "linker_driver_override",
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
        "include_only",
    };
    for(auto &e:Util::filter_exclude(Util::keys(tg),Util::CArrayIteratorAdaptor(valid_keys))){
        warnings_out.push_back("Ignored Unknown Element "+Util::quote_str_single(e));
    }
}

Targets::target& Targets::target::operator+=(const target& other) {
    sources.insert(sources.end(),other.sources.begin(),other.sources.end());
    includes.insert(includes.end(),other.includes.begin(),other.includes.end());
    defines_all.insert(defines_all.end(),other.defines_all.begin(),other.defines_all.end());
    defines_asm.insert(defines_asm.end(),other.defines_asm.begin(),other.defines_asm.end());
    defines_c_cpp.insert(defines_c_cpp.end(),other.defines_c_cpp.begin(),other.defines_c_cpp.end());
    defines_c.insert(defines_c.end(),other.defines_c.begin(),other.defines_c.end());
    defines_cpp.insert(defines_cpp.end(),other.defines_cpp.begin(),other.defines_cpp.end());
    flags_all.insert(flags_all.end(),other.flags_all.begin(),other.flags_all.end());
    flags_asm.insert(flags_asm.end(),other.flags_asm.begin(),other.flags_asm.end());
    flags_c_cpp.insert(flags_c_cpp.end(),other.flags_c_cpp.begin(),other.flags_c_cpp.end());
    flags_c.insert(flags_c.end(),other.flags_c.begin(),other.flags_c.end());
    flags_cpp.insert(flags_cpp.end(),other.flags_cpp.begin(),other.flags_cpp.end());
    linker_flags.insert(linker_flags.end(),other.linker_flags.begin(),other.linker_flags.end());
    linker_libs.insert(linker_libs.end(),other.linker_libs.begin(),other.linker_libs.end());
    linker_order.insert(linker_order.end(),other.linker_order.begin(),other.linker_order.end());
    if(!binary_folder_override&&other.binary_folder_override)binary_folder_override=*other.binary_folder_override;
    if(!project_binary_override&&other.project_binary_override)project_binary_override=*other.project_binary_override;
    if(!target_folder_override&&other.target_folder_override)target_folder_override=*other.target_folder_override;
    if(!compiler_driver_override_c&&other.compiler_driver_override_c)compiler_driver_override_c=*other.compiler_driver_override_c;
    if(!compiler_driver_override_cpp&&other.compiler_driver_override_cpp)compiler_driver_override_cpp=*other.compiler_driver_override_cpp;
    if(!compiler_driver_override_c_cpp&&other.compiler_driver_override_c_cpp)compiler_driver_override_c_cpp=*other.compiler_driver_override_c_cpp;
    if(!compiler_driver_override_asm&&other.compiler_driver_override_asm)compiler_driver_override_asm=*other.compiler_driver_override_asm;
    if(!compiler_driver_override_all&&other.compiler_driver_override_all)compiler_driver_override_all=*other.compiler_driver_override_all;
    if(!linker_driver_override&&other.linker_driver_override)linker_driver_override=*other.linker_driver_override;
    if(!compiler_binary_override_c&&other.compiler_binary_override_c)compiler_binary_override_c=*other.compiler_binary_override_c;
    if(!compiler_binary_override_cpp&&other.compiler_binary_override_cpp)compiler_binary_override_cpp=*other.compiler_binary_override_cpp;
    if(!compiler_binary_override_c_cpp&&other.compiler_binary_override_c_cpp)compiler_binary_override_c_cpp=*other.compiler_binary_override_c_cpp;
    if(!compiler_binary_override_asm&&other.compiler_binary_override_asm)compiler_binary_override_asm=*other.compiler_binary_override_asm;
    if(!compiler_binary_override_all&&other.compiler_binary_override_all)compiler_binary_override_all=*other.compiler_binary_override_all;
    if(!linker_binary_override_c&&other.linker_binary_override_c)linker_binary_override_c=*other.linker_binary_override_c;
    if(!linker_binary_override_cpp&&other.linker_binary_override_cpp)linker_binary_override_cpp=*other.linker_binary_override_cpp;
    if(!linker_binary_override_c_cpp&&other.linker_binary_override_c_cpp)linker_binary_override_c_cpp=*other.linker_binary_override_c_cpp;
    if(!linker_binary_override_other&&other.linker_binary_override_other)linker_binary_override_other=*other.linker_binary_override_other;
    if(!linker_binary_override_all&&other.linker_binary_override_all)linker_binary_override_all=*other.linker_binary_override_all;
    return *this;
}

void Targets::process_includes(std::vector<std::string> &warnings_out){
    std::vector<std::string> pending;
    std::vector<std::string> include_only;
    for(auto &t:targets){
        if(!t.second.includes.empty()){
            pending.push_back(t.first);
        }
        if(t.second.include_only){
            include_only.push_back(t.first);
        }
    }
    size_t processed;
    do {
        processed=0;
        for(const std::string &t:pending){
            if(auto it=targets.find(t);it!=targets.end()){
                target &tt=it->second;
                for(const std::string &i:tt.includes){
                    if(auto it2=targets.find(i);it2!=targets.end()){
                        target &ii=it2->second;
                        if(ii.includes.empty()){
                            tt+=ii;
                            tt.includes.erase(std::remove(tt.includes.begin(),tt.includes.end(),i),tt.includes.end());
                            processed++;
                        }
                    }
                }
            }
        }
        pending.erase(std::remove_if(pending.begin(),pending.end(), [this](const std::string &t)-> bool {
            return targets.at(t).includes.empty();
        }), pending.end());
    } while(processed!=0);
    if(pending.size()>0){
        for(auto &t:pending){
            targets.erase(t);
        }
        warnings_out.emplace_back("Failed to process includes for "+std::string(pending.size()==1?"target ":"targets: ")+Util::join(Util::map(pending,&Util::quote_str_single),", "));
    }
    for(auto &t:include_only){
        targets.erase(t);
    }
}

Targets::Targets(const JSON::object_t &targets_obj,std::vector<std::string> &warnings_out) {
    for(const auto & t:targets_obj) try {
        std::vector<std::string> target_warnings;
        targets.insert({t.first,{t.second.get_obj(),t.first,target_warnings}});
        Util::extract_warnings(std::move(Util::inplace_map(target_warnings,[&t](const std::string &s)->std::string{return "In target "+Util::quote_str_single(t.first)+": "+s;})),warnings_out);
    } catch(JSON::JSON_Exception &e) {
        throw JSON::JSON_Exception("In target "+Util::quote_str_single(t.first)+": "+e.msg_top);
    }
    process_includes(warnings_out);
}
