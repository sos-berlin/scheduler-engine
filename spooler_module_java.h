// $Id: spooler_module_java.h,v 1.4 2002/11/07 17:57:24 jz Exp $

#ifndef __SPOOLER_MODULE_JAVA_H
#define __SPOOLER_MODULE_JAVA_H

#include "../kram/thread_data.h"

#define JAVA_IDISPATCH_CLASS "sos/spooler/Idispatch"

namespace sos {
namespace spooler {

struct Java_vm;

//-----------------------------------------------------------------------------------------Java_env

struct Java_env
{
                              //Java_env                    ( Java_vm* vm, JNIEnv* jenv )           : _java_vm(vm), _jenv( jenv ) {}

                                operator JNIEnv*            ()                                      { return _jenv; }
    JNIEnv*                     operator ->                 ()                                      { return _jenv; }

    jclass                      find_class                  ( const string& name );
    jmethodID                   get_method_id               ( jclass, const string& name, const string& signature );
    jclass                      get_object_class            ( jobject );

    Java_vm*                   _java_vm;
    JNIEnv*                    _jenv;
};

//---------------------------------------------------------------------------------Java_thread_data

struct Java_thread_data
{
                                Java_thread_data            ()                                      : _zero_(this+1) {}

    Fill_zero                  _zero_;
    Java_env                   _env;
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

  //                            operator JavaVM*            ()                                      { return vm(); }
  //JavaVM*                     operator ->                 ()                                      { return vm(); }

    JavaVM*                     vm                          ();
    Java_env&                   env                         ();

    Z_NORETURN void             throw_java                  ( int return_value, const string&, const string& = "" );
    

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
    jclass                     _idispatch_jclass;

    Thread_data<Java_thread_data> _thread_data;
};

//-------------------------------------------------------------------------------Java_global_object

struct Java_global_object : Object, Non_cloneable
{
                                Java_global_object          ( Spooler*, jobject = NULL );
                               ~Java_global_object          ();

    void                        operator =                  ( jobject jo )                          { assign( jo ); }
                                operator jobject            ()                                      { return _jobject; }
    void                        assign                      ( jobject );

    Spooler*                   _spooler;
    jobject                    _jobject;
};

//-------------------------------------------------------------------------------------------------

struct Java_idispatch : Java_global_object
{
                                Java_idispatch              ( Spooler* sp, IDispatch*, const string& subclass = JAVA_IDISPATCH_CLASS );
                               ~Java_idispatch              ();

    ptr<IDispatch>             _idispatch;
};

//-----------------------------------------------------------------------------Java_module_instance
// Für Java-Objekte

struct Java_module_instance : Module_instance
{
                                Java_module_instance        ( Module* module )                      : Module_instance(module), _zero_(this+1), _jobject(_module->_spooler) {}
                               ~Java_module_instance        ()                                      { close(); }

    void                        init                        ();
    void                        load                        ();
    void                        close                       ();
    Variant                     call                        ( const string& name );
    Variant                     call                        ( const string& name, int param );
    virtual bool                name_exists                 ( const string& name );
    bool                        callable                    ()                                      { return _jobject != NULL; }
    void                        add_obj                     ( const ptr<IDispatch>& object, const string& name );


    Fill_zero                  _zero_;
    Java_vm*                   _java_vm;
    Java_env                   _env;
    Java_global_object         _jobject;

    typedef list< ptr<Java_idispatch> >  Added_objects;
    Added_objects              _added_jobjects;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
