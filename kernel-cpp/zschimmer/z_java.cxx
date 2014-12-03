// $Id: z_java.cxx 11394 2005-04-03 08:30:29Z jz $

#include "zschimmer.h"
#include "z_java.h"


namespace zschimmer {
namespace java {

//------------------------------------------------------------Registered_method::~Registered_method

Registered_method::~Registered_method()
{
    close();
}

//------------------------------------------------------------Registered_method::~Registered_method

void Registered_method::close()
{
    int ret = _jenv->UnregisterNatives( _jclass );
    if( ret < 0 )  _vm->throw_java( ret, "RegisterNatives" );
}

//---------------------------------------------------------------------------Registered_method::reg

void Registered_method::reg( jclass cls, const string& method_name, const string& signature, void* function )
{
    _jclass = cls;

    JNINativeMethod m;

    m.name      = method_name.c_str();
    m.signature = signature.c_str();
    m.fnPtr     = function;

    int ret = _jenv->RegisterNatives( _jclass, &m, 1 );
    if( ret < 0 )  _vm->throw_java( ret, "RegisterNatives" );
}

//-------------------------------------------------------------------------------------------------

} //nameepace java
} //namespace zschimmer
