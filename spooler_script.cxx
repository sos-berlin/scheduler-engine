// $Id: spooler_script.cxx,v 1.14 2002/09/11 10:05:15 jz Exp $
/*
    Hier sind implementiert

    Script
    Script_instance
*/


#include "spooler.h"
#include "../file/anyfile.h"

using namespace std;

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------check_result

bool check_result( const CComVariant& vt )
{
    if( vt.vt == VT_EMPTY    )  return true;                       // Keine Rückgabe? True, also weiter machen
    if( vt.vt == VT_NULL     )  return false;                      // NULL? False
    if( vt.vt == VT_DISPATCH )  return vt.pdispVal != NULL;        // Nothing => False, also Ende
    if( vt.vt == VT_BOOL     )  return vt.bVal != NULL;            // Nothing => False, also Ende

    CComVariant v = vt;

    HRESULT hr = v.ChangeType( VT_BOOL );
    if( FAILED(hr) )  throw_ole( hr, "VariantChangeType" );
    return vt.bVal != 0;
}

//----------------------------------------------------------------------------------Script::set_xml

void Script::set_xml( const xml::Element_ptr& element, const string& include_path )
{
    clear();

    _language       = as_string( element->getAttribute( L"language" ) );
    _source         = text_from_xml_with_include( element, include_path );
    _com_class_name = as_string( element->getAttribute( L"com_class" ) );
    _filename       = as_string( element->getAttribute( L"filename" ) );

    if( _com_class_name != "" )
    {
        if( _language != "" )  throw_xc( "SPOOLER-145" );
    }
    else
    {
        if( _language == "" )  _language = "VBScript";
    }

    string use_engine = as_string( element->getAttribute( L"use_engine" ) );
    
    if( use_engine == ""
     || use_engine == "task" )  _reuse = reuse_task;
    else
    if( use_engine == "job"  )  _reuse = reuse_job;

    _set = true;
}

//----------------------------------------------------------------Script_instance::~Script_instance

Script_instance::~Script_instance()
{
    try {
        close();
    }
    catch( const Xc& ) {}
}

//----------------------------------------------------------------------------Script_instance::init

void Script_instance::init( Script* script )
{
    _script = script;

    if( !_script->set() )  throw_xc( "SPOOLER-146" );

    if( _script->_language != "" )
    {
        _script_site = new Script_site;
        _script_site->_engine_name = _script->_language;
        _script_site->init_engine();

        _idispatch = _script_site->dispatch();
    }
    else
    {
        HRESULT hr;
        CLSID   clsid = string_as_clsid( _script->_com_class_name );

        if( _script->_filename != "" )
        {
            if( !_module )
            {
                _log->debug( "LoadLibrary " + _script->_filename );
                _module = LoadLibrary( _script->_filename.c_str() );
                if( !_module )  throw_mswin_error( "LoadLibrary", _script->_filename.c_str() );   
            }


            _DllGetClassObject = (DllGetClassObject_func)GetProcAddress( _module, "DllGetClassObject" );
            if( !_DllGetClassObject )  throw_mswin_error( "GetProcAddress DllGetClassObject", _script->_filename.c_str() );


            CComPtr<IClassFactory> class_factory;

            hr = _DllGetClassObject( &clsid, (IID*)&IID_IClassFactory, (void**)&class_factory );
            if( FAILED(hr) )  throw_ole( hr, (_script->_filename + "::DllGetClassObject").c_str(), _script->_com_class_name.c_str() );

            hr = class_factory->CreateInstance( NULL, IID_IDispatch, (void**)&_idispatch );
            if( FAILED(hr) )  throw_ole( hr, "CreateInstance", _script->_com_class_name.c_str() );
        }
        else
        {
            hr = _idispatch.CoCreateInstance( clsid );
            if( FAILED(hr) )  throw_ole( hr, "CoCreateInstance", _script->_com_class_name.c_str() );
        }
    }

    //HRESULT hr = _script_site->_script->SetScriptState( SCRIPTSTATE_INITIALIZED );
    //if( FAILED( hr ) )  throw_ole( hr, "IActiveScript::SetScriptState", "SCRIPTSTATE_INITIALIZED" );

    _com_context = new Com_context;
}

//-------------------------------------------------------------------------Script_instance::add_obj

