// $Id: com.cxx 13691 2008-09-30 20:42:20Z jz $

#include "zschimmer.h"

#ifndef _WIN32

#define _GNU_SOURCE 1   // wcscasecmp() gibt's nur in Gnu wchar.h
#include <wchar.h>

#include <string>
#include <wchar.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

#include "threads.h"
#include "log.h"
#include "com.h"

#include "z_com.h"
#include "com_server.h"
#include <dlfcn.h>

// Einige CLSID:
#include "perl_scripting_engine.h"
#include "spidermonkey_scripting_engine.h"

//------------------------------------------------------------------------------------------hostole
//#include "../hostole/hostole.h":

DEFINE_GUID( CLSID_Type_info                        , 0x9F716A0C, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Type_param                       , 0x9F716A0D, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Field_type                       , 0x9F716A10, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Record_type                      , 0x9F716A0E, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Field_descr                      , 0x9F716A0F, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Dyn_obj                          , 0x9F716A06, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_File                             , 0x9F716A03, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Global                           , 0x9F716A05, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Variable                         , 0x9F716A21, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Variables                        , 0x9F716A23, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Variables_enumerator             , 0x9F716A23, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Variable2                        , 0x9F716A27, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Variables2                       , 0x9F716A29, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Variables2_enumerator            , 0x9F716A31, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Variables2_idispatch_enumerator  , 0x9F716A25, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x02 );
DEFINE_GUID( CLSID_Factory_text_constructor         , 0x6EB42D32, 0xA6BF, 0x11D0, 0x83, 0x81, 0x00, 0xA0, 0xC9, 0x1E, 0xF7, 0xB9 );
DEFINE_GUID( CLSID_Factory_rtf_constructor          , 0x6EB42D34, 0xA6BF, 0x11D0, 0x83, 0x81, 0x00, 0xA0, 0xC9, 0x1E, 0xF7, 0xB9 );
DEFINE_GUID( CLSID_Factory_processor                , 0x9F716A41, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Rerun                            , 0x82AD717A, 0xA075, 0x400b, 0x86, 0x73, 0xB6, 0x57, 0x8F, 0xCB, 0xD8, 0x35 );
DEFINE_GUID( CLSID_Script_object                    , 0x9F716A43, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );

extern "C" BOOL WINAPI      hostole_DllMain( HANDLE hInst, DWORD ul_reason_being_called, void* )            COM_LINKAGE;
extern "C" HRESULT APIENTRY hostole_DllGetClassObject( const CLSID& rclsid, const IID& riid, void** ppv )   COM_LINKAGE;

//-------------------------------------------------------------------------------------------------

using namespace std;
using namespace zschimmer;
using namespace zschimmer::com;

typedef HRESULT APIENTRY (*DllGetClassObject_func)( const CLSID&, const IID&, void** );
typedef BOOL WINAPI      (*DllMain_func          )( HANDLE, DWORD, void* );

//----------------------------------------------------------------------------------Com_thread_data

struct Com_thread_data
{
                                Com_thread_data         ()                                          : _zero_(this+1) {}

    Fill_zero                  _zero_;
    ptr<IErrorInfo>            _current_errorinfo;
    long32                     _coinitialize_count;
    int                        _last_error;           // Für SetLastError() und GetLastError()
};

//-------------------------------------------------------------------------------------------static

static Thread_data<Com_thread_data> thread_data;
static Mutex                    get_class_mutex         ( "com_get_class" );
static Com_module_params*       com_module_params_ptr   = NULL;
static Com_module_params        com_module_params;

//static map<string,void*>    loaded_modules;         // Mit dlopen() geladene Module

//--------------------------------------------------------------------------------------------const

extern DEFINE_GUID( IID_IClassFactory       , 0x00000001, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 );
extern DEFINE_GUID( IID_ITypeInfo           , 0x00020401, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 );
extern DEFINE_GUID( IID_ITypeLib            , 0x00020402, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 );
extern DEFINE_GUID( IID_IEnumVARIANT        , 0x00020404, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 );
extern DEFINE_GUID( IID_IErrorInfo          , 0x1CF2B120, 0x547D, 0x101B, 0x8E, 0x65, 0x08, 0x00, 0x2B, 0x2B, 0xD1, 0x19 );
extern DEFINE_GUID( IID_ISupportErrorInfo   , 0xDF0B3D60, 0x548F, 0x101B, 0x8E, 0x65, 0x08, 0x00, 0x2B, 0x2B, 0xD1, 0x19 );
extern DEFINE_GUID( IID_ICreateErrorInfo    , 0x22F03340, 0x547D, 0x101B, 0x8E, 0x65, 0x08, 0x00, 0x2B, 0x2B, 0xD1, 0x19 );
extern DEFINE_GUID( IID_IProvideClassInfo   , 0xB196B283, 0xBAB4, 0x101A, 0xB6, 0x9C, 0x00, 0xAA, 0x00, 0x34, 0x1D, 0x07 );

//--------------------------------------------------------------------------------------error_codes

static Message_code_text error_codes[] =
{
    { "COM-80004001", "E_NOTIMPL - Funktion nicht implementiert" },
    { "COM-80004002", "E_NOINTERFACE" },
    { "COM-80004003", "E_POINTER" },
    { "COM-80004004", "E_ABORT" },
    { "COM-80004005", "E_FAIL" },
    { "COM-8000FFFF", "E_UNEXPECTED" },
    { "COM-80020001", "DISP_E_UNKNOWNINTERFACE" },
    { "COM-80020003", "DISP_E_MEMBERNOTFOUND" },
    { "COM-80020004", "DISP_E_PARAMNOTFOUND" },
    { "COM-80020005", "DISP_E_TYPEMISMATCH" },
    { "COM-80020006", "DISP_E_UNKNOWNNAME" },
    { "COM-80020008", "DISP_E_BADVARTYPE" },
    { "COM-80020009", "DISP_E_EXCEPTION" },
    { "COM-8002000A", "DISP_E_OVERFLOW" },
    { "COM-8002000B", "DISP_E_BADINDEX" },
    { "COM-8002000E", "DISP_E_BADPARAMCOUNT  Invalid number of parameters" },
    { "COM-8002802B", "TYPE_E_ELEMENTNOTFOUND" },
    { "COM-80040110", "CLASS_E_NOAGGREGATION" },
    { "COM-80040111", "CLASS_E_CLASSNOTAVAILABLE" },
    { "COM-80040112", "CLASS_E_NOTLICENSED" },
    { "COM-80040151", "REGDB_E_WRITEREGDB" },
    { "COM-80040154", "REGDB_E_CLASSNOTREG" },
    { "COM-800401F0", "CO_E_NOTINITIALIZED" },
    { "COM-800401F8", "CO_E_DLLNOTFOUND" },
    { "COM-800401F9", "CO_E_ERRORINDLL" },
    { "COM-80070005", "E_ACCESSDENIED" },
    { "COM-80070006", "E_HANDLE" },
    { "COM-8007000E", "E_OUTOFMEMORY" },
    { "COM-80070057", "E_INVALIDARG" },
    { NULL }
};

