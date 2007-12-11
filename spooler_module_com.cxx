// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com
/*
    Hier sind implementiert

    Com_module_instance_base
    Com_module_instance
    Scripting_engine_module_instance
*/


#include "spooler.h"
#include "../file/anyfile.h"


using namespace std;

namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------------check_result

bool check_result( const Variant& vt )
{
    if( vt.vt == VT_BOOL     )  return V_BOOL(&vt) != 0;
    if( vt.vt == VT_EMPTY    )  return true;                       // Keine Rückgabe? True, also weiter machen
    if( vt.vt == VT_ERROR  &&  vt.scode == DISP_E_UNKNOWNNAME )  return true;   // Funktion nicht bekannt?
    if( vt.vt == VT_NULL     )  return false;                      // NULL? False
    if( vt.vt == VT_DISPATCH )  return vt.pdispVal != NULL;        // Nothing => False, also Ende

    Variant v = vt;

    HRESULT hr = v.ChangeType( VT_BOOL );
    if( FAILED(hr) )  throw_ole( hr, "VariantChangeType" );

    return V_BOOL(&v) != 0;
}

//-------------------------------------------------------------------Com_module_instance_base::init

void Com_module_instance_base::init()
{
    Module_instance::init();
}

//-------------------------------------------------------------Com_module_instance_base::close__end

void Com_module_instance_base::close__end()
{
    close_monitor();

    _idispatch = NULL;
}

//------------------------------------------------------------Com_module_instance_base::name_exists

bool Com_module_instance_base::name_exists( const string& name )
{ 
    bool exists;

    if( !_idispatch )  return false;

    map<string,bool>::iterator it = _names.find( name );
    if( it == _names.end() )  exists = _names[name] = com_name_exists( _idispatch, name );
                        else  exists = it->second;

    return exists;                        
}

//-------------------------------------------------------------------Com_module_instance_base::call

Variant Com_module_instance_base::call( const string& name )
{
    In_call in_call ( this, name );

    return com_call( _idispatch, name );
}

//-------------------------------------------------------------------Com_module_instance_base::call

Variant Com_module_instance_base::call( const string& name, const Variant& param, const Variant& param2 )
{
    In_call in_call ( this, name );

    return param2.is_missing()? com_call( _idispatch, name, param )
                              : com_call( _idispatch, name, param, param2 );
}

//-----------------------------------------------------------Com_module_instance_base::property_get
/*
Variant Com_module_instance_base::property_get( const string& name )
{
    return com_property_get( _idispatch, name );
}
*/
//---------------------------------------------------------Com_module_instance::Com_module_instance
#ifdef Z_WINDOWS

Com_module_instance::Com_module_instance( Module* module )
: 
    Com_module_instance_base(module), 
    _zero_(_end_) 
{
    _com_context = new Com_context;
}

//--------------------------------------------------------Com_module_instance::~Com_module_instance

Com_module_instance::~Com_module_instance()
{
    try {
        close__end();   // Synchron
    }
    catch( exception& ) {}
}

//------------------------------------------------------------------------Com_module_instance::init

void Com_module_instance::init()
{
    if( _idispatch )  throw_xc( "Com_module_instance::init" );

    Com_module_instance_base::init();

    HRESULT hr;
    CLSID   clsid = string_as_clsid( _module->_com_class_name );

    if( _module->_filename != "" )
    {
        if( !_module )
        {
            _log.debug( "LoadLibrary " + _module->_filename );
            _com_module = LoadLibrary( _module->_filename.c_str() );
            if( !_com_module )  throw_mswin_error( "LoadLibrary", _module->_filename.c_str() );   
        }


        _DllGetClassObject = (DllGetClassObject_func)GetProcAddress( _com_module, "DllGetClassObject" );
        if( !_DllGetClassObject )  throw_mswin_error( "GetProcAddress DllGetClassObject", _module->_filename.c_str() );


        ptr<IClassFactory> class_factory;
        void*              void_ptr     = NULL;

        hr = _DllGetClassObject( &clsid, (IID*)&IID_IClassFactory, &void_ptr );
        if( FAILED(hr) )  throw_ole( hr, (_module->_filename + "::DllGetClassObject").c_str(), _module->_com_class_name.c_str() );
        class_factory._ptr = static_cast<IClassFactory*>( void_ptr );

        hr = class_factory->CreateInstance( NULL, IID_IDispatch, &void_ptr );
        if( FAILED(hr) )  throw_ole( hr, "CreateInstance", _module->_com_class_name.c_str() );
        _idispatch._ptr = static_cast<IDispatch*>( void_ptr );
    }
    else
    {
        hr = _idispatch.CoCreateInstance( clsid );
        if( FAILED(hr) )  throw_ole( hr, "CoCreateInstance", _module->_com_class_name.c_str() );
    }
}

//------------------------------------------------------------------------Com_module_instance::load

bool Com_module_instance::load()
{
    bool ok = Com_module_instance_base::load();
    if( !ok )  return false;

    if( _com_context  &&  name_exists( "spooler_set_context" ) )
    {
        Variant com_context_vt = +_com_context;
        com_call( _idispatch, "spooler_set_context", &com_context_vt );
    }

    return true;
}