void Script_instance::add_obj( const CComPtr<IDispatch>& object, const string& name )
{
    if( _script_site )
    {
        CComBSTR name_bstr;
        name_bstr.Attach( SysAllocString_string( name ) );

        _script_site->add_obj( object, name_bstr );
    }
    else
    {
        if( name == "spooler_log"    )  _com_context->_log     = (CComQIPtr<spooler_com::Ilog>)    object;
        else
        if( name == "spooler"        )  _com_context->_spooler = (CComQIPtr<spooler_com::Ispooler>)object;
        else
        if( name == "spooler_thread" )  _com_context->_thread  = (CComQIPtr<spooler_com::Ithread>) object;
        else
        if( name == "spooler_job"    )  _com_context->_job     = (CComQIPtr<spooler_com::Ijob>)    object;
        else
        if( name == "spooler_task"   )  _com_context->_task    = (CComQIPtr<spooler_com::Itask>)   object;
        else
            throw_xc( "Script_instance::add_obj", name.c_str() );
    }
}

//----------------------------------------------------------------------------Script_instance::load

void Script_instance::load()
{
    if( _script_site )     // Scripting Engine?
    {
        /*if( !_script_site )  init( script._language );
                       else*/  if( _script_site->_engine_name != _script->_language )  throw_xc( "SPOOLER-117" );

        if( _com_context->_log     )  _script_site->add_obj( _com_context->_log, L"spooler_log"    );
        if( _com_context->_spooler )  _script_site->add_obj( _com_context->_log, L"spooler"        );
        if( _com_context->_thread  )  _script_site->add_obj( _com_context->_log, L"spooler_thread" );
        if( _com_context->_job     )  _script_site->add_obj( _com_context->_log, L"spooler_job"    );
        if( _com_context->_task    )  _script_site->add_obj( _com_context->_log, L"spooler_task"   );

        HRESULT hr = _script_site->_script->SetScriptState( SCRIPTSTATE_INITIALIZED );
        if( FAILED( hr ) )  throw_ole( hr, "IActiveScript::SetScriptState", "SCRIPTSTATE_INITIALIZED" );

        Z_FOR_EACH_CONST( Source_with_parts::Parts, _script->_source._parts, it )
        {
            _script_site->parse( it->_text, it->_linenr );
        }
    }

    if( name_exists( "spooler_set_context" ) )
    {
        com_call( _idispatch, "spooler_set_context", &CComVariant(_com_context) );
    }

    _loaded = true;
}

//---------------------------------------------------------------------------Script_instance::start

void Script_instance::start()
{
    if( _script_site )
    {
        HRESULT hr = _script_site->_script->SetScriptState( SCRIPTSTATE_STARTED );
        if( FAILED( hr ) )  throw_ole( hr, "IActiveScript::SetScriptState", "SCRIPTSTATE_STARTED" );
    }
}

//---------------------------------------------------------------------------Script_instance::close

void Script_instance::close()
{
    if( _com_context )  _com_context->close();

    if( _idispatch )
    {
        try
        {
            call_if_exists( "spooler_exit" );
        }
        catch( const Xc& x ) { _log->error( x.what() ); }
    }

    _idispatch = NULL;

    if( _script_site )
    {
        _script_site->close_engine();
        _script_site = NULL;
    }

    if( _module ) 
    {
        _log->debug( "FreeLibrary " + _script->_filename );
        FreeLibrary( _module );
        _module = NULL;
    }

    _loaded = false;
}

//---------------------------------------------------------------------Script_instance::name_exists

bool Script_instance::name_exists( const string& name )
{ 
    bool exists;

    map<string,bool>::iterator it = _names.find( name );
    if( it == _names.end() )  exists = _names[name] = com_name_exists( _idispatch, name );
                        else  exists = it->second;

    return exists;                        
}

//------------------------------------------------------------------Script_instance::call_if_exists

CComVariant Script_instance::call_if_exists( const char* name )
{
    if( name_exists(name) )  return call( name );
                       else  return CComVariant();
}

//----------------------------------------------------------------------------Script_instance::call

CComVariant Script_instance::call( const char* name )
{
    //return _script_site->call( name );
    return com_call( _idispatch, name );
}

//----------------------------------------------------------------------------Script_instance::call

CComVariant Script_instance::call( const char* name, int param )
{
    //return _script_site->call( name, param );
    return com_call( _idispatch, name, param );
}

//--------------------------------------------------------------------Script_instance::property_get

CComVariant Script_instance::property_get( const char* name )
{
    //return _script_site->property_get( name );
    return com_property_get( _idispatch, name );
}

//-----------------------------------------------------------Script_instance::optional_property_put

void Script_instance::optional_property_put( const char* name, const CComVariant& v )
{
    try 
    {
        property_put( name, v );
    }
    catch( const Xc& )
    {
        // Ignorieren, wenn das Objekt die Eigenschaft nicht kennt
    }
}

//-----------------------------------------------------------------------Script_instance::interrupt
/*
void Script_instance::interrupt()
{
    if( _script_site )  _script_site->interrupt();
}
*/
//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
