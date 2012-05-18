// $Id: z_com.cxx 13870 2010-06-03 15:29:33Z jz $

// §1719


#include "zschimmer.h"
#include <stdio.h>
#include <jni.h>            // JavaVM
#include <math.h>

#include "com.h"
#include "z_com.h"
#include "regex_class.h"
#include "log.h"
#include "loaded_module.h"
#include "java.h"

#ifdef Z_WINDOWS
#   include "z_windows.h"
#   include "z_windows_registry.h"
    using namespace zschimmer::windows;
#endif


using namespace std;

namespace zschimmer { 


//-------------------------------------------------------------------------------------------------

static Message_code_text error_codes[] =
{
    { "Z-COM-001", "SAFEARRAY($1) ist nicht um Typ $2" },
    { "Z-COM-002", "SAFEARRAY ist null" },
    { NULL }
};

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( z_com )
{
    add_message_code_texts( error_codes ); 
}

//-------------------------------------------------------------------------------------------Xc::Xc

Xc::Xc( const _com_error& x )
:
    _what( com::string_from_bstr( x.Description() ) ),
    _code( printf_string( "COM-%08X", x.Error() ) ),
    _return_code(0)
{
    if( _what == "" )  _what = get_mswin_msg_text( x.Error() );
    Z_LOG( "[ERROR " << _what << "]\n" );
}

//-------------------------------------------------------------------------------------------------

namespace com {

//-----------------------------------------------------------------------------------Loaded_modules
    
struct Loaded_modules
{
    typedef stdext::hash_map< string, ptr<Loaded_module> >   Map;

    ptr<Loaded_module>          load                    ( const string& filename );
    bool                        is_loaded               ( const string& filename );

