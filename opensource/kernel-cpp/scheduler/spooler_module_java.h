// $Id: spooler_module_java.h 14094 2010-10-20 11:28:15Z jz $

#ifndef __SPOOLER_MODULE_JAVA_H
#define __SPOOLER_MODULE_JAVA_H

#include "../kram/thread_data.h"
#include "../zschimmer/java.h"
#include "../zschimmer/java_com.h"

#define JAVA_IDISPATCH_CLASS "sos/spooler/Idispatch"

namespace sos {
namespace scheduler {

struct Java_thread_data;

//---------------------------------------------------------------------------------Java_thread_data

struct Java_thread_data
{
                                Java_thread_data            ()                                      : _zero_(this+1) {}


    Fill_zero                  _zero_;

    z::javabridge::Java_idispatch_container   _idispatch_container;        // Hält alle in einer nativen Methode erzeugten IDispatchs, bis release_objects()
};

//-----------------------------------------------------------------------Java_idispatch_stack_frame
// Der Destruktur gibt alle von nativen Methoden erzeugten Java_idispatchs wieder frei

struct Java_idispatch_stack_frame
{
                                Java_idispatch_stack_frame  ()                                      {}
                               ~Java_idispatch_stack_frame  ();
};

//-----------------------------------------------------------------------------Java_module_instance
// Für Java-Objekte

struct Java_module_instance : Module_instance
{
    static void                 init_java_vm                ( javabridge::Vm* );


                                Java_module_instance        ( Module* );
                               ~Java_module_instance        ()                                      { close__end(); }

    void                        close__end                  ();
    void                        init                        ();
    void                        add_obj                     ( IDispatch* object, const string& name );
    void                        check_api_version           ();
    bool                        load                        ();
    Variant                     call                        ( const string& name );
    Variant                     call                        ( const string& name, const Variant& param, const Variant& );
    virtual bool                name_exists                 ( const string& name );
    bool                        loaded                      ()                                      { return _jobject != NULL; }
    bool                        callable                    ()                                      { return _jobject != NULL; }

    void                        make_class                  ();
    jmethodID                   java_method_id              ( const string& name );                 // in spooler_module_java.cxx

    virtual string              obj_name                    () const                                { return "Java_module_instance"; }


    Fill_zero                  _zero_;
    javabridge::Global_jobject _jobject;

    javabridge::global_jobject<jclass> _java_class;
    typedef map<string,jmethodID>  Method_map;
    Method_map                 _method_map;

    typedef list< ptr<z::javabridge::Java_idispatch> >  Added_objects;
    Added_objects              _added_jobjects;

    Fill_end                   _end_;
};

//----------------------------------------------------------------------Java_module_script_instance
// Für Scripting-Objekte via javax

struct Java_module_script_instance : Java_module_instance
{
                                Java_module_script_instance ( Module* );
    void                        init                        ();
    string                      java_adapter_job            () const                            { return "sos/spooler/jobs/ScriptAdapterJob"; }
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
