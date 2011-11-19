// $Id: sosscrpt.cxx 13691 2008-09-30 20:42:20Z jz $

// §1826

// Klasse Script_site für IActiveScript (Microsoft Scripting Engine)

#include "precomp.h"

#include <map>

#include "sos.h"
#include "sosarray.h"
#include "sos_java.h"
#include "com_simple_standards.h"
#include "thread_semaphore.h"

#ifdef SYSTEM_WIN
#   include <initguid.h>       // GUIDs hier initialisieren!
#endif

#include "sosscrpt.h"

#include "../zschimmer/z_com.h"
#include "../zschimmer/loaded_module.h"
#include "../zschimmer/spidermonkey_scripting_engine.h"

using zschimmer::ptr;
using namespace zschimmer::com; //::Bstr;

//struct JavaVM;


//---------------------------------------------------------------------------------------------

using namespace std;
namespace sos {

//--------------------------------------------------------------------Script_site::Factory_site

Script_site::Script_site()
: 
    _zero_(this+1),
    _engine_name( "VBScript" )
{
}
                             
//--------------------------------------------------------------------Script_site::~Script_site

Script_site::~Script_site()
{
    LOGI( "~Script_site\n" );

    close_engine();
/*
    if( _engine_factory ) {
        _engine_factory->Release();
        _engine_factory = NULL;
    }
*/
    SOS_DELETE( _script_exception );
}
                             
//---------------------------------------------------------------------------Script_site::clear
// Vor CoUninitialize() rufen! 

void Script_site::clear()
{ 
/*  Stürzt manchmal ab, also lassen wir das. jz 21.2.01

    if( scripting_engine_factory_map_used ) 
    {
        scripting_engine_factory_map_used = false;
        scripting_engine_factory_map.clear();
    }
*/
}

//------------------------------------------------------------------Script_site::QueryInterface

STDMETHODIMP_( HRESULT ) Script_site::QueryInterface( REFIID riid, void ** ppvObject )
{
    *ppvObject = NULL;

    if( riid == IID_IUnknown 
     || riid == IID_IActiveScriptSite )  *ppvObject = this;

    if( !*ppvObject )  return E_NOINTERFACE;

    ((IUnknown*)*ppvObject)->AddRef();
    return NOERROR;
}

//--------------------------------------------------------------------Script_site::OnScriptError

STDMETHODIMP_( HRESULT ) Script_site::OnScriptError( IActiveScriptError *pscripterror )
{
    DWORD       dwCookie;
    LONG        column_no;
    ULONG       line_no;
    BSTR        source_line = NULL;
    Sos_string  source;
    EXCEPINFO   excepinfo; 
    Sos_string  description;
    
    LOGI( "Script_site::OnScriptError\n" );

    memset( &excepinfo, 0, sizeof excepinfo );

    pscripterror->GetSourcePosition( &dwCookie, &line_no, &column_no );
    pscripterror->GetSourceLineText( &source_line );
    pscripterror->GetExceptionInfo( &excepinfo );

    if( excepinfo.bstrSource  &&  excepinfo.bstrSource[ 0 ] ) {
        source = bstr_as_string( excepinfo.bstrSource );
        //swprintf( text, OLESTR("%s, "), excepinfo.bstrSource );    // Dateiname oder "Kompilierungsfehler in VBScript"
    }

    if( source_line )  source = ", scriptline=" + bstr_as_string( source_line );

    SOS_DELETE( _script_exception );

    string descr = bstr_as_string( excepinfo.bstrDescription );
    
    if( stricmp( c_str( source ), "msg" ) == 0 ) 
    {
        _script_exception = new Xc( "", descr.c_str() );  // Nur Meldung zeigen, sonst nix, für Document Factory
        _script_exception->set_name( "MSG" );   // Wird in Basic zu Source
    } 
    else 
    {
        if( !empty( _source ) )  source = _source + " " + source;

        char code[20]; 
        if( excepinfo.scode )  sprintf( code, "COM-%08lX", (long)excepinfo.scode );
                         else  strcpy( code, "SOS-1425" );

        _script_exception = new Xc( code, descr.c_str(), Source_pos( c_str(source), line_no, column_no ) );
    }
    if( !_script_exception )  throw_no_memory_error();

    SysFreeString( source_line );
    SysFreeString( excepinfo.bstrSource );
    SysFreeString( excepinfo.bstrDescription );
    SysFreeString( excepinfo.bstrHelpFile );

    return E_FAIL;
}

//---------------------------------------------------------------------Script_site::GetItemInfo

STDMETHODIMP_( HRESULT ) Script_site::GetItemInfo( LPCOLESTR pstrName, DWORD dwReturnMask, 
                                                   IUnknown** ppunkItem, ITypeInfo** ppTypeInfo )
{
    HRESULT     hr;

    if( dwReturnMask & SCRIPTINFO_IUNKNOWN )   *ppunkItem = NULL;
    if( dwReturnMask & SCRIPTINFO_ITYPEINFO )  *ppTypeInfo = NULL;

    Bstr nam = pstrName;
    nam.to_lower();

    Obj_map::iterator it = _obj_map.find( nam );
    if( it == _obj_map.end() )  return TYPE_E_ELEMENTNOTFOUND;

    if( dwReturnMask & SCRIPTINFO_IUNKNOWN ) 
    {
        if( !it->second )  return TYPE_E_ELEMENTNOTFOUND;        // Sollte nicht passieren

        *ppunkItem = it->second;
        (*ppunkItem)->AddRef();
    }
    
    if( dwReturnMask & SCRIPTINFO_ITYPEINFO )
    {
        IProvideClassInfo* pClassInfo = NULL;

        hr = it->second->QueryInterface( IID_IProvideClassInfo, (void**)&pClassInfo );
        if( FAILED( hr ) )  { LOG( "IProvideClassInfo::QueryInterface versagt\n" );  return hr; } //throw_ole( hr, "IProvideClassInfo::QueryInterface" );  // E_NOINTERFACE

        hr = pClassInfo->GetClassInfo( ppTypeInfo );

        pClassInfo->Release();

        if( FAILED( hr ) )  { LOG( "IProvideClassInfo::GetClassInfo versagt\n" );  return hr; }  //throw_ole( hr, "IProvideClassInfo::GetClassInfo" );
    }

    return S_OK;
}

//----------------------------------------------------------------------------Script_site::init

void Script_site::init()
{
}
                             
//-----------------------------------------------------------------------Script_site::init_engine

void Script_site::init_engine()
{
    HRESULT hr;
    ptr<IClassFactory>  factory;

    if( _script_parse || _script )  throw_xc( "Script_site::init_engine" );

    // Scripting Engine holen

    #ifdef Z_WINDOWS
    if( stricmp( _engine_name.c_str(), "spidermonkey" ) == 0 
     || stricmp( _engine_name.c_str(), "javascript"   ) == 0 )   // Für Microsoft's Javascript nehmen wir "JScript"
    {
        _script_parse = static_cast<IActiveScriptParse*>( com_create_instance( CLSID_Spidermonkey, IID_IActiveScriptParse, NULL, "spidermonkey" ) );
    }
    else
    #endif
    {
        CLSID clsid;

        try
        {
            clsid = stricmp( _engine_name.c_str(), "perl" ) == 0? string_as_clsid( "PerlScript" )
                                                                : string_as_clsid( _engine_name );
        }
        catch( const exception& x )
        {
            if( zschimmer::string_begins_with( x.what(), "COM-80040154" ) )  throw_xc( "SOS-1450", _engine_name, x.what() );
            throw;
        }

        if( !factory )
        {
            hr = CoGetClassObject( clsid, CLSCTX_ALL, NULL, IID_IClassFactory, (void**)&factory );
            if( FAILED( hr ) )  throw_ole( hr, "CoGetClassObject", _engine_name.c_str() );
        }

        hr = factory->CreateInstance( NULL, IID_IActiveScriptParse, (void**)&_script_parse );
        if( FAILED(hr) )  throw_ole( hr, "IClassFactory::CreateInstance", "IID_IActiveScriptParse" );
    }


    // Scripting Engine initialisieren:

    hr = _script_parse->InitNew();
    if( FAILED(hr) )  throw_ole( hr, "IActiveScriptParse::InitNew" );


    hr = _script_parse->QueryInterface( IID_IActiveScript, (void**)&_script );
    if( FAILED(hr) )  throw_ole( hr, "IActiveScriptParse::QueryInterface" );


    hr = _script->SetScriptSite( this );
    if( FAILED(hr) )  throw_ole( hr, "IActiveScript::SetScriptSite" );

    _dispatch = NULL;
    hr = _script->GetScriptDispatch( NULL, _dispatch.pp() );
    if( FAILED(hr) )  throw_ole( hr, "IActiveScript::GetScriptDispatch" );


    _init_engine_called = true;


#   ifdef SYSTEM_WIN
        // ActiveState Perl 5.8 meldet COM-Fehler nicht mehr. Dies muss eingeschaltet werden.
        if( stricmp( _engine_name.c_str(), "PerlScript" ) == 0 
         || stricmp( _engine_name.c_str(), "Perl"       ) == 0 )
        {
            parse( "Win32::OLE->Option( Warn => 3 );\n" );
        }
#   endif
}

//----------------------------------------------------------------------Script_site::close_engine

void Script_site::close_engine()
{
    //LOGI( "Script_site::close_engine\n" );

    HRESULT hr;

    _dispatch = NULL;

    if( _script ) 
    { 
        Z_LOG2( "zschimmer", "IActiveScript::Close()\n" );
        hr = _script->Close();
        if( FAILED( hr ) )  LOG( "Script_site::close_engine: IActiveScript::Close versagt\n" );
        _script->Release();  
        _script = NULL; 
    }

    if( _script_parse ) 
    { 
        LOGI( "Script_site::close_engine()  Release script\n" );
        _script_parse->Release();  
        _script_parse = NULL; 
    }

    _obj_map.clear();

    _init_engine_called = false;

    //LOG( "Script_site::close_engine OK\n" );
}

//-------------------------------------------------------------------Script_site::reset_engine

void Script_site::reset_engine()
{
    LOGI( "Script_site::reset_engine\n" );
    HRESULT hr;

    hr = _script->SetScriptState( SCRIPTSTATE_INITIALIZED );    
    if( FAILED(hr) )  throw_ole( hr, "IActiveScript::SetScriptState", "SCRIPTSTATE_INITIALIZED" );
}

//------------------------------------------------------------------------Script_site::add_obj

void Script_site::add_obj( IDispatch* obj, const OLECHAR* name )
{
    add_obj( obj, name, SCRIPTTEXT_ISVISIBLE ); 
}

//------------------------------------------------------------------------Script_site::add_obj

void Script_site::add_obj( IDispatch* obj, const OLECHAR* name, uint4 flags )
{
    HRESULT hr;

    if( !obj     )  throw_xc( "SOS-1411", w_as_string(name) );
    if( !_script )  throw_xc( "SOS-1428" );
    if( name == NULL  ||  name[0] == 0 )  throw_xc( "SOS-1456", "add_obj/AddNamedItem" );

    Bstr nam = name;
    nam.to_lower();

    _obj_map[ nam ] = obj;      // Vor AddNamedItem(), denn PerlScript von ActiveState ruft sofort GetItemInfo() auf.
                                // Bei einen Fehler in AddNamedItem() lassen wir das Objekt erstmal stehen.

    hr = _script->AddNamedItem( name, flags );
    if( FAILED( hr ) )  throw_ole( hr, "IActiveScript::AddNamedItem" );
}

//--------------------------------------------------------------------------Script_site::parse

Variant Script_site::parse( const string& script_text, Scripttext_flags flags, int linenr )
{
    HRESULT   hr;
    EXCEPINFO excep;        
    Variant   result;

    memset( &excep, 0, sizeof excep );

    LOGI( "Script_site::parse(" << count( script_text.begin(), script_text.end(), '\n' ) << " lines,, at line " << linenr << ")\n" );

    SOS_DELETE( _script_exception );

    Bstr script_text_bstr = script_text;
    if( !script_text_bstr )  throw_xc( "EMPTY-SCRIPT", __FUNCTION__ );      // Sonst stürzt ParseScriptText() ab.

    hr = _script_parse->ParseScriptText( script_text_bstr,
                                         NULL, 
                                         NULL, 
                                         NULL, 
                                         0, 
                                         linenr,
                                         flags,   // SCRIPTTEXT_ISVISIBLE, 
                                         &result,  // Nur für SCRIPTTEXT_ISEXPRESSION
                                         &excep
                                       );


    if( _script_exception ) {
        SysFreeString( excep.bstrSource );
        SysFreeString( excep.bstrDescription );
        SysFreeString( excep.bstrHelpFile );
        throw *_script_exception;
    }

    if( FAILED( hr ) ) {
        if( !excep.bstrSource  ||  !excep.bstrSource[0] )  excep.bstrSource = SysAllocString_string( _source );
        throw_ole_excepinfo( hr, &excep, "IActiveScriptParse::ParseScriptText" ); 
    }


    _dispatch = NULL;
    hr = _script->GetScriptDispatch( NULL, &_dispatch._ptr );
    if( FAILED(hr) )  throw_ole( hr, "GetScriptDispatch" );

    return result;
}

//--------------------------------------------------------------------------------Script_site::call

Variant Script_site::call( const string& name )
{
    try 
    {
        return com_call( _dispatch, name );
    }
    catch( const Xc& )
    {
        if( _script_exception )  throw *_script_exception;
        throw;
    }
}

//---------------------------------------------------------------------------------Script_site::call

Variant Script_site::call( const string& name, const Variant& par )
{
    try 
    {
        return com_call( _dispatch, name, par );
    }
    catch( const Xc& )
    {
        if( _script_exception )  throw *_script_exception;
        throw;
    }
}

//---------------------------------------------------------------------------------Script_site::call

Variant Script_site::call( const string& name, Variant* par )
{
    try 
    {
        return com_call( _dispatch, name, par );
    }
    catch( const Xc& )
    {
        if( _script_exception )  throw *_script_exception;
        throw;
    }
}

//-------------------------------------------------------------------------Script_site::property_put

void Script_site::property_put( const string& name, const Variant& value )
{
    try 
    {
        com_property_put( _dispatch, name, value );
    }
    catch( const Xc& )
    {
        if( _script_exception )  throw *_script_exception;
        throw;
    }
}

//----------------------------------------------------------------------Script_site::property_putref

void Script_site::property_putref( const string& name, const Variant& value )
{
    try 
    {
        com_property_putref( _dispatch, name, value );
    }
    catch( const Xc& )
    {
        if( _script_exception )  throw *_script_exception;
        throw;
    }
}

//-------------------------------------------------------------------------Script_site::property_get

Variant Script_site::property_get( const string& name )
{
    try 
    {
        return com_property_get( _dispatch, name );
    }
    catch( const Xc& )
    {
        if( _script_exception )  throw *_script_exception;
        throw;
    }
}

//-------------------------------------------------------------------------Script_site::name_exists

bool Script_site::name_exists( const string& name )
{ 
    bool exists;

    map<string,bool>::iterator it = _names.find( name );
    
    if( it == _names.end() )  exists = _names[name] = com_name_exists( _dispatch, name );
                        else  exists = it->second;

    return exists;
}

//---------------------------------------------------------------------------Script_site::interrupt

void Script_site::interrupt()
{
    EXCEPINFO      excepinfo;
    SCRIPTTHREADID thread_id;
    Bstr           description;
    HRESULT hr;

    memset( &excepinfo, 0, sizeof excepinfo );
    description = SysAllocStringLen_char( "SOS-1427", 8 );
    excepinfo.bstrDescription = description;

    hr = _script->GetCurrentScriptThreadID( &thread_id );
    if( FAILED(hr) )  throw_ole( hr, "GetCurrentScriptThreadID" );

    _script->InterruptScriptThread( thread_id, &excepinfo, 0 );
    if( FAILED(hr) )  throw_ole( hr, "InterruptScriptThread" );
}

//-------------------------------------------------------------------------------------------------

string detect_script_language( const string& script )
{
    string engine_name = "VBScript";    // Default

    // Sprache erkennen:
    const char* p = c_str( script );
    while( isspace( p[0] ) )  p++;

    if( p[0] == '\''
     || strnicmp( p, "sub ", 4 ) == 0
     || strnicmp( p, "dim ", 4 ) == 0
     || strnicmp( p, "set ", 4 ) == 0 ) 
    {
        engine_name = "VBScript";
    }
    else
    if( p[0] == '/'  
     || strncmp( p, "function ", 9 ) == 0 )
    {
        engine_name = "JavaScript";
    }
    else
    if( p[0] == '#' 
     || strncmp( p, "use ", 4 ) == 0 )
    {
        engine_name = "PerlScript";
    }

    return engine_name;
}

//--------------------------------------------------------------------------------------------

} //namespace sos
