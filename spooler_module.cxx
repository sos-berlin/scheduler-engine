// $Id: spooler_module.cxx,v 1.36 2003/08/29 20:44:25 jz Exp $
/*
    Hier sind implementiert

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

extern const string spooler_init_name       = "spooler_init()Z";
extern const string spooler_exit_name       = "spooler_exit()V";
extern const string spooler_open_name       = "spooler_open()Z";
extern const string spooler_close_name      = "spooler_close()V";
extern const string spooler_process_name    = "spooler_process()Z";
extern const string spooler_on_error_name   = "spooler_on_error()V";
extern const string spooler_on_success_name = "spooler_on_success()V";

//----------------------------------------------------------------xml::Element_ptr Source_part::dom

xml::Element_ptr Source_part::dom( const xml::Document_ptr& doc ) const
{
    xml::Element_ptr part_element = doc.createElement( "part_element" );

    part_element.setAttribute( "linenr", as_string( _linenr ) );
    part_element.setAttribute( "modtime", _modification_time.as_string() );
    part_element.appendChild( doc.createTextNode( _text ) );

    return part_element;
}

//---------------------------------------------------------xml::Document_ptr Source_with_parts::dom

xml::Element_ptr Source_with_parts::dom( const xml::Document_ptr& doc ) const
{
    xml::Element_ptr source_element = doc.createElement( "source" ); 

    Z_FOR_EACH_CONST( Parts, _parts, part )
    {
        xml::Element_ptr part_element = part->dom(doc);
        source_element.appendChild( part_element );
    }

    return source_element;
}

//---------------------------------------------------------xml::Document_ptr Source_with_parts::dom

xml::Document_ptr Source_with_parts::dom_doc() const
{
    xml::Document_ptr doc;

    doc.create();
    doc.appendChild( doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"iso-8859-1\"" ) );
    doc.appendChild( dom(doc) );

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
    _parts.push_back( Source_part( linenr, text, mod_time ) );

    if( mod_time  &&  _max_modification_time < mod_time )   _max_modification_time = mod_time;
}

//----------------------------------------------------------------------------------Module::set_dom

void Module::set_dom_without_source( const xml::Element_ptr& element )
{
    _source.clear();  //clear();

    _language         = element.getAttribute     ( "language"  );
    _com_class_name   = element.getAttribute     ( "com_class" );
    _filename         = element.getAttribute     ( "filename"  );
    _java_class_name  = element.getAttribute     ( "java_class" );
    _recompile        = element.bool_getAttribute( "recompile" );

    bool separate_process_default = false;
#   ifndef Z_WINDOWS
        // Bei Perl automatisch separate_process="yes", weil Perl sonst beim zweiten Aufruf abstürzt
        separate_process_default = string_begins_with( lcase(_language), "perl" );   
#   endif

    _separate_process = element.bool_getAttribute( "separate_process", separate_process_default );
                                                                                                                           

    string use_engine = element.getAttribute     ( "use_engine" );
    
    if( use_engine == ""
     || use_engine == "task" )  _reuse = reuse_task;
    else
    if( use_engine == "job"  )  _reuse = reuse_job;

    init();
}

//----------------------------------------------------------------------Module::set_dom_source_only

void Module::set_dom_source_only( const xml::Element_ptr& element, const Time& xml_mod_time, const string& include_path )
{
    set_source_only( text_from_xml_with_include( element, xml_mod_time, include_path ) );
}

//--------------------------------------------------------------------------Module::set_source_only

void Module::set_source_only( const Source_with_parts& source )
{
    _source = source;

    switch( _kind )
    {
        case kind_remote:
            break;

        case kind_java:
            //if( !_source.empty() )  throw_xc( "SPOOLER-167" );
            break;

        case kind_scripting_engine:
            if( _source.empty() )  throw_xc( "SPOOLER-173" );
            break;

#     ifdef Z_WINDOWS
        case kind_com:
            if( !_source.empty() )  throw_xc( "SPOOLER-167" );
            break;
#     endif

        default: 
            throw_xc( "Module::set_source_only" );
    }

    clear_java();

    _set = true;
}

//-------------------------------------------------------------------------------------Module::init

void Module::init()
{
    if( _separate_process )
    {
        if( _process_class_name != "" )  throw_xc( "SPOOLER-194" );
        //_process_class_name = temporary_process_class_name;
    }

    if( _use_process_class )  _spooler->process_class( _process_class_name );     // Fehler, wenn der Name nicht bekannt ist.


    if( _separate_process  ||  _use_process_class )
    {
        _kind = kind_remote;
    }
    else
    {
#     ifdef Z_WINDOWS
        if( _com_class_name != "" )
        {
            _kind = kind_com;
        
            if( _language        != "" )  throw_xc( "SPOOLER-145" );
            if( _java_class_name != "" )  throw_xc( "SPOOLER-168" );
        }
        else
#     endif

        if( _java_class_name != ""  ||  lcase(_language) == "java" )
        {
            _kind = kind_java;
     
            if( _language == "" )  _language = "Java";

            if( lcase(_language) != "java" )  throw_xc( "SPOOLER-166" );
            if( _com_class_name  != ""     )  throw_xc( "SPOOLER-168" );

            if( _spooler )  _spooler->_has_java = true;
        }
        else
        {
            _kind = kind_scripting_engine;
             if( _language == "" )  _language = SPOOLER_DEFAULT_LANGUAGE;
        }
    }
}

//--------------------------------------------------------------------------Module::create_instance

ptr<Module_instance> Module::create_instance()
{
    switch( _kind )
    {
        case kind_java:              
        {
            //if( !_spooler->_java_vm->running() )  throw_xc( "SPOOLER-177" );

            _java_vm = get_java_vm();
            ptr<Java_module_instance> p = Z_NEW( Java_module_instance( _java_vm, this ) );
            return +p;
        }

        case kind_scripting_engine:  
        {
            ptr<Scripting_engine_module_instance> p = Z_NEW( Scripting_engine_module_instance( this ) );
            return +p;
        }

#     ifdef Z_WINDOWS
        case kind_com:               
        {
            ptr<Com_module_instance> p = Z_NEW( Com_module_instance( this ) );
            return +p;
        }
#     endif

        case kind_remote:
        {
            ptr<Remote_module_instance_proxy> p = Z_NEW( Remote_module_instance_proxy( this, _process_class_name ) );
            return +p;
        }

        default:                     
            throw_xc( "SPOOLER-173" );
    }
}

//----------------------------------------------------------------Module_instance::In_call::In_call
/*
Module_instance::In_call::In_call( Job* job, const string& name ) 
: 
    _job(job),
    _name(name),
    _result_set(false)
{ 
    int pos = name.find( '(' );
    string my_name = pos == string::npos? name : name.substr( 0, pos );
    
    _job->set_in_call( my_name ); 
    LOG( *job << '.' << my_name << "() begin\n" );

    Z_WINDOWS_ONLY( _ASSERTE( _CrtCheckMemory( ) ); )
}
*/
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
        LOG( *_module_instance << '.' << _name << "() begin\n" );

        Z_WINDOWS_ONLY( _ASSERTE( _CrtCheckMemory() ); )
    }
}

