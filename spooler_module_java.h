// $Id: spooler_module_java.h,v 1.1 2002/11/01 09:27:11 jz Exp $

#ifndef __SPOOLER_MODULE_JAVA_H
#define __SPOOLER_MODULE_JAVA_H

#include "../kram/thread_data.h"

namespace sos {
namespace spooler {

//---------------------------------------------------------------------------------Java_thread_data

struct Java_thread_data
{
    JNIEnv*                    _env;
};

//------------------------------------------------------------------------------------------Java_vm

struct Java_vm                  // Java virtual machine
{
                                Java_vm                     ()                                      : _zero_(this+1) {}
                               ~Java_vm                     ()                                      { close(); }

    void                        init                        ();
    void                        close                       ();

    JNIEnv*                     attach_thread               ();
    void                        detach_thread               ();

                                operator JavaVM*            ()                                      { return vm(); }
    JavaVM*                     operator ->                 ()                                      { return vm(); }

    JavaVM*                     vm                          ();
    JNIEnv*                     env                         ();

    Z_NORETURN void             throw_java                  ( const string& text );
    
  //ptr<Java_class>             class                       ( const string& name );



    void                        check_for_exception         ();


    Fill_zero                  _zero_;
    string                     _filename;
    string                     _class_path;
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
    JNIEnv*                    _env;
    jobject                    _object;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
