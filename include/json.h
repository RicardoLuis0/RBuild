#pragma once

#include <variant>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <stdexcept>
#include <optional>


#include "util.h"

enum JSON_Literal {
    JSON_FALSE,
    JSON_TRUE,
    JSON_NULL,
};

namespace JSON {
    class Element;
    using object_t=std::map<std::string,Element>;
    using array_t=std::vector<Element>;
    
    inline std::string json_except_format(const std::string &pre,const std::string &expected,const std::string &is){
        return pre+"Expected type "+Util::quote_str_single(expected)+", got type "+Util::quote_str_single(is);
    }
    inline std::string json_except_format(const std::string &pre,const std::vector<std::string> &expected,const std::string &is){
        return pre+"Expected "+(expected.size()==1?"type":"types")+" "+Util::join_or(Util::map(expected,Util::quote_str_single))+", got type "+Util::quote_str_single(is);
    }
    inline std::string json_except_format(const std::string &expected,const std::string &is){
        return json_except_format("",expected,is);
    }
    inline std::string json_except_format(const std::vector<std::string> &expected,const std::string &is){
        return json_except_format("",expected,is);
    }
    
    class JSON_Exception : public std::runtime_error {
    public:
        std::string msg_top;
        JSON_Exception(const std::string &s):runtime_error("JSON: "+s),msg_top(s){}
        
        JSON_Exception(const std::string &pre,const std::string &expected,const std::string &is):
            JSON_Exception(json_except_format(pre,expected,is)){}
        JSON_Exception(const std::string &pre,const std::vector<std::string> &expected,const std::string &is):
            JSON_Exception(json_except_format(pre,expected,is)){}
        JSON_Exception(const std::string &expected,const std::string &is):
            JSON_Exception(json_except_format(expected,is)){}
        JSON_Exception(const std::vector<std::string> &expected,const std::string &is):
            JSON_Exception(json_except_format(expected,is)){}
    };
    
    class Element {
        public:
            
            using data_t = std::variant<int64_t,double,std::string,array_t,object_t,JSON_Literal>;
            data_t data;
            
            //explicit constructors using std::variant
            explicit inline Element(const data_t & v) : data(v) {}
            explicit inline Element(data_t && v) : data(std::move(v)) {}
            
            //conversion constructors for simple data types
            inline Element(int i) : data(i) {}
            inline Element(int64_t i) : data(i) {}
            inline Element(double d) : data(d) {}
            inline Element(const std::string &s) : data(s) {}
            inline Element(std::string &&s) : data(std::move(s)) {}
            inline Element(bool b) : data(b?JSON_TRUE:JSON_FALSE) {}
            inline Element(std::nullptr_t) : data(JSON_NULL) {}
            inline Element(JSON_Literal l) : data(l) {}
            
            //helper access methods, may throw std::bad_variant_access if trying to access wrong types
            inline int64_t& get_int(){ return is_int()?std::get<int64_t>(data):throw JSON_Exception("Integer",type_name()); }
            inline const int64_t& get_int() const { return is_int()?std::get<int64_t>(data):throw JSON_Exception("Integer",type_name()); }
            
            inline double& get_double(){ return is_double()?std::get<double>(data):throw JSON_Exception("Double",type_name()); }
            inline const double& get_double() const { return is_double()?std::get<double>(data):throw JSON_Exception("Double",type_name()); }
            
            inline int get_number_int() const { return is_double()?static_cast<int64_t>(std::get<double>(data)):is_int()?std::get<int64_t>(data):throw JSON_Exception("Number",type_name()); }
            inline double get_number_double() const { return is_double()?std::get<double>(data):is_int()?static_cast<double>(std::get<int64_t>(data)):throw JSON_Exception("Number",type_name()); }
            
            inline std::string& get_str(){ return is_str()?std::get<std::string>(data):throw JSON_Exception("String",type_name()); }
            inline const std::string& get_str() const { return is_str()?std::get<std::string>(data):throw JSON_Exception("String",type_name()); }
            
            inline array_t& get_arr(){ return is_arr()?std::get<array_t>(data):throw JSON_Exception("Array",type_name()); }
            inline const array_t& get_arr() const { return is_arr()?std::get<array_t>(data):throw JSON_Exception("Array",type_name()); }
            
            inline object_t& get_obj(){ return is_obj()?std::get<object_t>(data):throw JSON_Exception("Object",type_name()); }
            inline const object_t& get_obj() const { return is_obj()?std::get<object_t>(data):throw JSON_Exception("Object",type_name()); }
            
            inline bool get_bool() const { return std::holds_alternative<JSON_Literal>(data)?(std::get<JSON_Literal>(data)!=JSON_NULL?std::get<JSON_Literal>(data)==JSON_TRUE:throw JSON_Exception("Boolean",type_name())):throw JSON_Exception("Boolean",type_name()); }
            
