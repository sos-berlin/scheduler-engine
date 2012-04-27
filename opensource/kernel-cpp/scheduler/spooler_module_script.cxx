#include "spooler.h"

#include "../kram/sos_java.h"
#include "../zschimmer/java.h"

using namespace std;
using namespace zschimmer::javabridge;

namespace sos {
namespace scheduler {

//------------------------------------------------Java_module_script_instance::Java_module_instance

Java_module_script_instance::Java_module_script_instance( Module* module )
: 
    Java_module_instance(module)
{
}
//----------------------------------------------------------------Java_module_script_instance::init

void Java_module_script_instance::init()
{
    Env         env;
    Local_frame local_frame ( 10 );
    Java_idispatch_stack_frame stack_frame;

    Module_instance::init();

    _java_class = env.find_class( java_adapter_job() );
    jmethodID method_id = java_method_id( "<init>(Ljava/lang/String;Ljava/lang/String;)V" );   // Konstruktor
    if( !method_id )  env.throw_java( "GetMethodID", java_adapter_job() );

    jstring language = env.jstring_from_string(_module->_language);
    jstring code = env.jstring_from_string(_module->read_source_script());

    assert( _jobject == NULL );
    _jobject = env->NewObject( _java_class, method_id, language, code );
    if( !_jobject || env->ExceptionCheck() )  env.throw_java( java_adapter_job() + " Konstruktor" );

}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