//----------------------------------------------------------------------------------Com_class_entry

struct Com_module
{
    const char*                   _module_filename;
    const char*                   _dllmain_name;
    DllMain_func                  _DllMain;
    const char*                   _dllgetclassobject_name;
    DllGetClassObject_func        _DllGetClassObject;

    void*                         _module;
    bool                          _DllMain_called;
};

//#if !defined Z_HPUX && !defined Z_AIX  // HP-UX kennt kein "weak" und will im Scheduler hostole einbinden.
#ifdef Z_LINK_STATIC
    static Com_module hostware_module = { "libhostole.so", "hostole_DllMain", hostole_DllMain, "hostole_DllGetClassObject", hostole_DllGetClassObject };
#else
    static Com_module hostware_module = { "libhostole.so", "hostole_DllMain", NULL, "hostole_DllGetClassObject", NULL };
#endif

#ifdef Z_LINK_STATIC
    static Com_module sosperl_module       = { "libsosperlscript.so", "sosperl_DllMain", sosperl_DllMain , "sosperl_DllGetClassObject", sosperl_DllGetClassObject };
# else
    static Com_module sosperl_module       = { "libsosperlscript.so", "DllMain"        , NULL            , "DllGetClassObject"        , NULL                      };   // Dynamisch einbinden
#endif

#if defined Z_LINK_STATIC
    static Com_module spidermonkey_module  = { "libspidermonkey.so" , "spidermonkey_DllMain", spidermonkey_DllMain, "spidermonkey_DllGetClassObject", spidermonkey_DllGetClassObject };
# else
    static Com_module spidermonkey_module  = { "libspidermonkey.so" , "DllMain"             , NULL                , "DllGetClassObject"             , NULL                           };
#endif


struct Com_class_entry
{
    CLSID                      _clsid;
    const char*                _class_name;
    Com_module*                _module;
};

static CLSID null_clsid;

static Com_class_entry clsid_table[] =
{
    { CLSID_Type_info                 , "Hostware.Type_info"                  , &hostware_module },
    { CLSID_Type_param                , "Hostware.Type_param"                 , &hostware_module },
    { CLSID_Field_type                , "Hostware.Field_type"                 , &hostware_module },
    { CLSID_Record_type               , "Hostware.Record_type"                , &hostware_module },
    { CLSID_Field_descr               , "Hostware.Field_descr"                , &hostware_module },
    { CLSID_Dyn_obj                   , "Hostware.Dyn_obj"                    , &hostware_module },
    { CLSID_File                      , "Hostware.File"                       , &hostware_module },
    { CLSID_Global                    , "Hostware.Global"                     , &hostware_module },
    { CLSID_Variable                  , "Hostware.Variable"                   , &hostware_module },
    { CLSID_Variables                 , "Hostware.Variables"                  , &hostware_module },
    { CLSID_Rerun                     , "Hostware.Rerun"                      , &hostware_module },
    { CLSID_Factory_text_constructor  , "Hostware.Factory_text_constructor"   , &hostware_module },
    { CLSID_Factory_rtf_constructor   , "Hostware.Factory_rtf_constructor"    , &hostware_module },
    { CLSID_Factory_processor         , "Hostware.Factory_processor"          , &hostware_module },
//#if !defined Z_HPUX || defined __IA64__  // HP-UX kennt kein "weak" und will im Spooler hostole einbinden.
    { CLSID_Spidermonkey              , "Spidermonkey"                        , &spidermonkey_module  },
    { CLSID_Spidermonkey              , "JavaScript"                          , &spidermonkey_module  },
//#endif

    { CLSID_PerlScript                , "PerlScript"                          , &sosperl_module  },
    { CLSID_PerlScript                , "Perl"                                , &sosperl_module  },

    { null_clsid                      , NULL                                  , NULL }
};

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( zschimmer_com )
{
    add_message_code_texts( error_codes );
}

//-----------------------------------------------------------------------------------CoInitializeEx

HRESULT CoInitializeEx( void* p, DWORD )
{ 
    return CoInitialize( p );
}

//-------------------------------------------------------------------------------------CoInitialize

HRESULT CoInitialize( void* )
{ 
    if( InterlockedIncrement( &thread_data->_coinitialize_count ) == 1 )
    {
        //add_message_code_texts( error_codes );
        return S_OK;
    }
    else
        return FALSE; 
}

//-----------------------------------------------------------------------------------CoUninitialize

void CoUninitialize()
{ 
    if( InterlockedDecrement( &thread_data->_coinitialize_count ) == 0 )
    {
        // ?
    }
}

//-----------------------------------------------------------------------------------CoTaskMemAlloc

void* CoTaskMemAlloc( ULONG size )
{
    void* result;

    if( static_com_context_ptr )  
    {
        result = static_com_context_ptr->CoTaskMemAlloc( size );
    }
    else
    {
        result = malloc( size );
        //Z_LOG( "CoTaskMemAlloc(" << size << ") ==> " << result << "\n" );
    }

    return result;
}

//------------------------------------------------------------------------------------CoTaskMemFree

void CoTaskMemFree( void* p )
{
    if( static_com_context_ptr )  
    {
        static_com_context_ptr->CoTaskMemFree( p );
    }
    else
    {
        //Z_LOG( "CoTaskMemFree(" << (void*)p << ")\n" );
        free( p );
    }
}

//---------------------------------------------------------------------------class_entry_from_clsid

static const Com_class_entry* class_entry_from_clsid( const CLSID& clsid )
{
    if( !clsid_table->_class_name )  fprintf( stderr, "*** Die statischen Variablen sind nicht initialisiert! ***\n" );

    for( Com_class_entry* c = clsid_table; c->_class_name; c++ )  
    {
        if( c->_clsid == clsid )  return c;
    }

    return NULL;
}

//----------------------------------------------------------------------------------StringFromCLSID

WINOLEAPI StringFromCLSID( REFCLSID clsid, LPOLESTR* str )
{
    *str = NULL;
 
    HRESULT  hr;
    OLECHAR  buffer [38+1];

    buffer[0] = 0;
    
    hr = StringFromGUID2( clsid, buffer, sizeof buffer );
    if( FAILED(hr) )  return hr;

    int len1 = wcslen( buffer );

    *str = (OLECHAR*)CoTaskMemAlloc( len1 * sizeof(OLECHAR) );
    memcpy( *str, buffer, len1 * sizeof(OLECHAR) );

/*
    const Com_class_entry* c = class_entry_from_clsid( clsid );
    if( !c )  
    {
        OLECHAR buffer [38+1];
        buffer[0] = 0;
        StringFromGUID2( clsid, string_from_ole(buffer), sizeof buffer );
        Z_LOG( "StringFromCLSID(" << buffer << ") => REGDB_E_CLASSNOTREG\n" );
        return REGDB_E_CLASSNOTREG;  //CLASS_E_CLASSNOTAVAILABLE;
    }

    int len = strlen( c->_class_name ) + 1;

    *str = (OLECHAR*)CoTaskMemAlloc( len * sizeof(OLECHAR) );
    MultiByteToWideChar( CP_ACP, 0, c->_class_name, len, *str, len );
*/
    return S_OK;
}

