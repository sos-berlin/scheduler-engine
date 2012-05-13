// $Id: java_odl.h 11394 2005-04-03 08:30:29Z jz $


/*  Ersatz für java.odl für Systeme ohne COM. 
    2004 Zschimmer GmbH
*/


#ifndef __ZSCHIMMER_JAVA_ODL_H
#define __ZSCHIMMER_JAVA_ODL_H

#include "zschimmer.h"


#ifdef Z_WINDOWS

#   if defined _DEBUG
#       import "Win32/Debug/java.tlb"   rename_namespace("odl") raw_interfaces_only named_guids
#    else
#       import "Win32/Release/java.tlb" rename_namespace("odl") raw_interfaces_only named_guids
#   endif

#else

#   include "com_server.h"


namespace odl 
{

//extern const LIBID LIBID_zschimmer_java;
extern const IID   IID_Ihas_java_class_name;

//-----------------------------------------------------------------------------Ihas_java_class_name

struct Ihas_java_class_name : IUnknown
{
    DEFINE_UUIDOF( Ihas_java_class_name )

  //Z_DEFINE_GETIDSOFNAMES_AND_INVOKE

    virtual HRESULT     get_Java_class_name             ( BSTR* result ) = 0;
    virtual char*     const_java_class_name             () = 0;
};

//------------------------------------------------------------------------------Has_java_class_name
/*

struct Has_java_class_name : Ihas_java_class_name
{
    STDMETHODIMP            get_java_class_name         ( BSTR* result )                            { return string_to_bstr( const_java_class_name(), result ); }
};

*/
//-------------------------------------------------------------------------------------------------

} //namespace odl

#endif
#endif
