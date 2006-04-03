// $Id$
// §1172
/*
    Hier sind implementiert

    Source_part
    Source_with_parts
    Module
    Com_module_instance
*/


#include "spooler.h"
#include "../file/anyfile.h"
#include "../kram/sos_java.h"

using namespace std;

namespace sos {
namespace spooler {

//--------------------------------------------------------------------------------------------const

extern const string spooler_init_name           = "spooler_init()Z";
extern const string spooler_exit_name           = "spooler_exit()V";
extern const string spooler_open_name           = "spooler_open()Z";
extern const string spooler_close_name          = "spooler_close()V";
extern const string spooler_process_name        = "spooler_process()Z";
extern const string spooler_on_error_name       = "spooler_on_error()V";
extern const string spooler_on_success_name     = "spooler_on_success()V";

// Monitor-Methoden:
const string spooler_task_before_name    = "spooler_task_before()Z";       
const string spooler_task_after_name     = "spooler_task_after()V";
const string spooler_process_before_name = "spooler_process_before()Z";    
const string spooler_process_after_name  = "spooler_process_after(Z)Z";    

const string shell_language_name         = "shell";

//-------------------------------------------------------------------------Source_part::Source_part

Source_part::Source_part( int linenr, const string& text, const Time& mod_time )
: 
    _linenr(linenr), 
    _text(text), 
    _modification_time(mod_time) 
{
}

//--------------------------------------------------------xml::Element_ptr Source_part::dom_element

xml::Element_ptr Source_part::dom_element( const xml::Document_ptr& doc ) const
{
    xml::Element_ptr part_element = doc.createElement( "part_element" );

    part_element.setAttribute( "linenr", as_string( _linenr ) );
    part_element.setAttribute( "modtime", _modification_time.as_string( Time::without_ms ) );
    part_element.appendChild( doc.createTextNode( _text ) );

    return part_element;
}

//-------------------------------------------------xml::Document_ptr Source_with_parts::dom_element

xml::Element_ptr Source_with_parts::dom_element( const xml::Document_ptr& doc ) const
{
    xml::Element_ptr source_element = doc.createElement( "source" ); 

    Z_FOR_EACH_CONST( Parts, _parts, part )
    {
        xml::Element_ptr part_element = part->dom_element(doc);
        source_element.appendChild( part_element );
    }

    return source_element;
}

//------------------------------------------------xml::Document_ptr Source_with_parts::dom_document

xml::Document_ptr Source_with_parts::dom_document() const
{
    xml::Document_ptr doc;

    doc.create();
    doc.appendChild( doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"" + scheduler_character_encoding + "\"" ) );
    doc.appendChild( dom_element(doc) );

