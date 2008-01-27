// $Id$
// §1172
/*
    Hier sind implementiert

    Source_part
    Text_with_includes
    Module
    Com_module_instance
*/


#include "spooler.h"
#include "../file/anyfile.h"
#include "../kram/sos_java.h"

using namespace std;

namespace sos {
namespace scheduler {

//--------------------------------------------------------------------------------------------const

extern const string spooler_init_name           = "spooler_init()Z";
extern const string spooler_exit_name           = "spooler_exit()V";
extern const string spooler_open_name           = "spooler_open()Z";
extern const string spooler_close_name          = "spooler_close()V";
extern const string spooler_process_name        = "spooler_process()Z";
extern const string spooler_on_error_name       = "spooler_on_error()V";
extern const string spooler_on_success_name     = "spooler_on_success()V";
extern const string spooler_api_version_name    = "spooler_api_version()Ljava.lang.String;";
const string        default_monitor_name        = "scheduler";

// Monitor-Methoden:
const string spooler_task_before_name    = "spooler_task_before()Z";       
const string spooler_task_after_name     = "spooler_task_after()V";
const string spooler_process_before_name = "spooler_process_before()Z";    
const string spooler_process_after_name  = "spooler_process_after(Z)Z";    

const string shell_language_name         = "shell";

////-------------------------------------------------------------------------Source_part::Source_part
//
//Source_part::Source_part( int linenr, const string& text, const Time& mod_time )
//: 
//    _linenr(linenr), 
//    _text(text), 
//    _modification_time(mod_time) 
//{
//}
//
////--------------------------------------------------------xml::Element_ptr Source_part::dom_element
//
//xml::Element_ptr Source_part::dom_element( const xml::Document_ptr& doc ) const
//{
//    xml::Element_ptr part_element = doc.createElement( "part_element" );
//
//    part_element.setAttribute( "linenr", as_string( _linenr ) );
//    part_element.setAttribute( "modtime", _modification_time.as_string( Time::without_ms ) );
//    part_element.appendChild( doc.createTextNode( _text ) );
//
//    return part_element;
//}
//
//-------------------------------------------------------------------Text_with_includes::initialize

void Text_with_includes::initialize()
{
    _dom_document.load_xml( "<source/>" );
}

//--------------------------------------------------------------------Text_with_includes::append_dom

void Text_with_includes::append_dom( const xml::Element_ptr& element )
{
    int linenr_base = element.line_number();

    for( xml::Node_ptr node = element.firstChild(); node; node = node.nextSibling() )
    {
        string text;

        if( int n = node.line_number() ) 
            if( linenr_base < n )  linenr_base = n;     // libxml2 liefert bei Textknoten keine neue Zeilennummer, deshalb zählen wir selbst.

        switch( node.nodeType() )
        {
            case xml::CDATA_SECTION_NODE:
            {
                xml::CDATASection_ptr c = node;
                text = c.data();
                goto TEXT;
            }

            case xml::TEXT_NODE:
            {
                xml::Text_ptr t = node;
                text = t.data();
                goto TEXT;
            }

            TEXT:
            {
                xml::Element_ptr e = _dom_document.documentElement().append_new_cdata_or_text_element( "source_part", text );

                e.setAttribute( "linenr", linenr_base );
              //e.setAttribute( "modtime", _modification_time.as_string( Time::without_ms ) );

                linenr_base += count( text.begin(), text.end(), '\n' );
                break;
            }

            case xml::COMMENT_NODE:
            {
                xml::Comment_ptr t = node;
                text = t.data();
                linenr_base += count( text.begin(), text.end(), '\n' );
                break;
            }

            case xml::ELEMENT_NODE:     // <include file="..."/>
            {
                xml::Element_ptr e = node;

                if( e.nodeName_is( "include" ) )
                {
                    xml::Element_ptr include_element = _dom_document.createElement( "include" );
                    include_element.setAttribute( "file", e.getAttribute( "file" ) );

                    _dom_document.documentElement().appendChild( include_element );
                }
                else
                    z::throw_xc( Z_FUNCTION, e.nodeName() );

                break;
            }

            default: ;
        }
    }
}

//--------------------------------------------------------------------Text_with_includes::read_text

string Text_with_includes::read_text( const string& include_path ) 
{
    String_list result;

    DOM_FOR_EACH_ELEMENT( dom_element(), element )
    {
        result.append( read_text_element( element, include_path ) );
    }

    return result;
}

//------------------------------------------------------------Text_with_includes::read_text_element

string Text_with_includes::read_text_element( const xml::Element_ptr& element, const string& include_path )
{
    string result;

    if( element.nodeName_is( "source_part" ) )
    {
        result = element.text();
    }
    else
    if( element.nodeName_is( "include" ) )
    {
        try
        {
            File_path path = subst_env( element.getAttribute( "file" ) );
            if( path.is_relative_path() )  path.prepend_directory( include_path );

            result = string_from_file( path );
        }
        catch( exception& x )  { z::throw_xc( "SCHEDULER-399", x ); }
    }
    else
        z::throw_xc( Z_FUNCTION, element.nodeName() );

    return result;
}

//----------------------------------------------------------Text_with_includes::text_element_linenr

int Text_with_includes::text_element_linenr( const xml::Element_ptr& element )
{
    return element.int_getAttribute( "linenr", 1 );
}

//--------------------------------------------------------Text_with_includes::text_element_filepath

string Text_with_includes::text_element_filepath( const xml::Element_ptr& element )
{
    string result = element.getAttribute( "file" );
    if( result == "" )  result = "<script>";
    return result;
}

//---------------------------------------------------------------------Text_with_includes::is_empty

bool Text_with_includes::is_empty() const
{ 
    if( !_dom_document )  return true;
    if( !_dom_document.documentElement().firstChild() )  return true;

    
    // Jira JS-60: Die SOS schreibt gerne <script> </script>, was dasselbe sein soll wie <script/>.

    if( _dom_document.documentElement().first_child_element() )  return false;

    string text = _dom_document.documentElement().text();
    const char* p = text.c_str();
    while( *p  &&  isspace( (unsigned char)*p ) )  p++;
    return *p == '\0';
}

//-----------------------------------------------------------------------------------Module::Module

Module::Module( Spooler* sp, const string& include_path, Has_log* log )
: 
    _zero_(_end_), 
    _spooler(sp), 
    _log(log),
    _process_environment( new Com_variable_set() ),
    _include_path(include_path)
{
    init0();
}

//-----------------------------------------------------------------------------------Module::Module
    
Module::Module( Spooler* sp, const xml::Element_ptr& e, const string& include_path )  
: 
    _zero_(_end_),
    _spooler(sp),
    _process_environment( new Com_variable_set() ),
    _include_path(include_path)
{ 
    init0();
    set_dom( e ); 
}

//-------------------------------------------------------------------------------------------------

void Module::init0()
{
#   ifndef Z_WINDOWS
        _process_environment->_ignore_case = false;
#   endif

    _monitors = Z_NEW( Module_monitors( this ) );
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
    //_source.clear();
    _set = true;
}

//----------------------------------------------------------------------------------Module::set_dom

void Module::set_dom( const xml::Element_ptr& element )  
{ 
    if( !element )  return;

    _text_with_includes.append_dom( element );
    //_dom_element_list.push_back( element );

    //_source.clear();  //clear();

    _recompile = element.bool_getAttribute( "recompile", true );

    set_checked_attribute( &_language          , element, "language"         );
    set_checked_attribute( &_com_class_name    , element, "com_class" , true );
    set_checked_attribute( &_filename          , element, "filename"         );
    set_checked_attribute( &_java_class_name   , element, "java_class", true );

    //if( _use_process_class )
    //{
        //set_checked_attribute( &_process_class_string, element, "process_class"    );
        //if( _process_class_string != "" )  _process_class_path = Absolute_path( _folder_path, _process_class_string );
    //}

    bool separate_process_default = false;

    string use_engine = element.getAttribute     ( "use_engine" );
    
    if( use_engine == ""
     || use_engine == "task" )  _reuse = reuse_task;
    else
        z::throw_xc( "SCHEDULER-196", use_engine );
  //if( use_engine == "job"  )  _reuse = reuse_job;

    _set = true;
}

//---------------------------------------------------------------Module::set_xml_text_with_includes

void Module::set_xml_text_with_includes( const string& x )
{
    _text_with_includes.set_xml( x );
    _set = true;
}

//-------------------------------------------------------------------------------------Module::init

void Module::init()
{
    if( _initialized )  return;
    
    _monitors->initialize();


    if( _kind == kind_none )    // Wenn nicht, dann kind_internal
    {
#       ifdef Z_WINDOWS
        if( _com_class_name != "" )
        {
            _kind = kind_com;
        
            if( _language        != "" )  z::throw_xc( "SCHEDULER-145" );
            if( _java_class_name != "" )  z::throw_xc( "SCHEDULER-168" );
        }
        else
#       endif
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
    }

    _real_kind = _kind;     int REAL_KIND_UND_KIND_IDENTIFIZIEREN;  // _real_kind kann weg


    if( _kind != kind_internal )
    {
        if( _spooler )  _use_process_class = !_spooler->_ignore_process_classes;  //process_class_subsystem()->has_process_classes();

        if( _dont_remote )  _use_process_class = false, _process_class_path.clear();

        //if( _use_process_class )  
        //{
        //    //if( _process_class_path != "" )  _process_class_path.set_absolute_if_relative( _folder_path ); 
        //}
        //if( _use_process_class )   _kind = kind_remote;     // Bei _kind==kind_process: Nur wirksam, wenn Prozessklasse auch remote_scheduler hat!
    }


    switch( _real_kind )
    {
        case kind_internal:             if( _process_class_path != ""  )
                                            if( Process_class* process_class = process_class_or_null() )
                                                if( process_class->remote_scheduler() )  z::throw_xc( "SCHEDULER-REMOTE-INTERNAL?" );
                                        break;

        case kind_remote:               break;

        case kind_java:                 if( _spooler )
                                        {
                                            if( has_source_script() )  _spooler->_has_java_source = true;       // work_dir zum Compilieren bereitstellen
                                            if( !_use_process_class )  _spooler->_has_java = true;              // Java laden    
                                        }

                                        break;
        
        case kind_scripting_engine:     if( !has_source_script() )  z::throw_xc( "SCHEDULER-173" );
                                        break;

        case kind_process:              if( !has_source_script()  &&  _process_filename.empty() )  z::throw_xc( "SCHEDULER-173" );
                                        break;

#       ifdef Z_WINDOWS
            case kind_com:              if( has_source_script() )  z::throw_xc( "SCHEDULER-167" );
                                        break;
#       endif

        default:                        assert(0), z::throw_xc( Z_FUNCTION );
    }

    _initialized = true;
}

//--------------------------------------------------------------------------Module::create_instance

ptr<Module_instance> Module::create_instance()
{
    ptr<Module_instance> result = create_instance_impl();


    if( !_monitors->is_empty() )
    {
        if( _real_kind == kind_process  )  z::throw_xc( "SCHEDULER-315" );
        if( _real_kind == kind_internal )  z::throw_xc( "SCHEDULER-315", "Internal job" );
        
        if( !_use_process_class )  
        {
            vector<Module_monitor*> ordered_monitors = _monitors->ordered_monitors();

            Z_FOR_EACH( vector<Module_monitor*>, ordered_monitors, m )
            {
                Module_monitor* monitor = *m;

                result->_monitor_instance_list.push_back( Z_NEW( Module_monitor_instance( monitor, monitor->_module->create_instance() ) ) );
            }
        }
    }

    return result;
}

//---------------------------------------------------------------------Module::create_instance_impl

ptr<Module_instance> Module::create_instance_impl()
{
    ptr<Module_instance> result;


    Kind kind = _kind;
    
    if( _use_process_class  &&
        ( _real_kind != kind_process  ||  process_class()->is_remote_host() ) )     // Nicht-API-Tasks (einfache Prozesse) nicht über Prozessklasse abwickeln
    {
        kind = kind_remote;                 
    }

    switch( kind )
    {
        case kind_java:              
        {
            if( _spooler )  if( !_spooler->java_subsystem()->java_vm()  ||  !_spooler->java_subsystem()->java_vm()->running() )  z::throw_xc( "SCHEDULER-177" );

            _java_vm = get_java_vm( false );
            _java_vm->set_destroy_vm( false );   //  Nicht DestroyJavaVM() rufen, denn das hängt manchmal

            if( !_java_vm->running() )
            {
                Java_module_instance::init_java_vm( _java_vm );     // Native Java-Methoden (Callbacks) bekannt machen
            }
            
            ptr<Java_module_instance> p = Z_NEW( Java_module_instance( this ) );
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

        case kind_internal:
        {
            ptr<Internal_module_instance> p = Z_NEW( Internal_module_instance( this ) );
            result = +p;
            break;
        }

        default:                     
            z::throw_xc( "SCHEDULER-173" );
    }

    result->_kind = kind;

    return result;
}

//--------------------------------------------------------------------Module::process_class_or_null

Process_class* Module::process_class_or_null() const
{ 
    Process_class* result = NULL;

    if( _use_process_class )
    {
        result = _spooler->process_class_subsystem()->process_class_or_null( _process_class_path );
    }

    return result;
}

//----------------------------------------------------------------------------Module::process_class

Process_class* Module::process_class() const
{ 
    //kind_process darf das (für remote_scheduler)  if( !_use_process_class )  assert(0), z::throw_xc( "NO_PROCESS_CLASS", Z_FUNCTION );

    return _spooler->process_class_subsystem()->process_class( _process_class_path );
}

//-------------------------------------------------------------------------------Module::needs_java

bool Module::needs_java() 
{
    bool result = _real_kind == Module::kind_java  &&  has_source_script();

    if( !result )  result = _monitors->needs_java();

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
    _log(module?module->_log:NULL),

//#   ifdef Z_WINDOWS
        _process_environment( variable_set_from_environment() )   
//#    else
//        _process_environment( new Com_variable_set() )            // Unix vererbt automatisch die Umgebungsvariablen
//#   endif

{
    _com_task    = new Com_task;
    _com_log     = new Com_log;
    _process_environment->merge( _module->_process_environment );
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

    Z_FOR_EACH( Module_monitor_instance_list, _monitor_instance_list, m )  
    {
        try
        {
            (*m)->_module_instance->init();
        }
        catch( exception& x ) { z::throw_xc( "SCHEDULER-447", (*m)->obj_name(), x ); }
    }
}

//---------------------------------------------------------------------------Module_instance::clear

void Module_instance::clear()
{ 
    _object_list.clear(); 

    Z_FOR_EACH_REVERSE( Module_monitor_instance_list, _monitor_instance_list, m )  
    {
        try
        {
            (*m)->_module_instance->clear();
        }
        catch( exception& x ) { z::throw_xc( "SCHEDULER-447", (*m)->obj_name(), x ); }
    }
}

//--------------------------------------------------------------------Module_instance::set_job_name

void Module_instance::set_job_name( const string& job_name )
{
    _job_name = job_name; 

    Z_FOR_EACH( Module_monitor_instance_list, _monitor_instance_list, m )  
    {
        try
        {
            (*m)->_module_instance->set_job_name( job_name );
        }
        catch( exception& x ) { z::throw_xc( "SCHEDULER-447", (*m)->obj_name(), x ); }
    }
}

//---------------------------------------------------------------------Module_instance::set_task_id

void Module_instance::set_task_id( int id )
{ 
    _task_id = id; 

    Z_FOR_EACH( Module_monitor_instance_list, _monitor_instance_list, m )  
    {
        try
        {
            (*m)->_module_instance->set_task_id( id );
        }
        catch( exception& x ) { z::throw_xc( "SCHEDULER-447", (*m)->obj_name(), x ); }
    }
}

//-------------------------------------------------------------------------Module_instance::set_log

void Module_instance::set_log( Prefix_log* log )
{ 
    _log = log; 
    _com_log->set_log ( log );

    Z_FOR_EACH( Module_monitor_instance_list, _monitor_instance_list, m )
    {
        try
        {
            (*m)->_module_instance->set_log( log );
        }
        catch( exception& x ) { z::throw_xc( "SCHEDULER-447", (*m)->obj_name(), x ); }
    }
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
    _task = task;

    set_log( log );
    _com_task->set_task( task );

    _task_id = task->id();
    //_title = task->obj_name();          // Titel für Prozess

    Z_FOR_EACH( Module_monitor_instance_list, _monitor_instance_list, m )
    {
        try
        {
            (*m)->_module_instance->attach_task( task, log );
        }
        catch( exception& x ) { z::throw_xc( "SCHEDULER-447", (*m)->obj_name(), x ); }
    }

    
    // Environment, eigentlich nur bei einem Prozess nötig, also nicht bei <process_classes ignore="yes"> und <monitor>)
    if( task->environment_or_null() )  _process_environment->merge( task->environment_or_null() );
}

//---------------------------------------------------------------------Module_instance::detach_task

void Module_instance::detach_task()
{
    close_monitor();

    _com_task->set_task( NULL );
    _com_log ->set_log ( NULL );
    
    _task = NULL;
    _task_id = 0;
    //_title = "";

    Z_FOR_EACH_REVERSE( Module_monitor_instance_list, _monitor_instance_list, m )
    {
        try
        {
            (*m)->_module_instance->detach_task();
        }
        catch( exception& x ) { z::throw_xc( "SCHEDULER-447", (*m)->obj_name(), x ); }
    }
}

//-------------------------------------------------------------------------Module_instance::add_obj

void Module_instance::add_obj( IDispatch* object, const string& name )
{
    Z_FOR_EACH( Module_monitor_instance_list, _monitor_instance_list, m )  
    {
        try
        {
            (*m)->_module_instance->add_obj( object, name );
        }
        catch( exception& x ) { z::throw_xc( "SCHEDULER-447", (*m)->obj_name(), x ); }
    }
}

//--------------------------------------------------------------------------Module_instance::object

IDispatch* Module_instance::object( const string& name )
{
    IDispatch* result = object( name, NULL );
    if( !result )  assert(0), throw_xc( "Module_instance::object", name );
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

    Z_FOR_EACH( Module_monitor_instance_list, _monitor_instance_list, m )  
    {
        try
        {
            Module_monitor_instance* monitor_instance = *m;

            if( !monitor_instance->_module_instance->_load_called )  
            {
                ok = monitor_instance->_module_instance->implicit_load_and_start();
                if( !ok )  return false;

                Variant result = monitor_instance->_module_instance->call_if_exists( spooler_task_before_name );
                if( !result.is_missing() )  ok = check_result( result );

                if( !ok )  break;
            }
        }
        catch( exception& x ) { z::throw_xc( "SCHEDULER-447", (*m)->obj_name(), x ); }
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

Variant Module_instance::call_if_exists( const string& name, const Variant& param )
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
    Z_FOR_EACH_REVERSE( Module_monitor_instance_list, _monitor_instance_list, m )  
    {
        Module_monitor_instance* monitor_instance = *m;

        try
        {
            monitor_instance->_module_instance->call_if_exists( spooler_task_after_name );
        }
        catch( exception& x )
        {
            _log.error( monitor_instance->obj_name() + " " + spooler_task_after_name + ": " + x.what() );
        }

        try
        {
            monitor_instance->_module_instance->close();
        }
        catch( exception& x )
        {
            _log.error( monitor_instance->obj_name() + " " + x.what() );
        }
    }

    _monitor_instance_list.clear();
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
    Variant result;

    if( _monitor_instance_list.empty() )
    {
        result = call_if_exists( spooler_process_name );
    }
    else
    {
        Z_FOR_EACH( Module_monitor_instance_list, _monitor_instance_list, m )  
        {
            Module_monitor_instance* monitor_instance = *m;

            result = monitor_instance->_module_instance->call_if_exists( spooler_process_before_name );
            if( !check_result( result ) )  break;
        }

        if( check_result( result ) )
        {
            try
            {
                result = call_if_exists( spooler_process_name );
            }
            catch( Xc& x )
            {
                com_call( object( "spooler_task", _com_task ), "Set_error_code_and_text", x.code(), x.what() );
                result = false;
            }
            catch( zschimmer::Xc& x )
            {
                com_call( object( "spooler_task", _com_task ), "Set_error_code_and_text", x.code(), x.what() );
                result = false;
            }
            catch( exception& x )
            {
                com_call( object( "spooler_task", _com_task ), "Set_error_code_and_text", "", x.what() );
                result = false;
            }

            Z_FOR_EACH_REVERSE( Module_monitor_instance_list, _monitor_instance_list, m )  
            {
                Module_monitor_instance* monitor_instance = *m;

                Variant call_result = monitor_instance->_module_instance->call_if_exists( spooler_process_after_name, check_result( result ) );
                if( result.vt != VT_ERROR  &&  V_ERROR( &result ) != DISP_E_UNKNOWNNAME )  result = call_result;
            }
        }
    }

    return result;
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

//------------------------------------------------------------------------Module_instance::end_task

void Module_instance::end_task()
{
    assert( _task );
    if( _task )  _task->cmd_end();
}

//-------------------------------------------------------------------------Module_monitors::set_dom

void Module_monitors::set_dom( const xml::Element_ptr& element )
{
    if( !element.nodeName_is( "monitor" ) )  assert(0), z::throw_xc( "SCHEDULER-409", "monitor", element.nodeName() );
    
    string name = element.getAttribute( "name", default_monitor_name );

    ptr<Module_monitor> monitor = monitor_or_null( name );

    if( !monitor )
    {
        monitor = Z_NEW( Module_monitor() );
        monitor->_name   = name;
        monitor->_module = Z_NEW( Module( _main_module->_spooler, _main_module->_spooler->include_path(), &_main_module->_log ) );
        add_monitor( monitor );
    }

    monitor->_ordering = element.int_getAttribute( "ordering", monitor->_ordering );

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "script" ) )  
        {
            monitor->_module->set_dom( e );
        }
    }
}

//----------------------------------------------------------------------Module_monitors::needs_java

bool Module_monitors::needs_java() 
{
    bool result = false;

    Z_FOR_EACH( Module_monitors::Monitor_map, _monitor_map, m )
    {
        Module_monitor* monitor = m->second;
        result = monitor->_module->needs_java();
        if( result )  break;
    }

    return result;
}
//-----------------------------------------------------------------Module_monitors::monitor_or_null

Module_monitor* Module_monitors::monitor_or_null( const string& name )
{
    Module_monitor* result = NULL;

    Monitor_map::iterator m = _monitor_map.find( name );
    if( m != _monitor_map.end() )  result = m->second;

    return result;
}

//----------------------------------------------------------------------Module_monitors::initialize

void Module_monitors::initialize()
{
    vector<Module_monitor*> ordered_monitors = this->ordered_monitors();

    Z_FOR_EACH( vector<Module_monitor*>, ordered_monitors, m )
    {
        Module_monitor* monitor = *m;

        monitor->_module->init();
    }
}

//-------------------------------------------------------------Module_monitors::ordered_module_list

vector<Module_monitor*> Module_monitors::ordered_monitors()
{
    vector<Module_monitor*> result;

    result.reserve( _monitor_map.size() );
    Z_FOR_EACH( Monitor_map, _monitor_map, m )  result.push_back( m->second );
    sort( result.begin(), result.end(), Module_monitor::less_ordering );

    return result;
}

//-------------------------------------------------Module_monitor_instance::Module_monitor_instance

Module_monitor_instance::Module_monitor_instance( Module_monitor* monitor, Module_instance* module_instance )
:
    _module_instance(module_instance),
    _obj_name( "Script_monitor " + monitor->name() )
{
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