//----------------------------------------------------------------------------------CLSIDFromProgID

HRESULT CLSIDFromProgID( const OLECHAR* progid_w, CLSID* clsid )
{
    string progid = string_from_ole( progid_w );

    *clsid = CLSID_NULL;

    if( !clsid_table->_class_name )  fprintf( stderr, "*** Die statischen Variablen sind nicht initialisiert! ***\n" );
    Com_class_entry* c;
    for( c = clsid_table; c->_class_name; c++ )  if( stricmp( progid.c_str(), c->_class_name ) == 0 )  break;

    if( !c->_class_name )  
    {
        fprintf( stderr, "CLSIDFromProgID(\"%s\") => REGDB_E_CLASSNOTREG\n", progid.c_str() );
        return REGDB_E_CLASSNOTREG;
    }

    *clsid = c->_clsid;

    return NOERROR;
}

//----------------------------------------------------------------------------------StringFromGUID2

int StringFromGUID2( const GUID& guid, OLECHAR* str, int size )
{
    // Liefert z.B. {00000000-0000-0000-0000-000000000000}
    char buffer [38+1];

    if( size < (int)sizeof buffer )  return 0;

    int len = sprintf( buffer, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                               (unsigned int)guid.Data1,
                               (unsigned short)guid.Data2,
                               (unsigned short)guid.Data3,
                               (unsigned char)guid.Data4[0], (unsigned char)guid.Data4[1], 
                               (unsigned char)guid.Data4[2], (unsigned char)guid.Data4[3], 
                               (unsigned char)guid.Data4[4], (unsigned char)guid.Data4[5], 
                               (unsigned char)guid.Data4[6], (unsigned char)guid.Data4[7] );


    assert( len < (int)sizeof buffer );

    MultiByteToWideChar( CP_ACP, 0, buffer, len+1, str, size );

    return len;
}

//----------------------------------------------------------------------------------CLSIDFromString

WINOLEAPI CLSIDFromString( const OLECHAR* name_w, CLSID* clsid )
{
    return Com_set_error( "CLSIDFromString nicht implementiert" );
/*
    string name = string_from_ole( name_w );
    const char* p = name.c_str();
    
    if( *p++ != '{' )  return E_FAIL;

    int count = sscanf( p, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                            (unsigned int)guid.Data1,
                            (unsigned short)guid.Data2,
                            (unsigned short)guid.Data3,
                            (unsigned char)guid.Data4[0], (unsigned char)guid.Data4[1], 
                            (unsigned char)guid.Data4[2], (unsigned char)guid.Data4[3], 
                            (unsigned char)guid.Data4[4], (unsigned char)guid.Data4[5], 
                            (unsigned char)guid.Data4[6], (unsigned char)guid.Data4[7] );
    sscanf
*/
}

//---------------------------------------------------------------------------------CoCreateInstance

HRESULT CoCreateInstance( const CLSID& clsid, IUnknown* outer, DWORD dwClsContext, const IID& iid, void** p )
{
    HRESULT         hr;
    IClassFactory*  cf       = NULL;
    void*           void_ptr = NULL;
    
    hr = CoGetClassObject( clsid, dwClsContext, NULL, IID_IClassFactory, &void_ptr ); 
    if( FAILED(hr) )  return hr;

    cf = static_cast<IClassFactory*>( void_ptr );

    hr = cf->CreateInstance( outer, iid, p ) ;

    cf->Release(); 

    return hr;
}

//-------------------------------------------------------------------------------CoCreateInstanceEx

HRESULT CoCreateInstanceEx( const CLSID& clsid, IUnknown* outer, DWORD context, COSERVERINFO* server_info, ulong32 count, MULTI_QI* query_interfaces )
{
    if( server_info )  return E_FAIL;
/*
    {
        if( server_info->dwReserved1 
         || server_info->dwReserved2
         || server_info->pAuthInfo 

    }
*/

    HRESULT         hr;
    IClassFactory*  cf;
    void*           void_ptr = NULL;
    int             i;
    
    hr = CoGetClassObject( clsid, context, NULL, IID_IClassFactory, &void_ptr ); 
    if( FAILED(hr) )  return hr;

    cf = static_cast<IClassFactory*>( void_ptr );


    for( i = 0; i < count; i++ )  query_interfaces[i].pItf = NULL, query_interfaces[i].hr = E_FAIL;

    for( i = 0; i < count; i++ )
    {
        void_ptr = NULL;
        query_interfaces[i].hr   = cf->CreateInstance( outer, *query_interfaces[i].pIID, &void_ptr );
        query_interfaces[i].pItf = static_cast<IUnknown*>( void_ptr );

        if( FAILED(query_interfaces[i].hr) )
        {
            if( query_interfaces[i].hr == E_NOINTERFACE )  
            {
                hr = E_NOINTERFACE;
            }
            else
            {
                hr = query_interfaces[i].hr;
                break;
            }
        }
        else
            if( hr == E_NOINTERFACE )  hr = CO_S_NOTALLINTERFACES;      // Wenigstens ein Interface ist da (kein Fehler).
    }

    cf->Release(); 

    if( FAILED(hr) )
    {
        for( i = 0; i < count; i++ )  if( query_interfaces[i].pItf )  query_interfaces[i].pItf->Release(), query_interfaces[i].pItf = NULL;
    }

    return hr;
}

//---------------------------------------------------------------------------------CoGetClassObject