            //helper type check methods
            
            inline bool is_int() const { return std::holds_alternative<int64_t>(data); }
            
            inline bool is_double() const { return std::holds_alternative<double>(data); }
            
            inline bool is_number() const { return std::holds_alternative<int64_t>(data)||std::holds_alternative<double>(data); }
            
            inline bool is_str() const { return std::holds_alternative<std::string>(data); }
            
            inline bool is_arr() const { return std::holds_alternative<std::vector<Element>>(data); }
            
            inline bool is_obj() const { return std::holds_alternative<std::map<std::string,Element>>(data); }
            
            inline bool is_bool() const { return std::holds_alternative<JSON_Literal>(data)&&std::get<JSON_Literal>(data)!=JSON_NULL; }
            
            inline bool is_null() const { return std::holds_alternative<JSON_Literal>(data)&&std::get<JSON_Literal>(data)==JSON_NULL; }
            
            inline const char * type_name() const {
                if(is_int()){
                    return "Integer";
                }else if(is_double()){
                    return "Double";
                }else if(is_str()){
                    return "String";
                }else if(is_arr()){
                    return "Array";
                }else if(is_obj()){
                    return "Object";
                }else if(is_null()){
                    return "Null";
                }else if(is_bool()){
                    return "Boolean";
                }else{
                    return "Unknown";
                }
            }
            
            //serialize with spaces/newlines
            std::string to_json(bool trailing_quote=true,size_t depth=0) const;
            
            //serialize without spaces/newlines
            std::string to_json_min() const;
            
            Element& operator[](size_t index){//array access
                return get_arr().at(index);
            }
            
            Element& operator[](const char * index){//object access
                return get_obj().at(index);
            }
            
            Element& operator[](std::string index){//object access
                return get_obj().at(index);
            }
            
            operator int64_t(){
                return is_int()?get_int():is_double()?get_double():throw std::bad_variant_access();
            }
            
            operator double(){
                return is_int()?get_int():is_double()?get_double():throw std::bad_variant_access();
            }
            
            operator std::string(){
                return is_str()?get_str():throw std::bad_variant_access();
            }
            
    };
    
    inline Element Int(int64_t i){ return Element(i); }
    inline Element Boolean(bool b){ return Element(b?JSON_TRUE:JSON_FALSE); }
    inline Element True(){ return Element(JSON_TRUE); }
    inline Element False(){ return Element(JSON_FALSE); }
    inline Element Null(){ return Element(JSON_NULL); }
    inline Element Double(double d){ return Element(d); }
    inline Element String(std::string s){ return Element(s); }
    inline Element Array(const std::vector<Element> & v){ return Element(Element::data_t(v)); }
    inline Element Array(std::vector<Element> && v){ return Element(Element::data_t(std::move(v))); }
    inline Element Object(const std::map<std::string,Element> & m){ return Element(Element::data_t(m)); }
    inline Element Object(std::map<std::string,Element> && m){ return Element(Element::data_t(std::move(m))); }
    
    inline std::vector<std::string> mkstrlist(const array_t &arr) try {
        std::vector<std::string> strlist;
        strlist.reserve(arr.size());
        for(const auto &elem:arr){
            strlist.emplace_back(elem.get_str());
        }
        return strlist;
    } catch(JSON_Exception &e){
        throw JSON_Exception("In String Array: "+e.msg_top);
    }
    
    inline std::vector<std::string> strlist_opt(const object_t &obj,const std::string &name) try {
        auto it=obj.find(name);
        return (it!=obj.end())?mkstrlist(it->second.get_arr()):std::vector<std::string>{};
    } catch(JSON_Exception &e){
        throw JSON_Exception("In String Array "+Util::quote_str_single(name)+": "+e.msg_top);
    }
    
