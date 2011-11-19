// $Id: z_java.h 13870 2010-06-03 15:29:33Z jz $

#ifndef __Z_JAVA_H
#define __Z_JAVA_H

// Include-Path für jni.h (z.B.):
// c:\Programme\Borland\jbuilder5\jdk1.3\include
// c:\Programme\Borland\jbuilder5\jdk1.3\include\win32
#include <jni.h>

#include "../zschimmer/zschimmer.h"
#include "../zschimmer/com.h"
#include "../zschimmer/z_com.h"

/*
void*   get_my_data         ( JNIEnv*, jobject );
void    set_java_exception  ( JNIEnv*, const char* what );
void    set_java_exception  ( JNIEnv*, const exception& );
void    set_java_exception  ( JNIEnv*, const _com_error& );
*/

namespace zschimmer {
namespace javabridge {

//------------------------------------------------------------------------------------------Java_vm

struct Java_vm
{
    JNIEnv*                     env                         ();

    JavaVM*                    _vm;
}

//-------------------------------------------------------------------------------Registered_methods

struct Registered_methods, Non_cloneable
{
                                Registered_method           ( Java_vm* vm )                         : _fill_zero(this+1), { _java_vm = vm; }
                               ~Registered_method           ()                                      { close(); }

    void                        set_class                   ( jclass cls );
    void                        set_class                   ( const string& class_name );

    void                        add                         ( const string& method_name, const string& signature, void* function );
    void                        close                       ();

    Java_vm*                   _java_vm;
    jclass                     _jclass;
};

//-------------------------------------------------------------------------------------------------

} // namespace javabridge
} // namespace zschimmer

#endif