    Map                        _map;
    Mutex                      _mutex;
};

//--------------------------------------------------------------------------------------------const

Z_DEFINE_GUID( CLSID_Module_interface2, 0xfeee4707, 0x6c1b, 0x11d8, 0x81, 0x03, 0x00, 0x04, 0x76, 0xee, 0x8a, 0xfb );
Z_DEFINE_GUID(  IID_Imodule_interface2, 0xfeee4706, 0x6c1b, 0x11d8, 0x81, 0x03, 0x00, 0x04, 0x76, 0xee, 0x8a, 0xfb );
//Z_DEFINE_GUID( CLSID_Module_interface, 0xfeee46e9, 0x6c1b, 0x11d8, 0x81, 0x03, 0x00, 0x04, 0x76, 0xee, 0x8a, 0xfb );
//Z_DEFINE_GUID(  IID_Imodule_interface, 0xfeee46ea, 0x6c1b, 0x11d8, 0x81, 0x03, 0x00, 0x04, 0x76, 0xee, 0x8a, 0xfb );

const Com_context com_context =
{
    1,
    8,
    NULL,

    #ifdef Z_WINDOWS
        NULL, NULL, NULL, NULL,
    #else
        CoTaskMemAlloc,
        CoTaskMemFree,
        SetErrorInfo,
        GetErrorInfo,
    #endif

    NULL, //javabridge::Vm::set_jvm,
  //javabridge::Vm::jvm,
    javabridge::Vm::request_jvm,
    javabridge::Vm::release_jvm
};

const extern Variant empty_variant         = Variant();
const extern Variant error_variant         ( Variant::vt_error );
const extern Variant missing_variant ( Variant::vt_error, DISP_E_PARAMNOTFOUND );

const DISPID Dispparams::dispid_propertyput = DISPID_PROPERTYPUT;

struct Vartype_name
{
    VARTYPE     _vt;
    const char* _name;
};


// Tabelle nicht ändern, die Namen werden von Variables.xml benutzt und stehen in XML-Dokumenten.
const Vartype_name vartype_names[] =
{
    { VT_EMPTY           , "VT_EMPTY"           },
    { VT_NULL            , "VT_NULL"            },
    { VT_I1              , "VT_I1"              },
    { VT_I2              , "VT_I2"              },
    { VT_I4              , "VT_I4"              },
    { VT_I8              , "VT_I8"              },
    { VT_UI1             , "VT_UI1"             },
    { VT_UI2             , "VT_UI2"             },
    { VT_UI4             , "VT_UI4"             },
    { VT_UI8             , "VT_UI8"             },
    { VT_INT             , "VT_INT"             },
    { VT_UINT            , "VT_UINT"            },
    { VT_R4              , "VT_R4"              },
    { VT_R8              , "VT_R8"              },
    { VT_CY              , "VT_CY"              },
    { VT_DATE            , "VT_DATE"            },
    { VT_BSTR            , "VT_BSTR"            },
    { VT_DISPATCH        , "VT_DISPATCH"        },
    { VT_ERROR           , "VT_ERROR"           },
    { VT_BOOL            , "VT_BOOL"            },
    { VT_VARIANT         , "VT_VARIANT"         },
    { VT_UNKNOWN         , "VT_UNKNOWN"         },
    { VT_DECIMAL         , "VT_DECIMAL"         },
    { VT_VOID            , "VT_VOID"            },
    { VT_HRESULT         , "VT_HRESULT"         },
    { VT_PTR             , "VT_PTR"             },
    { VT_SAFEARRAY       , "VT_SAFEARRAY"       },
    { VT_CARRAY          , "VT_CARRAY"          },
    { VT_USERDEFINED     , "VT_USERDEFINED"     },
    { VT_LPSTR           , "VT_LPSTR"           },
    { VT_LPWSTR          , "VT_SPWSTR"          },
    { VT_FILETIME        , "VT_FILETIME"        },
    { VT_BLOB            , "VT_BLOB"            },
    { VT_STREAM          , "VT_STREAM"          },
    { VT_STORAGE         , "VT_STORAGE"         },
    { VT_STREAMED_OBJECT , "VT_STREAMED_OBJECT" },
    { VT_STORED_OBJECT   , "VT_STORED_OBJECT"   },
    { VT_BLOB_OBJECT     , "VT_BLOB_OBJECT"     },
    { VT_CF              , "VT_CF"              },
    { VT_CLSID           , "VT_CLSID"           },
    { VT_ILLEGALMASKED   , "VT_ILLEGALMASKED"   },
    {},
};

//-------------------------------------------------------------------------------------------static

extern Com_context const*       static_com_context_ptr  = NULL;
static Loaded_modules           loaded_modules;

//--------------------------------------------------------------------------------hash_value(CLSID)

size_t hash_value( const CLSID& clsid )
{
    return clsid.Data1 ^ clsid.Data2 ^ clsid.Data3 ^ clsid.Data4[7];
}

//-----------------------------------------------------------------------------Loaded_modules::load

ptr<Loaded_module> Loaded_modules::load( const string& filename )
{
    ptr<Loaded_module> result;

    Z_MUTEX( _mutex )
    {
        Map::iterator m = _map.find( filename );
        if( m == _map.end() )
        {
            result = Z_NEW( Loaded_module );
            result->load( filename );
            _map[ filename ] = result;
        }
        else
        {
            result = m->second;
        }
    }

    return result;
}

//------------------------------------------------------------------------Loaded_modules::is_loaded

bool Loaded_modules::is_loaded( const string& filename )
{
    return _map.find( filename ) != _map.end();
}

//------------------------------------------------------------------------------com_create_instance

IUnknown* com_create_instance( const CLSID& clsid, const IID& iid, IUnknown* outer, DWORD context )
{ 
    void* result = NULL;

    HRESULT hr = CoCreateInstance( clsid, outer, context, iid, &result ); 
    if( FAILED(hr) )  throw_com( hr, "CoCreateInstance", string_from_clsid( clsid ) );

    return static_cast<IUnknown*>( result );
}

//------------------------------------------------------------------------------com_create_instance

IUnknown* com_create_instance( const char* class_name, const IID& iid, IUnknown* outer, DWORD context )
{ 
    return com_create_instance( clsid_from_name( class_name ), iid, outer, context );
}

//------------------------------------------------------------------------------com_create_instance

IUnknown* com_create_instance( const CLSID& clsid, const IID& iid, IUnknown* outer, const string& module_filename )
{
    HRESULT             hr;
    bool                was_loaded  = loaded_modules.is_loaded( module_filename );
    ptr<Loaded_module>  module      = loaded_modules.load( module_filename );

    module->dont_unload();


    typedef HRESULT APIENTRY DllGetClassObject( const CLSID&, const IID&, void** );
    DllGetClassObject* dllGetClassObject = (DllGetClassObject*)module->addr_of( "DllGetClassObject" );


    if( !was_loaded )
    {
        // JavaVM etc. übergeben
        ptr<Imodule_interface2> module_interface;
        void*                   void_ptr        = NULL;

        hr = dllGetClassObject( CLSID_Module_interface2, IID_Imodule_interface2, &void_ptr );
        if( !FAILED( hr ) )
        {
            module_interface._ptr = static_cast<Imodule_interface2*>( void_ptr );

            hr = module_interface->putref_Com_context( static_com_context_ptr? static_com_context_ptr : &com_context );
            hr = module_interface->put_Log_categories( Bstr( static_log_categories.to_string() ) );
            hr = module_interface->put_Log_context( Log_ptr::get_log_context() );

            module_interface._ptr->AddRef();    // Wird nie freigegeben. Besser: In Ableitung von Loaded_module speichern und
                                                // wenn einmal das Modul entladen werden kann, sollte module_interface mit dem Modul freigegeben werden.
        }
    }

    ptr<IClassFactory>  class_factory;
    ptr<IUnknown>       iunknown;
    void*               void_ptr     = NULL;

    hr = dllGetClassObject( clsid, IID_IClassFactory, &void_ptr );
    if( FAILED( hr ) )  throw_com( hr, "DllGetClassObject", string_from_clsid( clsid ).c_str() );
    class_factory._ptr = static_cast< IClassFactory*>( void_ptr );

    hr = class_factory->CreateInstance( outer, iid, &void_ptr );
    if( FAILED( hr ) )  throw_com( hr, "IFactory::CreateInstance", string_from_clsid( clsid ).c_str() );
    iunknown._ptr = static_cast<IUnknown*>( void_ptr );


    return iunknown.take();
}

//------------------------------------------------------------------------------com_query_interface

IUnknown* com_query_interface( IUnknown* object, const IID& iid )
{ 
    HRESULT hr     = S_OK;
    void*   result = NULL;

    if( object )  hr = object->QueryInterface( iid, &result );    
            else  hr = E_POINTER;

    if( FAILED( hr ) )  throw_com( hr, "QueryInterface", name_of_iid_or_empty( iid ) );  

    return static_cast<IUnknown*>( result ); 
}

//------------------------------------------------------------------------------com_query_interface

IUnknown* com_query_interface_or_null( IUnknown* object, const IID& iid )
{ 
    HRESULT   hr     = S_OK;
    void*     result = NULL;

    if( object )  hr = object->QueryInterface( iid, &result );    
            else  hr = E_POINTER;

    if( FAILED( hr ) )
    {
        if( hr == E_NOINTERFACE )  return NULL;
        throw_com( hr, "QueryInterface", name_of_iid_or_empty( iid ) );  
    }

    return static_cast<IUnknown*>( result ); 
}

//---------------------------------------------------------------------Com_exception::Com_exception

Com_exception::Com_exception( HRESULT hresult, const string& function, const char* ins1, const char* ins2 )
:
    _hresult(hresult)
{
    string source;
    string description;


    if( hresult == DISP_E_EXCEPTION )
    {
        ptr<IErrorInfo> error_info;
        HRESULT my_hr = GetErrorInfo( 0, error_info.pp() );
        if( my_hr == S_OK )
        {
            Bstr source_bstr;
            error_info->GetSource( &source_bstr._bstr );
            source = string_from_bstr( source_bstr );

            Bstr description_bstr;   
            error_info->GetDescription( &description_bstr._bstr );
            description = string_from_bstr( description_bstr );


            // Prüfen, ob description mit einem Fehlercode anfängt: (s.a. kram/com_simple_standards.cxx)
            // Hostware: XXX-XXX-X9A
            // Rapid:    D999
            Regex regex = "^((([A-Z]+-)+[0-9A-Z]+)|(D[0-9]{3}))( | *$)";       
            Regex_submatches matches = regex.match_subresults( description );
            if( matches )
            {
                string error_code = matches[1];
                description = ltrim( description.substr( error_code.length() ) );

                Xc x ( error_code.c_str(), description.c_str() );
                if( source != "" )  x.append_text( source );
                if( ins1 )          x.append_text( ins1 );
                if( ins2 )          x.append_text( ins2 );
                if( source != "" )  x.set_name( source );

                throw x;
            }


            if( !ins1  ||  !ins1[0] )  ins1 = description.c_str();
            else
            if( !ins2  ||  !ins2[0] )  ins2 = description.c_str();
            else
            {
                description = string(ins2) + " / " + description;
                ins2 = description.c_str();
            }
        }
    }

    if( source != "" )  set_name( source );
    set_code( printf_string( "COM-%08X", hresult ) );
    insert( 1, function );
}

//----------------------------------------------------------------------------------------throw_com

void throw_com( HRESULT hresult, const string& function, const char* ins1 , const char* ins2 )
{
    //Z_DEBUG_ONLY( fprintf( stderr, "throw_com %X %s %s %s\n", (uint)hr, function, ins1, ins2 ) );

    Com_exception x ( hresult, function );
    x.insert( 2, ins1 );
    x.insert( 3, ins2 );

    throw x;
}

//----------------------------------------------------------------------------------------throw_com

void throw_com( HRESULT hr, const string& function, const OLECHAR* ins1_w )
{
    string ins1 = string_from_ole( ins1_w );

    throw_com( hr, function, ins1.c_str() );  
}

//----------------------------------------------------------------------------------------throw_com

void throw_com( HRESULT hr, const string& function, const Bstr& bstr )
{
    throw_com( hr, function, string_from_bstr( bstr ) );
}

//------------------------------------------------------------------------------string_from_hresult

string string_from_hresult( HRESULT hresult )
{
    switch( hresult )
    {
        case S_OK: return "S_OK";
        case TRUE: return "TRUE";

        default: 
        {
            try
            {
                throw_com( hresult, "" );
                return "";
            }
            catch( const exception& x )
            {
                return x.what(); 
            }
        }
    }
}

//------------------------------------------------------------------------------throw_com_excepinfo

void throw_com_excepinfo( HRESULT hresult, EXCEPINFO* excepinfo, const string& function, const char* ins )
{
    if( excepinfo->pfnDeferredFillIn ) (*excepinfo->pfnDeferredFillIn)( excepinfo );

    string source = string_from_bstr( excepinfo->bstrSource );
    string descr  = string_from_bstr( excepinfo->bstrDescription );

    HRESULT hr = excepinfo->scode? excepinfo->scode : hresult;

    SysFreeString( excepinfo->bstrSource );         excepinfo->bstrSource      = NULL;
    SysFreeString( excepinfo->bstrDescription );    excepinfo->bstrDescription = NULL;
    SysFreeString( excepinfo->bstrHelpFile );       excepinfo->bstrHelpFile    = NULL;

    throw_com( hr, function, ins, descr.c_str() ); // Source_pos(source)
}

//------------------------------------------------------------------------------------Com_set_error
/*
HRESULT Com_set_error( const Xc& x, const char* method )
{
    HRESULT hr;

    //set_hostole_exception( x );

    string what = x.what();
    hr = com_set_error( x.name(), what.c_str(), method );

    //Als hr sollte DISP_E_EXCEPTION zurückgegeben werden, denn sonst verschwindet der Fehlertext (mit Einfügungen) oder manchmal sogar der ganze Fehler (in JScript) jz 24.6.01
    //if( strncmp( x.code(), "MSWIN-", 6 ) == 0 )  try{  hr = hex_as_int32( x.code() + 6 );  }catch(const Xc&){}
    //if( strcmp( x.name(), "OLE" ) == 0  &&  FAILED( ((Ole_error*)&x)->_hresult ) )  hr = ((Ole_error*)&x)->_hresult;

    return hr;
}
*/
//------------------------------------------------------------------------------------Com_set_error

HRESULT Com_set_error( const _com_error& x, const string& method )
{
    //set_hostole_exception( Ole_error(x) );

#   ifdef SYSTEM_HAS_COM

        string text = get_mswin_msg_text( x.Error() );
        if( x.Description().length() > 0 )  text += " / " + string_from_bstr( x.Description() );
        Com_set_error( text.c_str(), method );
        return x.Error();

#    else

        // Sollte nicht aufgerufen werden!
        return E_FAIL;

#   endif
}

//------------------------------------------------------------------------------------Com_set_error

HRESULT Com_set_error( const exception& x, const string& method )
{
#ifndef __GNUC__
    if( typeid( x ) == typeid( ::std::ios_base::failure ) )
    {
        try
        {
            throw_errno( errno, "ios" );
        }
        catch( const Xc& xc )
        {
            return Com_set_error( xc.what(), method );
        }
    }
#endif
/*
    else
    if( typeid( x ) == typeid( Xc ) )
    {
        return com_set_error( *(const Xc*)&x, method );
    }
*/

    return Com_set_error( x.what(), method );
}

//------------------------------------------------------------------------------------Com_set_error

HRESULT Com_set_error( const char* descr, const string& source )
{
    if( !descr  )  descr  = "";
    //if( !source )  source = "";
    string description = descr;
    //if( method )  description += string( " (Aufruf " ) + method + ")";

    Z_LOG( "Com_set_error(\"" << descr << "\",\"" << source << ")\n" );

    HRESULT                 hr;
    ptr<ICreateErrorInfo>   create_error_info;
    ptr<IErrorInfo>         error_info;

    hr = CreateErrorInfo( create_error_info.pp() );
    if( FAILED( hr ) )  return hr;

    //create_error_info->SetGUID( clsid );

    create_error_info->SetSource     ( Bstr( source      ) );
    create_error_info->SetDescription( Bstr( S() << description << ", in " << source ) );

    hr = error_info.Assign_qi( create_error_info ); 
    if( SUCCEEDED(hr) && error_info )  SetErrorInfo( 0, error_info );

    return DISP_E_EXCEPTION;
}

//-----------------------------------------------------------------------------Call_query_interface

HRESULT Call_query_interface( IUnknown* iunknown, const IID& iid, void** result )
{
//#   ifdef Z_UNIX
//        Z_LOG2( "zschimmer", Z_FUNCTION << "  iid=" << iid << "\n" );
//#   endif

    return iunknown->QueryInterface( iid, result );
}

//--------------------------------------------------------------------------------operator << GUID

ostream& operator << ( ostream& s, const GUID& guid )
{
    s << string_from_guid( guid );

    string name = name_of_guid_or_empty( guid );
    if( !name.empty() )  s << ' ' << name;

    return s;
}

//----------------------------------------------------------------------------name_of_guid_or_empty

string name_of_guid_or_empty( const GUID& guid )
{
    string name;
    
    name = name_of_clsid_or_empty( guid );
    if( !name.empty() )  return name;
    
    name = name_of_iid_or_empty( guid );
    if( !name.empty() )  return name;
    
    name = name_of_typelib_or_empty( guid );
    if( !name.empty() )  return name;

    return "";
}

//---------------------------------------------------------------------------------string_from_guid

string string_from_guid( const GUID& guid )
{
    OLECHAR guid_text [ 38+1 ];

    guid_text[ 0 ] = 0;
    StringFromGUID2( guid, guid_text, sizeof guid_text );

    return string_from_ole( guid_text );
}

//---------------------------------------------------------------------------name_of_clsid_or_empty

string name_of_clsid_or_empty( const CLSID& clsid )
{
#   ifdef Z_COM

        return "";

#   else

        return windows::registry_query_value( HKEY_CLASSES_ROOT, "CLSID\\" + string_from_guid( clsid ), "" );

#   endif
}

//----------------------------------------------------------------------------name_of_iid_or_empty

string name_of_iid_or_empty( const IID& iid )
{
#   ifdef Z_COM

        return "";

#   else

        return windows::registry_query_value( HKEY_CLASSES_ROOT, "Interface\\" + string_from_guid( iid ), "" );

#   endif
}

//------------------------------------------------------------------------name_of_typelib_or_empty

string name_of_typelib_or_empty( const GUID& typelib_id )
{
#   ifdef Z_COM

        return "";

#   else

        return windows::registry_query_value( HKEY_CLASSES_ROOT, "Typelib\\" + string_from_guid( typelib_id ), "" );

#   endif
}

//--------------------------------------------------------------------------name_of_dispid_or_empty

string name_of_dispid_or_empty( IDispatch*, DISPID )
{
/* Nicht getestet, denn Sos_ole_object::GetTypoInfo() liefert E_NOTIMPL.

    HRESULT         hr;
    ptr<ITypeInfo>  typeinfo;

    hr = idispatch->GetTypeInfo( 0, STANDARD_LCID, typeinfo.pp() );
    if( FAILED(hr) )  return "";

    Bstr            name_bstr;
    unsigned int    count = 0;

    hr = typeinfo->GetNames( dispid, &name_bstr, 1, &count );
    if( !FAILED(hr) )  return name_bstr;
*/
    return "";
}

//-------------------------------------------------------------------------------olechar_from_wchar

void olechar_from_wchar( OLECHAR* o, const wchar_t* w, size_t len )
{
#   ifdef Z_OLECHAR_IS_WCHAR
        memcpy( o, w, len * sizeof *o );
#    else
        const wchar_t* w_end = w + len;
        while( w < w_end )  *o++ = (OLECHAR)*w++;
#   endif
}

//--------------------------------------------------------------------------------Olechar_to_string

Z_DEBUG_ONLY( static int olechar_to_string_static_buffer_counter );
Z_DEBUG_ONLY( static int olechar_to_string_new_counter );

HRESULT Olechar_to_string( const OLECHAR* olechar, int len, string* result )  throw()
{
    HRESULT hr = S_OK;

    if( len == 0 )  
    {
        *result = "";
    }
    else
    {
        int flags = WC_NO_BEST_FIT_CHARS;       // Das wirkt nicht wie gewünscht. Man muss user_default_char setzen und eine eigene Fehlermeldung erzeugen,
                                                // um auf nicht erlaubte Zeichen zu prüfen.

#       ifdef Z_WINDOWS
            if( !is_windows2000_or_more() )  flags &= ~WC_NO_BEST_FIT_CHARS;
#       endif

        char  local_buffer [ 200+1 ];
        char* buffer;
        
        if( len < NO_OF( local_buffer ) )
        {
            buffer = local_buffer;
            Z_DEBUG_ONLY( olechar_to_string_static_buffer_counter++ );        
        }
        else
        {
            buffer = (char*)malloc( (len+1) * sizeof (char) );
            if( !buffer )  return E_OUTOFMEMORY;
            Z_DEBUG_ONLY( olechar_to_string_new_counter++ );        
        }

        len = WideCharToMultiByte( CP_ACP, flags, olechar, len, buffer, len, NULL, NULL );
        if( !len )  hr = HRESULT_FROM_WIN32( GetLastError() );

        *result = string( buffer, len );
        if( buffer != local_buffer )  free( buffer );

      //fprintf( stderr, "olechar_to_string %s\n", result.c_str() );
    }

    return hr;
}

//-----------------------------------------------------------------------------------Bstr_to_string

HRESULT Bstr_to_string( const BSTR bstr, string* result )  throw()
{
    return Olechar_to_string( bstr, SysStringLen( bstr ), result );
}

//----------------------------------------------------------------------------------string_from_ole

string string_from_ole( const OLECHAR* wstr )
{
    if( !wstr )  return "";
    return string_from_ole( wstr, wmemchr( wstr, 0, (size_t)-1 ) - wstr );
}

//----------------------------------------------------------------------------------string_from_ole

string string_from_ole( const OLECHAR* wstr, size_t len )
{
/*
    //fprintf( stderr, "string_from_ole ,%d\n", len );
    if( len == 0 )  return "";

    char* buffer = new char[ len+1 ];

    len = WideCharToMultiByte( CP_ACP, 0, wstr, len, buffer, len, NULL, NULL );
    if( !len ) { delete buffer; throw_mswin( "WideCharToMultiByte" ); }

    string result ( buffer, len );
    delete buffer;

    //fprintf( stderr, "string_from_ole %s\n", result.c_str() );
*/
    string result;
    HRESULT hr = Olechar_to_string( wstr, len, &result );
    if( FAILED(hr) )  throw_com( hr, "WideCharToMultiByte" );
    return result;
}

//---------------------------------------------------------------------------------string_from_bstr

string string_from_bstr( const BSTR bstr )
{
    //fprintf( stderr, "string_from_bstr %d, %x %x %x\n", bstr? ((int*)bstr)[-1] : 0, bstr[0], bstr[1], bstr[2] );

    if( !bstr )  return "";
    return string_from_ole( bstr, SysStringLen( bstr ) );
}

//-----------------------------------------------------------------------------------String_to_bstr

HRESULT String_to_bstr( const OLECHAR* str, BSTR* bstr ) throw()
{
    return String_to_bstr( str, str? wcslen( str ) : 0, bstr );
}

//-----------------------------------------------------------------------------------String_to_bstr

HRESULT String_to_bstr( const Bstr& str, BSTR* bstr ) throw()
{
    return String_to_bstr( str, str.length(), bstr );
}

//-----------------------------------------------------------------------------------String_to_bstr

HRESULT String_to_bstr( const OLECHAR* str, size_t len, BSTR* bstr ) throw()
{
//fprintf(stderr,"string_to_bstr len=%d ", len );
    if( len == 0 )
    {
        *bstr = NULL;
    }
    else
    {
        //for( int i = 0; i < len; i++ )  fputc(str[i],stderr);
        *bstr = SysAllocStringLen( str, len );

        if( !*bstr )  return E_OUTOFMEMORY;
    }
    
//fprintf(stderr,"\n");
    return S_OK;
}

//-----------------------------------------------------------------------------------String_to_bstr

HRESULT String_to_bstr( const char* str, size_t len, BSTR* bstr ) throw()
{
    if( len == 0 )
    {
        *bstr = NULL;
    }
    else
    {
        int count = MultiByteToWideChar( CP_ACP, 0, str, len, NULL, 0 );
        if( !count )  return E_INVALIDARG;


        *bstr = SysAllocStringLen( (OLECHAR*)NULL, count );
        if( !*bstr )  return E_OUTOFMEMORY;

        int c = MultiByteToWideChar( CP_ACP, 0, str, len, *bstr, len );
        if( !c )  
        {
            if( GetLastError() == ERROR_INSUFFICIENT_BUFFER )  return E_OUTOFMEMORY;
            return HRESULT_FROM_WIN32( GetLastError() );  //return E_INVALIDARG;
        }

        (*bstr)[ count ] = L'\0';
    }

    return S_OK;
}

//-------------------------------------------------------------------------------------Bstr_to_bstr

HRESULT Bstr_to_bstr( const BSTR src, BSTR* dst ) throw()
{
    size_t len = SysStringLen( src );
    if( len == 0 )
    {
        *dst = NULL;
    }
    else
    {
        *dst = SysAllocStringLen( src, len );
        if( !*dst )  return E_OUTOFMEMORY;
    }

    return S_OK;
}

//---------------------------------------------------------------------------------bstr_from_string

BSTR bstr_from_string( const char* single_byte_text, size_t len )
{
    if( len == 0 )  return NULL;


    size_t count = MultiByteToWideChar( CP_ACP, 0, single_byte_text, len, NULL, 0 );
    if( !count )  throw_mswin( "bstr_from_string/MultiByteToWideChar" );

    OLECHAR* ptr = SysAllocStringLen( (OLECHAR*)NULL, count );
    if( !ptr )  throw_com( E_OUTOFMEMORY, "bstr_from_string/SysAllocStringLen" );

    int c = MultiByteToWideChar( CP_ACP, 0, single_byte_text, len, ptr, len );
    if( !c )  throw_mswin( "bstr_from_string/MultiByteToWideChar" );

    ptr[ count ] = L'\0';

    return ptr;
}

//--------------------------------------------------------------------------------wstring_from_bstr
#ifndef __GNUC__   // gcc 3.2 hat wstring nicht in der Bibliothek
#ifndef Z_OLECHAR_IS_WCHAR

std::wstring wstring_from_bstr( const BSTR bstr )
{
    size_t  len = SysStringLen( bstr );
    wstring result;

    result.resize( len );

    for( size_tt i = 0; i < len; i++ )  result[i] = (wchar_t)bstr[i];

    return result;
}

#endif
//--------------------------------------------------------------------------------bstr_from_wstring

BSTR bstr_from_wstring( const std::wstring& str )
{
#   ifndef Z_OLECHAR_IS_WCHAR

        return str.length()? SysAllocStringLen( str.data(), str.length() ) : NULL; }

#   else