HRESULT CoGetClassObject( const CLSID& clsid, DWORD dwClsContext, COSERVERINFO*, const IID& iid, void** p )
{
    *p = NULL;

    const Com_class_entry* e = class_entry_from_clsid( clsid );
    if( !e )  
    {
        fprintf( stderr, "CoGetClassObject(\"%s\")  CLSID nicht registriert\n", string_from_clsid( clsid ).c_str() );
        return REGDB_E_CLASSNOTREG;
    }

    Com_module* m = e->_module;

    if( !m )
    {
        fprintf( stderr, "CoGetClassObject(\"%s\")  Modul nicht eingtragen\n", string_from_clsid( clsid ).c_str() );
        return REGDB_E_CLASSNOTREG;
    }

    Z_MUTEX( get_class_mutex )
    {
        if( !m->_DllGetClassObject )
        {
            if( !m->_module_filename  ||  !m->_module_filename[0] )  return CO_E_DLLNOTFOUND;

            string module_filename = m->_module_filename;
            // Auf HP-UX wirkt LD_LIBRARY_PATH nicht. Deshalb kann der Moduldateiname über eine Umgebungsvariable eingestellt werden:
            if( const char* e = getenv( replace_regex( module_filename, "\\.", "_" ).c_str() ) )  module_filename = e;

            Z_LOG( "dlopen(\"" << module_filename << "\",RTLD_LAZY|RTLD_GLOBAL)\n" );
            m->_module = dlopen( module_filename.c_str(), RTLD_LAZY | RTLD_GLOBAL );
            if( !m->_module )  
            {
                string errtext = dlerror();
                fprintf( stderr, "dlopen(\"%s\"): %s\n", module_filename.c_str(), errtext.c_str() ); 
                Z_LOG( errtext << "\nLD_LIBRARY_PATH=" << getenv("LD_LIBRARY_PATH") << "\n" ); 
                return CO_E_DLLNOTFOUND; 
            }

            Z_LOG( "dlsym(\"" << m->_dllmain_name << "\")\n" );
            m->_DllMain = (DllMain_func)dlsym( m->_module, m->_dllmain_name );
            if( !m->_DllMain )  
            { 
                string errtext = dlerror();
                fprintf( stderr, "dlsym(\"%s\",\"%s\"): %s\n", module_filename.c_str(), m->_dllmain_name, errtext.c_str() ); 
                Z_LOG( errtext << "\n" ); 
                dlclose(m->_module); 
                m->_module=NULL; 
                return CO_E_DLLNOTFOUND; 
            }

            Z_LOG( "dlsym(\"" << m->_dllgetclassobject_name << "\")\n" );
            m->_DllGetClassObject = (DllGetClassObject_func)dlsym( m->_module, m->_dllgetclassobject_name );
            if( !m->_DllGetClassObject  )  
            { 
                string errtext = dlerror();
            //fprintf( stderr, "dlsym(\"%s\",\"%s\"): %s\n", module_filename.c_str(), m->_dllgetclassobject_name, errtext.c_str() ); 
                Z_LOG( dlerror() << "\n" ); 
                dlclose( m->_module );
                m->_module=NULL; 
                return CO_E_DLLNOTFOUND; 
            }
        }

        if( !m->_DllMain_called )
        {
            BOOL ok;

            Z_LOG( "DllMain(DLL_PROCESS_ATTACH)\n" );
            ok = m->_DllMain( (HANDLE)1, DLL_PROCESS_ATTACH, NULL );
            if( !ok )  return CO_E_DLLNOTFOUND;

            Z_LOG( "DllMain(Z_DLL_COM_ATTACH)\n" );

            Com_module_params* p = com_module_params_ptr;
            if( !p )
            {
                if( com_module_params._version == 0 )
                {
                    com_module_params._version     = 2; // 10.3.2004
                  //com_module_params._subversion  = 0; 
                    com_module_params._subversion  = 1; // 3. 3.2006
                    com_module_params._com_context = static_com_context_ptr? static_com_context_ptr : &com_context;
                    //Log_ptr::get_stream_and_system_mutex( &com_module_params._log_stream, &com_module_params._log_system_mutex );
                    com_module_params._log_context = Log_ptr::get_log_context();
                }
                p = &com_module_params;
            }

            ok = m->_DllMain( (HANDLE)1, Z_DLL_COM_ATTACH, p );     // Eigene Erweiterung
            if( !ok )  
            {
                Z_LOG( "\n*** " << m->_module_filename << " ist nicht kompatibel zu diesem Programm ***\n\n" );
                return CO_E_DLLNOTFOUND;
            }

            m->_DllMain_called = true;
        }   
    }

    Z_LOGI2( "com", "DllGetClassObject(" << string_from_clsid( clsid ) << "," << string_from_clsid( iid ) << ")\n" );
    HRESULT hr = m->_DllGetClassObject( clsid, iid, p );
    if( FAILED(hr) )  Z_LOG2( "com", "DllGetClassObject() ==> " << string_from_hresult( hr ) );

    return hr;
}

//------------------------------------------------------------------------------MultiByteToWideChar

int WideCharToMultiByte( UINT CodePage, DWORD dwFlags, 
                         const OLECHAR* lpWideCharStr, int cchWideChar,
                         char* lpMultiByteStr, int cbMultiByte,  
                         const char* lpDefaultChar, BOOL* lpUsedDefaultChar )
{
    int result;

    if( CodePage != CP_ACP      // CP_ACP ist die "ANSI code page". Ist das ANSI Latin I, code page 1252?
     && CodePage != windows_codepage_iso_8859_1 ) { SetLastError( ERROR_INVALID_PARAMETER );  return 0; }
    

    if( cbMultiByte == 0 )
    {
        if( cchWideChar == -1 )  result = lpWideCharStr? wcslen( lpWideCharStr ) + 1 : 1;
                           else  result = cchWideChar;
    }
    else
    {
        char*           c     = lpMultiByteStr;
        char*           c_end = c + cbMultiByte;
        const OLECHAR*  w     = lpWideCharStr;

        if( cchWideChar == -1 )   // lpWideCharStr ist null-terminiert
        {
            while( c < c_end )
            {
                *c = char_from_wchar( *w );
                if( !*w )  break;
                w++;
                c++;
            }
        }
        else
        {
            const OLECHAR*  w_end = w + cchWideChar;

            while( w < w_end  &&  c < c_end )
            {
                *c = char_from_wchar( *w );
                w++;
                c++;
            }

            if( w < w_end )  { SetLastError( ERROR_INSUFFICIENT_BUFFER );  return 0; }
        }

        result = c - lpMultiByteStr;
    }

    return result;
}

//------------------------------------------------------------------------------MultiByteToWideChar

int MultiByteToWideChar( UINT CodePage, DWORD dwFlags, 
                         const char* lpMultiByteStr, int cbMultiByte, 
                         OLECHAR* lpWideCharStr, int cchWideChar )
{
    if( CodePage != windows_codepage_iso_8859_1  &&  CodePage != CP_ACP ) { SetLastError( ERROR_INVALID_PARAMETER );  return 0; }
    // CP_ACP ist die "ANSI code page", offenbar Windows-1252
    
    int result;


    if( cchWideChar == 0 )
    {
        if( cbMultiByte == -1 )  result = lpMultiByteStr? strlen( lpMultiByteStr ) + 1 : 1;
                           else  result = cbMultiByte;
    }
    else
    {
        OLECHAR*    w     = lpWideCharStr;
        OLECHAR*    w_end = w + cchWideChar;
        const char* c     = lpMultiByteStr;
        const char* c_end = c + cbMultiByte;

        if( cbMultiByte == -1 )
        {
            while( w < w_end )
            {
                *w = (unsigned char)*c;
                if( !*c )  break;
                w++;
                c++;
            }
        }
        else
        {

            while( w < w_end  &&  c < c_end )
            {
                *w = (unsigned char)*c;
                w++;
                c++;
            }
        }

        if( c < c_end )  { SetLastError( ERROR_INSUFFICIENT_BUFFER );  return 0; }

        result = w - lpWideCharStr;
    }

    return result;
}

//--------------------------------------------------------------------------------SysAllocStringLen

