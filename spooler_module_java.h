// $Id: spooler_module_java.h,v 1.10 2002/11/24 15:12:50 jz Exp $

#ifndef __SPOOLER_MODULE_JAVA_H
#define __SPOOLER_MODULE_JAVA_H

#include "../kram/thread_data.h"

#define JAVA_IDISPATCH_CLASS "sos/spooler/Idispatch"

namespace sos {
namespace spooler {

struct Java_vm;
struct Java_idispatch;

//-----------------------------------------------------------------------------------------Java_env

struct Java_env : Non_cloneable
{
                              //Java_env                    ( Java_vm* vm, JNIEnv* jenv )           : _java_vm(vm), _jenv( jenv ) {}

                                operator JNIEnv*            ()                                      { return _jenv; }
    JNIEnv*                     operator ->                 ()                                      { return _jenv; }
    JNIEnv*                     env                         ()                                      { return _jenv; }

    void                        add_object                  ( Java_idispatch* );
    void                        release_objects             ()                                      { _java_idispatch_list.clear(); }

    // Einige Java-Methoden mit Fehlerprüfung:
    jclass                      find_class                  ( const string& name );
    jmethodID                   get_method_id               ( jclass, const string& name, const string& signature );
    jclass                      get_object_class            ( jobject );


    Java_vm*                   _java_vm;
    JNIEnv*                    _jenv;
    list< ptr<Java_idispatch> >_java_idispatch_list;     // Hält alle in einer native Methode erzeugten IDispatchs, bis release_objects()
};

//--------------------------------------------------------------------------------Java_object_stack
// Der Destruktur gibt alle von nativen Methoden erzeugten Java_idispatchs wieder frei

struct Java_idispatch_stack_frame
{
                                Java_idispatch_stack_frame  ( Java_env* e )                         : _java_env(e) {}
                               ~Java_idispatch_stack_frame  ()                                      { _java_env->release_objects(); }

    Java_env*                  _java_env;
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



                                Java_vm                     ( Spooler* s )                          : _zero_(this+1), _spooler(s), _log(s) {}
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
    Spooler*                   _spooler;
    Prefix_log                 _log;
    string                     _filename;                   // Dateiname der Java-VM
    string                     _work_class_dir;
    string                     _ini_class_path;
    string                     _config_class_path;
    string                     _complete_class_path;
    string                     _javac;                      // Dateiname das Java-Compilers
  //JDK1_1InitArgs             _vm_args;
    JavaVMInitArgs             _vm_args;
    vector<Option>             _options;
    JavaVM*                    _vm;
    jclass                     _idispatch_jclass;

    Thread_data<Java_thread_data> _thread_data;
};

//--------------------------------------------------------------------------------------Java_object

struct Java_object : Object, Non_cloneable
{
    Z_GNU_ONLY(                 Java_object                 ();  )                                  // Für gcc 3.2. Nicht implementiert.
                                Java_object                 ( Spooler*, jobject = NULL );
                               ~Java_object                 ();

    STDMETHODIMP                QueryInterface              ( const IID&, void** );

    void                        operator =                  ( jobject jo )                          { assign( jo ); }
                                operator jobject            ()                                      { return _jobject; }
    void                        assign                      ( jobject );
    void                        set_global                  ();

    Spooler*                   _spooler;
    jobject                    _jobject;
    bool                       _is_global;
};

//--------------------------------------------------------------------------------Java_global_object

struct Java_global_object : Java_object
{
                                Java_global_object          ( Spooler* sp, jobject jo = NULL )      : Java_object( sp, jo ) { if(jo) set_global(); }

    void                        operator =                  ( jobject jo )                          { assign( jo ); }
    void                        assign                      ( jobject jo )                          { Java_object::assign(jo); set_global(); }
};

//-------------------------------------------------------------------------------------------------

struct Java_idispatch : Java_object
{
    Z_GNU_ONLY(                 Java_idispatch              ();  )                                  // Für gcc 3.2. Nicht implementiert.
                                Java_idispatch              ( Spooler* sp, IDispatch*, const string& subclass );
                               ~Java_idispatch              ();

    ptr<IDispatch>             _idispatch;
    string                     _class_name;
};

//-----------------------------------------------------------------------------Java_module_instance
// Für Java-Objekte

struct Java_module_instance : Module_instance
{
                                Java_module_instance        ( Module* module )                      : Module_instance(module), _zero_(this+1), _jobject(_module->_spooler) {}
                               ~Java_module_instance        ()                                      { close(); }

    void                        close                       ();
    void                        init                        ();
    void                        add_obj                     ( const ptr<IDispatch>& object, const string& name );
    void                        load                        ();
    Variant                     call                        ( const string& name );
    Variant                     call                        ( const string& name, int param );
    virtual bool                name_exists                 ( const string& name );
    bool                        callable                    ()                                      { return _jobject != NULL; }

    void                        make_class                  ();


    Fill_zero                  _zero_;
    Java_vm*                   _java_vm;
    Java_env*                  _env;
    Java_global_object         _jobject;

    typedef list< ptr<Java_idispatch> >  Added_objects;
    Added_objects              _added_jobjects;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