        size_t len = str.length();
        if( len == 0 )  return NULL;

        BSTR bstr = SysAllocStringLen( NULL, len );
        if( !bstr )  throw_com( E_OUTOFMEMORY, "bstr_from_wstring/SysAllocStringLen" );

        for( size_t i = 0; i < len; i++ )  bstr[i] = str[i];
        bstr[ len ] = 0;

        return bstr;

#endif
}

#endif

//----------------------------------------------------------------------------------operator<< BSTR

ostream& operator<< ( std::ostream& s, BSTR bstr )
{
    if( bstr )
    {
        OLECHAR* b = bstr;
        while( b[0] )  s << char_from_wchar( b[0] ),  b++;
    }

    return s;
}

//------------------------------------------------------------------------compare_olechar_with_char

int compare_olechar_with_char( const OLECHAR* o, const char* c )
{
    while( *o | *c )
    {
        if( (unsigned int)*o < (unsigned char)*c )  return -1;
        if( (unsigned int)*o > (unsigned char)*c )  return +1;
        o++, c++;
    }

    return 0;
}

//----------------------------------------------------------------------------olestring_begins_with

bool olestring_begins_with( const OLECHAR* str, const char* prefix )
{
    if( !str )  return !prefix || prefix[0] == '\0';

    const OLECHAR* s = str;
    const char*    p = prefix;

    if( !p )  return false;

    while( *p  &&  *s == (unsigned char)*p )  s++, p++;

    return *p == '\0';
}

//-------------------------------------------------------------------------------------vartype_name

string vartype_name( VARTYPE vt )
{
    if( vt == (VARTYPE)-1 )  return "VT_invalid";

    string  result;
    VARTYPE type = (VARTYPE)( vt & VT_TYPEMASK );

    for( const Vartype_name* vn = vartype_names; vn->_name; vn++ )
    {
        if( ( type == vn->_vt ) )  { result = vn->_name;  break; }
    }

    if( result.empty() )  result = "VT_" + as_string( (int)type );
/*
    switch( vt & VT_TYPEMASK ) 
    {
        case VT_EMPTY:              result = "VT_EMPTY";            break;
        case VT_NULL:               result = "VT_NULL";             break;
        case VT_I1:                 result = "VT_I1";               break;
        case VT_I2:                 result = "VT_I2";               break;
        case VT_I4:                 result = "VT_I4";               break;
        case VT_I8:                 result = "VT_I8";               break;
        case VT_UI1:                result = "VT_UI1";              break;
        case VT_UI2:                result = "VT_UI2";              break;
        case VT_UI4:                result = "VT_UI4";              break;
        case VT_UI8:                result = "VT_UI8";              break;
        case VT_INT:                result = "VT_INT";              break;
        case VT_UINT:               result = "VT_UINT";             break;
        case VT_R4:                 result = "VT_R4";               break;
        case VT_R8:                 result = "VT_R8";               break;
        case VT_CY:                 result = "VT_CY";               break;
        case VT_DATE:               result = "VT_DATE";             break;
        case VT_BSTR:               result = "VT_BSTR";             break;
        case VT_DISPATCH:           result = "VT_DISPATCH";         break;
        case VT_ERROR:              result = "VT_ERROR";            break;
        case VT_BOOL:               result = "VT_BOOL";             break;
        case VT_VARIANT:            result = "VT_VARIANT";          break;
        case VT_UNKNOWN:            result = "VT_UNKNOWN";          break;
        case VT_DECIMAL:            result = "VT_DECIMAL";          break;
        case VT_VOID:               result = "VT_VOID";             break;
        case VT_HRESULT:            result = "VT_HRESULT";          break;
        case VT_PTR:                result = "VT_PTR";              break;
        case VT_SAFEARRAY:          result = "VT_SAFEARRAY";        break;
        case VT_CARRAY:             result = "VT_CARRAY";           break;
        case VT_USERDEFINED:        result = "VT_USERDEFINED";      break;
        case VT_LPSTR:              result = "VT_LPSTR";            break;
        case VT_LPWSTR:             result = "VT_SPWSTR";           break;
        case VT_FILETIME:           result = "VT_FILETIME";         break;
        case VT_BLOB:               result = "VT_BLOB";             break;
        case VT_STREAM:             result = "VT_STREAM";           break;
        case VT_STORAGE:            result = "VT_STORAGE";          break;
        case VT_STREAMED_OBJECT:    result = "VT_STREAMED_OBJECT";  break;
        case VT_STORED_OBJECT:      result = "VT_STORED_OBJECT";    break;
        case VT_BLOB_OBJECT:        result = "VT_BLOB_OBJECT";      break;
        case VT_CF:                 result = "VT_CF";               break;
        case VT_CLSID:              result = "VT_CLSID";            break;
        case VT_ILLEGALMASKED:      result = "VT_ILLEGALMASKED";    break;
        default:                    result = "VT_" + as_string( (int)( vt & VT_TYPEMASK ) );
    }
*/

    vt &= ~VT_TYPEMASK;

    if( vt & VT_VECTOR    )  result += "|VT_VECTOR",  vt &= ~VT_VECTOR;
    if( vt & VT_ARRAY     )  result += "|VT_ARRAY",   vt &= ~VT_ARRAY;
    if( vt & VT_BYREF     )  result += "|VT_BYREF",   vt &= ~VT_BYREF;
    if( vt  )  result += printf_string( "|%X", vt & VT_TYPEMASK );

    return result;
}

//--------------------------------------------------------------------------------vartype_from_name

VARTYPE vartype_from_name( const string& name )
{
    if( isdigit( (unsigned char)( name[0] ) ) )
    {
        VARTYPE vt = (VARTYPE)as_int( name );
        
        for( const Vartype_name* vn = vartype_names; vn->_name; vn++ )
        {
            if( vt == vn->_vt )  return vn->_vt;
        }
    }
    else
    {
        for( const Vartype_name* vn = vartype_names; vn->_name; vn++ )
        {
            if( name == vn->_name )  return vn->_vt;
        }
    }

    throw_xc( "WRONG-VARTYPE", name );
/*
    Etwa so:
    vector<string> names = split_vector( "|", name );
    for( int i = 0; i < names.size(); i++ )
    {
        if( names[i] == "VT_VECTOR" )  vt |= VT_VECTOR;
        else
        if( names[i] == "VT_ARRAY"  )  vt |= VT_ARRAY;
        else
        if( names[i] == "VT_BYREF"  )  vt |= VT_BEREF;
        else


    if( result.empty() )  result = "VT_" + as_string( type );

    vt &= ~VT_TYPEMASK;

    if( vt & VT_VECTOR    )  result += "|VT_VECTOR",  vt &= ~VT_VECTOR;
    if( vt & VT_ARRAY     )  result += "|VT_ARRAY",   vt &= ~VT_ARRAY;
    if( vt & VT_BYREF     )  result += "|VT_BYREF",   vt &= ~VT_BYREF;
    if( vt  )  result += printf_string( "|%X", vt & VT_TYPEMASK );

    return result;
*/
}

//--------------------------------------------------------------------------get_string_from_variant

bool get_string_from_variant( const VARIANT& variant, LCID lcid, string* result )
{
    // Wird von VariantChangeTypeEx() gerufen. Also das nicht rekursiv benutzen!

    char buffer[100+1];
    bool ok = false;
    
    *result = "";
    //char* old_locale = setlocale( LC_NUMERIC, "C" );

    //fprintf( stderr, "string_from_variant " );

    switch( V_VT( &variant ) ) 
    {
        case VT_EMPTY         :  *result = ""; ok = true; break;
        case VT_NULL          :  *result = ""; ok = true; break;

        case VT_I1:              z_snprintf( buffer, sizeof buffer, "%i"     , (int)          V_I1    ( &variant ) );  *result = buffer; ok = true; break;
        case VT_I1  | VT_BYREF:  z_snprintf( buffer, sizeof buffer, "%i"     , (int)         *V_I1REF ( &variant ) );  *result = buffer; ok = true; break;
        case VT_UI1:             z_snprintf( buffer, sizeof buffer, "%u"     , (unsigned int) V_UI1   ( &variant ) );  *result = buffer; ok = true; break;
        case VT_UI1 | VT_BYREF:  z_snprintf( buffer, sizeof buffer, "%u"     , (unsigned int)*V_UI1REF( &variant ) );  *result = buffer; ok = true; break;
        case VT_I2:              z_snprintf( buffer, sizeof buffer, "%i"     , (int)          V_I2    ( &variant ) );  *result = buffer; ok = true; break;
        case VT_I2  | VT_BYREF:  z_snprintf( buffer, sizeof buffer, "%i"     , (int)         *V_I2REF ( &variant ) );  *result = buffer; ok = true; break;
        case VT_UI2:             z_snprintf( buffer, sizeof buffer, "%u"     , (uint16)       V_UI2   ( &variant ) );  *result = buffer; ok = true; break;
        case VT_UI2 | VT_BYREF:  z_snprintf( buffer, sizeof buffer, "%u"     , (uint16)      *V_UI2REF( &variant ) );  *result = buffer; ok = true; break;
        case VT_I4:              z_snprintf( buffer, sizeof buffer, "%i"     , (int)          V_I4    ( &variant ) );  *result = buffer; ok = true; break;
        case VT_I4  | VT_BYREF:  z_snprintf( buffer, sizeof buffer, "%i"     , (int)         *V_I4REF ( &variant ) );  *result = buffer; ok = true; break;
        case VT_UI4:             z_snprintf( buffer, sizeof buffer, "%u"     , (uint)         V_UI4   ( &variant ) );  *result = buffer; ok = true; break;
        case VT_UI4 | VT_BYREF:  z_snprintf( buffer, sizeof buffer, "%u"     , (uint)        *V_UI4REF( &variant ) );  *result = buffer; ok = true; break;
        case VT_I8:              z_snprintf( buffer, sizeof buffer, "%" Z_PRINTF_INT64 , (int64)  V_I8    ( &variant ) );  *result = buffer; ok = true; break;
        case VT_I8  | VT_BYREF:  z_snprintf( buffer, sizeof buffer, "%" Z_PRINTF_INT64 , (int64) *V_I8REF ( &variant ) );  *result = buffer; ok = true; break;
        case VT_UI8:             z_snprintf( buffer, sizeof buffer, "%" Z_PRINTF_UINT64, (uint64) V_UI8   ( &variant ) );  *result = buffer; ok = true; break;
        case VT_UI8 | VT_BYREF:  z_snprintf( buffer, sizeof buffer, "%" Z_PRINTF_UINT64, (uint64)*V_UI8REF( &variant ) );  *result = buffer; ok = true; break;
        case VT_INT:             z_snprintf( buffer, sizeof buffer, "%i"     , (int)          V_INT    ( &variant ) );  *result = buffer; ok = true; break;
        case VT_INT | VT_BYREF:  z_snprintf( buffer, sizeof buffer, "%i"     , (int)         *V_INTREF ( &variant ) );  *result = buffer; ok = true; break;
        case VT_UINT:            z_snprintf( buffer, sizeof buffer, "%u"     , (int)          V_UINT   ( &variant ) );  *result = buffer; ok = true; break;
        case VT_UINT | VT_BYREF: z_snprintf( buffer, sizeof buffer, "%u"     , (uint)        *V_UINTREF( &variant ) );  *result = buffer; ok = true; break;
        case VT_R4:              z_snprintf( buffer, sizeof buffer, "%-.6g"  , (double)       V_R4     ( &variant ) );  goto RESPECT_LOCALE;
        case VT_R4 | VT_BYREF:   z_snprintf( buffer, sizeof buffer, "%-.6g"  , (double)      *V_R4REF  ( &variant ) );  goto RESPECT_LOCALE;
        case VT_R8:              z_snprintf( buffer, sizeof buffer, "%-.15g" , (double)       V_R8     ( &variant ) );  goto RESPECT_LOCALE;
        case VT_R8 | VT_BYREF:   z_snprintf( buffer, sizeof buffer, "%-.15g" , (double)      *V_R8REF  ( &variant ) );  goto RESPECT_LOCALE;

        case VT_CY:             
        case VT_CY | VT_BYREF:  
        {
            int64 cy = V_VT(&variant) & VT_BYREF? V_CYREF(&variant)->int64
                                                : V_CY(&variant).int64;
            int len = z_snprintf( buffer, sizeof buffer, "%" Z_PRINTF_INT64 ".%04d", cy / 10000, (int)( cy % 10000 ) );
            if( len > 5  &&  memcmp( buffer + len - 5, ".0000", 5 ) == 0 )  buffer[ len - 5 ] = '\0';       // ".0000" abschneiden

            goto RESPECT_LOCALE;
        }

        case VT_DATE:
        case VT_DATE | VT_BYREF:
        {
            char        buffer [ 50 ];
            SYSTEMTIME  systemtime;
            double      variant_time = V_VT( &variant ) & VT_BYREF? *V_DATEREF( &variant ) 
                                                                  :  V_DATE   ( &variant );

            int ok2  = VariantTimeToSystemTime( variant_time, &systemtime );
            if( !ok2 )  return false;

            z_snprintf( buffer, sizeof buffer, "%04d-%02d-%02d %02d:%02d:%02d", systemtime.wYear, systemtime.wMonth, systemtime.wDay,
                                                                                systemtime.wHour, systemtime.wMinute, systemtime.wSecond );

            *result = buffer;
            ok = true;
            break;
        }


        case VT_BSTR:            *result = string_from_bstr(  V_BSTR   ( &variant ) );  ok = true; break;
        case VT_BSTR | VT_BYREF: *result = string_from_bstr( *V_BSTRREF( &variant ) );  ok = true; break;

/* Invoke() nur mit Erlaubnis aufrufen, denn das Objekt kann in einem anderen Prozess laufen.
        case VT_DISPATCH:
        {
            Variant value;
            Dispparams params;
            V_DISPATCH(&variant)->Invoke( DISPID_VALUE, IID_NULL, lcid, DISPATCH_PROPERTYGET, &params, &value, NULL, NULL );
            if( value.vt == VT_DISPATCH )  return false;
            ok = get_string_from_variant( value, lcid, result );
            break;
        }
*/        
        case VT_ERROR:           *result = variant.scode == DISP_E_PARAMNOTFOUND? "DISP_E_PARAMNOTFOUND (missing value)"
                                    : "VT_ERROR: COM-" + printf_string( "%08X  ", variant.scode ) + get_mswin_msg_text( variant.scode );  ok = true; break;

        case VT_BOOL:            *result =  V_BOOL   ( &variant )? "1" : "0";  ok = true; break;
        case VT_BOOL | VT_BYREF: *result = *V_BOOLREF( &variant )? "1" : "0";  ok = true; break;
      //case VT_VARIANT:
      //case VT_UNKNOWN:
      //case VT_DECIMAL:
      //case VT_VOID:
      //case VT_HRESULT:
      //case VT_PTR:
      //case VT_SAFEARRAY:
      //case VT_CARRAY:
      //case VT_USERDEFINED:
      //case VT_LPSTR:
      //case VT_LPWSTR:
      //case VT_FILETIME:
      //case VT_BLOB:
      //case VT_STREAM:
      //case VT_STORAGE:
      //case VT_STREAMED_OBJECT:
      //case VT_STORED_OBJECT:
      //case VT_BLOB_OBJECT:
      //case VT_CF:
      //case VT_CLSID:

        default: ;
    }

    if(0) {
RESPECT_LOCALE:
        char decimal_char = '.';

        switch( lcid )
        {
            case LOCALE_SYSTEM_DEFAULT: decimal_char = ','; break;  //decimal_char = *localeconv()->decimal_point;  break;
          //case LANG_GERMAN: decimal_char =   Einstellung des Betriebssystems lesen
            default :;
        }

        if( decimal_char != '.' )
        {
            char* p = strchr( buffer, '.' );
            if( p )  *p = decimal_char;
        }

        *result = buffer; 
        ok = true;
    }

    //setlocale( LC_NUMERIC, old_locale );

    return ok;
}

//--------------------------------------------------------------------------------Variant_to_string

HRESULT Variant_to_string( const VARIANT& variant, string* result, LCID lcid )
{
    HRESULT hr = S_FALSE;

    if( !variant_is_missing( variant ) )
    {
        bool ok = get_string_from_variant( variant, lcid, result );
        if( ok )  return S_OK;

        Variant v;
        hr = VariantChangeType( &v, const_cast<VARIANT*>(&variant), 0, VT_BSTR );

        if( !FAILED(hr) )  hr = Bstr_to_string( V_BSTR(&v), result );
    }

    return hr;
}

//----------------------------------------------------------------------------------Variant_to_bool

HRESULT Variant_to_bool( const VARIANT& variant, bool* result )
{
    HRESULT hr = S_FALSE;

    if( !variant_is_missing( variant ) )
    {
        Variant v;
        hr = VariantChangeType( &v, const_cast<VARIANT*>( &variant ), 0, VT_BOOL );

        if( !FAILED(hr) )  *result = V_BOOL( &v ) == VARIANT_FALSE? false : true;
    }

    return hr;
}

//------------------------------------------------------------------------------string_from_variant

string string_from_variant( const VARIANT& variant, LCID lcid )
{
    string result;

    HRESULT hr = Variant_to_string( variant, &result, lcid );
    if( FAILED(hr) )  throw_com( hr, "string_from_variant", vartype_name( variant.vt ) );

    return result;
/*
    bool ok = get_string_from_variant( variant, lcid, &result );
    if( ok )  return result;

    Variant v;
    HRESULT hr = VariantChangeType( &v, const_cast<VARIANT*>(&variant), 0, VT_BSTR );
    if( FAILED(hr) )  throw_com( hr, "VariantChangeType" );
    
    return string_from_bstr( V_BSTR(&v) );
*/
}

//----------------------------------------------------------------------------string_from_variant_2

string string_from_variant_2( const VARIANT& variant, LCID lcid )
{
    string result;

    bool ok = get_string_from_variant( variant, lcid, &result );
    if( !ok )  throw_com( DISP_E_BADVARTYPE, "string_from_variant_2", vartype_name(variant.vt) );

    return result;
}

//------------------------------------------------------------------------debug_string_from_variant

string debug_string_from_variant( const VARIANT& v )
{
    try
    {
        if( &v == NULL )  return "NULL-Pointer";

        switch( V_VT( &v ) ) 
        {
            case VT_EMPTY           :
            case VT_NULL            : return vartype_name(v.vt);
            case VT_UNKNOWN         : return printf_string( "IUnknown:%X" , (size_t)V_UNKNOWN(&v) );
            case VT_DISPATCH        : return printf_string( "IDispatch:%X", (size_t)V_DISPATCH(&v) );

            case VT_ERROR:
                if( v.scode == DISP_E_PARAMNOTFOUND )  return "DISP_E_PARAMNOTFOUND (missing value)";   // Nicht angegebener optionaler Parameter: Keine Fehlermeldung zeigen!
                return string_from_variant( v );
            
            default:
                if( ( V_VT( &v ) & ~VT_TYPEMASK ) == VT_ARRAY )
                {
                    S result;
                    result << "VT_ARRAY";
                    
                    if( V_VT( &v ) & VT_TYPEMASK )  result << "|" << vartype_name( V_VT( &v ) & VT_TYPEMASK );

                    if( V_ARRAY( &v ) == NULL )
                    {
                        result << "NULL";
                    }
                    else
                    {

                        result << ":{";

                        Locked_safearray<Variant> array ( V_ARRAY( &v ) );

                        for( int i = 0; i < array.count(); i++ )
                        {
                            if( i > 5 )  { result << ",..."; break; }
                            if( i > 0 )  result << ',';
                          //result += string_from_variant( array[i] );       // Bei einem Fehler hier lassen wir abbrechen.
                            result << debug_string_from_variant( array[i] ).substr( 0, 30 );
                        }

                        result << '}';
                    }
                    return result;
                }

                return string_from_variant( v );
        }
    }
    catch( const exception& x )
    {
        return S() << vartype_name(v.vt) << ":[ERROR " << x.what() << "]";
    }
}

//--------------------------------------------------------------------------------bool_from_variant

bool bool_from_variant( const VARIANT& v )
{
    switch( v.vt & VT_TYPEMASK )
    {
        case VT_I1: 
        case VT_I2: 
        case VT_I4: 
      //case VT_I8: 
        case VT_INT:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
      //case VT_UI8:
        case VT_UINT:   return int_from_variant( v ) != 0;

        default:        return as_bool( string_from_variant(v).c_str() );
    }
}

//---------------------------------------------------------------------------------int_form_variant

int int_from_variant( const VARIANT& v ) //, int deflt )
{
    switch( v.vt & ~VT_BYREF )
    {
        case VT_I1:     return v.vt & VT_BYREF? *V_I1REF( &v ) : V_I1( &v );
        case VT_I2:     return v.vt & VT_BYREF? *V_I2REF( &v ) : V_I2( &v );
        case VT_I4:     return v.vt & VT_BYREF? *V_I4REF( &v ) : V_I4( &v );
      //case VT_I8:     return v.vt & VT_BYREF? *V_I8REF( &v ) : V_I8( &v );
        case VT_INT:    return v.vt & VT_BYREF? *V_INTREF( &v ) : V_INT( &v );
        case VT_UI1:    return v.vt & VT_BYREF? *V_UI1REF( &v ) : V_UI1( &v );
        case VT_UI2:    return v.vt & VT_BYREF? *V_UI2REF( &v ) : V_UI2( &v );
        case VT_UI4:    return v.vt & VT_BYREF? *V_UI4REF( &v ) : V_UI4( &v );
      //case VT_UI8:    return v.vt & VT_BYREF? *V_UI8REF( &v ) : V_UI8( &v );
        case VT_UINT:   return v.vt & VT_BYREF? *V_UINTREF( &v ) : V_UINT( &v );

      //case VT_NULL:   return deflt;
      //case VT_EMPTY:  return deflt;

        default:        string value = string_from_variant( v );
                      //if( value.length() == 0  ||  !isdigit( (unsigned char)value[0] )  ||  rtrim(value).empty() )  return deflt;
                        return as_int( value );
    }
}

//-------------------------------------------------------------------------------int64_form_variant

int64 int64_from_variant( const VARIANT& v )
{
    switch( v.vt & VT_TYPEMASK )
    {
        case VT_I1:
        case VT_I2:
        case VT_I4:
        case VT_INT:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_UINT:   return int_from_variant( v );

        case VT_I8:     return V_I8( &v );
        case VT_UI8:    return V_UI8( &v );

        default:        return as_int64( string_from_variant(v).c_str() );
    }
}

//------------------------------------------------------------------------------double_from_variant

double double_from_variant( const VARIANT& v )
{
    switch( v.vt & VT_TYPEMASK )
    {
        case VT_I1:     return V_I1(&v);
        case VT_I2:     return V_I2(&v);
        case VT_I4:     return V_I4(&v);
        case VT_I8:     return (double)V_I8(&v);
        case VT_INT:    return V_INT(&v);
        case VT_UI1:    return V_UI1(&v);
        case VT_UI2:    return V_UI2(&v);
        case VT_UI4:    return V_UI4(&v);
        case VT_UI8:    return (double)V_UI8(&v);
        case VT_UINT:   return V_UINT(&v);
        case VT_R4:     return V_R4(&v);
        case VT_R8:     return V_R8(&v);
        case VT_CY:     return V_CY(&v).int64 * (double)0.0001;

        default:        return as_double( string_from_variant(v).c_str() );
    }
}

//-------------------------------------------------------------------------------variant_is_integer

bool variant_is_integer( VARTYPE vt )  
{ 
    switch( vt & VT_TYPEMASK )
    {
        case VT_I1:
        case VT_I2:
        case VT_I4:
      //case VT_I8:
        case VT_INT:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
      //case VT_UI8:
        case VT_UINT:   return true;

        default:        return false;
    }
}

//-------------------------------------------------------------------------------variant_is_integer

bool variant_is_int64( VARTYPE vt )  
{ 
    switch( vt & VT_TYPEMASK )
    {
        case VT_I1:
        case VT_I2:
        case VT_I4:
        case VT_I8:
        case VT_INT:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_UI8:
        case VT_UINT:   return true;

        default:        return false;
    }
}

//--------------------------------------------------------------------------------variant_is_double

bool variant_is_double( VARTYPE vt )  
{ 
    switch( vt & VT_TYPEMASK )
    {
        case VT_R4:
        case VT_R8:     return true;

        default:        return false;
    }
}

//-------------------------------------------------------------------------------variant_is_numeric

bool variant_is_numeric( VARTYPE vt )  
{ 
    return variant_is_integer(vt) 
        || variant_is_double(vt) 
        || vt == VT_DECIMAL
        || vt == VT_CY;
}

//-------------------------------------------------------------------------------------variant_kind

Variant_kind variant_kind( VARTYPE vt )
{
    switch( vt )
    {
        case VT_EMPTY            : return vk_empty;
        case VT_NULL             : return vk_null;
        case VT_I2               : return vk_integer;
        case VT_I4               : return vk_integer;
        case VT_R4               : return vk_float;
        case VT_R8               : return vk_float;
      //case VT_CY               : return vk_decimal;
        case VT_DATE             : return vk_date;
        case VT_BSTR             : return vk_string;
      //case VT_DISPATCH         : return vk_
      //case VT_ERROR            : return vk_
        case VT_BOOL             : return vk_integer;
      //case VT_VARIANT          : return vk_
      //case VT_UNKNOWN          : return vk_
      //case VT_DECIMAL          : return vk_;
        case VT_I1               : return vk_integer;
        case VT_UI1              : return vk_integer;
        case VT_UI2              : return vk_integer;
        case VT_UI4              : return vk_integer;
        case VT_I8               : return vk_integer;
        case VT_UI8              : return vk_integer;
        case VT_INT              : return vk_integer;
        case VT_UINT             : return vk_integer;
      //case VT_VOID             : return vk_
        case VT_HRESULT          : return vk_integer;
      //case VT_PTR              : return vk_
      //case VT_SAFEARRAY        : return vk_
      //case VT_CARRAY           : return vk_
      //case VT_USERDEFINED      : return vk_
      //case VT_LPSTR            : return vk_
      //case VT_LPWSTR           : return vk_
      //case VT_RECORD           : return vk_
      //case VT_FILETIME         : return vk_
      //case VT_BLOB             : return vk_
      //case VT_STREAM           : return vk_
      //case VT_STORAGE          : return vk_
      //case VT_STREAMED_OBJECT  : return vk_
      //case VT_STORED_OBJECT    : return vk_
      //case VT_BLOB_OBJECT      : return vk_
      //case VT_CF               : return vk_
      //case VT_CLSID            : return vk_
        default                  : return vk_none;
    }
}

//------------------------------------------------------------------------------string_from_vartype
/*
string string_from_vartype( VARTYPE vt )
{
    string result;

    switch( vt & VT_TYPEMASK )
    {
        case VT_EMPTY            : result = "VT_EMPTY";         break;
        case VT_NULL             : result = "VT_NULL";  break;
        case VT_I2               : result = "VT_I2";  break;
        case VT_I4               : result = "VT_I4";  break;
        case VT_R4               : result = "VT_R4";  break;
        case VT_R8               : result = "VT_R8";  break;
        case VT_CY               : result = "VT_CY";  break;
        case VT_DATE             : result = "VT_DATE";  break;
        case VT_BSTR             : result = "VT_BSTR";  break;
        case VT_DISPATCH         : result = "VT_DISPATCH";  break;
        case VT_ERROR            : result = "VT_ERROR";  break;
        case VT_BOOL             : result = "VT_BOOL";  break;
        case VT_VARIANT          : result = "VT_VARIANT";  break;
        case VT_UNKNOWN          : result = "VT_UNKNOWN";  break;
        case VT_DECIMAL          : result = "VT_DECIMAL";  break;
        case VT_I1               : result = "VT_I1";  break;
        case VT_UI1              : result = "VT_UI1";  break;
        case VT_UI2              : result = "VT_UI2";  break;
        case VT_UI4              : result = "VT_UI4";  break;
        case VT_I8               : result = "VT_I8";  break;
        case VT_UI8              : result = "VT_UI8";  break;
        case VT_INT              : result = "VT_INT";  break;
        case VT_UINT             : result = "VT_UINT";  break;
        case VT_VOID             : result = "VT_VOID";  break;
        case VT_HRESULT          : result = "VT_HRESULT";  break;
        case VT_PTR              : result = "VT_PTR";  break;
        case VT_SAFEARRAY        : result = "VT_SAFEARRAY";  break;
        case VT_CARRAY           : result = "VT_CARRAY";  break;
        case VT_USERDEFINED      : result = "VT_USERDEFINED";  break;
        case VT_LPSTR            : result = "VT_LPSTR";  break;
        case VT_LPWSTR           : result = "VT_LPWSTR";  break;
        case VT_RECORD           : result = "VT_RECORD";  break;
        case VT_FILETIME         : result = "VT_FILETIME";  break;
        case VT_BLOB             : result = "VT_BLOB";  break;
        case VT_STREAM           : result = "VT_STREAM";  break;
        case VT_STORAGE          : result = "VT_STORAGE";  break;
        case VT_STREAMED_OBJECT  : result = "VT_STREAMED_OBJECT";  break;
        case VT_STORED_OBJECT    : result = "VT_STORED_OBJECT";  break;
        case VT_BLOB_OBJECT      : result = "VT_BLOB_OBJECT";  break;
        case VT_CF               : result = "VT_CF";  break;
        case VT_CLSID            : result = "VT_CLSID";  break;
        default                  : result = printf_string( "%X", vt & VT_TYPE_MASK );  break;
    }

    vt &= ~VT_TYPEMASK;

    if( vt & VT_VECTOR    )  result += "|VT_VECTOR",  vt &= ~VT_VECTOR;
    if( vt & VT_ARRAY     )  result += "|VT_ARRAY",   vt &= ~VT_ARRAY;
    if( vt & VT_BYREF     )  result += "|VT_BYREF",   vt &= ~VT_BYREF;
    if( vt  )  result += printf_string( "%X", vt & VT_TYPE_MASK );

    return result;
}
*/
//--------------------------------------------------------------------------------string_from_clsid

string string_from_clsid( const CLSID& clsid )
{
    HRESULT  hr;
    OLECHAR  buffer [38+1];

    buffer[0] = 0;
    
    hr = StringFromGUID2( clsid, buffer, sizeof buffer );
    if( FAILED(hr) )  throw_com( hr, "StringFromGUID2" );

    return string_from_ole( buffer );
}

//----------------------------------------------------------------------------------------str::Bstr

Bstr::Bstr( const Bstr& s )
: 
    _bstr( s.copy() ) 
{
}

//--------------------------------------------------------------------------------------Bstr::~Bstr

Bstr::~Bstr()
{ 
    if (_bstr) {
        // SysFreeString() setzt GetLastError() zurück (obwohl nicht bei Microsoft dokumentiert).
        // Wir retten den Fehlercode für Aufrufe der Form: WindowsSystemCall(Bstr(name)).
        // ~Bstr belässt dann den Fehlercode von WindowsSystemCall.

        DWORD e = GetLastError();
        SysFreeString(_bstr); 
        SetLastError(e);
    }
}

//-------------------------------------------------------------------------------------Bstr::append

void Bstr::append( const OLECHAR* s, size_t s_len )
{
    if( s_len > 0 )
    {
        size_t len = length();
        BSTR   b   = ::SysAllocStringLen( NULL, len + s_len );

        if( !b )  throw_com( E_OUTOFMEMORY, "Bstr::append" );
    
        memcpy( b, _bstr, len * sizeof (OLECHAR) );
        memcpy( b + len, s, s_len * sizeof (OLECHAR) );
        b[ len + s_len ] = 0;

        SysFreeString(_bstr);
        _bstr = b;
    }
}

//-------------------------------------------------------------------------------------Bstr::append
#ifndef Z_OLECHAR_IS_WCHAR

void Bstr::append( const wchar_t* s, size_t s_len )
{
    if( s_len > 0 )
    {
        size_t len = length();
        BSTR   b   = ::SysAllocStringLen( NULL, len + s_len );

        if( !b )  throw_com( E_OUTOFMEMORY, "Bstr::append" );
    
        memcpy( b, _bstr, len * sizeof (OLECHAR) );

        olechar_from_wchar( b + len, s, s_len );

        b[ len + s_len ] = 0;

        SysFreeString(_bstr);
        _bstr = b;
    }
}

#endif

//-------------------------------------------------------------------------------------Bstr::append

void Bstr::append( const char* s, size_t s_len )
{
    if( s_len > 0 )
    {
        size_t len = length();
        BSTR   b   = ::SysAllocStringLen( NULL, len + s_len );

        if( !b )  throw_com( E_OUTOFMEMORY, "Bstr::append" );
    
        memcpy( b, _bstr, len * sizeof (OLECHAR) );
        
        for( size_t i = 0; i < s_len; i++ )  b[ len + i ] = (unsigned char)s[ i ];

        b[ len + s_len ] = 0;

        SysFreeString(_bstr);
        _bstr = b;
    }
}

//------------------------------------------------------------------------------------Bstr::copy_to

void Bstr::copy_to( BSTR* bstr )
{
    if( !bstr )  return;

    size_t len = SysStringLen( _bstr );
    if( len == 0 )
    {
        *bstr = NULL;
    }
    else
    {
        *bstr = ::SysAllocStringLen( _bstr, len );
    
        if( *bstr == NULL )  throw_com( E_OUTOFMEMORY, "Bstr::copy_to" );
    }
}

//-------------------------------------------------------------------------------Bstr::alloc_string

void Bstr::alloc_string( const Bstr& s )
{ 
    alloc_string( s._bstr, s.length() ); 
}

//-------------------------------------------------------------------------------Bstr::alloc_string

void Bstr::alloc_string( const OLECHAR* s )
{
    alloc_string( s, s? wcslen( s ) : 0 );
}

//-------------------------------------------------------------------------------Bstr::alloc_string

void Bstr::alloc_string( const OLECHAR* s, size_t len )
{
    BSTR old_bstr = _bstr;      // Falls _bstr == s

    if( len == 0 )
    {
        _bstr = NULL;
    }
    else
    {
        _bstr = ::SysAllocStringLen( s, len );
        if( !_bstr )  throw_com( E_OUTOFMEMORY, "SysAllocStringLen" );
    }

    ::SysFreeString( old_bstr );
}

//-------------------------------------------------------------------------------Bstr::alloc_string
#ifndef Z_OLECHAR_IS_WCHAR

void Bstr::alloc_string( const wchar_t* s )
{
    alloc_string( s, wcslen( s ) );
}

#endif
//-------------------------------------------------------------------------------Bstr::alloc_string
#ifndef Z_OLECHAR_IS_WCHAR

void Bstr::alloc_string( const wchar_t* s, size_t len )
{
    ::SysFreeString( _bstr );

    if( len == 0 )
    {
        _bstr = NULL;
    }
    else
    {
        _bstr = ::SysAllocStringLen( NULL, len );
        if( !_bstr )  throw_com( E_OUTOFMEMORY, "SysAllocStringLen" );

        olechar_from_wchar( _bstr, s, len );
        _bstr[len] = 0;
    }
}

#endif
//-------------------------------------------------------------------------------Bstr::alloc_string
#ifndef Z_OLECHAR_IS_UINT16

void Bstr::alloc_string( const uint16* s )
{
    assert( sizeof (OLECHAR) == sizeof (uint16) );

    alloc_string( reinterpret_cast<const OLECHAR*>( s ) );
}

#endif
//-------------------------------------------------------------------------------Bstr::alloc_string
#ifndef Z_OLECHAR_IS_UINT16

void Bstr::alloc_string( const uint16* s, size_t len )
{
    assert( sizeof (OLECHAR) == sizeof (uint16) );

    const OLECHAR* olechars = reinterpret_cast<const OLECHAR*>( s );
    alloc_string( olechars, len );
}

#endif
//------------------------------------------------------------------------------------Bstr::compare

int Bstr::compare( const OLECHAR* s ) const
{ 
    if( _bstr == s )  return 0;

    if( length() == 0  &&  ( s == NULL || s[0] == 0 ) )  return 0;      // Leer-String kann NULL oder "" sein.

    return wcscmp( _bstr, s );
}

//------------------------------------------------------------------------------------Bstr::compare

int Bstr::compare( const Bstr& s ) const
{ 
    const OLECHAR* a = _bstr;
    const OLECHAR* b = s._bstr;

    int n = min( length(), s.length() );
    int i;

    for( i = 0; i < n  &&  a[i] == b[i]; i++ );

    int result = i < n? a[i] < b[i]? -1 :
                        a[i] > b[i]? +1 
                                   :  0
                      : 
                        length() < s.length()? -1 :
                        length() > s.length()? +1
                                             :  0;

    return result;
}

//------------------------------------------------------------------------------------Bstr::compare

int Bstr::compare( const char* s ) const
{
    const OLECHAR* a = _bstr;
    const char*    b = s;

    if( a == NULL )  return b == NULL  ||  b[0] == '\0'? 0 : -1;
    if( b == NULL )  return +1;

    while( *a   &&  *a == *b )  a++, b++;

    return *a < *b? -1 :
           *a > *b? +1 
                  : 0;
}

//-----------------------------------------------------------------------------------Bstr::to_lower

void Bstr::to_lower()
{
    OLECHAR* p = _bstr;

    if( p )
    {
        while( *p ) 
        {
            if( (uint)*p < 0x100  &&  isupper( (uint)*p ) )  *p = tolower( (uint)*p );
            p++;
        }
    }
}

//---------------------------------------------------------------------------------Bstr::hash_value

size_t Bstr::hash_value() const
{
    size_t      length   = this->length();
    size_t      result   = 0xDeadBeef;
    const int   n        = 10;
    const int   distance = length < n? 1 : length / n;

    for( size_t i = 0; i < length; i += distance )  result += (uint)_bstr[i];

    //printf( "%s %s => %d\n", Z_FUNCTION, string_from_ole(_bstr).c_str(), result );
    return result;
}

//---------------------------------------------------------------------------------operator << Bstr

ostream& operator << ( ostream& s, const Bstr& bstr )
{
    int n = bstr.length();
    for( int i = 0; i < n; i++ )  s << char_from_wchar( bstr[i] );
    return s;
}

//---------------------------------------------------------------------------------Variant::Variant

Variant::Variant( Vt_array, int size, Variant_type vartype )
{
    init();

    V_ARRAY(this) = SafeArrayCreateVector( vartype, 0, size );
    //Z_LOG2( "zschimmer", "SafeArrayCreateVector(" << vartype << ",0," << size << ") ==> " << (void*)V_ARRAY(this) << "\n" );
    vt = VT_ARRAY | vartype;
}

//------------------------------------------------------------------------------Variant::operator =

Variant& Variant::operator = ( IUnknown* s )                             
{ 
    clear();

    if( s )
    {
        pdispVal = NULL;
        
        void* void_ptr = NULL;
        HRESULT hr = s->QueryInterface( IID_IDispatch, &void_ptr );
        
        if( SUCCEEDED(hr) )
        {
            pdispVal = static_cast<IDispatch*>( void_ptr );
            return *this;
        }

        s->AddRef();
    }

    vt = VT_UNKNOWN; 
    punkVal = s;

    return *this;
}

//------------------------------------------------------------------------------Variant::operator = 

Variant& Variant::operator = ( const OLECHAR* s )
{
    clear();

    vt = VT_BSTR;
    HRESULT hr = String_to_bstr( s, &bstrVal );

    if( FAILED(hr) ) throw_com( hr, "Variant::operator=" );

    return *this;
}

//------------------------------------------------------------------------------Variant::operator = 

Variant& Variant::operator = ( const Bstr& s )
{
    clear();

    vt = VT_BSTR;
    HRESULT hr = String_to_bstr( s, &bstrVal );

    if( FAILED(hr) ) throw_com( hr, "Variant::operator=" );

    return *this;
}

//------------------------------------------------------------------------------Variant::operator =
#ifndef Z_OLECHAR_IS_WCHAR

Variant& Variant::operator = ( const wchar_t* s )
{ 
    clear(); 

    vt = VT_BSTR;
    bstrVal = NULL;

    if( s )
    {
        size_t len = wcslen(s);
        if( len )
        {
            bstrVal = ::SysAllocStringLen( NULL, len );
            if( bstrVal == NULL )  throw_com( E_OUTOFMEMORY, "Variant::operator=" );

            olechar_from_wchar( bstrVal, s, len );
            bstrVal[len] = 0;
        }
    }

    return *this;
}

#endif
//------------------------------------------------------------------------------Variant::operator = 

Variant& Variant::operator = ( const char* s )
{
    clear();

    vt = VT_BSTR;
    bstrVal = bstr_from_string( s );

    return *this;
}

//------------------------------------------------------------------------------Variant::operator = 

Variant& Variant::operator = ( const string& s )
{
    clear();

    vt = VT_BSTR;
    bstrVal = bstr_from_string( s );

    return *this;
}

//------------------------------------------------------------------------------Variant::operator = 
#ifndef __GNUC__   // gcc 3.2 hat wstring nicht in der Bibliothek

Variant& Variant::operator = ( const wstring& s )
{
    clear();

    vt = VT_BSTR;
    bstrVal = bstr_from_wstring( s );

    return *this;
}

#endif
//-----------------------------------------------------------------------------Variant::assign_bstr

void Variant::assign_bstr( const BSTR& s )
{ 
    clear(); 

    vt = VT_BSTR;
    bstrVal = NULL;

    size_t len = SysStringLen( s );
    if( len )
    {
        bstrVal = ::SysAllocStringLen( s, SysStringLen(s) );
        if( bstrVal == NULL )  throw_com( E_OUTOFMEMORY, "Variant::operator=" );
    }
}

//------------------------------------------------------------------------------------Variant_equal

struct Variant_equal
{
    template< typename A, typename B >
    bool operator() ( const A& a, const B& b ) const
    { 
        return a == b; 
    }

