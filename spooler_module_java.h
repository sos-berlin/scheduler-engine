// $Id: spooler_module_java.h,v 1.29 2004/02/15 15:53:37 jz Exp $

#ifndef __SPOOLER_MODULE_JAVA_H
#define __SPOOLER_MODULE_JAVA_H

#include "../kram/thread_data.h"
#include "../zschimmer/java.h"

#define JAVA_IDISPATCH_CLASS "sos/spooler/Idispatch"

namespace sos {
namespace spooler {

//struct Java_vm;
struct Java_thread_data;

extern zschimmer::Thread_data<Java_thread_data> thread_data;

//---------------------------------------------------------------------------------Java_thread_data

struct Java_thread_data
{
                                Java_thread_data            ()                                      : _zero_(this+1) {}


    Fill_zero                  _zero_;

    z::java::Java_idispatch_container   _idispatch_container;        // Hält alle in einer nativen Methode erzeugten IDispatchs, bis release_objects()
};

//-----------------------------------------------------------------------Java_idispatch_stack_frame
// Der Destruktur gibt alle von nativen Methoden erzeugten Java_idispatchs wieder frei

struct Java_idispatch_stack_frame
{
                                Java_idispatch_stack_frame  ()                                      {}
                               ~Java_idispatch_stack_frame  ()                                      { thread_data->_idispatch_container.release_objects(); }
};

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
    static void                 init_java_vm                ( java::Vm* );


                                Java_module_instance        ( java::Vm*, Module* );
                               ~Java_module_instance        ()                                      { close__end(); }

    void                        close__end                  ();
    void                        init                        ();
    void                        add_obj                     ( IDispatch* object, const string& name );
    void                        load                        ();
    Variant                     call                        ( const string& name );
    Variant                     call                        ( const string& name, int param );
    virtual bool                name_exists                 ( const string& name );
    bool                        loaded                      ()                                      { return _jobject != NULL; }
    bool                        callable                    ()                                      { return _jobject != NULL; }

    void                        make_class                  ();

    virtual string              obj_name                    ()                                      { return "Java_module_instance"; }


    Fill_zero                  _zero_;
    java::Global_jobject       _jobject;

    typedef list< ptr<z::java::Java_idispatch> >  Added_objects;
    Added_objects              _added_jobjects;

    Fill_end                   _end_;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