//---------------------------------------------------------------Module_instance::In_call::~In_call

Module_instance::In_call::~In_call()
{ 
    if( _module_instance )
    {
        _module_instance->set_in_call( NULL ); 

        {
            Log_ptr log;

            if( log )
            {
                *log << *_module_instance << '.' << _name << "() end";
                if( _result_set )  *log << "  result=" << ( _result? "true" : "false" );
                *log << '\n';
            }
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
    _com_context = new Com_context;

    _spooler_exit_called = false;
}

//----------------------------------------------------------------------------Module_instance::init

void Module_instance::init()
{
    _spooler = _module->_spooler;

    if( !_module->set() )  throw_xc( "SPOOLER-146" );
}

//--------------------------------------------------------------------------------Task::set_in_call

void Module_instance::set_in_call( In_call* in_call, const string& extra )
{
    //THREAD_LOCK( _lock )
    {
        _in_call = in_call;

        if( in_call  &&  _spooler  &&  _spooler->_debug )
        {
            _log.debug( in_call->_name + "()  " + extra );
        }
    }
}

//-------------------------------------------------------------------------Module_instance::add_obj

void Module_instance::add_obj( const ptr<IDispatch>& object, const string& name )
{
    if( name == "spooler_log"    )  _com_context->_log     = (qi_ptr<spooler_com::Ilog>)    object;
    else
    if( name == "spooler"        )  _com_context->_spooler = (qi_ptr<spooler_com::Ispooler>)object;
    else
  //if( name == "spooler_thread" )  _com_context->_thread  = (qi_ptr<spooler_com::Ithread>) object;
  //else
    if( name == "spooler_job"    )  _com_context->_job     = (qi_ptr<spooler_com::Ijob>)    object;
    else
    if( name == "spooler_task"   )  _com_context->_task    = (qi_ptr<spooler_com::Itask>)   object;
    else
        throw_xc( "Module_instance::add_obj", name.c_str() );
}

//----------------------------------------------------------------------------Module_instance::call
/*
Variant Module_instance::call( const string& name )
{
    if( _in_call.empty() )
    {
        In_call in_call ( this, name );
        call( name );
    }
    else
    {
        call( name );
    }
}

//----------------------------------------------------------------------------Module_instance::call

Variant Module_instance::call( const string& name, int param )
{
    if( _in_call.empty() )
    {
        In_call in_call ( this, name );
        call( name, param );
    }
    else
    {
        call( name, param );
    }
}
*/
//------------------------------------------------------------------Module_instance::call_if_exists

Variant Module_instance::call_if_exists( const string& name )
{
    if( name_exists(name) )  return call( name );
                       else  return Variant();
}

//---------------------------------------------------------------------------Module_instance::close

void Module_instance::close()
{
    if( !_spooler_exit_called  &&  callable() )     // _spooler_exit_called wird auch von Remote_module_instance_server gesetzt.
    {
        try
        {
            _spooler_exit_called = true;
            call_if_exists( "spooler_exit()V" );
        }
        catch( const exception& x ) 
        { 
            _log.error( x.what() ); 
        }
    }

  //if( _com_log )  _com_log->close();
  //if( _com_task )  _com_task->close();

    if( _com_context )  _com_context->close(), _com_context = NULL;
}

//--------------------------------------------------------------------Module_instance::begin__start

Async_operation* Module_instance::begin__start()  // const Object_list& object_list )
{
    return NULL;
}

//----------------------------------------------------------------------Module_instance::begin__end

bool Module_instance::begin__end()
{
    if( !loaded() )
    {
        init();
        FOR_EACH_CONST( Object_list, _object_list, o )  add_obj( o->_object, o->_name );
        load();
        start();

        _spooler_init_called = true;
        bool ok = check_result( call_if_exists( spooler_init_name ) );
        if( !ok )  return ok;
    }

    return check_result( call_if_exists( spooler_open_name ) );
}

//----------------------------------------------------------------------Module_instance::end__start

Async_operation* Module_instance::end__start( bool success )
{
    return NULL;
}

//------------------------------------------------------------------------Module_instance::end__end

void Module_instance::end__end()
{
    call_if_exists( spooler_close_name );

    // Exception abfangen und spooler_on_error rufen // _com_task->set_error( x )?
/*
    if( _task )
    {
        if( loaded() && success )
        {
            // spooler_on_success() wird nicht gerufen, wenn spooler_init() false lieferte

            try 
            {
                if( loaded()  &&  name_exists( spooler_on_success_name ) )
                {
                    In_call in_call ( _task, spooler_on_success_name );
                    call( spooler_on_success_name );
                }
            }
            catch( const exception& x ) { _task->log().error( string(spooler_on_error_name) + ": " + x.what() ); }
        }
        else
        if( !success )
        {
            try 
            {
                if( loaded()  &&  name_exists( spooler_on_error_name ) )
                {
                    In_call in_call ( _task, spooler_on_error_name );
                    call( spooler_on_error_name );
                }
            }
            catch( const exception& x ) { _task->set_error( x ); }
        }
    }
*/

    if( _spooler_init_called  &&  !_spooler_exit_called )
    {
        _spooler_exit_called = true;
        call_if_exists( spooler_exit_name );
    }

    if( _close_instance_at_end )
    {
        close();
        _com_task = new Com_task();
    }
}

//---------------------------------------------------------------------Module_instance::step__start

Async_operation* Module_instance::step__start()
{
    return NULL;
}

//-----------------------------------------------------------------------Module_instance::step__end

bool Module_instance::step__end()
{
    if( !name_exists( spooler_process_name ) )  return false;

    return check_result( call( spooler_process_name ) );
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