    bool operator() ( const BSTR& a, const BSTR& b ) const 
    { 
        return ::SysStringByteLen( a ) == ::SysStringByteLen( b )  &&  ::memcmp( a, b, ::SysStringByteLen( a ) ) == 0;
    }
};

//------------------------------------------------------------------------------------Variant_lower

struct Variant_lower
{
    template< typename A, typename B >
    bool operator() ( const A& a, const B& b ) const
    { 
        return a < b; 
    }

    bool operator() ( BSTR a, BSTR b ) const 
    { 
        bool       a_is_shorter = ::SysStringByteLen( a ) < ::SysStringByteLen( b );
        size_t     len          = a_is_shorter? ::SysStringByteLen( a ) : ::SysStringByteLen( b );
        const BSTR a_end        = a + len;

        while( a < a_end )  
        {
            if( *a < *b )  return true;
            if( *a > *b )  return false;
            a++, b++;
        }

        return a_is_shorter;
    }
};

//----------------------------------------------------------------------------------variant_compare

template< class CMP >
inline bool variant_compare( const VARIANT& a, const VARIANT& b )
{
    CMP cmp;

    if( a.vt == b.vt )
    {
        switch( a.vt )
        {
            case VT_EMPTY:
            case VT_NULL:       return true;

            case VT_INT:        return cmp( V_INT(&a), V_INT(&b) );
            case VT_I8:         return cmp( V_I8 (&a), V_I8 (&b) );
            case VT_UI8:        return cmp( V_UI8(&a), V_UI8(&b) );
            case VT_I4:         return cmp( V_I4 (&a), V_I4 (&b) );
            case VT_UI4:        return cmp( V_UI4(&a), V_UI4(&b) );
            case VT_I2:         return cmp( V_I2 (&a), V_I2 (&b) );
            case VT_UI2:        return cmp( V_UI2(&a), V_UI2(&b) );
            case VT_I1:         return cmp( V_I1 (&a), V_I1 (&b) );
            case VT_UI1:        return cmp( V_UI1(&a), V_UI1(&b) );
            case VT_CY:         return cmp( a.cyVal.int64, b.cyVal.int64 );
            case VT_R4:         return cmp( V_R4 (&a), V_R4 (&a) );
            case VT_R8:         return cmp( V_R8 (&a), V_R8 (&a) );
            case VT_BOOL:       return cmp( ( a.boolVal != 0 ), ( b.boolVal != 0 ) );
          //case VT_DECIMAL:
            case VT_BSTR:       return cmp( a.bstrVal, b.bstrVal );
            case VT_DATE:       return cmp( a.date    , b.date );
            case VT_ERROR:      return cmp( a.scode   , b.scode );
            case VT_DISPATCH:   return cmp( a.pdispVal, b.pdispVal );
            case VT_UNKNOWN:    return cmp( a.punkVal , b.punkVal );

            default:            throw_com( DISP_E_BADVARTYPE, "variant_compare", vartype_name(a.vt) );
        }
    }


    // Typen sind verschieden


#define ICMP( MEMBER )                                                                          \
    if( b.vt == VT_INT )  return cmp( (a.MEMBER), V_INT(&b) );                                  \
    if( b.vt == VT_I4  )  return cmp( (a.MEMBER), V_I4(&b) );                                   \
    if( b.vt == VT_I2  )  return cmp( (a.MEMBER), V_I2(&b) );                                   \
    if( b.vt == VT_UI1 )  return cmp( (a.MEMBER), V_UI1(&b) );                                  \
    if( b.vt == VT_I8  )  return cmp( (a.MEMBER), b.cyVal.int64 );                              \
    if( b.vt == VT_CY  )  return cmp( (a.MEMBER) * (int64)10000, b.cyVal.int64 );               \
    if( b.vt == VT_R4  )  return cmp( (a.MEMBER), V_R4(&b) );                                   \
    if( b.vt == VT_R8  )  return cmp( (a.MEMBER), V_R8(&b) );


    switch( a.vt )
    {
        case VT_INT:        ICMP( lVal   );  break;
        case VT_I4:         ICMP( lVal   );  break;
        case VT_I2:         ICMP( iVal   );  break;
        case VT_UI1:        ICMP( bVal   );  break;
        case VT_I8:         ICMP( cyVal.int64 );  break;
      //case VT_CY:         break;
        case VT_R4:         ICMP( fltVal );  break;
        case VT_R8:         ICMP( dblVal );  break;
        case VT_DISPATCH:
        case VT_UNKNOWN:    return ( b.vt == VT_DISPATCH || b.vt == VT_UNKNOWN )  &&  cmp( a.pdispVal, b.pdispVal );
        default:            break;
    }

    if( a.vt == VT_CY    &&  b.vt != VT_CY   )  return variant_compare<CMP>( b, a );
  //if( b.vt == VT_DATE  &&  a.vt != VT_DATE )  return variant_compare<CMP>( b, a );


    // Wenn alles nichts hilft, als Strings vergleichen:
    {
        Variant        aa, bb;
        const VARIANT* a_ptr = &a;
        const VARIANT* b_ptr = &b;

        if( a.vt != VT_BSTR )  aa.change_type( VT_BSTR, a ),  a_ptr = &aa;
        if( b.vt != VT_BSTR )  bb.change_type( VT_BSTR, b ),  b_ptr = &bb;

        return cmp( V_BSTR(a_ptr), V_BSTR(b_ptr) );
    }
}

//-------------------------------------------------------------------------------variants_are_equal

bool variants_are_equal( const VARIANT& a, const VARIANT& b )
{
    return variant_compare< Variant_equal > ( a, b );
}

//---------------------------------------------------------------------------------variant_is_lower
/*

Wenn die Funktion scharf ist, erzeugt Microsoft VS 6 falschen Code. variants_are_equal() nutzt dann die Klasse Variant_lower! 3.10.2002

bool variant_is_lower( const VARIANT& a, const VARIANT& b )
{
    return variant_compare< Variant_lower > ( a, b );
}
*/
//-----------------------------------------------------------------------------Variant::operator == 
/*
bool Variant::operator == ( const VARIANT& s ) const
{
    int64  i = 0;
    double d = 0;

    switch( vt )
    {
        case VT_I8:  i = i64Val;        goto INTEGER_;
        case VT_I4:  i = lVal;          goto INTEGER_;
        case VT_I2:  i = iVal;          goto INTEGER_;
        case VT_UI1: i = bVal;          goto INTEGER_;
INTEGER_:    
            switch( s.vt )
            {
                case VT_I8:  return i = s.i64Val;
                case VT_I4:  return i = s.lVal;
                case VT_UI4: return i = s.ulVal;
                case VT_I2:  return i = s.iVal;
                case VT_UI2: return i = s.uiVal;
                case VT_I1:  return i = s.cVal;
                case VT_UI1: return i = s.bVal;
                case VT_CY:  return i = s.dblVal;
                case VT_R8:  return i = s.dblVal;
                case VT_R4:  return i = s.fltVal;
                default:     throw_com( DISP_E_BADVARTYPE, "Variant::operator ==" );
            }
            break;

        case VT_R4:  d = fltVal;        goto DOUBLE_;
        case VT_R8:  d = dblVal;        goto DOUBLE_;

    }

*/
//----------------------------------------------------------------------------------Variant::Attach

HRESULT Variant::Attach( VARIANT* s )        
{
    HRESULT hr = Clear();

    if( !FAILED(hr) )
    {
        memcpy( this, s, sizeof (VARIANT) );
        s->vt = VT_EMPTY;
        hr = S_OK;
    }

    return hr;
}

//----------------------------------------------------------------------------------Variant::Detach

HRESULT Variant::Detach( VARIANT* dest )
{
    HRESULT hr = ::VariantClear( dest );
    
    if( !FAILED(hr) )
    {
        memcpy( dest, this, sizeof (VARIANT) );
        vt = VT_EMPTY;
        hr = S_OK;
    }

    return hr;
}

//-----------------------------------------------------------------Variant::is_null_or_empty_string

bool Variant::is_null_or_empty_string() const
{ 
    return vt == VT_NULL  
        || vt == VT_EMPTY
        || vt == VT_BSTR && SysStringLen( V_BSTR(this) ) == 0
        || vt == VT_DISPATCH && V_DISPATCH(this) == NULL
        || vt == VT_UNKNOWN  && V_UNKNOWN(this) == NULL
        || is_missing(); 
}

//-----------------------------------------------------------------------------------Variant::clear

void Variant::clear()
{ 
    HRESULT hr = Clear(); 
    if( FAILED(hr) ) throw_com( hr, "Variant::Clear" ); 
}

//------------------------------------------------------------------------------------Variant::copy

void Variant::copy( const VARIANT* s )
{ 
    HRESULT hr = Copy(s);
    if( FAILED(hr) ) throw_com( hr, "Variant::Copy" ); 
}

//----------------------------------------------------------------------------------Variant::attach

void Variant::attach( VARIANT* s )
{ 
    HRESULT hr = Attach(s); 
    if( FAILED(hr) ) throw_com( hr, "Variant::Attach" ); 
}

//----------------------------------------------------------------------------------Variant::detach

void Variant::detach( VARIANT* s )
{ 
    HRESULT hr = Detach(s); 
    if( FAILED(hr) ) throw_com( hr, "Variant::Detach" ); 
}

//-----------------------------------------------------------------------------Variant::change_type

void Variant::change_type( VARTYPE new_vt, LCID lcid ) 
{ 
    HRESULT hr = ChangeType( new_vt, lcid ); 
    if( FAILED(hr) ) throw_com( hr, "Variant::ChangeType" ); 
}

//-----------------------------------------------------------------------------Variant::change_type

void Variant::change_type( VARTYPE new_vt, const VARIANT& s, LCID lcid ) 
{ 
    HRESULT hr = ChangeType( new_vt, &s, lcid ); 
    if( FAILED(hr) ) throw_com( hr, "Variant::ChangeType" ); 
}

//----------------------------------------------------------------------------------Variant::set_vt

void Variant::set_vt( VARTYPE t )
{ 
    if( vt != t )  
    { 
        clear(); 
        vt = t; 
    } 
}

//------------------------------------------------------------------------------afearray::Safearray
/*
Safearray::Safearray( int size )
{ 
    SAFEARRRAYBOUND bounds;

    bounds.lLbound   = 0;
    bounds.cElements = size;

    _safearray = SafeArrayCreate( VT_VARIANT, 1, bounds ); 
    if( !_safearray )  throw_com( E_OUTOFMEMORY, "SafeArrayCreate(" + as_string(size)+ ")" );
}

//----------------------------------------------------------------------------Safearray::~Safearray

Safearray::~Safearray()                                      
{ 
    HRESULT hr = SafeArrayDestroy( _safearray );
    if( FAILED( hr ) )  Z_LOG( "~SafeArrayDestroy() => " << as_hex_string( hr ) << "\n" );
}
*/

//---------------------------------------------------------------------------vartype_from_safearray

VARTYPE vartype_from_safearray( SAFEARRAY* safearray )
{
    VARTYPE result = (VARTYPE)-1;
    
    if( safearray )
    {
        HRESULT hr = z_SafeArrayGetVartype( safearray, &result );

        if( FAILED( hr ) )  
        {
            result = (VARTYPE)-1;
        }
    }

    return result;
}

//----------------------------------------------------------------------------z_SafeArrayGetVartype

HRESULT z_SafeArrayGetVartype( SAFEARRAY* safearray, VARTYPE* result )
{
    if( !safearray )  return E_POINTER;

    HRESULT hr = SafeArrayGetVartype( safearray, result );
    if( FAILED( hr ) )
    {
#       ifdef Z_WINDOWS
            if( hr == E_INVALIDARG  &&  !is_windows_xp_or_more()  &&  safearray )         // Windows 2000?
            {
                if( safearray->fFeatures & FADF_VARIANT )  
                {
                    *result = VT_VARIANT;
                    return S_OK;
                }
                if( safearray->fFeatures & FADF_BSTR )  
                {
                    *result = VT_BSTR;
                    return S_OK;
                }
                if( safearray->fFeatures & FADF_UNKNOWN )  
                {
                    *result = VT_UNKNOWN;
                    return S_OK;
                }
                if( safearray->fFeatures & FADF_DISPATCH )  
                {
                    *result = VT_DISPATCH;
                    return S_OK;
                }
                //if( safearray->fFeatures == 0x2000 )   // Undokumentiert. Beobachtet unter Windows 2000 am 20.2.07
                //{
                //    *result = VT_UI1;
                //    return S_OK;
                //}
            }
#       endif

        Z_LOG( Z_FUNCTION << " SafeArrayGetVartype(" << (void*)safearray << ",fFeatures=0x" << hex << safearray->fFeatures << dec << ") ==> " << string_from_hresult( hr ) << "\n" );
        //2007-02-11 throw_com( hr, "SafeArrayGetVartype" );
    }

    return hr;
}

//-------------------------------------------------------Locked_any_safearray::Locked_any_safearray

Locked_any_safearray::Locked_any_safearray( SAFEARRAY* safearray )
:
    _safearray ( safearray ),
    _delete( false )
{ 
    //fprintf(stderr,"Locked_any_safearray::Locked_any_safearray(%X) _safearray->cbElements=%d\n", (int)(void*)_safearray, _safearray->cbElements );

    if( safearray == NULL )  throw_xc( "Z-COM-002" );

    HRESULT hr = SafeArrayLock( _safearray ); 
    if( FAILED(hr) )  throw_com( hr, "SafeArrayLock" );

    set_count();
}

//-------------------------------------------------------Locked_any_safearray::Locked_any_safearray

Locked_any_safearray::Locked_any_safearray( SAFEARRAY* safearray, VARENUM expected_vartype )
:
    _safearray ( safearray ),
    _delete( false )
{ 
    //fprintf(stderr,"Locked_any_safearray::Locked_any_safearray(%X) _safearray->cbElements=%d\n", (int)(void*)_safearray, _safearray->cbElements );
    if( safearray == NULL )  throw_xc( "Z-COM-002" );

    HRESULT hr = SafeArrayLock( _safearray ); 
    if( FAILED(hr) )  throw_com( hr, "SafeArrayLock" );

    set_count();

    if( _count > 0 )
    {
        if( vartype() != expected_vartype )     // vartype() nur gültig, wenn count > 0
        {
            SafeArrayUnlock( _safearray ), _safearray = NULL;
            throw_xc( "Z-COM-001", vartype_name( vartype() ), vartype_name( expected_vartype ) );
        }
    }
}

//-------------------------------------------------------Locked_any_safearray::Locked_any_safearray

Locked_any_safearray::Locked_any_safearray()
:
    _safearray( NULL ),
    _count(0),
    _delete( false )
{ 
}

//---------------------------------------------------------------------Locked_any_safearray::create

void Locked_any_safearray::create( int size, VARENUM variant_type )
{ 
    _delete = true;

    _safearray = SafeArrayCreateVector( variant_type, 0, size );
    if( !_safearray )  throw_com( E_OUTOFMEMORY, "SafeArrayCreateVector", as_string(size) );

    HRESULT hr = SafeArrayLock( _safearray ); 
    if( FAILED(hr) )  throw_com( hr, "SafeArrayLock" );

    _count = size;
}

//------------------------------------------------------Locked_any_safearray::~Locked_any_safearray

Locked_any_safearray::~Locked_any_safearray()
{ 
    if( _safearray )  
    {
        HRESULT hr = SafeArrayUnlock( _safearray ); 
        if( FAILED(hr) )  Z_LOG( "SafeArrayUnlock() => " << hex_from_int( hr ) << "\n" );//throw_com( hr, "SafeArrayUnlock" );

        if( _delete )
        {
            HRESULT hr = SafeArrayDestroy( _safearray );
            if( FAILED( hr ) )  Z_LOG( "SafeArrayDestroy() => " << hex_from_int( hr ) << "\n" );
        }
    }
}

//--------------------------------------------------------------------Locked_any_safearray::vartype

VARTYPE Locked_any_safearray::vartype() const
{
    VARTYPE result = 0;
    HRESULT hr = z_SafeArrayGetVartype( _safearray, &result );
    if( FAILED( hr ) )  throw_com( hr, "SafeArrayGetVartype" );

    return result;
}

//------------------------------------------------------------------Locked_any_safearray::set_count

void Locked_any_safearray::set_count()
{
    HRESULT hr;
    long32  lbound, ubound;
    
    hr = SafeArrayGetLBound( _safearray, 1, &lbound );
    if( FAILED(hr) )  throw_com( hr, "SafeArrayGetLBound" );

    hr = SafeArrayGetUBound( _safearray, 1, &ubound );
    if( FAILED(hr) )  throw_com( hr, "SafeArrayGetUBound" );

    _count = ubound - lbound + 1;
}

//---------------------------------------------------------Locked_any_safearray::take_safearray

SAFEARRAY* Locked_any_safearray::take_safearray()
{ 
    SAFEARRAY* result = _safearray;
 
    _safearray = NULL;
    _array     = NULL;
    _count     = 0;
    _delete    = false;

    return result;
}
 
//----------------------------------------------------------Locked_any_safearray::throw_index_error

void Locked_any_safearray::throw_index_error( int index )
{
    throw_com( DISP_E_BADINDEX, "Locked_safearray", as_string(index) ); 
}

//----------------------------------------------------------------------------Excepinfo::~Excepinfo

Excepinfo::~Excepinfo()
{
    SysFreeString( bstrSource );         bstrSource      = NULL;
    SysFreeString( bstrDescription );    bstrDescription = NULL;
    SysFreeString( bstrHelpFile );       bstrHelpFile    = NULL;
}

//-------------------------------------------------------------------------------Multi_qi::Multi_qi

Multi_qi::Multi_qi( int size )
: 
    _size(0), 
    _multi_qi(NULL) 
{
    allocate( size );
    if( size < 0 )  throw_xc( "Multi_qi::Multi_qi" );
    for( int i = 0; i < size; i++ )  memset( &_multi_qi[i], 0, sizeof (MULTI_QI) );
}

//------------------------------------------------------------------------------Multi_qi::~Multi_qi

Multi_qi::~Multi_qi()
{ 
    clear();
}

//-------------------------------------------------------------------------------Multi_qi::allocate

void Multi_qi::allocate( int size )
{
    clear();
    if( size > 0 )  _multi_qi = new MULTI_QI[size],  memset( _multi_qi, 0, sizeof (MULTI_QI) * size );
    _size = size;
}

//----------------------------------------------------------------------------------Multi_qi::clear

void Multi_qi::clear()
{
    for( int i = 0; i < _size; i++ )  
    { 
        MULTI_QI* m = &_multi_qi[i];

        delete (IID*)m->pIID;  
        if( m->pItf )  m->pItf->Release(); 
    }

    delete [] _multi_qi;  
    _multi_qi = NULL;  
    _size = 0; 
}

//-----------------------------------------------------------------------------Multi_qi::operator[] 

MULTI_QI& Multi_qi::operator[] ( int index )
{
    if( (uint)index >= (uint)_size )  throw_xc( "MULTI_QI-INDEX", index );

    return _multi_qi[index];
}

//--------------------------------------------------------------------------------Multi_qi::set_iid

void Multi_qi::set_iid( int index, const IID& iid )
{
    MULTI_QI* m = &(*this)[ index ];
    if( m->pIID )  delete (IID*)m->pIID;
    m->pIID = new IID( iid );
}

//--------------------------------------------------------------------------Multi_qi::set_interface

void Multi_qi::set_interface( int index, IUnknown* iunknown )
{
    MULTI_QI* m = &(*this)[ index ];
    if( m->pItf )  m->pItf->Release();
    m->pItf = iunknown;
    if( iunknown )  iunknown->AddRef();
}

//-------------------------------------------------------------Idispatch_dispids::Idispatch_dispids

Idispatch_dispids::Idispatch_dispids( IDispatch* idispatch, const Bstr& name ) 
: 
    _idispatch(idispatch), 
    _name(name) 
{
    _map[ "" ] = DISPID_VALUE;
}

//--------------------------------------------------------------------Idispatch_dispids::get_dispid

DISPID Idispatch_dispids::get_dispid( const Bstr& name )
{
    DISPID  result;

    HRESULT hr = Get_dispid( name, &result );
    if( FAILED(hr) )  throw_com( hr, "GetIDsOfNames", string_from_bstr(name).c_str() );

    return result;
}

//--------------------------------------------------------------------Idispatch_dispids::Get_dispid

HRESULT Idispatch_dispids::Get_dispid( const Bstr& name, DISPID* result )
{
    HRESULT hr = S_OK;

    if( !_idispatch )  return E_POINTER;

    Map::iterator it = _map.find( name );
    if( it != _map.end() )
    {
        *result = it->second;
    }
    else
    {
        hr = _idispatch->GetIDsOfNames( IID_NULL, const_cast<OLECHAR**>( &name._bstr ), 1, STANDARD_LCID, result );
        if( FAILED(hr) )  return hr;

        //_map.insert( it, Map::value_type( name, result ) );       Nicht für __GNUC__
        _map[ name ] = *result;
    }

    return hr;
}

//-------------------------------------------------------------------Com_initialize::Com_initialize

Com_initialize::Com_initialize()
{ 
    HRESULT hr = CoInitialize( NULL ); 
    if( FAILED(hr) ) throw_com( hr, "CoInitialize" ); 
}

//------------------------------------------------------------------Com_initialize::~Com_initialize

Com_initialize::~Com_initialize()
{ 
    CoUninitialize(); 
}

//---------------------------------------------------------------------------------com_property_get

Variant com_property_get( IDispatch* idispatch, const string& property )
{
    Dispparams dispparams;
    Excepinfo  excepinfo;
    uint       arg_nr = (uint)-1;
    DISPID     dispid;
    Variant    result;
    HRESULT    hr;
    Bstr       property_bstr ( property );

    hr = idispatch->GetIDsOfNames( IID_NULL, &property_bstr._bstr, 1, STANDARD_LCID, &dispid );
    if( FAILED(hr) )  throw_com( hr, "IDispatch::GetIDsOfNames", property.c_str() );

    hr = idispatch->Invoke( dispid, IID_NULL, STANDARD_LCID, DISPATCH_PROPERTYGET, &dispparams, &result, &excepinfo, &arg_nr );
    if( FAILED(hr) )  throw_com_excepinfo( hr, &excepinfo, "IDispatch::Invoke", property.c_str() );

    return result;
}

//------------------------------------------------------------------------------------Name_to_clsid

HRESULT Name_to_clsid( const char* class_name, CLSID* result )
{
    return Name_to_clsid( Bstr(class_name), result );
}

//------------------------------------------------------------------------------------Name_to_clsid

HRESULT Name_to_clsid( const OLECHAR* class_name, CLSID* result )
{
    if( class_name == NULL )  return REGDB_E_CLASSNOTREG;


    if( class_name[0] == '{' )
    {
        return CLSIDFromString( const_cast<OLECHAR*>( class_name ), result );
    }
    else
    {
        return CLSIDFromProgID( class_name, result ); 
    }
}

//-----------------------------------------------------------------------------------------Com_call
/*
HRESULT Com_call( VARIANT* result, IDispatch* idispatch, const string& method, 
                  const Variant& p1, 
                  const Variant& p2, 
                  const Variant& p3, 
                  const Variant& p4, 
                  const Variant& p5 )
{
    HRESULT hr;

    vector<Variant> variant_array ( 2 );
    variant_array[0] = par2;
    variant_array[1] = par1;
    return com_invoke( DISPATCH_METHOD, object, method, &variant_array );
    
    ...

}
*/
//----------------------------------------------------------------------------------clsid_from_name

CLSID clsid_from_name( const char* class_name )
{
    CLSID clsid;

    HRESULT hr = Name_to_clsid( class_name, &clsid );
    if( FAILED(hr) ) throw_com( hr, "name_to_clsid", class_name );

    return clsid;
}

//-----------------------------------------------------------------com_date_from_seconds_since_1970

const uint64 msec_until_1970 = ( (int64)70*365250 / 1000 * 1000 ) * 24*3600 + 2*24*3600*1000;

DATE com_date_from_seconds_since_1970( double seconds_since_1970 )
{ 
    uint64       msec   = (uint64)( seconds_since_1970 * 1000 + 0.5 );  // Sonst gibt's Rundungsfehler
    DATE         result = (double)( msec + msec_until_1970 ) / (24*3600*1000); 
    
    //DATE result = ( msec / (24*3600*1000) + floor( 70*365.25 ) + 2; 

#   ifdef Z_DEBUG
        VARIANT v;  v.vt = VT_DATE;  V_DATE( &v ) = result;     // TEST
#   endif

    return result;
}

//-----------------------------------------------------------com_date_from_local_seconds_since_1970

DATE com_date_from_local_seconds_since_1970( double seconds_since_1970 )
{ 
    return com_date_from_seconds_since_1970( seconds_since_1970 - timezone ); 
}

//-----------------------------------------------------------------------------time_t_from_com_date

double seconds_since_1970_from_com_date( DATE date )
{ 
    return ( (uint64)( date * 24*3600*1000 ) - msec_until_1970 ) / 1000.0;
}

//-----------------------------------------------------------seconds_since_1970_from_local_com_date

double seconds_since_1970_from_local_com_date( DATE date )
{ 
    return seconds_since_1970_from_com_date( date ) + timezone; 
}

//-------------------------------------------------------------------------------------------------

} //namespace com
} //namespace zschimmer
