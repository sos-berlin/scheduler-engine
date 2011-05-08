// $Id$

#ifndef __SOS_JAVA_H
#define __SOS_JAVA_H

#include "sosprof.h"
#include "../zschimmer/zschimmer.h"
#include "../zschimmer/java.h"

namespace sos {

//------------------------------------------------------------z::ptr<z::javabridge::Vm> get_java_vm

inline zschimmer::ptr<z::javabridge::Vm> get_java_vm( bool start = true )
{
    zschimmer::ptr<z::javabridge::Vm> vm = zschimmer::javabridge::Vm::get_vm( false );

    if( !vm->running()  &&  !vm->sos_initialized() )    
    {
        if( read_profile_bool( "", "java", "debug", false ) )  vm->set_debug( true );

        vm->set_javac_filename( z::subst_env( read_profile_string( "", "java"   , "javac"      , vm->javac_filename() ) ) );
        vm->set_filename      ( z::subst_env( read_profile_string( "", "java"   , "vm"         , vm->filename() ) ) );
        vm->prepend_class_path( z::subst_env( read_profile_string( "", "java"   , "class_path" ) ) );
        vm->new_instances     (               read_profile_string( "", "java"   , "instances"  ) );
        vm->set_options       ( z::subst_env( read_profile_string( "", "java"   , "options"    ) ) );

        vm->set_sos_initialized();  // Besser in Sos_static unterbringen (aber ohne java.h einzuziehen, nicht so einfach.)
    }

    if( start )  vm->start();

    return vm;
}

//----------------------------------------------------------------------------------java_is_running

inline bool java_is_running()
{
    return zschimmer::javabridge::Vm::get_vm( false )->running();
}

//-------------------------------------------------------------------------------------------------

} //namespace sos

#endif