BSTR SysAllocStringLen( const OLECHAR* s, UINT len )
{
    BSTR p = (OLECHAR*)( 1 + (int*)CoTaskMemAlloc( sizeof(int) + (len+1) * sizeof (OLECHAR) ) );
  //BSTR p = (OLECHAR*)( 1 + (int*)malloc( sizeof(int) + (len+1) * sizeof (OLECHAR) ) );

    ((int*)p)[-1] = len;
    p[len] = 0;

    if( s )  memcpy( p, s, len * sizeof (OLECHAR) );
       else  memset( p, 0, len * sizeof (OLECHAR) );

    //fprintf(stderr,"SysAllocStringLen=0x%X %d: ", (int)p, (int)len ); if(s) for( UINT i = 0; i < min((int)30,(int)(len+0)); i++ )  fputc( p[i], stderr );  fputc('\n',stderr);
    return p;
}

//-----------------------------------------------------------------------------------SysAllocString

BSTR SysAllocString( const OLECHAR* s ) 
{
    int len = s? wcslen(s) : 0;
    
    return len == 0? NULL : SysAllocStringLen( s, len );
}

//-----------------------------------------------------------------------------------SysAllocString

BSTR SysAllocString( const wchar_t* s ) 
{
    int len = s? wcslen(s) : 0;

    if( len == 0 )  return NULL;

    BSTR result = SysAllocStringLen( NULL, len );

    for( int i = 0; i <= len; i++ )  result[i] = s[i];

    return result;
}

//------------------------------------------------------------------------------------SysFreeString

void SysFreeString( BSTR bstr )
{
    //Z_LOG( __PRETTY_FUNCTION__ << "  " << bstr << "\n" );
    //if( bstr )  fprintf( stderr, "SysFreeString 0x%X\n", (int)bstr );

    //if( bstr )  free( ((int*)bstr)-1 );
    if( bstr )  CoTaskMemFree( ((int*)bstr)-1 );
}

//------------------------------------------------------------------------------SysReAllocStringLen

int SysReAllocStringLen( BSTR* bstr_p, const OLECHAR* s, UINT len )
{
    BSTR p = SysAllocStringLen( s, len );
    if( !p )  return FALSE;

    SysFreeString( *bstr_p );
    *bstr_p = p;

    return TRUE;
}

//-------------------------------------------------------------------------------------SysStringLen

UINT SysStringLen( BSTR bstr )
{
    return bstr? ((int*)bstr)[-1] : 0;
}

//----------------------------------------------------------------------------------CreateErrorInfo

HRESULT CreateErrorInfo( ICreateErrorInfo** result )
{
    //if( static_com_context_ptr )  return static_com_context_ptr->CreateErrorInfo( result );

    *result = (ICreateErrorInfo*) new ErrorInfo_impl;
    (*result)->AddRef();
    return S_OK;
}

//--------------------------------------------------------------------------------release_ErrorInfo
/*
ULONG release_ErrorInfo( ICreateErrorInfo* ierrorinfo )
{
    return ierrorinfo->Release();
}
*/
//-------------------------------------------------------------------------------------SetErrorInfo

HRESULT SetErrorInfo( DWORD res, IErrorInfo* ierrorinfo )
{
    //Z_LOG( "SetErrorInfo(" << (void*)ierrorinfo << ")\n" );

    if( static_com_context_ptr )  return static_com_context_ptr->SetErrorInfo( res, ierrorinfo );

    thread_data->_current_errorinfo = ierrorinfo;
    return S_OK;
}

//-------------------------------------------------------------------------------------SetErrorInfo

HRESULT GetErrorInfo( DWORD res, IErrorInfo** ierrorinfo )
{
    //fprintf( stderr, "GetErrorInfo %X\n", (int)(IUnknown*)current_errorinfo );

    if( static_com_context_ptr )  return static_com_context_ptr->GetErrorInfo( res, ierrorinfo );

    //Z_LOG( "GetErrorInfo(" << (void*)current_errorinfo << ")\n" );

    thread_data->_current_errorinfo.CopyTo( ierrorinfo );
    if( *ierrorinfo )  
    {
        thread_data->_current_errorinfo = NULL;
        return S_OK;
    }
    else
        return S_FALSE;
}

//--------------------------------------------------------------------------ErrorInfo_impl::Release

ULONG ErrorInfo_impl::Release()
{ 
    if( _ref_count-- > 1 )  return _ref_count;  
    delete this; 
    return 0; 
} 

//-------------------------------------------------------------------ErrorInfo_impl::QueryInterface

