// $Id$
#include "zschimmer.h"

#ifdef Z_COM

#include "java_odl.h"
namespace odl {
    
//-------------------------------------------------------------------------------------------------

DEFINE_GUID( LIBID_zschimmer_java, 0xED57B226, 0xCD4F, 0x490a, 0xB4, 0x7E, 0xE7, 0xD4, 0x4A, 0x89, 0x52, 0x73 );

//-----------------------------------------------------------------------------Ihas_java_class_name

DEFINE_GUID(  IID_Ihas_java_class_name, 0x748E665E, 0x6252, 0x418e, 0x88, 0x7A, 0x55, 0xB1, 0x1F, 0xD8, 0x28, 0x70 );

//-------------------------------------------------------------------Ihas_java_class_name::_methods
/*
#ifdef Z_COM

const Com_method Ihas_java_class_name::_methods[] =
{ 
   // _flags              ,     _name               , _method                                                      , _result_type, _types        , _default_arg_count
    { DISPATCH_PROPERTYGET,  0, "java_class_name"   , (Com_method_ptr)&Ihas_java_class_name::get_java_class_name   , VT_BSTR     },
    {}
};

#endif
*/
//-------------------------------------------------------------------------------------------------

} //namespace odl

#endif
