// $Id: spooler_module_java.h,v 1.21 2003/03/18 10:44:19 jz Exp $

#ifndef __SPOOLER_MODULE_JAVA_H
#define __SPOOLER_MODULE_JAVA_H

#include "../kram/thread_data.h"
#include "../zschimmer/java.h"

#define JAVA_IDISPATCH_CLASS "sos/spooler/Idispatch"

namespace sos {
namespace spooler {

//struct Java_vm;
struct Java_idispatch;
struct Java_thread_data;

extern zschimmer::Thread_data<Java_thread_data> thread_data;

//---------------------------------------------------------------------------------Java_thread_data

struct Java_thread_data
{
                                Java_thread_data            ()                                      : _zero_(this+1) {}

    void                        add_object                  ( Java_idispatch* );
    void                        release_objects             ()                                      { _java_idispatch_list.clear(); }

    Fill_zero                  _zero_;

    list<ptr<Java_idispatch> > _java_idispatch_list;        // Hält alle in einer nativen Methode erzeugten IDispatchs, bis release_objects()
};

//-----------------------------------------------------------------------Java_idispatch_stack_frame
// Der Destruktur gibt alle von nativen Methoden erzeugten Java_idispatchs wieder frei

struct Java_idispatch_stack_frame
{
                                Java_idispatch_stack_frame  ()                                      {}
                               ~Java_idispatch_stack_frame  ()                                      { thread_data->release_objects(); }
};

//------------------------------------------------------------------------------------Java_idispatch

struct Java_idispatch : java::Global_jobject, Object
{
    Z_GNU_ONLY(                 Java_idispatch              ();  )                                  // Für gcc 3.2. Nicht implementiert.
                                Java_idispatch              ( java::Vm* vm, IDispatch*, const string& subclass );
                               ~Java_idispatch              ();

    ptr<IDispatch>             _idispatch;
    string                     _class_name;
};

//------------------------------------------------------------------------------------------Java_vm
/*
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
    JavaVMInitArgs             _vm_args;
    vector<Option>             _options;
    JavaVM*                    _vm;
    zschimmer::Thread_data<Java_thread_data> _thread_data;

    string                     _filename;                   // Dateiname der Java-VM
    string                     _work_class_dir;
    string                     _ini_class_path;
    string                     _config_class_path;
    string                     _complete_class_path;
    string                     _javac;                      // Dateiname das Java-Compilers
  //JDK1_1InitArgs             _vm_args;
    jclass                     _idispatch_jclass;
};
*/
//--------------------------------------------------------------------------------------Java_object
/*
struct Java_object : Object, Non_cloneable
{
    Z_GNU_ONLY(                 Java_object                 ();  )                                  // Für gcc 3.2. Nicht implementiert.
                                Java_object                 ( Spooler*, jobject = NULL );
                               ~Java_object                 ();

    STDMETHODIMP                QueryInterface              ( const IID&, void** );

    void                        operator =                  ( jobject jo )                          { assign( jo ); }
                                operator jobject            ()                                      { return _jobject; }
    virtual void                assign                      ( jobject );
    void                        set_global                  ();

    Spooler*                   _spooler;
    jobject                    _jobject;
    bool                       _is_global;
};

//-------------------------------------------------------------------------------Java_global_object

struct Java_global_object : Java_object
{
                                Java_global_object          ( Spooler* sp, jobject jo = NULL )      : Java_object( sp, jo )  { if(jo) set_global(); }

    void                        operator =                  ( jobject jo )                          { assign( jo ); }
    void                        assign                      ( jobject jo )                          { Java_object::assign(jo); set_global(); }
};
*/
//--------------------------------------------------------------------------------Java_local_object
/*
struct Java_local_object : Java_object
{
                                Java_local_object           ( Spooler* sp, jobject jo = NULL )      : Java_object( sp, NULL )  { assign( jo ); }
                               ~Java_local_object           ()                                      { assign( NULL ); }

    void                        operator =                  ( jobject jo )                          { assign( jo ); }
    void                        assign                      ( jobject );
};
*/
//-------------------------------------------------------------------------------------------------
/*
struct Java_idispatch : Java_object
{
    Z_GNU_ONLY(                 Java_idispatch              ();  )                                  // Für gcc 3.2. Nicht implementiert.
                                Java_idispatch              ( Spooler* sp, IDispatch*, const string& subclass );
                               ~Java_idispatch              ();

    ptr<IDispatch>             _idispatch;
    string                     _class_name;
};
*/
//-----------------------------------------------------------------------------Java_module_instance
// Für Java-Objekte

struct Java_module_instance : Module_instance, java::Has_vm
{
                                Java_module_instance        ( java::Vm*, Module* );
                               ~Java_module_instance        ()                                      { close(); }

    void                        close                       ();
    void                        init                        ();
    void                        add_obj                     ( const ptr<IDispatch>& object, const string& name );
    void                        load                        ();
    Variant                     call                        ( const string& name );
    Variant                     call                        ( const string& name, int param );
    virtual bool                name_exists                 ( const string& name );
    bool                        loaded                      ()                                      { return _jobject != NULL; }
    bool                        callable                    ()                                      { return _jobject != NULL; }

    void                        make_class                  ();


    Fill_zero                  _zero_;
    java::Global_jobject       _jobject;

    typedef list< ptr<Java_idispatch> >  Added_objects;
    Added_objects              _added_jobjects;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