    return doc;
}

//--------------------------------------------------------------------Source_with_parts::assign_dom
// Für spooler_module_remote_server.cxx

void Source_with_parts::assign_dom( const xml::Element_ptr& source_element )
{
    clear();

    if( !source_element.nodeName_is( "source" ) )  throw_xc( "Source_with_parts", "dom" );

    DOM_FOR_EACH_ELEMENT( source_element, part )
    {
        Sos_optional_date_time dt = part.getAttribute( "modtime" );
        add( part.int_getAttribute( "linenr", 1 ), part.getTextContent(), dt.time_as_double() );
    }
}

//---------------------------------------------------------------------------Source_with_parts::add

void Source_with_parts::add( int linenr, const string& text, const Time& mod_time )
{ 
    // text ist nicht leer?
    int i;
    for( i = 0; i < text.length(); i++ )  if( !isspace( (unsigned char)text[i] ) )  break;
    if( i < text.length() )
    {
        _parts.push_back( Source_part( linenr, text, mod_time ) );
    }

    if( mod_time  &&  _max_modification_time < mod_time )   _max_modification_time = mod_time;
}

//------------------------------------------------------------------------check_unchanged_attribute
/*
static void check_unchanged_attribute( const xml::Element_ptr& element, const string& attribute_name, const string& current_value )
{
    if( element.hasAttribute( attribute_name  )  &&  element.getAttribute( attribute_name ) != current_value )  
        z::throw_xc( "SCHEDULER-234", attribute_name + "+" + current_value );
}
*/
//------------------------------------------------------------------------------------odule::Module

Module::Module( Spooler* sp, Prefix_log* log )
: 
    _zero_(_end_), 
    _spooler(sp), 
    _log(log),
    _process_environment( new Com_variable_set() )
{
#   ifndef Z_WINDOWS
        _process_environment->_ignore_case = false;
#   endif
}

//-----------------------------------------------------------------------------------Module::Module
    
Module::Module( Spooler* sp, const xml::Element_ptr& e, const Time& xml_mod_time, const string& include_path )  
: 
    _zero_(_end_),
    _spooler(sp),
    _process_environment( new Com_variable_set() )
{ 
#   ifndef Z_WINDOWS
        _process_environment->_ignore_case = false;
#   endif

    set_dom(e,xml_mod_time,include_path); 
}

//-----------------------------------------------------------------------------Module::set_priority
    
void Module::set_priority( const string& priority )
{
    if( priority != "" )
    {
#       ifdef Z_WINDOWS
            windows::priority_class_from_string( priority );    // Prüfen
#        else
            posix::priority_from_string( priority );            // Prüfen
#       endif
    }

    _priority = priority;
}

//----------------------------------------------------------------------------set_checked_attribute

void Module::set_checked_attribute( string* variable, const xml::Element_ptr& element, const string& attribute_name, bool modify_allowed )
{
    if( !_initialized  ||  ( modify_allowed && *variable != "" ) ) 
    {
        *variable = element.getAttribute( attribute_name, *variable );
    }
    else
    if( element.hasAttribute( attribute_name )  &&  element.getAttribute( attribute_name ) != *variable )  
        z::throw_xc( "SCHEDULER-234", attribute_name + "=\"" + *variable + '"' );
}

//------------------------------------------------------------------------------Module::set_process
// <process> hat kein <script>, deshalb dieser Aufruf
// Besser wäre, <process> durch <script language="shell"> zu ersetzen

void Module::set_process()
{
    _language = shell_language_name;
    _source.clear();
    _set = true;
}

//-------------------------------------------------------------------Module::set_dom_without_source

void Module::set_dom_without_source( const xml::Element_ptr& element, const Time& xml_mod_time )
{
    if( !element )  return;

    _dom_document = element.ownerDocument();
    _dom_element  = element;
    _xml_mod_time = xml_mod_time;

    _source.clear();  //clear();

    _recompile = element.bool_getAttribute( "recompile", true );

    set_checked_attribute( &_language          , element, "language"         );
    set_checked_attribute( &_com_class_name    , element, "com_class" , true );
    set_checked_attribute( &_filename          , element, "filename"         );
    set_checked_attribute( &_java_class_name   , element, "java_class", true );
    set_checked_attribute( &_process_class_name, element, "process_class" );

    bool separate_process_default = false;

    _separate_process = element.bool_getAttribute( "separate_process", separate_process_default );

    string use_engine = element.getAttribute     ( "use_engine" );
    
    if( use_engine == ""
     || use_engine == "task" )  _reuse = reuse_task;
    else
        z::throw_xc( "SCHEDULER-196", use_engine );
  //if( use_engine == "job"  )  _reuse = reuse_job;
}

//----------------------------------------------------------------------Module::set_dom_source_only

void Module::set_dom_source_only( const string& include_path )
{
    set_source_only( text_from_xml_with_include( _dom_element, _xml_mod_time, include_path ) );
}

//--------------------------------------------------------------------------Module::set_source_only

void Module::set_source_only( const Source_with_parts& source )
{
    _source = source;
    _compiled = false;

    clear_java();

    _set = true;
}

//-------------------------------------------------------------------------------------Module::init

void Module::init()
{
    if( _initialized )  return;
    
    if( _monitor )  _monitor->init();


# ifdef Z_WINDOWS
    if( _com_class_name != "" )
    {
        _kind = kind_com;
    
        if( _language        != "" )  z::throw_xc( "SCHEDULER-145" );
        if( _java_class_name != "" )  z::throw_xc( "SCHEDULER-168" );
    }
    else
# endif
    if( _java_class_name != ""  ||  lcase(_language) == "java" )
    {
        _kind = kind_java;
    
        if( _language == "" )  _language = "Java";

        if( lcase(_language) != "java" )  z::throw_xc( "SCHEDULER-166" );
        if( _com_class_name  != ""     )  z::throw_xc( "SCHEDULER-168" );
    }
    else
    if( _process_filename != ""  || _language == shell_language_name )  //   Z_POSIX_ONLY( || _language == ""  &&  string_begins_with( _source, "#!" ) ) )
    {
        _kind = kind_process;
    }
    else
    {
        _kind = kind_scripting_engine;
        if( _language == "" )  _language = SPOOLER_DEFAULT_LANGUAGE;
    }


    _real_kind = _kind;

    if( _real_kind == kind_java  &&  !_source.empty()  &&  _spooler )  _spooler->_has_java_source = true;       // work_dir zum Compilieren bereitstellen


    if( _kind != kind_process )
    {
        if( _spooler )  _use_process_class = _spooler->has_process_classes();

        if( _dont_remote )  _separate_process = false, _use_process_class = false, _process_class_name = "";

        if( _separate_process )
        {
            if( _process_class_name != "" )  z::throw_xc( "SCHEDULER-194" );
            //_process_class_name = temporary_process_class_name;
        }

        if( _use_process_class )  
        {
            _process_class = _spooler->process_class( _process_class_name );     // Fehler, wenn der Name nicht bekannt ist.
            _process_class->_module_use_count++;
        }
    }

    if( _separate_process  ||  _use_process_class )   _kind = kind_remote;

    if( _kind == kind_java  &&  _spooler )  _spooler->_has_java = true;


    switch( _kind )
    {
        case kind_remote:               break;
        case kind_java:                 break;
        
        case kind_scripting_engine:     if( _source.empty() )  z::throw_xc( "SCHEDULER-173" );
                                        break;

        case kind_process:              if( _source.empty()  &&  _process_filename.empty() )  z::throw_xc( "SCHEDULER-173" );
                                        break;

#       ifdef Z_WINDOWS
            case kind_com:              if( !_source.empty() )  z::throw_xc( "SCHEDULER-167" );
                                        break;
#       endif

        default:                        z::throw_xc( __FUNCTION__ );
    }

    _initialized = true;
}

//--------------------------------------------------------------------------Module::create_instance

ptr<Module_instance> Module::create_instance()
{
    ptr<Module_instance> result;
    ptr<Module_instance> monitor_instance;


    switch( _kind )
    {
        case kind_java:              
        {
            if( _spooler )  if( !_spooler->_java_vm  ||  !_spooler->_java_vm->running() )  z::throw_xc( "SCHEDULER-177" );

            _java_vm = get_java_vm( false );
            _java_vm->set_destroy_vm( false );   //  Nicht DestroyJavaVM() rufen, denn das hängt manchmal

            if( !_java_vm->running() )
            {
                Java_module_instance::init_java_vm( _java_vm );     // Native Java-Methoden (Callbacks) bekannt machen
            }
            
            ptr<Java_module_instance> p = Z_NEW( Java_module_instance( _java_vm, this ) );
            result = +p;
            break;
        }

        case kind_scripting_engine:  
        {
            ptr<Scripting_engine_module_instance> p = Z_NEW( Scripting_engine_module_instance( this ) );
            result = +p;
            break;
        }

#     ifdef Z_WINDOWS
        case kind_com:               
        {
            ptr<Com_module_instance> p = Z_NEW( Com_module_instance( this ) );
            result = +p;
            break;
        }
#     endif

        case kind_process:
        {
            ptr<Process_module_instance> p = Z_NEW( Process_module_instance( this ) );
            result = +p;
            break;
        }

        case kind_remote:
        {
            ptr<Remote_module_instance_proxy> p = Z_NEW( Remote_module_instance_proxy( this ) );
            result = +p;
            break;
        }

        default:                     
            z::throw_xc( "SCHEDULER-173" );
    }


    if( _monitor  &&  _kind != kind_remote )  result->_monitor_instance = _monitor->create_instance();



    return result;
}

//----------------------------------------------------------------Module_instance::In_call::In_call

Module_instance::In_call::In_call( Module_instance* module_instance, const string& name, const string& extra ) 
: 
    _module_instance(NULL),
    _result_set(false)
{ 
    if( !module_instance->_in_call )       // In_call kann doppelt gerufen werden (explizit und implizit). Der zweite Aufruf wirkt nicht.
    {
        _module_instance = module_instance;

        int pos = name.find( '(' );
        _name = pos == string::npos? name : name.substr( 0, pos );

        _module_instance->set_in_call( this, extra ); 
        Z_LOG2( "scheduler.call", *_module_instance << '.' << _name << "() begin\n" );

        Z_WINDOWS_ONLY( _ASSERTE( _CrtCheckMemory() ); )
    }
}

//---------------------------------------------------------------Module_instance::In_call::~In_call

Module_instance::In_call::~In_call()
{ 
    if( _module_instance )
    {
        _module_instance->set_in_call( NULL ); 

        if( z::Log_ptr log = "scheduler.call" )
        {
            *log << *_module_instance << '.' << _name << "() end";
            if( _result_set )  *log << "  result=" << ( _result? "true" : "false" );
            *log << '\n';
        }

        Z_WINDOWS_ONLY( _ASSERTE( _CrtCheckMemory() ); )
    }
}

//-----------------------------------------------------------------Module_instance::Module_instance

Module_instance::Module_instance( Module* module )
: 
    _zero_(_end_), 
    _module(module),
    _log(module?module->_log:NULL) 
{
    _com_task    = new Com_task;
    _com_log     = new Com_log;

    _spooler_exit_called = false;

  //_close_instance_at_end;         // Das verhindert aber use_engine="job". Aber vielleicht braucht das keiner.
}

//----------------------------------------------------------------Module_instance::~Module_instance

Module_instance::~Module_instance()
{
    if( _com_log  )  _com_log ->set_log ( NULL );
    if( _com_task )  _com_task->set_task( NULL );
}

//----------------------------------------------------------------------------Module_instance::init

void Module_instance::init()
{
    _initialized = true;
    _spooler = _module->_spooler;

    if( !_module->set() )  z::throw_xc( "SCHEDULER-146" );

    if( _monitor_instance  &&  !_monitor_instance->_initialized )  _monitor_instance->init();
}

//---------------------------------------------------------------------------Module_instance::clear

void Module_instance::clear()
{ 
    _object_list.clear(); 

    if( _monitor_instance )  _monitor_instance->clear();
}

//--------------------------------------------------------------------Module_instance::set_job_name

void Module_instance::set_job_name( const string& job_name )
{
    _job_name = job_name; 

    if( _monitor_instance )  _monitor_instance->set_job_name( job_name );
}

//---------------------------------------------------------------------Module_instance::set_task_id

void Module_instance::set_task_id( int id )
{ 
    _task_id = id; 

    if( _monitor_instance )  _monitor_instance->set_task_id( id );
}

//-------------------------------------------------------------------------Module_instance::set_log

void Module_instance::set_log( Prefix_log* log )
{ 
    _log = log; 

    if( _monitor_instance )  _monitor_instance->set_log( log );
}

//--------------------------------------------------------------------------------Task::set_in_call

void Module_instance::set_in_call( In_call* in_call, const string& extra )
{
    _in_call = in_call;

    if( in_call  &&  _spooler  &&  _spooler->_debug )
    {
        _log.debug( in_call->_name + "()  " + extra );
    }
}

//---------------------------------------------------------------------Module_instance::attach_task

void Module_instance::attach_task( Task* task, Prefix_log* log )
{
  //_task = task;

    _com_task->set_task( task );
    _com_log ->set_log ( log );

    _task_id = task->id();
    //_title = task->obj_name();          // Titel für Prozess

    if( _monitor_instance )  _monitor_instance->attach_task( task, log );
}

//---------------------------------------------------------------------Module_instance::detach_task

void Module_instance::detach_task()
{
    close_monitor();

    _com_task->set_task( NULL );
    _com_log ->set_log ( NULL );
    
    _task_id = 0;
    //_title = "";

    if( _monitor_instance )  _monitor_instance->detach_task();
}

//-------------------------------------------------------------------------Module_instance::add_obj

void Module_instance::add_obj( IDispatch* object, const string& name )
{
    if( _monitor_instance )  _monitor_instance->add_obj( object, name );
}

//--------------------------------------------------------------------------Module_instance::object

IDispatch* Module_instance::object( const string& name )
{
    IDispatch* result = object( name, NULL );
    if( !result )  throw_xc( "Module_instance::object", name );
    return result;
}

//--------------------------------------------------------------------------Module_instance::object

IDispatch* Module_instance::object( const string& name, IDispatch* deflt )
{
    Z_FOR_EACH( Object_list, _object_list, o )
    {
        if( o->_name == name )  return o->_object;
    }
    
    return deflt;
}

//----------------------------------------------------------------------------Module_instance::load

bool Module_instance::load()
{
    bool ok = true;

    if( _monitor_instance  &&  !_monitor_instance->_load_called )  
    {
        bool ok = _monitor_instance->implicit_load_and_start();
        if( !ok )  return false;

        Variant result = _monitor_instance->call_if_exists( spooler_task_before_name );
        if( !result.is_missing() )  ok = check_result( result );
    }

    _load_called = true;

    return ok;
}

//---------------------------------------------------------------------------Module_instance::start

void Module_instance::start()
{
    //Schon in implicit_load_and_start() erledigt: if( _monitor_instance ) ...
}

//------------------------------------------------------------------Module_instance::call_if_exists

Variant Module_instance::call_if_exists( const string& name )
{
    if( name_exists(name) )  return call( name );
                       else  return Variant( Variant::vt_error, DISP_E_UNKNOWNNAME );
}

//------------------------------------------------------------------Module_instance::call_if_exists

Variant Module_instance::call_if_exists( const string& name, bool param )
{
    if( name_exists(name) )  return call( name, param );
                       else  return Variant( Variant::vt_error, DISP_E_UNKNOWNNAME );
}

//---------------------------------------------------------------------------Module_instance::close

void Module_instance::close()
{
    Async_operation* op = close__start();
    if( !op->async_finished() )  _log.warn( message_string( "SCHEDULER-293" ) );        // "Warten auf Schließen der Modulinstanz ..."
    close__end();
}

//--------------------------------------------------------------------Module_instance::close__start

Async_operation* Module_instance::close__start()
{ 
    return &dummy_sync_operation; 
}

//----------------------------------------------------------------------Module_instance::close__end

void Module_instance::close__end()
{
    close_monitor();
}

//-------------------------------------------------------------------Module_instance::close_monitor

void Module_instance::close_monitor()
{
    if( _monitor_instance )  
    {
        try
        {
            _monitor_instance->call_if_exists( spooler_task_after_name );
        }
        catch( exception& x )
        {
            _log.error( spooler_task_after_name + ": " + x.what() );
        }

        try
        {
            _monitor_instance->close();
        }
        catch( exception& x )
        {
            _log.error( string("Monitor: ") + x.what() );
        }

        _monitor_instance = NULL;
    }
}

//--------------------------------------------------------------------Module_instance::begin__start

Async_operation* Module_instance::begin__start()
{
    return &dummy_sync_operation;
}

//---------------------------------------------------------Module_instance::implicit_load_and_start

bool Module_instance::implicit_load_and_start()
{
    if( !_initialized )  init();

    FOR_EACH_CONST( Object_list, _object_list, o )  add_obj( o->_object, o->_name );

    bool ok = load();
    if( !ok )  return false;

    start();
    return true;
}

//----------------------------------------------------------------------Module_instance::begin__end

bool Module_instance::begin__end()
{
    if( !_load_called )
    {
        bool ok = implicit_load_and_start();
        if( !ok )  return false;
    }

    if( !_spooler_init_called )
    {
        _spooler_init_called = true;
        bool ok = check_result( call_if_exists( spooler_init_name ) );
        if( !ok )  return ok;
    }

    _spooler_open_called = true;
    return check_result( call_if_exists( spooler_open_name ) );
}

//----------------------------------------------------------------------Module_instance::end__start

Async_operation* Module_instance::end__start( bool )
{
    return &dummy_sync_operation;
}

//------------------------------------------------------------------------Module_instance::end__end

void Module_instance::end__end()
{
    if( !loaded() )  return;

    if( _spooler_open_called  &&  !_spooler_close_called )
    {
        _spooler_close_called = true;
        call_if_exists( spooler_close_name );
    }
}

//---------------------------------------------------------------------Module_instance::step__start

Async_operation* Module_instance::step__start()
{
    return &dummy_sync_operation;
}

//-----------------------------------------------------------------------Module_instance::step__end

Variant Module_instance::step__end()
{
    Variant ok;

    if( !_monitor_instance )
    {
        ok = call_if_exists( spooler_process_name );
    }
    else
    {
        ok = _monitor_instance->call_if_exists( spooler_process_before_name );
        if( check_result( ok ) )
        {
            try
            {
                ok = call_if_exists( spooler_process_name );
            }
            catch( Xc& x )
            {
                com_call( object( "spooler_task", _com_task ), "Set_error_code_and_text", x.code(), x.what() );
                ok = false;
            }
            catch( zschimmer::Xc& x )
            {
                com_call( object( "spooler_task", _com_task ), "Set_error_code_and_text", x.code(), x.what() );
                ok = false;
            }
            catch( exception& x )
            {
                com_call( object( "spooler_task", _com_task ), "Set_error_code_and_text", "", x.what() );
                ok = false;
            }

            Variant result = _monitor_instance->call_if_exists( spooler_process_after_name, check_result( ok ) );
            if( !result.is_missing() )  ok = result;
        }
    }

    return ok;
}

//---------------------------------------------------------------------Module_instance::call__start

Async_operation* Module_instance::call__start( const string& method )
{
    _call_method = method;
    return &dummy_sync_operation;
}

//-----------------------------------------------------------------------Module_instance::call__end

Variant Module_instance::call__end()
{
    if( _call_method == spooler_exit_name  &&  !loaded() )  return true;

  //if( _call_method == wait_for_subprocesses_name )        // Keine Methode des Jobs.
  //{
  //    return _com_task->_task->wait_for_subprocesses();   // Siehe auch Com_remote_module_instance_server
  //}
  //else
    if( _call_method == spooler_on_success_name   
     || _call_method == spooler_on_error_name )
    {
        if( !_spooler_open_called )  return true;
    }
    else
    if( _call_method == spooler_exit_name )  
    {
        if( _spooler_exit_called )  return true;
        _spooler_exit_called = true;
    }

    return call_if_exists( _call_method );
}

//------------------------------------------------------------------Module_instance::release__start

Async_operation* Module_instance::release__start()
{
    return &dummy_sync_operation;
}

//--------------------------------------------------------------------Module_instance::release__end

void Module_instance::release__end()
{
    //close();
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