    inline std::vector<std::string> strlist_multiopt(const object_t &obj,const std::vector<std::string> &names) {
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
                    err_str+=JSON::json_except_format("In String Array "+Util::quote_str_single(name2)+": ","Array",it->second.type_name());
                    continue;
                }
                if(it!=obj.end()){
                    err=true;
                    if(!err_str.empty()) err_str+=", ";
                    err_str+="In String Array "+Util::quote_str_single(name)+": Duplicate Entry "+Util::quote_str_single(name2);
                    continue;
                }
                it=it2;
                name=name2;
            }
        }
        if(err){
            throw JSON::JSON_Exception(err_str);
        }
        try {
            return (it!=obj.end())?mkstrlist(it->second.get_arr()):std::vector<std::string>{};
        } catch(JSON_Exception &e){
            throw JSON_Exception("In String Array "+Util::quote_str_single(name)+": "+e.msg_top);
        }
    }
    
    inline std::vector<std::string> strlist_nonopt(const object_t &obj,const std::string &name) try {
        return mkstrlist(obj.at(name).get_arr());
    } catch(JSON_Exception &e){
        throw JSON_Exception("In String Array "+Util::quote_str_single(name)+": "+e.msg_top);
    } catch(std::out_of_range &e){
        throw JSON_Exception("Missing String Array Element "+Util::quote_str_single(name));
    }
    
    template<typename T>
    T enum_opt(const object_t &obj,const std::string &name,const std::map<std::string,T> &valid_values,const T &default_value) try {
        auto it=obj.find(name);
        if(it!=obj.end()){
            std::string s=it->second.get_str();
            if(auto vit=valid_values.find(s);vit!=valid_values.end()){
                return vit->second;
            }else{
                throw std::runtime_error("Invalid value "+Util::quote_str_single(s)+" for "+Util::quote_str_single(name)+", must be one of { "+Util::join(Util::map(Util::keys(valid_values),&Util::quote_str_single),", ")+" }");
            }
        }else{
            return default_value;
        }
    } catch(JSON_Exception &e){
        throw JSON_Exception("In "+Util::quote_str_single(name)+": "+e.msg_top);
    }
    
    template<typename T>
    T enum_nonopt(const object_t &obj,const std::string &name,const std::map<std::string,T> &valid_values) try {
        std::string s=obj.at(name).get_str();
        if(auto vit=valid_values.find(s);vit!=valid_values.end()){
            return vit->second;
        }else{
            throw std::runtime_error("Invalid value "+Util::quote_str_single(s)+" for "+Util::quote_str_single(name)+", must be one of { "+Util::join(Util::map(Util::keys(valid_values),&Util::quote_str_single),", ")+" }");
        }
    } catch(JSON_Exception &e){
        throw JSON_Exception("In "+Util::quote_str_single(name)+": "+e.msg_top);
    } catch(std::out_of_range &e){
        throw JSON_Exception("Missing Enum Element "+Util::quote_str_single(name));
    }
    
    inline JSON::object_t obj_nonopt(const object_t &obj,const std::string &name) try {
        return obj.at(name).get_obj();
    } catch(JSON_Exception &e){
        throw JSON_Exception("In "+Util::quote_str_single(name)+": "+e.msg_top);
    } catch(std::out_of_range &e){
        throw JSON_Exception("Missing Object Element "+Util::quote_str_single(name));
    }
    
    inline bool bool_opt(const object_t &obj,const std::string &name,bool opt_default) try {
        auto it=obj.find(name);
        return (it!=obj.end())?it->second.get_bool():opt_default;
    } catch(JSON_Exception &e){
        throw JSON_Exception("In "+Util::quote_str_single(name)+": "+e.msg_top);
    }
    
    inline std::string str_nonopt(const object_t &obj,const std::string &name) try {
        return obj.at(name).get_str();
    } catch(JSON_Exception &e) {
        throw JSON_Exception("In "+Util::quote_str_single(name)+": "+e.msg_top);
    } catch(std::out_of_range &e){
        throw JSON_Exception("Missing String Element "+Util::quote_str_single(name));
    }
    
    inline std::string str_opt(const object_t &obj,const std::string &name,const std::string &opt_default) try {
        auto it=obj.find(name);
        return (it!=obj.end())?it->second.get_str():opt_default;
    } catch(JSON_Exception &e){
        throw JSON_Exception("In "+Util::quote_str_single(name)+": "+e.msg_top);
    }
    
    inline std::optional<std::string> str_opt(const object_t &obj,const std::string &name) try {
        auto it=obj.find(name);
        return (it!=obj.end())?it->second.get_str():std::optional<std::string>{std::nullopt};
    } catch(JSON_Exception &e){
        throw JSON_Exception("In "+Util::quote_str_single(name)+": "+e.msg_top);
    }
    
    Element parse(const std::string &data);
    
    
    inline int number_int_nonopt(const object_t &obj,std::string name) try {
        return obj.at(name).get_number_int();
    } catch(JSON_Exception &e) {
        throw JSON_Exception("In "+Util::quote_str_single(name)+": "+e.msg_top);
    } catch(std::out_of_range &e){
        throw JSON_Exception("Missing Numeric Element "+Util::quote_str_single(name));
    }
    
    inline std::optional<int> number_int_opt(const object_t &obj,std::string name) try {
        auto it=obj.find(name);
        return (it!=obj.end())?it->second.get_number_int():std::optional<int>{std::nullopt};
    } catch(JSON_Exception &e){
        throw JSON_Exception("In "+Util::quote_str_single(name)+": "+e.msg_top);
    }
    
    inline int number_int_opt(const object_t &obj,std::string name,int default_opt) try {
        auto it=obj.find(name);
        return (it!=obj.end())?it->second.get_number_int():default_opt;
    } catch(JSON_Exception &e){
        throw JSON_Exception("In "+Util::quote_str_single(name)+": "+e.msg_top);
    }
    
}