HRESULT ErrorInfo_impl::QueryInterface( const IID& iid, void** p )
{
    if( iid == IID_IUnknown         )  *p = (IUnknown*)(IErrorInfo*)this; 
    else
    if( iid == IID_ICreateErrorInfo )  *p = (ICreateErrorInfo*)this;
    else
    if( iid == IID_IErrorInfo       )  *p = (IErrorInfo*)this;
    else
    {
        *p = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}

//-------------------------------------------------------------------------------------VariantClear

HRESULT VariantClear( VARIANT* v ) 
{
    HRESULT hr = S_OK;

    switch( v->vt )
    {
        case VT_BSTR:       SysFreeString( V_BSTR(v) );  
                            break;

        case VT_UNKNOWN:
        case VT_DISPATCH:   if( V_UNKNOWN(v) )  V_UNKNOWN(v)->Release();
                            break;


        case VT_SAFEARRAY:  hr = SafeArrayDestroy( V_SAFEARRAY(v) );
                            break;

        default:            if( ( v->vt|VT_TYPEMASK ) == ( VT_ARRAY|VT_TYPEMASK ) )  hr = SafeArrayDestroy( V_SAFEARRAY(v) );  
    }

    memset( v, 0, sizeof *v );

    return S_OK;
}

//--------------------------------------------------------------------------------VariantChangeType

HRESULT VariantChangeType( VARIANT* dest, const VARIANT* src, USHORT wFlags, VARTYPE vt ) 
{ 
    return VariantChangeTypeEx( dest, src, STANDARD_LCID, wFlags, vt ); 
}

//------------------------------------------------------------------------------VariantChangeTypeEx

HRESULT VariantChangeTypeEx( VARIANT* dest, const VARIANT* src, LCID lcid, unsigned short flags, VARTYPE vt )
{
    HRESULT hr = S_OK;
    string  str;

    //Z_LOG2( "joacim.VariantChangeType", "VariantChangeTypeEx(," << debug_string_from_variant(*src) << ',' << lcid << ',' << flags << ",new_vt=" << vartype_name(vt) << ")\n" );

    if( vt == src->vt )
    {
        if( dest == src )  return hr;   // Nichts zu tun
        
        VariantInit( dest );
        hr = VariantCopy( dest, src );   
    }
    else
    {
        VARIANT result;

        VariantInit( &result );

        try
        {
            if( src->vt == VT_DISPATCH )
            {
                if( flags & VARIANT_NOVALUEPROP )  { hr = DISP_E_TYPEMISMATCH; goto ENDE; }

                Variant     value;
                Dispparams  params;
                Excepinfo   excepinfo;
                uint        argnr;

                if( V_DISPATCH(src) == NULL )  { hr = E_POINTER; goto ENDE; }

                hr = V_DISPATCH(src)->Invoke( DISPID_VALUE, IID_NULL, lcid, DISPATCH_PROPERTYGET, &params, &value, &excepinfo, &argnr );
                if( FAILED(hr) )  
                {
                    try { throw_com_excepinfo( hr, &excepinfo, "Invoke DISPID_VALUE", "" ); }
                    catch( const exception& x ) { Z_LOG( "*** VariantChangeTypeEx(): FEHLER " << x << "\n" ); }
                    goto ENDE;
                }

                if( vt == VT_BSTR  &&  value.vt == VT_BSTR )
                {
                    if( src == dest )  
                    {
                        hr = VariantClear( dest );
                        if( FAILED(hr) )  goto ENDE;
                    }

                    memcpy( dest, &value, sizeof *dest );
                    value.vt = VT_EMPTY;

                    return S_OK;
                }

                str = string_from_variant_2( value, lcid );   // ohne VariantChangeTypeEx() zu rufen
            }
            else
            {
                str = string_from_variant_2( *src, lcid );   // ohne VariantChangeTypeEx() zu rufen
            }


            switch( vt )
            {
                case VT_EMPTY:      V_BSTR(&result) = NULL;                      break;
              //case VT_NULL: 
                case VT_I2:         V_I2  (&result) = as_int16 ( str.c_str() );  break;
                case VT_I4:         V_I4  (&result) = as_int32 ( str.c_str() );  break;
                case VT_R4:         V_R4  (&result) = as_double( str.c_str() );  break;
                case VT_R8:         V_R8  (&result) = as_double( str.c_str() );  break;

                case VT_CY: 
                {
                    return DISP_E_TYPEMISMATCH;
                    /*
                    int VT_CY_TESTEN;
                    CURRENCY cy;

                    size_t point = str.find( '.' );
                    if( point == string::npos )
                    {
                        cy.int64 = 10000 * as_int64( str );
                    }
                    else
                    {
                        str[ point ] = '\0';
                        cy.int64 = point == 0? 0 
                                             : 10000 * as_int64( str.data() );      // Punkt ohne Vorkommastellen ist erlaubt

                        if( point < str.length() - 1 )                              // Punkt ohne Nachkommastellen ist erlaubt
                        {
                            const char* t = str.c_str() + point + 1;
                            int         e = 10000;

                            while( isdigit( (Byte)*t ) ) 
                            {
                                e /= 10;
                                if( e == 0  &&  *t != '0' )   return DISP_E_TYPEMISMATCH;
                                cy.int64 += ( *t++ - '0' ) * e;
                            }
                        }
                    }

                    V_CY( &result ) = cy;

                    break;
                    */
                }

                case VT_DATE:       // str == "yyyy-mm-dd hh:mm:ss"
                {
                    return DISP_E_TYPEMISMATCH;
                    /*
                    int VT_DATE_TESTEN;
                    Z_LOG2( "zschimmer", "VT_DATE str=" << str << "\n" );

                    str = rtrim( str );
                    if( str.length() != 19 )  return DISP_E_TYPEMISMATCH;
                    if( str[  4 ] != '-' )  return DISP_E_TYPEMISMATCH;
                    if( str[  7 ] != '-' )  return DISP_E_TYPEMISMATCH;
                    if( str[ 10 ] != ' ' )  return DISP_E_TYPEMISMATCH;
                    if( str[ 13 ] != ':' )  return DISP_E_TYPEMISMATCH;
                    if( str[ 16 ] != ':' )  return DISP_E_TYPEMISMATCH;

                    str[4] = str[7] = str[10] = str[13] = str[16] = '\0';

                    SYSTEMTIME  systemtime;
                    systemtime.wYear   = as_uint( str.data() +  0  );
                    systemtime.wMonth  = as_uint( str.data() +  4+1 );
                    systemtime.wDay    = as_uint( str.data() +  7+1 );
                    systemtime.wHour   = as_uint( str.data() + 10+1 );
                    systemtime.wMinute = as_uint( str.data() + 13+1 );
                    systemtime.wSecond = as_uint( str.data() + 16+1 );

                    int ok2 = SystemTimeToVariantTime( &systemtime, &V_DATE( &result ) );
                    if( !ok2 )  return DISP_E_TYPEMISMATCH;

                    break;
                    */
                }

              //case VT_ERROR:
                case VT_BOOL:       V_BOOL(&result) = as_bool  ( str.empty()? "0" : str.c_str() );  break;      // Leerer String für Perl. Das Original scheint das auch so zu machen
              //case VT_I1:         V_I1  (&result) = as_int8  ( str.c_str() );  break;
              //case VT_UI1:        V_UI1 (&result) = as_uint8 ( str.c_str() );  break;
                case VT_UI2:        V_UI1 (&result) = as_uint16( str.c_str() );  break;
                case VT_UI4:        V_UI4 (&result) = as_uint32( str.c_str() );  break;
                case VT_I8:         V_I8  (&result) = as_int64 ( str.c_str() );  break;
                case VT_UI8:        V_I8  (&result) = as_uint64( str.c_str() );  break;
                case VT_INT:        V_INT (&result) = as_int   ( str.c_str() );  break;
                case VT_UINT:       V_UINT(&result) = as_uint  ( str.c_str() );  break;
              //case VT_VOID:
              //case VT_HRESULT:
              //case VT_PTR:
              //case VT_FILETIME:
                case VT_BSTR:       V_BSTR(&result) = bstr_from_string( str );  break;
              //case VT_DISPATCH:
              //case VT_VARIANT:
              //case VT_UNKNOWN:
              //case VT_DECIMAL:
              //case VT_SAFEARRAY:
              //case VT_CARRAY:
              //case VT_USERDEFINED:
              //case VT_LPSTR:
              //case VT_LPWSTR:
              //case VT_RECORD:
              //case VT_BLOB:
              //case VT_STREAM:
              //case VT_STORAGE:
              //case VT_STREAMED_OBJECT:
              //case VT_STORED_OBJECT:
              //case VT_BLOB_OBJECT:
              //case VT_CF:
              //case VT_CLSID:
                default:            hr = DISP_E_TYPEMISMATCH;
            }
        }
      //catch( const Overflow_error& ) { return DISP_E_OVERFLOW; }
        catch( exception& x ) 
        { 
            Z_LOG( Z_FUNCTION << ": ERROR " << x.what() << "\n" );
            hr = DISP_E_TYPEMISMATCH; 
        }

        if( SUCCEEDED(hr) )
        {
            if( src == dest )
            {
                hr = VariantClear( dest );
                if( FAILED(hr) )  goto ENDE;
            }

            result.vt = vt;
            memcpy( dest, &result, sizeof *dest );
        }
    }

ENDE:
    if( FAILED( hr ) )
    {
        Z_LOG( "*** VariantChangeTypeEx(" << vartype_name(vt) << "<-" << vartype_name(src->vt) << ",\"" << str << "\") => " << (void*)hr << " ***\n" );
    }

    return hr;
}

//--------------------------------------------------------------------------------------VariantCopy

HRESULT VariantCopy( VARIANT* dest, const VARIANT* src )
{
    HRESULT hr = S_OK;

    hr = VariantClear( dest );   if( FAILED(hr) )  return hr;

    if( src->vt & VT_BYREF )
    {
        memcpy( dest, src, sizeof *dest );
        return hr;
    }

    switch( src->vt )
    {
        case VT_UNKNOWN:
        case VT_DISPATCH:   dest->vt = src->vt;
                            V_UNKNOWN(dest) = V_UNKNOWN(src);
                            if( V_UNKNOWN(dest) )  V_UNKNOWN(dest)->AddRef();
                            break;

        case VT_EMPTY:
        case VT_NULL: 
        case VT_I2:
        case VT_I4:
        case VT_R4:
        case VT_R8:
        case VT_CY: 
        case VT_DATE:       
        case VT_ERROR:
        case VT_BOOL:
        case VT_I1: 
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_I8: 
        case VT_UI8:
        case VT_INT:
        case VT_UINT:
        case VT_VOID:
        case VT_HRESULT:
      //case VT_PTR:
        case VT_FILETIME:   memcpy( dest, src, sizeof *dest );
                            break;

        case VT_BSTR:       dest->vt = src->vt;
                            dest->bstrVal = NULL;
                            hr = String_to_bstr( src->bstrVal, SysStringLen( src->bstrVal ), &dest->bstrVal );
                            break;
      //case VT_VARIANT:
      //case VT_DECIMAL:
      //case VT_SAFEARRAY:
      //case VT_CARRAY:
      //case VT_USERDEFINED:
      //case VT_LPSTR:
      //case VT_LPWSTR:
      //case VT_RECORD:
      //case VT_BLOB:
      //case VT_STREAM:
      //case VT_STORAGE:
      //case VT_STREAMED_OBJECT:
      //case VT_STORED_OBJECT:
      //case VT_BLOB_OBJECT:
      //case VT_CF:
      //case VT_CLSID:
        default:            
        {
            if( ( src->vt & ~VT_TYPEMASK ) == VT_ARRAY )
            {
                VARTYPE vartype;
                hr = SafeArrayGetVartype( V_ARRAY(src), &vartype );
                if( FAILED(hr) )  return hr;
                if( vartype != ( src->vt & VT_TYPEMASK ) )  return DISP_E_BADVARTYPE;

                hr = SafeArrayCopy( V_ARRAY(src), &V_ARRAY(dest) );
                if( !FAILED(hr) )  V_VT(dest) = VT_ARRAY | vartype;

                /*
                if( SafeArrayGetDim( V_ARRAY(src) ) != 1 )  return DISP_E_BADVARTYPE;
    
                long32 lbound, ubound;
    
                hr = SafeArrayGetLBound( V_ARRAY(src), 1, &lbound );
                if( FAILED(hr) )  return hr;

                hr = SafeArrayGetUBound( V_ARRAY(src), 1, &ubound );
                if( FAILED(hr) )  return hr;

                V_ARRAY(dest) = SafeArrayCreateVector( src->vt & VT_TYPEMASK, lbound, ubound - lbound + 1 );
                dest->vt = src->vt;

                for( int i = lbound; i <= ubound; i++ )  VariantCopy( (Variant*)V_ARRAY(dest)->pvData + i, (Variant*)V_ARRAY(src)->pvData + i );
                */
            }
            else
                hr = DISP_E_BADVARTYPE;
        }
    }

    return hr;
}

//--------------------------------------------------------------------------SystemTimeToVariantTime

INT SystemTimeToVariantTime( SYSTEMTIME* systemtime, DATE* date )
{
    tm t;

    t.tm_year  = systemtime->wYear;
    t.tm_mon   = systemtime->wMonth;
    t.tm_mday  = systemtime->wDay;
  //t.tm_wday  = systemtime->wDayOfWeek;        // mktime berechnet das selbst
    t.tm_hour  = systemtime->wHour;
    t.tm_min   = systemtime->wMinute;
    t.tm_sec   = systemtime->wSecond;
    t.tm_isdst = -1;  // not available
    
    time_t n = mktime( &t );    if( n == (time_t)-1 )  return FALSE;

    *date = ( (double)n + (double)systemtime->wMilliseconds / 1000.0 )  /  (24*3600);

    return TRUE;
}

//--------------------------------------------------------------------------VariantTimeToSystemTime

INT VariantTimeToSystemTime( DATE date, SYSTEMTIME* systemtime )
{
    time_t n = (time_t)floor( date / (24*3600) );
    tm* t = localtime( &n );

    if( !t )  return FALSE;

    systemtime->wYear         = t->tm_year; 
    systemtime->wMonth        = t->tm_mon;
    systemtime->wDay          = t->tm_mday;
    systemtime->wDayOfWeek    = t->tm_wday;
    systemtime->wHour         = t->tm_hour;
    systemtime->wMinute       = t->tm_min;
    systemtime->wSecond       = t->tm_sec;
    systemtime->wMilliseconds = (DWORD)( ( date * (24*3600) - n ) * 1000 );
    if( (uint)systemtime->wMilliseconds > 999 )  systemtime->wMilliseconds = 999;     // paranoid

    return TRUE;
}

//----------------------------------------------------------------------------SafeArrayCreateVector

SAFEARRAY* SafeArrayCreateVector( VARTYPE vt, long32 lLbound, unsigned int cElements )
{
    //fprintf( stderr, "getpid=%d SafeArrayCreateVector(%d,%d,%d)\n", getpid(),vt,lLbound,cElements);

    USHORT features     = FADF_FIXEDSIZE;
    int    element_size;

    switch( vt )
    {
        case VT_VARIANT : element_size = sizeof (VARIANT   );  features |= FADF_VARIANT;   break;
        case VT_BSTR    : element_size = sizeof (BSTR      );  features |= FADF_BSTR;      break;
      //case VT_UNKNOWN : element_size = sizeof (IUnknown* );  features |= FADF_UNKNOWN;   break;
      //case VT_DISPATCH: element_size = sizeof (IDispatch*);  features |= FADF_DISPATCH;  break;
      //case VT_I1      : 
        case VT_UI1     : element_size = 1;                    features |= FADF_HAVEVARTYPE;  break;
      //case VT_I2      :
      //case VT_UI2     : element_size = 2;                    features |= FADF_HAVEVARTYPE;  break;
      //case VT_I4      : 
      //case VT_UI4     : element_size = 4;                    features |= FADF_HAVEVARTYPE;  break;
      //case VT_I8      :
      //case VT_UI8     : element_size = 8;                    features |= FADF_HAVEVARTYPE;  break;
        default         : return NULL;
    }                           

    int size = sizeof (SAFEARRAY_with_vartype) + cElements * element_size;
    SAFEARRAY_with_vartype* safearray = (SAFEARRAY_with_vartype*) new Byte[ size ];
    memset( safearray, 0, size );

    safearray->cDims       = 1;
    safearray->cbElements  = cElements;
    safearray->cLocks      = 0;
    safearray->fFeatures   = features;
    safearray->pvData      = safearray + 1;
    safearray->rgsabound[0].cElements = cElements;
    
    ((VARTYPE*)safearray->pvData)[ -2 ] = vt;
    //safearray->_vartype    = vt;


    return safearray;
};

//-----------------------------------------------------------------------------SafeArrayDestroyData

HRESULT SafeArrayDestroyData( SAFEARRAY* safearray )
{
    if( !safearray )  return E_POINTER;

    HRESULT hr = S_OK;
    VARTYPE vartype;

    hr = SafeArrayGetVartype( safearray, &vartype );
    if( FAILED(hr) )  return hr;

    switch( vartype )
    {
        case VT_UI1:
            break;
        
        case VT_VARIANT:
            for( int i = 0; i < safearray->cbElements; i++ )  VariantClear( (VARIANT*)safearray->pvData + i );
            break;

        default:
            return E_FAIL;
    }

    return hr;
}

//---------------------------------------------------------------------------------SafeArrayDestroy

HRESULT SafeArrayDestroy( SAFEARRAY* safearray )
{
    if( !safearray )  return E_POINTER;

    HRESULT hr = SafeArrayDestroyData( safearray );

    delete [] (Byte*)safearray;

    return hr;
}

//------------------------------------------------------------------------------SafeArrayGetVartype

HRESULT SafeArrayGetVartype( SAFEARRAY* safearray, VARTYPE* result )
{
    if( !safearray )  return E_POINTER;

    HRESULT hr = S_OK;

    *result = VT_EMPTY;

    VARTYPE vt = ((VARTYPE*)safearray->pvData)[ -2 ];

    if( safearray->fFeatures & FADF_VARIANT  )  *result = VT_VARIANT;
    else
    if( safearray->fFeatures & FADF_BSTR     )  *result = VT_BSTR;
    else
  //if( safearray->fFeatures & FADF_UNKNOWN  )  *result =  VT_UNKNOWN;
  //else
  //if( safearray->fFeatures & FADF_DISPATCH )  *result =  VT_DISPATCH;
  //else
    if( safearray->fFeatures & FADF_HAVEVARTYPE ) 
    {
        if( vt == VT_UI1 )  *result = vt;               // Plausi
                      else  hr = DISP_E_BADVARTYPE;   
    }
    else
    {
        hr = DISP_E_BADVARTYPE;
    }

    if( FAILED( hr ) ) 
    {
        Z_LOG( "SafeArrayGetVartype() => " << string_from_hresult(hr) << "  fFeatures=" << hex << safearray->fFeatures << " vt=" << vt << dec << "\n" );
    }

    return hr;
}

//-------------------------------------------------------------------------------SafeArrayGetLBound

HRESULT SafeArrayGetLBound( SAFEARRAY* safearray, unsigned int nDim, long32* plLbound )
{
    if( !safearray )  return E_POINTER;
    if( nDim != 1 )  return DISP_E_BADINDEX;

    *plLbound = safearray->rgsabound[ nDim-1 ].lLbound;

    return S_OK;
}

//-------------------------------------------------------------------------------SafeArrayGetUBound

HRESULT SafeArrayGetUBound( SAFEARRAY* safearray, unsigned int nDim, long32* plUbound )
{
    if( !safearray )  return E_POINTER;
    if( nDim != 1 )  return DISP_E_BADINDEX;

    *plUbound = safearray->rgsabound[ nDim-1 ].ubound();

    return S_OK;
}

//------------------------------------------------------------------------------SafeArrayAccessData

HRESULT SafeArrayAccessData( SAFEARRAY* safearray, void** ptr )
{
    if( !safearray )  return E_POINTER;

    safearray->cLocks++;
    *ptr = safearray->pvData;

    return S_OK;
}

//----------------------------------------------------------------------------SafeArrayUnaccessData

HRESULT SafeArrayUnaccessData( SAFEARRAY* safearray )
{
    if( !safearray )  return E_POINTER;

    if( safearray->cLocks == 0 )  return E_UNEXPECTED;
    
    safearray->cLocks--;

    return S_OK;
}

//-------------------------------------------------------------------------------SafeArrayPtrOfIndex
/*
HRESULT SafeArrayPtrOfIndex( SAFEARRAY* safearray, long32* rgIndices, void** ppvData )
{
    int index = *rgIndices;
    if( index < safearray->rgsabound[0].lLbound )  return DISP_E_BADINDEX;
    if( index > safearray->rgsabound[0].ubound() )  return DISP_E_BADINDEX;

    *ppvData = (XXXXVARIANT*)safearray->pvData + ( index - safearray->rgsabound[0].lLbound );

    return S_OK;
}
*/
//------------------------------------------------------------------------------------SafeArrayCopy

HRESULT SafeArrayCopy( SAFEARRAY* src, SAFEARRAY** dest )
{
    if( !src )  return E_POINTER;

    HRESULT hr = S_OK;
    
    *dest = NULL;

    if( src->cDims != 1 )  return DISP_E_BADVARTYPE;

    long32 lbound, ubound;
    VARTYPE vartype;

    hr = SafeArrayGetLBound( src, 1, &lbound );
    if( FAILED(hr) )  return hr;

    hr = SafeArrayGetUBound( src, 1, &ubound );
    if( FAILED(hr) )  return hr;

    int n = ubound - lbound + 1;

    hr = SafeArrayGetVartype( src, &vartype );
    if( FAILED(hr) )  return hr;

    *dest = SafeArrayCreateVector( vartype, lbound, n );

    switch( vartype )
    {
        case VT_UI1:
        case VT_I1:         
            memcpy( (*dest)->pvData, src->pvData, n * 1 );  
            break;

      //case VT_I4: 
      //    memcpy( (*dest)->pvData, src->pvData, n * 4 );  
      //    break;

        case VT_VARIANT:    
            for( int i = 0; i < n; i++ )
            {
                hr = VariantCopy( (VARIANT*)(*dest)->pvData + i, (VARIANT*)src->pvData + i );
                if( FAILED(hr) )  break;
            }
            break;

        default:
            hr = DISP_E_BADVARTYPE;
    }

    if( FAILED(hr)  &&  *dest )  SafeArrayDestroy( *dest ), *dest = NULL;

    return hr;
}

//-------------------------------------------------------------------------------------SetLastError

void SetLastError( int error )
{
    thread_data->_last_error = error;
}

//-------------------------------------------------------------------------------------GetLastError

int GetLastError()
{
    return thread_data->_last_error;
}

//--------------------------------------------------------------------------------------------------

#endif

