// $Id: spooler_module_java.h,v 1.2 2002/11/02 12:23:26 jz Exp $

#ifndef __SPOOLER_MODULE_JAVA_H
#define __SPOOLER_MODULE_JAVA_H

#include "../kram/thread_data.h"

namespace sos {
namespace spooler {

//---------------------------------------------------------------------------------Java_thread_data

struct Java_thread_data
{
                                Java_thread_data            ()                                      : _zero_(this+1) {}

    Fill_zero                  _zero_;
    JNIEnv*                    _env;
};

//------------------------------------------------------------------------------------------Java_vm

struct Java_vm                  // Java virtual machine
{
    struct Option
    {
                                Option                      ( const string& option, void* extra = NULL ) : _option(option), _extra(extra) {}

        string                 _option;
        void*                  _extra;
    };



                                Java_vm                     ( Spooler* s )                          : _zero_(this+1), _log(s) {}
                               ~Java_vm                     ()                                      { close(); }

    void                        init                        ();
    void                        close                       ();
    void                        get_options                 ();

    void                        attach_thread               ( const string& thread_name );
    void                        detach_thread               ();

                                operator JavaVM*            ()                                      { return vm(); }
    JavaVM*                     operator ->                 ()                                      { return vm(); }

    JavaVM*                     vm                          ();
    JNIEnv*                     env                         ();

    Z_NORETURN void             throw_java                  ( int return_value, const string&, const string& = "" );
    
  //ptr<Java_class>             class                       ( const string& name );



    void                        check_for_exception         ();


    Fill_zero                  _zero_;
    Prefix_log                 _log;
    string                     _filename;
    string                     _ini_class_path;
    string                     _config_class_path;
  //JDK1_1InitArgs             _vm_args;
    JavaVMInitArgs             _vm_args;
    vector<Option>             _options;
    JavaVM*                    _vm;

    Thread_data<Java_thread_data> _thread_data;
};

//---------------------------------------------------------------------------------------Java_class
/*
struct Java_class : Object
{
    void                        call                        ( const string& signature );
};
*/
//-----------------------------------------------------------------------------Java_module_instance
// Für Java-Objekte

struct Java_module_instance : Module_instance
{
                                Java_module_instance        ( Module* script )                      : Module_instance(script), _zero_(this+1) {}
                               ~Java_module_instance        ()                                      { close(); }

    void                        init                        ();
    void                        load                        ();
    void                        close                       ();
    Variant                     call                        ( const string& name );
    Variant                     call                        ( const string& name, int param );
    virtual bool                name_exists                 ( const string& name );
    bool                        callable                    ()                                      { return _object != NULL; }


    Fill_zero                  _zero_;
    Java_vm*                   _java_vm;
    JNIEnv*                    _env;
    jobject                    _object;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
