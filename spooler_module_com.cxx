// $Id: spooler_module_com.cxx,v 1.3 2002/11/24 15:12:50 jz Exp $
/*
    Hier sind implementiert

    Com_module_instance_base
    Com_module_instance
    Scripting_engine_module_instance
*/


#include "spooler.h"
#include "../file/anyfile.h"

#ifdef Z_WINDOWS

using namespace std;

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------check_result

bool check_result( const Variant& vt )
{
    if( vt.vt == VT_EMPTY    )  return true;                       // Keine Rückgabe? True, also weiter machen
    if( vt.vt == VT_NULL     )  return false;                      // NULL? False
    if( vt.vt == VT_DISPATCH )  return vt.pdispVal != NULL;        // Nothing => False, also Ende
    if( vt.vt == VT_BOOL     )  return vt.bVal != 0;               // Nothing => False, also Ende

    Variant v = vt;

    HRESULT hr = v.ChangeType( VT_BOOL );
    if( FAILED(hr) )  throw_ole( hr, "VariantChangeType" );

    return v.bVal != 0;
}

//--------------------------------------------------------Com_module_instance::~Com_module_instance

Com_module_instance::~Com_module_instance()
{
    try {
        close();
    }
    catch( const Xc& ) {}
}

//-------------------------------------------------------------------------Com_module_instance_base::init

void Com_module_instance_base::init()
{
    Module_instance::init();
}

//------------------------------------------------------------------Com_module_instance_base::close

void Com_module_instance_base::close()
{
    Module_instance::close();

    _idispatch = NULL;
    _loaded = false;
}

//------------------------------------------------------------------------Com_module_instance::load

void Com_module_instance::load()
{
    if( name_exists( "spooler_set_context" ) )
    {
        Variant com_context_vt = _com_context;
        com_call( _idispatch, "spooler_set_context", &_com_context_vt );
    }

    _loaded = true;
}

//------------------------------------------------------------Com_module_instance_base::name_exists

bool Com_module_instance_base::name_exists( const string& name )
{ 
    bool exists;

    map<string,bool>::iterator it = _names.find( name );
    if( it == _names.end() )  exists = _names[name] = com_name_exists( _idispatch, name );
                        else  exists = it->second;

    return exists;                        
}

//-------------------------------------------------------------------Com_module_instance_base::call

Variant Com_module_instance_base::call( const string& name )
{
    return com_call( _idispatch, name );
}

//-------------------------------------------------------------------Com_module_instance_base::call

Variant Com_module_instance_base::call( const string& name, int param )
{
    return com_call( _idispatch, name, param );
}

//-----------------------------------------------------------Com_module_instance_base::property_get
/*
Variant Com_module_instance_base::property_get( const string& name )
{
    return com_property_get( _idispatch, name );
}
*/
//------------------------------------------------------------------------Com_module_instance::init

void Com_module_instance::init()
{
    Com_module_instance_base::init();

    HRESULT hr;
    CLSID   clsid = string_as_clsid( _module->_com_class_name );

    if( _module->_filename != "" )
    {
        if( !_module )
        {
            _log->debug( "LoadLibrary " + _module->_filename );
            _com_module = LoadLibrary( _module->_filename.c_str() );
            if( !_com_module )  throw_mswin_error( "LoadLibrary", _module->_filename.c_str() );   
        }


        _DllGetClassObject = (DllGetClassObject_func)GetProcAddress( _com_module, "DllGetClassObject" );
        if( !_DllGetClassObject )  throw_mswin_error( "GetProcAddress DllGetClassObject", _module->_filename.c_str() );


        ptr<IClassFactory> class_factory;

        hr = _DllGetClassObject( &clsid, (IID*)&IID_IClassFactory, class_factory.void_pp() );
        if( FAILED(hr) )  throw_ole( hr, (_module->_filename + "::DllGetClassObject").c_str(), _module->_com_class_name.c_str() );

        hr = class_factory->CreateInstance( NULL, IID_IDispatch, _idispatch.void_pp() );
        if( FAILED(hr) )  throw_ole( hr, "CreateInstance", _module->_com_class_name.c_str() );
    }
    else
    {
        hr = _idispatch.CoCreateInstance( clsid );
        if( FAILED(hr) )  throw_ole( hr, "CoCreateInstance", _module->_com_class_name.c_str() );
    }
}

//-----------------------------------------------------------------------Com_module_instance::close

void Com_module_instance::close()
{
    Com_module_instance_base::close();

    if( _com_module ) 
    {
        _log->debug( "FreeLibrary " + _module->_filename );
        FreeLibrary( _com_module );
        _com_module = NULL;
    }

    _loaded = false;
}

//------------------------------Scripting_engine_module_instance::~Scripting_engine_module_instance

Scripting_engine_module_instance::~Scripting_engine_module_instance()
{
    try {
        close();
    }
    catch( const Xc& ) {}
}

//-----------------------------------------------------------Scripting_engine_module_instance::init

void Scripting_engine_module_instance::init()
{
    Com_module_instance_base::init();

    _script_site = new Script_site;
    _script_site->_engine_name = _module->_language;
    _script_site->init_engine();

    _idispatch = _script_site->dispatch();

    //HRESULT hr = _script_site->_module->SetScriptState( SCRIPTSTATE_INITIALIZED );
    //if( FAILED( hr ) )  throw_ole( hr, "IActiveScript::SetScriptState", "SCRIPTSTATE_INITIALIZED" );
}

//------------------------------------------------------------------Com_module_instance_base::close

void Scripting_engine_module_instance::close()
{
    Com_module_instance_base::close();

    if( _script_site )
    {
        _script_site->close_engine();
        _script_site = NULL;
    }

    _loaded = false;
}

//---------------------------------------------------------------------Scripting_engine_module_instance::load

void Scripting_engine_module_instance::load()
{
    if( _script_site->_engine_name != _module->_language )  throw_xc( "SPOOLER-117" );

    if( _com_context->_log     )  _script_site->add_obj( _com_context->_log    , L"spooler_log"    );
    if( _com_context->_spooler )  _script_site->add_obj( _com_context->_spooler, L"spooler"        );
    if( _com_context->_thread  )  _script_site->add_obj( _com_context->_thread , L"spooler_thread" );
    if( _com_context->_job     )  _script_site->add_obj( _com_context->_job    , L"spooler_job"    );
    if( _com_context->_task    )  _script_site->add_obj( _com_context->_task   , L"spooler_task"   );

    HRESULT hr = _script_site->_script->SetScriptState( SCRIPTSTATE_INITIALIZED );
    if( FAILED( hr ) )  throw_ole( hr, "IActiveScript::SetScriptState", "SCRIPTSTATE_INITIALIZED" );

    Z_FOR_EACH_CONST( Source_with_parts::Parts, _module->_source._parts, it )
    {
        _script_site->parse( it->_text, it->_linenr );
    }

    _loaded = true;
}

//------------------------------------------------------------------Scripting_engine_module_instance::add_obj

void Scripting_engine_module_instance::add_obj( const ptr<IDispatch>& object, const string& name )
{
    _script_site->add_obj( object, Bstr(name) );

    Com_module_instance_base::add_obj( object, name );
}

//--------------------------------------------------------------------Scripting_engine_module_instance::start

void Scripting_engine_module_instance::start()
{
    HRESULT hr = _script_site->_script->SetScriptState( SCRIPTSTATE_STARTED );
    if( FAILED( hr ) )  throw_ole( hr, "IActiveScript::SetScriptState", "SCRIPTSTATE_STARTED" );
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