//---------------------------------------------------------------------Com_module_instance::add_obj

void Com_module_instance::add_obj( IDispatch* object, const string& name )
{
    if( name == "spooler_log"    )  _com_context->_log     = object;
    else
    if( name == "spooler"        )  _com_context->_spooler = object;
    else
  //if( name == "spooler_thread" )  _com_context->_thread  = object;
  //else
    if( name == "spooler_job"    )  _com_context->_job     = object;
    else
    if( name == "spooler_task"   )  _com_context->_task    = object;
    else
        throw_xc( "Module_instance::add_obj", name.c_str() );


    Com_module_instance_base::add_obj( object, name );
}

//------------------------------------------------------------------Com_module_instance::close__end

void Com_module_instance::close__end()   // synchron
{
    Com_module_instance_base::close__end();   // Das ist auch synchron

    if( _com_context )  _com_context->close(), _com_context = NULL;

    if( _com_module ) 
    {
        _log.debug( "FreeLibrary " + _module->_filename );
        FreeLibrary( _com_module );
        _com_module = NULL;
    }
}

#endif
//------------------------------Scripting_engine_module_instance::~Scripting_engine_module_instance

Scripting_engine_module_instance::~Scripting_engine_module_instance()
{
    try {
        close__end();  // Synchron
    }
    catch( exception& ) {}
}

//-----------------------------------------------------------Scripting_engine_module_instance::init

void Scripting_engine_module_instance::init()
{
    Com_module_instance_base::init();

    assert( !_script_site );
    _script_site = new Script_site;
    _script_site->_engine_name = _module->_language;
    _script_site->init_engine();
}

//-------------------------------------------------------------Com_module_instance_base::close__end

void Scripting_engine_module_instance::close__end()
{
    close_monitor();

    if( _script_site )
    {
        _idispatch = NULL;

        _script_site->close_engine();
        _script_site = NULL;
    }
}

//------------------------------------------------------------------Scripting_engine_module_instance::add_obj

void Scripting_engine_module_instance::add_obj( IDispatch* object, const string& name )
{
    _script_site->add_obj( object, Bstr(name) );

    Module_instance::add_obj( object, name );
}

//---------------------------------------------------------------------Scripting_engine_module_instance::load

bool Scripting_engine_module_instance::load()
{
    bool ok = Com_module_instance_base::load();
    if( !ok )  return false;

    if( _script_site->_engine_name != _module->_language )  z::throw_xc( "SCHEDULER-117" );

    HRESULT hr = _script_site->_script->SetScriptState( SCRIPTSTATE_INITIALIZED );
    if( FAILED( hr ) )  throw_ole( hr, "IActiveScript::SetScriptState", "SCRIPTSTATE_INITIALIZED" );

    bool is_perl = string_begins_with( lcase( _script_site->_engine_name ), "perl" );

    DOM_FOR_EACH_ELEMENT( _module->_text_with_includes.dom_element(), element )
    {
        string text = _module->_text_with_includes.read_text_element( element, _module->_include_path );
        if( text != "" )
        {
            int line_number = _module->_text_with_includes.text_element_linenr( element );

            if( is_perl )
            {
                text = S() << "\n" 
                              "#line " << line_number << " " 
                                       << quoted_string( _module->_text_with_includes.text_element_filepath( element ), '"', '\\' ) 
                                       << "\n"
                           << text;

                --line_number;
            }

            _script_site->parse( text, line_number );
        }
    }

    _idispatch = _script_site->dispatch();

    return true;
}

//----------------------------------------------------------Scripting_engine_module_instance::start

void Scripting_engine_module_instance::start()
{
#   ifdef Z_WINDOWS
        // init_engine() bereits gerufen
#    else
//        _script_site->init_engine();
#   endif

    HRESULT hr = _script_site->_script->SetScriptState( SCRIPTSTATE_STARTED );
    if( FAILED( hr ) )  throw_ole( hr, "IActiveScript::SetScriptState", "SCRIPTSTATE_STARTED" );
}

//-----------------------------------------------------------Scripting_engine_module_instance::call

Variant Scripting_engine_module_instance::call( const string& name )
{
    SOS_DELETE( _script_site->_script_exception );

    In_call in_call ( this, name );

    try
    {
        return com_call( _idispatch, name );
    }
    catch( const exception& )
    {
        if( _script_site->_script_exception )  throw *_script_site->_script_exception;
                                         else  throw;
    }
}

//-----------------------------------------------------------Scripting_engine_module_instance::call

Variant Scripting_engine_module_instance::call( const string& name, const Variant& param, const Variant& param2 )
{
    SOS_DELETE( _script_site->_script_exception );

    In_call in_call ( this, name );

    try
    {
        return param2.is_missing()? com_call( _idispatch, name, param )
                                  : com_call( _idispatch, name, param, param2 );
    }
    catch( const exception& )
    {
        if( _script_site->_script_exception )  throw *_script_site->_script_exception;
                                         else  throw;
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
