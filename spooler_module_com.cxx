// $Id$
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
namespace spooler {

//-------------------------------------------------------------------------------------check_result

bool check_result( const Variant& vt )
{
    if( vt.vt == VT_EMPTY    )  return true;                       // Keine Rückgabe? True, also weiter machen
    if( vt.vt == VT_NULL     )  return false;                      // NULL? False
    if( vt.vt == VT_DISPATCH )  return vt.pdispVal != NULL;        // Nothing => False, also Ende
    if( vt.vt == VT_BOOL     )  return V_BOOL(&vt) != 0;           // Nothing => False, also Ende

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
    //Module_instance::close();

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

//------------------------------------------------------------------Com_module_instance_base::call

Variant Com_module_instance_base::call( const string& name )
{
    In_call in_call ( this, name );

    return com_call( _idispatch, name );
}

//------------------------------------------------------------------Com_module_instance_base::call

Variant Com_module_instance_base::call( const string& name, int param )
{
    In_call in_call ( this, name );

    return com_call( _idispatch, name, param );
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
    catch( const Xc& ) {}
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

//------------------------------------------------------------------------Com_module_instance::load

void Com_module_instance::load()
{
    if( _com_context  &&  name_exists( "spooler_set_context" ) )
    {
        Variant com_context_vt = +_com_context;
        com_call( _idispatch, "spooler_set_context", &com_context_vt );
    }
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
    catch( const Xc& ) {}
}

//-----------------------------------------------------------Scripting_engine_module_instance::init

void Scripting_engine_module_instance::init()
{
    Com_module_instance_base::init();

    _script_site = new Script_site;
    _script_site->_engine_name = _module->_language;

//#   ifdef Z_WINDOWS
        _script_site->init_engine();
//#    else
        // Perl auf Unix will init_engine() erst nachdem die Quelle vollständig geladen ist.
//#   endif

    //HRESULT hr = _script_site->_module->SetScriptState( SCRIPTSTATE_INITIALIZED );
    //if( FAILED( hr ) )  throw_ole( hr, "IActiveScript::SetScriptState", "SCRIPTSTATE_INITIALIZED" );
}

//-------------------------------------------------------------Com_module_instance_base::close__end

void Scripting_engine_module_instance::close__end()
{
    Com_module_instance_base::close__end();   // Synchron

    if( _script_site )
    {
        _script_site->close_engine();
        _script_site = NULL;
    }
}

//------------------------------------------------------------------Scripting_engine_module_instance::add_obj

void Scripting_engine_module_instance::add_obj( IDispatch* object, const string& name )
{
#   if 1  // def Z_WINDOWS

        _script_site->add_obj( object, Bstr(name) );

#    else
/*
        // Das ist der gleiche Code wie in factory_process.cxx. Zusammenfassen!

        HRESULT     hr;
        string      stmt;
        Bstr        name_bstr;
        Variant     index_vt;
        Variant     value_vt;

        hr = it->second->get_name( &name_bstr );                if(FAILED(hr)) throw_ole( hr, "Ihostware_variable::name" );
        hr = it->second->get_value( &index_vt, &value_vt );     if(FAILED(hr)) throw_ole( hr, "Ihostware_variable::value" );

        if( value_vt.vt != VT_EMPTY )
        {
            if( _var_prefix )  stmt = _var_prefix;
            stmt += bstr_as_string(name_bstr);
            stmt += '=';
        
            if( zschimmer::com::variant_is_numeric(value_vt) || value_vt.vt == VT_BOOL )  
                stmt += variant_as_string(value_vt);
            else 
                stmt += quoted_string( variant_as_string(value_vt), _quote_char, _use_backslash? '\\' : _quote_char );

            stmt += ';';
        
            _script_site->parse( stmt );
        }
*/
#   endif

    //jz 31.5.03  Com_module_instance_base::add_obj( object, name );
}

//---------------------------------------------------------------------Scripting_engine_module_instance::load

void Scripting_engine_module_instance::load()
{
    if( _script_site->_engine_name != _module->_language )  throw_xc( "SCHEDULER-117" );

/*
    if( _com_context->_log     )  _script_site->add_obj( _com_context->_log    , Bstr("spooler_log"    ) );
    if( _com_context->_spooler )  _script_site->add_obj( _com_context->_spooler, Bstr("spooler"        ) );
  //if( _com_context->_thread  )  _script_site->add_obj( _com_context->_thread , Bstr("spooler_thread" ) );
    if( _com_context->_job     )  _script_site->add_obj( _com_context->_job    , Bstr("spooler_job"    ) );
    if( _com_context->_task    )  _script_site->add_obj( _com_context->_task   , Bstr("spooler_task"   ) );
*/

    HRESULT hr = _script_site->_script->SetScriptState( SCRIPTSTATE_INITIALIZED );
    if( FAILED( hr ) )  throw_ole( hr, "IActiveScript::SetScriptState", "SCRIPTSTATE_INITIALIZED" );

    Z_FOR_EACH_CONST( Source_with_parts::Parts, _module->_source._parts, it )
    {
        _script_site->parse( it->_text, it->_linenr );
    }

    _idispatch = _script_site->dispatch();
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

Variant Scripting_engine_module_instance::call( const string& name, int param )
{
    SOS_DELETE( _script_site->_script_exception );

    In_call in_call ( this, name );

    try
    {
        return com_call( _idispatch, name, param );
    }
    catch( const exception& )
    {
        if( _script_site->_script_exception )  throw *_script_site->_script_exception;
                                         else  throw;
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
