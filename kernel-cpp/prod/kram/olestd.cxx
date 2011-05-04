// olestd.cxx                                 (C)1997 SOS GmbH Berlin
//                                            Joacim Zschimmer
// #rev 2#

#include "precomp.h"

//#define MODULE_NAME "hostole"
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "(C)1996 SOS GmbH Berlin"

#include <stdio.h>          // sprintf


#ifdef SYSTEM_MFC
#   include <afxdisp.h>
#else
#   include <windows.h>
#endif

#include <shellapi.h>

#include "sos.h"
#include "log.h"
#include "sysxcept.h"
#include "sosprof.h"
#include "sosfield.h"
#include "sosdate.h"
#include "stdfield.h"
#include "licence.h"
#include "../file/anyfile.h"

#include <ole2ver.h>

#define INITGUIDS
#include "olestd.h"
#include "olereg.h"

using namespace std;
namespace sos {


const LCID   std_lcid = (LCID)0;   //LOCALE_SYSTEM_DEFAULT;
const Variant      empty_variant;


//--------------------------------------------------------------ostream& operator << _com_error

ostream& operator << ( ostream& s, const _com_error& x )
{
    char buffer [30];
    sprintf( buffer, "MSWIN-%08X  ", x.Error() );
    s << buffer << get_mswin_msg_text( x.Error() );
    if( x.Description() != NULL )  s << " / " << x.Description();
    return s;
}

//-------------------------------------------------------------------------------------as_bool

bool as_bool( const VARIANT& variant )
{
    return as_bool( as_string( variant ) );
}

//-----------------------------------------------------------------------------------as_double

double as_double( const VARIANT& variant )
{
    Variant v = variant;
    v.ChangeType( VT_R8 );
    return v.dblVal;
}

//-----------------------------------------------------------------------------------as_bstr_t

_bstr_t as_bstr_t( const Big_int i )
{
    return as_bstr_t( as_string( i ) );
}

//-----------------------------------------------------------------------------------as_bstr_t

_bstr_t as_bstr_t( const Ubig_int i )
{
    return as_bstr_t( as_string( i ) );
}

//-----------------------------------------------------------------------------------as_bstr_t

_bstr_t as_bstr_t( const int i )
{ 
    return as_bstr_t( (Big_int)i ); 
}

//-----------------------------------------------------------------------------------as_bstr_t

_bstr_t as_bstr_t( const uint i )
{ 
    return as_bstr_t( (Ubig_int)i ); 
}

//---------------------------------------------------------------------------------operator == 
                 
bool operator == ( const _bstr_t& bstr, const string& str )  
{ 
    return as_string(bstr) == str; 
}

//---------------------------------------------------------------------------------operator == 

bool operator == ( const _bstr_t& bstr, const char* str )    
{ 
    return bstr == as_bstr_t(str); 
}


} //namespace sos
