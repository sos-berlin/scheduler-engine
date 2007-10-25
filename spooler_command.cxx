// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com
/*
    Hier ist implementiert

    Command_processor
*/


#include "spooler.h"
#include "spooler_version.h"
#include "../file/anyfile.h"
#include "../zschimmer/z_sql.h"
#include "../zschimmer/embedded_files.h"
#include "../zschimmer/z_gzip.h"

// Für temporäre Datei:
#include <sys/stat.h>               // S_IREAD, stat()
#include <fcntl.h>                  // O_RDONLY

#if defined SYSTEM_WIN
#   include <io.h>                  // open(), read() etc.
#   include <share.h>
#   include <direct.h>              // mkdir
#   include <windows.h>
#else
#   include <stdio.h>               // fileno
#   include <unistd.h>              // read(), write(), close()
#   include <signal.h>              // kill()
#endif

#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>


namespace sos {
namespace scheduler {

using namespace std;

//-------------------------------------------------------------------------------------------------

//#include "spooler_http_files.cxx"     // Generiert mit:  cd html && perl ../make/files_to_cxx.pl jz/*.html jz/*.js jz/*.xslt jz/*.css

//-------------------------------------------------------------------------------------------------

const string default_filename = "index.html";
extern const Embedded_files embedded_files_z;   // Zschimmers HTML-Dateien (/z/index.html etc.)

//---------------------------------------------------------------Remote_task_close_command_response

struct Remote_task_close_command_response : File_buffered_command_response
{
                                Remote_task_close_command_response( Process*, Communication::Connection* );
                               ~Remote_task_close_command_response()                                {}

    // Async_operation
    virtual bool                async_finished_             () const                                { return _state == s_finished; }
    virtual string              async_state_text_           () const                                { return "Remote_task_close_command_response"; }
    virtual bool                async_continue_             ( Continue_flags );

    void                        close                       ();

  private:
    void                        write_file                  ( const string& what, const File_path& );

    enum State { s_initial, s_waiting, s_finished };

    Fill_zero                  _zero_;
    pid_t                      _pid;    
    ptr<Process>               _process;
    ptr<Communication::Connection>  _connection;
    ptr<Async_operation>       _operation;
    State                      _state;
};

//----------------------------emote_task_close_command_response::Remote_task_close_command_response

Remote_task_close_command_response::Remote_task_close_command_response( Process* p, Communication::Connection* c )
: 
    _zero_(this+1), 
    _process(p), 
    _connection(c) 
{
}

//--------------------------------------------------------Remote_task_close_command_response::close

void Remote_task_close_command_response::close()
{
    File_buffered_command_response::close();
}

//----------------------------------------------Remote_task_close_command_response::async_continue_

bool Remote_task_close_command_response::async_continue_( Continue_flags )
{
    Z_DEBUG_ONLY( if( _operation )  assert( _operation->async_finished() ) );

    bool something_done = false;

    switch( _state )
    {
        case s_initial:
        {
            _pid = _process->pid();
            _operation = _process->close__start();
            _operation->set_async_parent( this );       // Weckt uns, wenn _operation fertig ist
            _state = s_waiting;
        }

        case s_waiting:
        {
            if( _operation->async_finished() )
            {
                _operation  = NULL;
                _process->close__end();


                // XML-Anwort 

                begin_standard_response();

                _xml_writer.begin_element( "ok" );
                write_file( "stdout", _process->stdout_path() );
                write_file( "stderr", _process->stderr_path() );
                _xml_writer.end_element( "ok" );

                _xml_writer.flush();

                end_standard_response();

                
                _state = s_finished;


                if( _connection->_operation_connection )  _connection->_operation_connection->unregister_task_process( _pid );

                _process    = NULL;
                _connection = NULL;
                
                something_done = true;
            }
            break;
        }

        default: ;
    }

    return something_done;
}

//---------------------------------------------------Remote_task_close_command_response::write_file

void Remote_task_close_command_response::write_file( const string& name, const File_path& path )
{
    string text;

    try
    {
        text = string_from_file( path );
        //for( int i = 0; i < text.length(); i++ )  if( text[i] == '\0' )  text[i] = ' ';
    }
    catch( exception& x ) { text = S() << "ERROR: " << x.what(); }

    _xml_writer.begin_element( "file" );
    _xml_writer.set_attribute( "name", name );
    _xml_writer.set_attribute( "encoding", "hex" );
    _xml_writer.write( lcase_hex( (const Byte*)text.data(), text.length() ) );
    _xml_writer.end_element( "file" );
}

//------------------------------------------------------------------------------------dom_append_nl

void dom_append_nl( const xml::Element_ptr& )
{
    //indent ersetzt diese Newlines.    element.appendChild( element.ownerDocument().createTextNode( "\n" ) );
}

//-----------------------------------------------------------------------------create_error_element

xml::Element_ptr create_error_element( const xml::Document_ptr& document, const Xc_copy& x, time_t gm_time )
{
    xml::Element_ptr e = document.createElement( "ERROR" );

    //timeb  tm;     // Ob die Sommerzeitverschiebung bei der Fehlerzeit berücksichtigt wird, hängt von der _aktuellen_ Zeit ab.
    //ftime( &tm );  // Nicht schön, aber es funktioniert, weil der Spooler sowieso nicht während der Zeitumstellung laufen soll.
    //e.setAttribute( "time", Sos_optional_date_time( (time_t)x.time() - timezone - ( tm.dstflag? _dstbias : 0 ) ).as_string() );
    if( gm_time )  e.setAttribute( "time", Sos_optional_date_time( localtime_from_gmtime( gm_time ) ).as_string() );

    if( !empty( x->name() )          )  e.setAttribute( "class" , x->name()          );

    e.setAttribute( "code", x->code() );
    e.setAttribute( "text", remove_password( x->what() ) );
    
    if( !empty( x->_pos.filename() ) )  e.setAttribute( "source", x->_pos.filename() );
    if( x->_pos._line >= 0           )  e.setAttribute( "line"  , as_string( x->_pos._line + 1 ) );
    if( x->_pos._col  >= 0           )  e.setAttribute( "col"   , as_string( x->_pos._col + 1  ) );

    return e;
}

//-----------------------------------------------------------------------------create_error_element

xml::Element_ptr create_error_element( const xml::Document_ptr& document, const zschimmer::Xc& x, time_t gm_time )
{
    Xc xc ( "" );
    xc.set_code( x.code().c_str() );
    xc.set_what( x.what() );

    Xc_copy xc_copy ( xc );
    xc_copy.set_time( (double)gm_time );

    return create_error_element( document, xc_copy );
}

//-----------------------------------------------------------------------------append_error_element

void append_error_element( const xml::Element_ptr& element, const Xc_copy& x )
{
    element.appendChild( create_error_element( element.ownerDocument(), x, (time_t)x.time() ) );
}


//--------------------------------------------------------------------------------xc_from_dom_error

Xc_copy xc_from_dom_error( const xml::Element_ptr& element )
{
    Xc x ( "" );

    x.set_code( element.getAttribute( "code" ).c_str() );
    x.set_what( element.getAttribute( "text" ) );

    return x;
}

//------------------------------------------------------------------------------how_what::Show_what

Show_what::Show_what( Show_what_enum what ) 
: 
    _zero_(this+1), 
    _what(what), 
    _max_orders(INT_MAX),
    _max_task_history(10),
    _folder_path( root_path )
{
}

//-------------------------------------------------------------Command_processor::Command_processor

Command_processor::Command_processor( Spooler* spooler, Security::Level security_level, Communication::Operation* cp )
: 
    _zero_(this+1),
    _spooler(spooler),
    _communication_operation( cp ),
    _validate(true),
    _security_level( security_level )
{
    _variable_set_map[ variable_set_name_for_substitution ] = _spooler->_environment;
    _spooler->_executing_command = true;

    begin_answer();
}

//------------------------------------------------------------Command_processor::~Command_processor

Command_processor::~Command_processor()
{
    _spooler->_executing_command = false;
}

//----------------------------------------------------------------Command_processor::execute_config

xml::Element_ptr Command_processor::execute_config( const xml::Element_ptr& config_element )
{
    if( _security_level < Security::seclev_all )  z::throw_xc( "SCHEDULER-121" );

    if( !config_element.nodeName_is( "config" ) )  z::throw_xc( "SCHEDULER-113", config_element.nodeName() );

    string spooler_id = config_element.getAttribute( "spooler_id" );
    if( spooler_id.empty()  ||  spooler_id == _spooler->id()  ||  _spooler->_manual )
    {
        if( _load_config_immediately )  _spooler->load_config( config_element, _source_filename );
                                  else  _spooler->cmd_load_config( config_element, _source_filename );
    }

    return _answer.createElement( "ok" );
}

//-------------------------------------------------------------Command_processor::execute_show_jobs

xml::Element_ptr Command_processor::execute_show_jobs( const Show_what& show )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    return _spooler->root_folder()->job_folder()->dom_element( _answer, show );
}

//----------------------------------------------------------Command_processor::execute_show_threads
/*
xml::Element_ptr Command_processor::execute_show_threads( const Show_what& show )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    return _spooler->threads_as_xml( _answer, show );
}
*/
//--------------------------------------------------Command_processor::execute_show_process_classes

xml::Element_ptr Command_processor::execute_show_process_classes( const Show_what& show )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    return _spooler->root_folder()->process_class_folder()->dom_element( _answer, show );
}

//------------------------------------------------------------Command_processor::execute_show_state

xml::Element_ptr Command_processor::execute_show_state( const xml::Element_ptr& element, const Show_what& show_ )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    Show_what show = show_;
    if( show.is_set( show_all_ ) )  show |= Show_what_enum( show_task_queue | show_description | show_remote_schedulers );

    if( element.nodeName_is( "s" ) )  show |= show_job_chains | show_job_chain_orders | show_operations | show_folders;


    return _spooler->state_dom_element( _answer, show );
}

//---------------------------------------------------------------Command_processor::get_id_and_prev

void Command_processor::get_id_and_next( const xml::Element_ptr& element, int* id, int* next )
{
    *id = element.uint_getAttribute( "id", -1 );

    string prev_str = element.getAttribute( "prev" );

    *next = prev_str == ""   ? ( *id == -1? -10 : 0 ) :
            prev_str == "all"? -INT_MAX 
                             : -as_int(prev_str);

    string next_str = element.getAttribute( "next" );
    if( next_str != "" )  *next = as_uint(next_str);

    const int max_n = 1000;
    if( abs(*next) > max_n )  *next = sgn(*next) * max_n,  _spooler->log()->warn( message_string( "SCHEDULER-285", max_n ) );
}

//---------------------------------------------------------Command_processor::execute_show_calendar

xml::Element_ptr Command_processor::execute_show_calendar( const xml::Element_ptr& element, const Show_what& show_what )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    Show_calendar_options options;

    options._from   = Time::now();
    options._before = Time::never;
    options._limit  = element.int_getAttribute( "limit", 100 );

    if( element.hasAttribute( "from"   ) )  options._from  .set_datetime( element.getAttribute( "from"  ) );
    if( element.hasAttribute( "before" ) )  options._before.set_datetime( element.getAttribute( "before" ) );
                                      else  options._before = options._from.midnight() + 7*24*3600 + 1;                 // Default: eine Woche




    xml::Element_ptr calendar_element = _answer.createElement( "calendar" );

    if( show_what.is_set( show_jobs )  &&  options._count < options._limit )
        _spooler->job_subsystem()->append_calendar_dom_elements( calendar_element, &options );

    if( show_what.is_set( show_orders )  &&  options._count < options._limit )
        _spooler->order_subsystem()->append_calendar_dom_elements( calendar_element, &options );

    return calendar_element;
}

//----------------------------------------------------------Command_processor::execute_show_history

xml::Element_ptr Command_processor::execute_show_history( const xml::Element_ptr& element, const Show_what& show_ )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    Show_what show = show_;
    if( show.is_set( show_all_ ) )  show |= show_log;

    int id, next;
    get_id_and_next( element, &id, &next );
    
    ptr<Job> job = _spooler->job_subsystem()->job( Absolute_path( root_path, element.getAttribute( "job" ) ) );

    return job->read_history( _answer, id, next, show );
}

//----------------------------------------------------Command_processor::execute_show_order_history
/*
xml::Element_ptr Command_processor::execute_show_order_history( const xml::Element_ptr& element, const Show_what& show )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    if( show.is_set( show_all_ ) )  show = Show_what_enum( show | show_log );

    int id, next;
    get_id_and_prev( element, &id, &next );
    
    Sos_ptr<Job_chain> job_chain = _spooler->order_subsystem()->job_chain( element.getAttribute( "job_chain" ) );

    return job_chain->read_order_history( _answer, id, next, show );
}
*/
//--------------------------------------------------------Command_processor::execute_modify_spooler

xml::Element_ptr Command_processor::execute_modify_spooler( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );

    int timeout = element.int_getAttribute( "timeout", 999999999 );

    string cmd = element.getAttribute( "cmd" );
  //if( !cmd.empty() )
    {
        if( cmd == "pause"                 )  _spooler->cmd_pause();
        else
        if( cmd == "continue"              )  _spooler->cmd_continue();
        else
      //if( cmd == "stop"                  )  _spooler->cmd_stop();
      //else
        if( cmd == "reload"                )  _spooler->cmd_reload();
        else
        if( cmd == "terminate"             )  _spooler->cmd_terminate( false, timeout );
        else
        if( cmd == "terminate_and_restart" )  _spooler->cmd_terminate_and_restart( timeout );
        else
        if( cmd == "let_run_terminate_and_restart" )  _spooler->cmd_let_run_terminate_and_restart();
        else
        if( cmd == "abort_immediately"             )  _spooler->abort_immediately(); 
        else
        if( cmd == "abort_immediately_and_restart" )  _spooler->abort_immediately( true );
        else
            z::throw_xc( "SCHEDULER-105", cmd );
    }
    
    return _answer.createElement( "ok" );
}

//-------------------------------------------------------------Command_processor::execute_terminate

xml::Element_ptr Command_processor::execute_terminate( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );

    bool   restart        = element.bool_getAttribute( "restart"                     , false );
    bool   all_schedulers = element.bool_getAttribute( "all_schedulers"              , false );
    int    timeout        = element. int_getAttribute( "timeout"                     , INT_MAX );
    string member_id      = element.     getAttribute( "cluster_member_id"           );
    bool   delete_dead    = element.bool_getAttribute( "delete_dead_entry"           , false );

  //string continue_excl  = element.     getAttribute( "continue_exclusive_operation", "non_backup" );
    string continue_excl  = element.bool_getAttribute( "continue_exclusive_operation" )? cluster::continue_exclusive_any 
                                                                                       : "non_backup";
    if( member_id == ""  ||  member_id == _spooler->cluster_member_id() )
    {
        _spooler->cmd_terminate( restart, timeout, continue_excl, all_schedulers );
    }
    else
    {
        if( !_spooler->_cluster )  z::throw_xc( Z_FUNCTION, "no cluster" );

        if( delete_dead )
        {
            _spooler->_cluster->delete_dead_scheduler_record( member_id );
        }
        else
        {
            S cmd;
            cmd << "<terminate";
            if( timeout < INT_MAX )  cmd << " timeout='" << timeout << "'";
            if( restart )  cmd << " restart='yes'";
            cmd << "/>";

            _spooler->_cluster->set_command_for_scheduler( (Transaction*)NULL, cmd, member_id );
        }
    }

    return _answer.createElement( "ok" );
}

//--------------------------------------------------------------Command_processor::execute_show_job

xml::Element_ptr Command_processor::execute_show_job( const xml::Element_ptr& element, const Show_what& show_ )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    Show_what show = show_;
    if( show.is_set( show_all_ ) )  show |= show_description | show_task_queue | show_orders;

    Job_chain*    job_chain      = NULL;
    Absolute_path job_chain_path = Absolute_path( root_path, element.getAttribute( "job_chain" ) );
    
    if( job_chain_path != "" )  job_chain = _spooler->order_subsystem()->job_chain( job_chain_path );
    
    return _spooler->job_subsystem()->job( Absolute_path( root_path, element.getAttribute( "job" ) ) ) -> dom_element( _answer, show, job_chain );
}

//------------------------------------------------------------Command_processor::execute_modify_job

xml::Element_ptr Command_processor::execute_modify_job( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    Absolute_path job_path = Absolute_path( root_path, element.getAttribute( "job" ) );
    string        cmd_name =                     element.getAttribute( "cmd" );

    Job::State_cmd cmd = cmd_name.empty()? Job::sc_none 
                                         : Job::as_state_cmd( cmd_name );

    Job* job = _spooler->job_subsystem()->job( job_path );


    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "run_time" ) )  { job->set_run_time( e );  break; }
    }


    if( cmd )  job->set_state_cmd( cmd );
    
    return _answer.createElement( "ok" );
}

//----------------------------------------------------------Command_processor::execute_show_cluster

xml::Element_ptr Command_processor::execute_show_cluster( const xml::Element_ptr&, const Show_what& show )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    xml::Element_ptr result = _answer.createElement( "cluster" );

    if( _spooler->_cluster )  result.appendChild( _spooler->_cluster->dom_element( _answer, show ) );

    result.appendChild( _spooler->_supervisor->dom_element( _answer, show ) );
    
    return result;
}

//-------------------------------------------------------------Command_processor::execute_show_task

xml::Element_ptr Command_processor::execute_show_task( const xml::Element_ptr& element, const Show_what& show )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    int task_id = element.int_getAttribute( "id" );

    ptr<Task> task = _spooler->get_task_or_null( task_id );
    if( task )
    {
        return task->dom_element( _answer, show );
    }
    else
    {
        return _spooler->_db->read_task( _answer, task_id, show );
    }
}

//-------------------------------------------------------------Command_processor::execute_kill_task

xml::Element_ptr Command_processor::execute_kill_task( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    int    id          = element. int_getAttribute( "id" );
    string job_path    = element.     getAttribute( "job" );              // Hilfsweise
    bool   immediately = element.bool_getAttribute( "immediately", false );
    

    _spooler->job_subsystem()->job( Absolute_path( root_path, job_path ) )->kill_task( id, immediately );
    
    return _answer.createElement( "ok" );
}

//-------------------------------------------------------------Command_processor::execute_start_job

xml::Element_ptr Command_processor::execute_start_job( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    string job_path        = element.getAttribute( "job"   );
    string task_name       = element.getAttribute( "name"  );
    string after_str       = element.getAttribute( "after" );
    string at_str          = element.getAttribute( "at"    );
    string web_service_name= element.getAttribute( "web_service" );

    Time start_at;

    if( at_str == ""       )  at_str = "now";
    if( at_str == "period" )  start_at = 0;                                     // start="period" => start_at = 0 (sobald eine Periode es zulässt)
                        else  start_at = Time::time_with_now( at_str );         // "now+..." möglich

    if( !after_str.empty() )  start_at = Time::now() + Time( as_int( after_str ) );     // Entweder at= oder after=


    ptr<Com_variable_set> params      = new Com_variable_set;
    ptr<Com_variable_set> environment = new Com_variable_set;

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "params"      ) )   params->set_dom( e, &_variable_set_map );
        else
        if( e.nodeName_is( "environment" ) )   environment = new Com_variable_set,  environment->set_dom( e, NULL, "variable" );
    }

    Job* job = _spooler->job_subsystem()->job( Absolute_path( root_path, job_path ) );
    ptr<Task> task = job->create_task( ptr<spooler_com::Ivariable_set>(params), task_name, start_at );
    task->set_web_service( web_service_name );
    if( environment )  task->merge_environment( environment );
    job->enqueue_task( task );

    xml::Element_ptr result = _answer.createElement( "ok" ); 
    result.appendChild( task->dom_element( _answer, Show_what() ) );
    return result;
}

//------------------------------------Command_processor::execute_remote_scheduler_start_remote_task

xml::Element_ptr Command_processor::execute_remote_scheduler_start_remote_task( const xml::Element_ptr& start_task_element )
{
    if( _security_level < Security::seclev_all )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    int tcp_port = start_task_element.int_getAttribute( "tcp_port" );


    ptr<Process> process = Z_NEW( Process( _spooler ) );

    process->set_controller_address( Host_and_port( _communication_operation->_connection->_peer_host_and_port._host, tcp_port ) );
    process->start();


    _communication_operation->_operation_connection->register_task_process( process );
    
    /*
        Prozess registrieren
            TCP-Verbindung bekommt ein Task-Prozess-Register
            Bei Verbindungsverlust werden alle Prozesse abgebrochen
            Ebenso bei Scheduler-Ende (also im Scheduler registrieren)

        stdout und stderr verbinden (über vorhandene TCP-Verbindung?)

        Prozess-Event überwachen (Unix: waitpid), Prozessende bemerken und protokollieren
    */

    if( _log )  _log->info( message_string( "SCHEDULER-848", process->pid() ) );

    xml::Element_ptr result = _answer.createElement( "process" ); 
    result.setAttribute( "pid", process->pid() );
    return result;
}

//------------------------------------Command_processor::execute_remote_scheduler_remote_task_close

xml::Element_ptr Command_processor::execute_remote_scheduler_remote_task_close( const xml::Element_ptr& close_element )
{
    if( _security_level < Security::seclev_all )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    int  pid  = close_element. int_getAttribute( "pid" );
    bool kill = close_element.bool_getAttribute( "kill", false );

    Process* process = _communication_operation->_operation_connection->get_task_process( pid );

    if( kill )  process->kill();

    ptr<Remote_task_close_command_response> response = Z_NEW( Remote_task_close_command_response( process, _communication_operation->_connection ) );
    response->set_async_manager( _spooler->_connection_manager );
    response->async_wake();
    _response = response;

    return NULL;
}

//---------------------------------------------------------Command_processor::execute_signal_object

//xml::Element_ptr Command_processor::execute_signal_object( const xml::Element_ptr& element )
//{
//    if( _security_level < Security::seclev_signal )  z::throw_xc( "SCHEDULER-121" );
//
//    string class_name = element.getAttribute( "class" );
//    Level  level      = as_int( element.getAttribute( "level" ) );
//
//    xml::Element_ptr jobs_element = _answer.createElement( "tasks" );
//
//    //_spooler->signal_object( class_name, level );
//    
//    return _answer.createElement( "ok" );
//}

//--------------------------------------------------------------Command_processor::execute_add_jobs

xml::Element_ptr Command_processor::execute_add_jobs( const xml::Element_ptr& add_jobs_element )
{
    if( _security_level < Security::seclev_all )  z::throw_xc( "SCHEDULER-121" );

    //ptr<Task_subsystem> thread = _spooler->get_thread( add_jobs_element.getAttribute( "thread" ) );
    _spooler->cmd_add_jobs( add_jobs_element );

    return _answer.createElement( "ok" );
}

//-------------------------------------------------------------------Command_processor::execute_job

xml::Element_ptr Command_processor::execute_job( const xml::Element_ptr& job_element )
{
    if( _security_level < Security::seclev_all )  z::throw_xc( "SCHEDULER-121" );

    _spooler->cmd_job( job_element );

    return _answer.createElement( "ok" );
}

//-------------------------------------------------------------Command_processor::execute_job_chain

xml::Element_ptr Command_processor::execute_job_chain( const xml::Element_ptr& job_chain_element )
{
    if( _security_level < Security::seclev_all )  z::throw_xc( "SCHEDULER-121" );


    // Siehe auch Spooler::set_dom()

    ptr<Job_chain> job_chain = new Job_chain( _spooler );
    job_chain->set_folder_path( root_path );
    job_chain->set_name( job_chain_element.getAttribute( "name" ) );
    job_chain->set_dom( job_chain_element );

    job_chain->initialize();
    _spooler->root_folder()->job_chain_folder()->add_job_chain( job_chain );
    job_chain->activate();

    return _answer.createElement( "ok" );
}

//-------------------------------------------------------Command_processor::execute_show_job_chains

xml::Element_ptr Command_processor::execute_show_job_chains( const xml::Element_ptr&, const Show_what& show_ )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    Show_what show = show_;
    if( show.is_set( show_all_   ) )  show |= show._what | show_description | show_orders;
    if( show.is_set( show_orders ) )  show |= show_job_chain_orders;

    return _spooler->root_folder()->job_chain_folder()->dom_element( _answer, show | show_job_chains | show_job_chain_jobs );
}

//--------------------------------------------------------Command_processor::execute_show_job_chain

xml::Element_ptr Command_processor::execute_show_job_chain( const xml::Element_ptr& show_job_chain_element, const Show_what& show_ )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    Show_what show = show_;
    if( show.is_set( show_all_   ) )  show |= show._what | show_description | show_orders;
    if( show.is_set( show_orders ) )  show |= show_job_chain_orders;

    Absolute_path job_chain_path = Absolute_path( root_path, show_job_chain_element.getAttribute( "job_chain" ) );

    return _spooler->order_subsystem()->job_chain( job_chain_path )->dom_element( _answer, show );
}

//------------------------------------------------------------Command_processor::execute_show_order

xml::Element_ptr Command_processor::execute_show_order( const xml::Element_ptr& show_order_element, const Show_what& show_ )
{
    xml::Element_ptr result;

    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    Show_what show = show_;
    if( show.is_set( show_all_ ) )  show = Show_what( show_standard );

    Absolute_path job_chain_path = Absolute_path( root_path, show_order_element.getAttribute( "job_chain" ) );
    Order::Id     id             = show_order_element.getAttribute( "order"     );
    string        history_id     = show_order_element.getAttribute( "history_id" );
    string        id_string      = string_from_variant( id );

    if( history_id == "" )
    {
        Job_chain* job_chain = _spooler->order_subsystem()->job_chain( job_chain_path );
        ptr<Order> order     = job_chain->order_or_null( id );

        if( !order  &&  job_chain->is_distributed() ) 
            order = _spooler->order_subsystem()->try_load_order_from_database( (Transaction*)NULL, job_chain_path, id );

        result = order->dom_element( _answer, show );
    }

    if( !result )
    {
        if( !_spooler->_db->opened() )  goto NO_ORDER;

        Read_transaction ta ( _spooler->_db );
    
        if( history_id == "" )
        {
            Any_file sel = ta.open_result_set(
                           " select max(\"HISTORY_ID\") as history_id_max "
                           "  from " + _spooler->_order_history_tablename +
                           "  where \"SPOOLER_ID\"=" + sql::quoted( _spooler->id_for_db() ) + 
                            " and \"JOB_CHAIN\"="   + sql::quoted( job_chain_path ) +
                            " and \"ORDER_ID\"="    + sql::quoted( id_string ),
                            Z_FUNCTION );

            if( sel.eof() )  goto NO_ORDER;

            history_id = sel.get_record().as_string( "history_id_max" );
            if( history_id == "" )  goto NO_ORDER;
        }

        S select_sql;
        select_sql <<  "select \"ORDER_ID\" as \"ID\", \"START_TIME\", \"TITLE\", \"STATE\", \"STATE_TEXT\""
                       "  from " << _spooler->_order_history_tablename <<
                       "  where \"HISTORY_ID\"=" << history_id;
        if( id_string != "" )  select_sql << " and `order_id`=" << sql::quoted( id_string ); 

        Any_file sel = ta.open_result_set( S() << select_sql, Z_FUNCTION );

        if( sel.eof() )  goto NO_ORDER;
        Record record = sel.get_record();

        //order = Z_NEW( Order( _spooler, sel.get_record() );
        ptr<Order> order = new Order( _spooler );
        order->set_id        ( record.as_string( "id"         ) );
        order->set_state     ( record.as_string( "state"      ) );
        order->set_state_text( record.as_string( "state_text" ) );
        order->set_title     ( record.as_string( "title"      ) );
      //order->set_priority  ( record.as_int   ( "priority"   ) );
        sel.close();

        string log;

        if( show.is_set( show_log ) )
        {
            log = file_as_string( S() << "-binary " GZIP_AUTO << _spooler->_db->db_name() << " -table=" + _spooler->_order_history_tablename << " -blob=\"LOG\"" 
                                     " where \"HISTORY_ID\"=" << history_id );
        }

        /* Payload steht nicht in der Historie
        if( show & show_payload )
        {
            string payload = file_as_string( GZIP_AUTO + _spooler->_db->db_name() + " -table=" + _spooler->_order_history_tablename + " -clob=\"PAYLOAD\"" 
                                             " where \"HISTORY_ID\"=" + history_id );
            if( payload != "" )  order->set_payload( payload );
        }
        */
    }

    if( result )  return result;


NO_ORDER:
    z::throw_xc( "SCHEDULER-162", id_string, job_chain_path );
}

//-------------------------------------------------------------Command_processor::execute_add_order

xml::Element_ptr Command_processor::execute_add_order( const xml::Element_ptr& add_order_element )
{
    if( _security_level < Security::seclev_all )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    //string job_name = add_order_element.getAttribute( "job" );

    ptr<Order> order = new Order( _spooler );
    order->set_dom( add_order_element, &_variable_set_map );


    //if( job_name == "" )
    //{
        bool       replace   = add_order_element.bool_getAttribute( "replace", true );
        Job_chain* job_chain = _spooler->order_subsystem()->job_chain( Absolute_path( root_path, add_order_element.getAttribute( "job_chain" ) ) );

        if( replace )  order->place_or_replace_in_job_chain( job_chain );
                 else  order->place_in_job_chain( job_chain );
    //}
    //else 
    //{
    //    order->add_to_job( job_name );
    //}


    xml::Element_ptr result = _answer.createElement( "ok" ); 
    result.appendChild( order->dom_element( _answer, Show_what() ) );
    return result;
}

//-----------------------------------------xml::Element_ptr Command_processor::execute_modify_order

xml::Element_ptr Command_processor::execute_modify_order( const xml::Element_ptr& modify_order_element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    Absolute_path job_chain_path = Absolute_path( root_path, modify_order_element.getAttribute( "job_chain" ) );
    Order::Id     id             = modify_order_element.getAttribute( "order"     );
    string        priority       = modify_order_element.getAttribute( "priority"  );
    string        state          = modify_order_element.getAttribute( "state"     );
    string        at             = modify_order_element.getAttribute( "at"        );

    ptr<Job_chain> job_chain = _spooler->order_subsystem()->job_chain( job_chain_path );
    ptr<Order>     order;

    try
    {
        order = job_chain->is_distributed()? job_chain->order_or_null( id ) 
                                           : job_chain->order( id );

        if( !order  &&  job_chain->is_distributed() ) 
            order = _spooler->order_subsystem()->load_order_from_database( (Transaction*)NULL, job_chain_path, id, Order_subsystem_interface::lo_lock );

        if( xml::Element_ptr run_time_element = modify_order_element.select_node( "run_time" ) )
        {
            order->set_run_time( run_time_element );
        }

        if( xml::Element_ptr params_element = modify_order_element.select_node( "params" ) )
        {
            ptr<Com_variable_set> params = new Com_variable_set;
            params->set_dom( params_element );
            order->params()->merge( params );
        }

        if( xml::Element_ptr xml_payload_element = modify_order_element.select_node( "xml_payload" ) )
        {
            order->set_xml_payload( xml_payload_element.first_child_element() );
        }

        if( priority != "" )  order->set_priority( as_int( priority ) );

        if( state != "" )  
        {
            order->assert_no_task( Z_FUNCTION );
            order->set_state( state );
        }

        if( at != "" )  order->set_at( Time::time_with_now( at ) );

        if( modify_order_element.hasAttribute( "setback" ) )
        {
            if( modify_order_element.bool_getAttribute( "setback" ) )
            {
                throw_xc( "SCHEDULER-351", modify_order_element.getAttribute( "setback" ) );
                //order->setback();
            }
            else
            {
                order->assert_no_task( Z_FUNCTION );
                order->clear_setback( true );        // order->_setback_count belassen
            }
        }

        if( modify_order_element.hasAttribute( "suspended" ) )
        {
            order->set_suspended( modify_order_element.bool_getAttribute( "suspended" ) );
        }

        if( modify_order_element.hasAttribute( "title" ) )
        {
            order->set_title( modify_order_element.getAttribute( "title" ) );
        }

        if( order->finished()  &&  !order->is_on_blacklist() )
        {
            order->remove_from_job_chain();
            order->close();
        }
        else
        {
            order->db_update( Order::update_anyway );
        }

        //Das löscht den Auftrag!  order->close();
    }
    catch( exception& )
    {
        //Das löscht den Auftrag!  if( order )  order->close();
        throw;
    }

    return _answer.createElement( "ok" );
}

//----------------------------------------------------------Command_processor::execute_remove_order

xml::Element_ptr Command_processor::execute_remove_order( const xml::Element_ptr& modify_order_element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    Absolute_path job_chain_path = Absolute_path( root_path, modify_order_element.getAttribute( "job_chain" ) );
    Order::Id     id             = modify_order_element.getAttribute( "order"     );

    ptr<Job_chain> job_chain = _spooler->order_subsystem()->job_chain( job_chain_path );
    ptr<Order>     order     = job_chain->is_distributed()? job_chain->order_or_null( id ) 
                                                          : job_chain->order( id );

    if( order )
    {
        order->remove( File_based::rm_base_file_too );
    }
    else
    {
        assert( job_chain->is_distributed() );

        for( Retry_transaction ta ( _spooler->_db ); ta.enter_loop(); ta++ ) try
        {
            sql::Delete_stmt delete_stmt ( _spooler->database_descriptor(), _spooler->_orders_tablename );

            delete_stmt.add_where( _spooler->order_subsystem()->order_db_where_condition( job_chain_path, id.as_string() ) );
          //delete_stmt.and_where_condition( "occupying_cluster_member_id", sql::null_value );
            
            ta.execute( delete_stmt, Z_FUNCTION );
            if( ta.record_count() == 0 )  z::throw_xc( "SCHEDULER-162", id.as_string() );
            
            //if( ta.record_count() == 0 )
            //{
            //    // Sollte Exception auslösen: nicht da oder belegt
            //    _spooler->order_subsystem()->load_order_from_database( job_chain_path, id );
            //    
            //    // Der Auftrag ist gerade freigegeben oder hinzugefügt worden
            //    delete_stmt.remove_where_condition( "occupying_cluster_member_id" );
            //    ta.execute_single( delete_stmt, Z_FUNCTION ); 
            //}

            ta.commit( Z_FUNCTION );
        }
        catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_orders_tablename, x ), Z_FUNCTION ); }
    }

    return _answer.createElement( "ok" );
}

//------------------------------------------------------Command_processor::execute_remove_job_chain

xml::Element_ptr Command_processor::execute_remove_job_chain( const xml::Element_ptr& modify_order_element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    Absolute_path job_chain_path = Absolute_path( root_path, modify_order_element.getAttribute( "job_chain" ) );

    _spooler->order_subsystem()->job_chain( job_chain_path )->remove( File_based::rm_base_file_too );

    return _answer.createElement( "ok" );
}

//---------------------------------------------Command_processor::execute_register_remote_scheduler

xml::Element_ptr Command_processor::execute_register_remote_scheduler( const xml::Element_ptr& register_remote_scheduler_element )
{
    if( !_communication_operation )  z::throw_xc( "SCHEDULER-222", register_remote_scheduler_element.nodeName() );

    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );

    _spooler->_supervisor->execute_register_remote_scheduler( register_remote_scheduler_element, _communication_operation );

    return _answer.createElement( "ok" );
}

//-------------------------------------------------------Command_processor::execute_service_request

xml::Element_ptr Command_processor::execute_service_request( const xml::Element_ptr& service_request_element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    ptr<Order> order = new Order( _spooler );

    order->set_state( Web_service::forwarding_job_chain_forward_state );
    order->set_payload( Variant( service_request_element.xml() ) );
    order->place_in_job_chain( _spooler->root_folder()->job_chain_folder()->job_chain( Web_service::forwarding_job_chain_name ) );
    
    return _answer.createElement( "ok" );
}

//-------------------------------------------------------Command_processor::execute_service_request

xml::Element_ptr Command_processor::execute_get_events( const xml::Element_ptr& )
{
    ptr<Get_events_command_response> response = Z_NEW( Get_events_command_response( _spooler->_scheduler_event_manager ) );
    _response = response;

    _spooler->_scheduler_event_manager->add_get_events_command_response( response );

    _response->write( "<events>\n" );

    return NULL;    // Antwort wird asynchron übergeben
}

//-----------------------------------------et_events_command_response::~Get_events_command_response

Get_events_command_response::~Get_events_command_response()
{
    _scheduler_event_manager->remove_get_events_command_response( this );
}

//---------------------------------------------------------------Get_events_command_response::close

void Get_events_command_response::close()
{
    if( !_closed )
    {
        _closed = true;

        write( "</events>" );
        //Z_DEBUG_ONLY( int NULL_BYTE_ANHAENGEN );
    }
}

//---------------------------------------------------------Get_events_command_response::write_event

void Get_events_command_response::write_event( const Scheduler_event& event )
{
    write( event.xml() );

    if( _append_0_byte )  write( io::Char_sequence( "\0", 1 ) );       // 0-Byte anhängen
                    else  write( "\n" );
}

//---------------------------------------------------------------Command_processor::execute_command

xml::Element_ptr Command_processor::execute_command( const xml::Element_ptr& element )
{
    xml::Element_ptr result;

    if( _log ) 
    {
        Message_string m ( "SCHEDULER-965" );
        //m.set_max_insertion_length( INT_MAX );
        m.insert( 1, element.xml( scheduler_character_encoding ) );
        _log->info( m );
    }

    Show_what show = show_jobs | show_tasks;        // Zur Kompatibilität. Besser: <show_state what="jobs,tasks"/>

    string max_orders = element.getAttribute( "max_orders" );
    if( max_orders != "" )  show._max_orders = as_int( max_orders );

    string max_order_history = element.getAttribute( "max_order_history" );
    if( max_order_history != "" )  show._max_order_history = as_int( max_order_history );

    string max_task_history = element.getAttribute( "max_task_history" );
    if( max_task_history != "" )  show._max_task_history = as_int( max_task_history );

    show._folder_path = Absolute_path( root_path, element.getAttribute( "path", show._folder_path ) );

    string what = element.getAttribute( "what" );

    const char* p = what.c_str();  // Bsp: "all"  "orders,description"  "task_queue,orders,description,"
    while( *p )
    {
        while( *p == ' ' )  p++;

        if( string_equals_prefix_then_skip( &p, "none"             ) )  show = show_standard;       // Setzt Flags zurück! (Provisorisch, solange jobs,tasks default ist)
        else
        if( string_equals_prefix_then_skip( &p, "all!"             ) )  show |= show_all;
        else
        if( string_equals_prefix_then_skip( &p, "all"              ) )  show |= show_all_;
        else
        if( string_equals_prefix_then_skip( &p, "task_queue"       ) )  show |= show_task_queue;
        else
        if( string_equals_prefix_then_skip( &p, "orders"           ) )  show |= show_orders;
        else
        if( string_equals_prefix_then_skip( &p, "job_chains"       ) )  show |= show_job_chains;
        else
        if( string_equals_prefix_then_skip( &p, "job_chain_orders" ) )  show |= show_job_chain_orders;
        else
        if( string_equals_prefix_then_skip( &p, "job_orders"       ) )  show |= show_job_orders;
        else
        if( string_equals_prefix_then_skip( &p, "description"      ) )  show |= show_description;
        else
        if( string_equals_prefix_then_skip( &p, "log"              ) )  show |= show_log;
        else
        if( string_equals_prefix_then_skip( &p, "task_history"     ) )  show |= show_task_history;
        else
        if( string_equals_prefix_then_skip( &p, "order_history"    ) )  show._max_order_history = 20;
        else
        if( string_equals_prefix_then_skip( &p, "remote_schedulers") )  show |= show_remote_schedulers;
        else
        if( string_equals_prefix_then_skip( &p, "run_time"         ) )  show |= show_run_time;
        else
        if( string_equals_prefix_then_skip( &p, "job_chain_jobs"   ) )  show |= show_job_chain_jobs;
        else
        if( string_equals_prefix_then_skip( &p, "jobs"             ) )  show |= show_jobs;
        else
        if( string_equals_prefix_then_skip( &p, "tasks"            ) )  show |= show_tasks;
        else
        if( string_equals_prefix_then_skip( &p, "job_commands"     ) )  show |= show_job_commands;
        else
        if( string_equals_prefix_then_skip( &p, "blacklist"        ) )  show |= show_blacklist;
        else
        if( string_equals_prefix_then_skip( &p, "order_source_files") )  show |= show_order_source_files;
        else
        if( string_equals_prefix_then_skip( &p, "payload"          ) )  show |= show_payload;
        else
        if( string_equals_prefix_then_skip( &p, "job_params"       ) )  show |= show_job_params;
        else
        if( string_equals_prefix_then_skip( &p, "cluster"          ) )  show |= show_cluster;
        else
        if( string_equals_prefix_then_skip( &p, "operations"       ) )  show |= show_operations;
        else
        if( string_equals_prefix_then_skip( &p, "folders"          ) )  show |= show_folders;
        else
        if( string_equals_prefix_then_skip( &p, "no_subfolders"    ) )  show |= show_no_subfolders;
        //else
        //if( string_equals_prefix_then_skip( &p, "subfolders"       ) )  show |= show_subfolders;
        else
        if( string_equals_prefix_then_skip( &p, "standard"         ) )  ;
        else
        if( string_equals_prefix_then_skip( &p, "check_folders"    ) )  
        {
#           ifdef Z_UNIX    // Weil wir unter Unix nur periodisch die Verzeichnisse prüfen
                Z_UNIX_ONLY( spooler()->folder_subsystem()->handle_folders( 1 ) );  
#           endif
        }
        else
            z::throw_xc( "SCHEDULER-164", what );

        if( *p != ','  &&  *p != ' '  &&  *p != '\0' )  z::throw_xc( "SCHEDULER-164", what );

        while( *p == ' '  ||  *p == ',' )  p++;

        if( *p == 0 )  break;
    }

    show._max_order_history = element.int_getAttribute( "max_order_history", show._max_order_history );



    string element_name = element.nodeName();
    
    if( string_begins_with( element_name, "job_chain_node." ) )
    {
        result = _spooler->order_subsystem()->job_chain( Absolute_path( root_path, element.getAttribute( "job_chain" ) ) ) ->
                    node_from_state( element.getAttribute( "state" ) )->execute_xml( this, element, show );
    }
    else
    if( string_begins_with( element_name, "job_chain." ) )
    {
        result = _spooler->order_subsystem()->job_chain( Absolute_path( root_path, element.getAttribute( "job_chain" ) ) )->execute_xml( this, element, show );
    }
    else
    if( element_name == "lock"  ||  string_begins_with( element_name, "lock." ) )
    {
        result = _spooler->lock_subsystem()->execute_xml( this, element, show );
    }
    else
    if( element_name == "process_class"  ||  string_begins_with( element_name, "process_class." ) )
    {
        result = _spooler->process_class_subsystem()->execute_xml( this, element, show );
    }
    else
    if( element.nodeName_is( "show_state"       ) 
     || element.nodeName_is( "s"                ) )  result = execute_show_state( element, show );
    else
    if( element.nodeName_is( "show_calendar"    ) )  result = execute_show_calendar( element, show );
    else
    if( element.nodeName_is( "show_history"     ) )  result = execute_show_history( element, show );
    else
    if( element.nodeName_is( "modify_spooler"   ) )  result = execute_modify_spooler( element );
    else
    if( element.nodeName_is( "terminate"        ) )  result = execute_terminate( element );
    else
    if( element.nodeName_is( "modify_job"       ) )  result = execute_modify_job( element );
    else
    if( element.nodeName_is( "show_job"         ) )  result = execute_show_job( element, show );
    else
    if( element.nodeName_is( "show_jobs"        ) )  result = execute_show_jobs( show );
    else
    if( element.nodeName_is( "start_job"        ) )  result = execute_start_job( element );
    else
    if( element.nodeName_is( "remote_scheduler.start_remote_task" ) )  result = execute_remote_scheduler_start_remote_task( element );
    else
    if( element.nodeName_is( "remote_scheduler.remote_task.close" ) )  result = execute_remote_scheduler_remote_task_close( element );
    else
    if( element.nodeName_is( "show_cluster"     ) )  result = execute_show_cluster( element, show );
    else
    if( element.nodeName_is( "show_task"        ) )  result = execute_show_task( element, show );
    else
    if( element.nodeName_is( "kill_task"        ) )  result = execute_kill_task( element );
    else
    if( element.nodeName_is( "add_jobs"         ) )  result = execute_add_jobs( element );
    else
    if( element.nodeName_is( "job"              ) )  result = execute_job( element );
    else
    if( element.nodeName_is( "job_chain"        ) )  result = execute_job_chain( element );
    else
    if( element.nodeName_is( "config"           ) )  result = execute_config( element );
    else
    if( element.nodeName_is( "show_job_chains"  ) )  result = execute_show_job_chains( element, show );
    else
    if( element.nodeName_is( "show_job_chain"   ) )  result = execute_show_job_chain( element, show );
    else
    if( element.nodeName_is( "show_order"       ) )  result = execute_show_order( element, show );
    else
    if( element.nodeName_is( "add_order"        ) ||
        element.nodeName_is( "order"            ) )  result = execute_add_order( element );
    else
    if( element.nodeName_is( "modify_order"     ) )  result = execute_modify_order( element );
  //else
  //if( element.nodeName_is( "show_order_history" ) )  result = execute_show_order_history( element, show );
    else
    if( element.nodeName_is( "register_remote_scheduler" ) )  result = execute_register_remote_scheduler( element );
    else
    if( element.nodeName_is( "remove_order"     ) )  result = execute_remove_order( element );
    else
    if( element.nodeName_is( "remove_job_chain" ) )  result = execute_remove_job_chain( element );
    else
    if( element.nodeName_is( "service_request"  ) )  result = execute_service_request( element );
    else
    if( _spooler->_zschimmer_mode && element.nodeName_is( "get_events"  ) )  result = execute_get_events( element );
    else
    {
        z::throw_xc( "SCHEDULER-105", element.nodeName() );
    }

    if( result )  _answer.documentElement().firstChild().appendChild( result );

    return result;
}

//------------------------------------------------------------------------------------xml_as_string

string xml_as_string( const xml::Document_ptr& document, bool indent )
{
    string result;

    try 
    {
        result = document.xml( "ASCII", indent );
        if( indent )  result = replace_regex( result, "\n", "\r\n" );      // Für Windows-telnet
    }
    catch( const exception&  ) { return "<?xml version=\"1.0\"?><ERROR/>"; }
    catch( const _com_error& ) { return "<?xml version=\"1.0\"?><ERROR/>"; }

    return result;
}

//-------------------------------------------------------------------Command_processor::execute_http
// Könnte als Unterklasse von Web_service_operation implementiert werden

void Command_processor::execute_http( http::Operation* http_operation, Http_file_directory* http_file_directory )
{
    http::Request*  http_request            = http_operation->request();
    http::Response* http_response           = http_operation->response();
    string          path                    = http_request->_path;
    string          response_body;
    string          response_content_type;
    string const    show_log_request        = "/show_log?";

    try
    {
        if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

        if( path.find( ".." ) != string::npos )  z::throw_xc( "SCHEDULER-214", path );
        if( path.find( ":" )  != string::npos )  z::throw_xc( "SCHEDULER-214", path );

        if( http_request->_http_cmd == "GET" )
        {
            if( string_begins_with( path, "/<" ) )   // Direktes XML-Kommando, z.B. <show_state/>, <show_state> oder nur <show_state
            {
                string xml = path.substr( 1 );
                if( !string_ends_with( path, "/>" ) )
                {
                    if( string_ends_with( path, ">" ) )  *xml.rbegin() = '/',  xml += ">";
                                                   else  xml += "/>";
                }
                
                http_response->set_header( "Cache-Control", "no-cache" );
                //if( _log )  _log->info( message_string( "SCHEDULER-932", _request ) );

                response_body = execute( xml, true );

                response_content_type = "text/xml";
            }
            else
            if( string_ends_with( path, "?" ) )
            {
                http_response->set_header( "Cache-Control", "no-cache" );

                if( string_ends_with( path, show_log_request ) )
                {
                    ptr<Prefix_log> log;

                    if( http_request->has_parameter( "job"   ) )
                    {
                        log = _spooler->job_subsystem()->job( Absolute_path( root_path, http_request->parameter( "job" ) ) )->_log;
                    }
                    else
                    if( http_request->has_parameter( "task"  ) )
                    {
                        int       task_id = as_int( http_request->parameter( "task" ) );
                        ptr<Task> task    = _spooler->get_task_or_null( task_id );

                        if( task )
                            log = task->log();
                        else
                        {
                            xml::Element_ptr task_element = _spooler->_db->read_task( _answer, task_id, show_log );
                            S title;  title << "Task " << task_id;

                            DOM_FOR_EACH_ELEMENT( task_element, e )
                            {
                                if( e.nodeName_is( "log" ) )
                                {
                                    //TODO Log wird im Speicher gehalten! Besser: In Datei schreiben, vielleicht sogar Task und Log anlegen
                                    http_response->set_chunk_reader( Z_NEW( http::Html_chunk_reader( Z_NEW( http::String_chunk_reader( e.nodeValue(), "text/plain; charset=" + scheduler_character_encoding ) ), title ) ) );
                                    return;
                                }
                            }

                            http_response->set_chunk_reader( Z_NEW( http::Html_chunk_reader( Z_NEW( http::String_chunk_reader( "Das Protokoll ist nicht lesbar." ) ), title ) ) );
                            return;
                        }
                    }
                    else
                    if( http_request->has_parameter( "order" ) )
                    {
                        Absolute_path job_chain_path = Absolute_path( root_path, http_request->parameter( "job_chain" ) );
                        string        order_id       = http_request->parameter( "order" );
                        string        history_id     = http_request->parameter( "history_id" );
                        

                        if( history_id == "" )
                        {
                            Job_chain* job_chain = _spooler->order_subsystem()->job_chain( job_chain_path );
                            ptr<Order> order     = job_chain->order_or_null( order_id );

                            if( !order  &&  job_chain->is_distributed() ) 
                            {
                                order = _spooler->order_subsystem()->try_load_order_from_database( (Transaction*)NULL, job_chain_path, order_id );
                                //TODO Log wird im Speicher gehalten! Besser: In Datei schreiben
                                http_response->set_chunk_reader( Z_NEW( http::Html_chunk_reader( Z_NEW( http::String_chunk_reader( order->log()->as_string(), "text/plain; charset=" + scheduler_character_encoding ) ), order->log()->title() ) ) );
                                return;
                            }

                            if( order )
                            {
                                log = order->_log;
                            }
                        }

                        if( !log )
                        {
                            Read_transaction ta ( _spooler->_db );

                            if( history_id == "" )
                            {
                                S select_sql;
                                select_sql << "select max( `history_id` ) as history_id_max "
                                               "  from " + _spooler->_order_history_tablename +
                                               "  where `spooler_id`=" << sql::quoted( _spooler->id_for_db() ) + 
                                                 " and `job_chain`="   << sql::quoted( job_chain_path.without_slash() ) +
                                                 " and `order_id`="    << sql::quoted( order_id );
                                if( order_id != "" )  select_sql << " and `order_id`=" << sql::quoted( order_id );

                                Any_file sel = ta.open_result_set( select_sql, Z_FUNCTION );

                                if( !sel.eof() )
                                {
                                    history_id = sel.get_record().as_string( "history_id_max" );
                                }
                            }

                            if( history_id != "" )
                            {
                                string log_text = file_as_string( "-binary " GZIP_AUTO + _spooler->_db->db_name() + " -table=" + _spooler->_order_history_tablename + " -blob=\"LOG\"" 
                                                                  " where `history_id`=" + history_id );
                                string title = "Auftrag " + order_id;
                                //TODO Log wird im Speicher gehalten! Besser: In Datei schreiben, vielleicht sogar Order und Log anlegen
                                http_response->set_chunk_reader( Z_NEW( http::Html_chunk_reader( Z_NEW( http::String_chunk_reader( log_text ) ), title ) ) );
                                return;
                            }

                            throw http::Http_exception( http::status_404_bad_request, "No order log" );
                        }
                    }
                    else
                    {
                        log = _spooler->_log;  // Hauptprotokoll
                    }

                    if( log )
                    {
                        http_response->set_chunk_reader( Z_NEW( http::Html_chunk_reader( Z_NEW( http::Log_chunk_reader( log ) ), log->title() ) ) );
                        return;
                    }
                }
                else
                if( string_ends_with( path, "/job_description?" ) )
                {
                    Job* job = _spooler->job_subsystem()->job( Absolute_path( root_path, http_request->parameter( "job" ) ) );
                    
                    if( job->_description == "" )  throw http::Http_exception( http::status_404_bad_request, "Der Job hat keine Beschreibung" );

                    response_content_type = "text/html";

                    response_body = "<html><head><title>Scheduler-Job " + job->name() + "</title>";
                    response_body += "<style type='text/css'> @import 'scheduler.css'; @import 'custom.css';</style>";
                    response_body += "<body id='job_description'>";
                    response_body += job->_description;
                    response_body += "</body></html>";
                }
                else
                if( string_ends_with( path, "/show_config?" ) )
                {
                    if( _spooler->_config_document )  response_body = _spooler->_config_document.xml();

                    response_content_type = "text/xml";
                }
                else
                    throw http::Http_exception( http::status_404_bad_request, "Ungültiger URL-Pfad: " + path );
            }
            else
            {
                if( filename_of_path( path ).find( '.' ) == string::npos )      // Kein Punkt: Es muss ein Verzeichnis sein!
                {
                    if( !string_ends_with( path, "/" )  &&  isalnum( (uint)*path.rbegin() ) )  // '?' am Ende führt zum erneuten GET mit demselben Pfad
                    {
                        // (Man könnte hier noch prüfen, ob's wirklich ein Verzeichnis ist.)
                        // Der Browser soll dem Verzeichnisnamen einen Schräger anhängen und das als Basisadresse für weitere Anfragen verwenden.
                        // http://localhost:6310/jz ==> http://localhost:6310/jz/, http://localhost:6310/jz/details.html
                        // Ohne diesen Mechanismus würde http://localhost:6310/details.html, also das Oberverzeichnis gelesen

                        path += "/";
                        http_response->set_status( http::status_301_moved_permanently );
                        http_response->set_header( "Location", "http://" + http_request->header( "host" ) + path );
                        return;
                    }

                    path += default_filename;
                }


                string filename;

                if( http_file_directory )
                {
                    filename = http_file_directory->file_path_from_url_path( path );
                }
                else
                {
                    if( _spooler->_http_server->directory().empty() )  z::throw_xc( "SCHEDULER-212" );
                    filename = File_path( _spooler->_http_server->directory(), path );
                }
/*
                struct stat st;
                memset( &st, 0, sizeof st );
                int err = stat( filename.c_str(), &st );
                if( !err  &&  stat.st_mode & S_IFDIR )
*/

                string extension = extension_of_path( filename );
             
                if( extension == "html"  
                 || extension == "htm"  )  response_content_type = "text/html";
                else
                if( extension == "xml"  )  response_content_type = "text/xml";
                else
                if( extension == "xsl"  )  response_content_type = "text/xml";  // wie xslt?      "text/xsl";
                else
                if( extension == "xslt" )  response_content_type = "text/xml";  // Firefox und Netscape verlangen text/xml!      "text/xslt";
                else
                if( extension == "xsd"  )  response_content_type = "text/xml";
                else
                if( extension == "js"   )  response_content_type = "text/javascript";
                else
                if( extension == "css"  )  response_content_type = "text/css";
                else
                if( extension == "ico"  )  response_content_type = "image/x-ico";
              //else
              //if( extension == "jar"  )  response_content_type = "application/x-java-archive";

                try
                {
                    File file ( filename, "r" );
                    //struct stat s;                                                   
                    //if( fstat( file, &s ) == 0 )  http_response->set_header( "Last-Modified", http::date_string( s.st_mtime ) );
                    response_body = string_from_fileno( file );
                }
                catch( exception& )
                {                                                        
                    string fn = path.substr( 1 );    // '/' abschneiden
                    if( string_begins_with( fn, "jz/" ) )  fn = "z/" + fn.substr( 3 );
                    const Embedded_file* f = embedded_files_z.get_embedded_file_or_null( "html/" + fn );
                    if( !f ) 
                    {
                        /*
                        if( fn == default_filename )
                        {
                            fn = "jz/" + fn;
                            for( f = inline_files; f->filename &&  f->filename != fn; f++ );
                            if( f->filename ) 
                            {
                                ptr<http::Response> response = Z_NEW( http::Response( http_request, NULL, "" ) );

                                path = "/" + fn;
                                response->set_status( 301, "" );
                                response->set_header_field( "Location", "http://" + http_request->header_field( "host" ) + path );
                                return +response;
                            }
                        }
                        */

                        if( fn == xml_schema_path )  f = embedded_files.get_embedded_file_or_null( fn );
                        if( !f )  throw;
                    }

                    //http_response->set_header( "Last-Modified", http::date_string( f->_last_modified_time ) );
                    response_body = string_gzip_deflate( f->_content, f->_length );
                    //response_body.assign( f->_content, f->_length );
                }
            }
        }
        else
        if( http_request->_http_cmd == "POST" )
        {
            response_body = execute( http_request->_body, true );
            response_content_type = "text/xml";
        }
        else
            throw http::Http_exception( http::status_501_not_implemented );


        if( response_body.empty() )
        {
            response_body = execute( "<show_state what=\"all,orders\"/>", true );
            response_content_type = "text/xml";
        }
    }
    catch( const exception& x )
    {
        _spooler->log()->debug( message_string( "SCHEDULER-311", http_request->_http_cmd + " " + path, x ) );

        throw http::Http_exception( http::status_404_bad_request, x.what() );
    }

    http_response->set_chunk_reader( Z_NEW( http::String_chunk_reader( response_body, response_content_type ) ) );
}

//------------------------------------------------------------------------Command_processor::execute

ptr<Command_response> Command_processor::response_execute( const string& xml_text_par, bool indent )
{
    try 
    {
        _error = NULL;

        string xml_text = xml_text_par;
        if( strchr( xml_text.c_str(), '<' ) == NULL )  xml_text = "<" + xml_text + "/>";

        execute_2( xml_text );

        if( !_answer.documentElement().firstChild().hasChildNodes()  &&  !_response )  z::throw_xc( "SCHEDULER-353" );
    }
    catch( const Xc& x        ) { append_error_to_answer( x );  if( _log ) _log->error( x.what() ); }
    catch( const exception& x ) { append_error_to_answer( x );  if( _log ) _log->error( x.what() ); }

    ptr<Command_response> result = _response;
    if( !result )
    {
        ptr<Synchronous_command_response> r = Z_NEW( Synchronous_command_response( xml_as_string( _answer, indent ) ) );
        result = +r;
    }
    
    return +result;
}

//------------------------------------------------------------------------Command_processor::execute

string Command_processor::execute( const string& xml_text_par, bool indent )
{
    return response_execute( xml_text_par, indent )->complete_text();
}

//------------------------------------------------------------------------Command_processor::execute

xml::Document_ptr Command_processor::execute( const xml::Document_ptr& command_document )
{
    try 
    {
        _error = NULL;
        execute_2( command_document );
    }
    catch( const Xc& x        ) { append_error_to_answer( x );  if( _log ) _log->error( x.what() ); }
    catch( const exception& x ) { append_error_to_answer( x );  if( _log ) _log->error( x.what() ); }

    //if( !_spooler->check_is_active() )  _spooler->abort_immediately( Z_FUNCTION );

    return _answer;
}

//-----------------------------------------------------------Command_processor::execute_config_file

void Command_processor::execute_config_file( const string& filename )
{
    try
    {
        _source_filename = filename;

        string content = string_from_file( filename );

        Z_LOGI2( "scheduler", Z_FUNCTION << "\n" << filename << ":\n" << content << "\n" );

        xml::Document_ptr dom_document = dom_from_xml( content );
        dom_document.select_node_strict( "/spooler/config" );

        _dont_log_command = true;
        execute_2( dom_document );
    }
    catch( zschimmer::Xc& x )
    {
        x.append_text( "in configuration file " + filename );
        throw;
    }
}

//------------------------------------------------------------------Command_processor::dom_from_xml

xml::Document_ptr Command_processor::dom_from_xml( const string& xml_text )
{
    Z_LOGI2( "scheduler.xml", "XML-Dokument wird gelesen ...\n" );

    xml::Document_ptr command_doc;
    command_doc.create();

    int ok = command_doc.try_load_xml( xml_text );
    if( !ok )
    {
        string text = command_doc.error_text();
        _spooler->log()->error( text );       // Log ist möglicherweise noch nicht geöffnet
        throw_xc( "XML-ERROR", text );
    }

    Z_LOG2( "scheduler.xml", "XML-Dokument ist eingelesen\n" );

    return command_doc;
}

//----------------------------------------------------------------------Command_processor::execute_2

void Command_processor::execute_2( const string& xml_text )
{
    try 
    {
        execute_2( dom_from_xml( xml_text ) );
    }
    catch( const _com_error& com_error ) { throw_com_error( com_error, "DOM/XML" ); }
}

//----------------------------------------------------------------------Command_processor::execute_2

void Command_processor::execute_2( const xml::Document_ptr& command_doc )
{
    try 
    {
        if( !_dont_log_command )  Z_LOG2( "scheduler", "Execute " << replace_regex( command_doc.xml( scheduler_character_encoding ), "\\?\\>\n", "?>", 1 ) );  // XML endet mit \n

        if( _spooler->_validate_xml  &&  _validate )  
        {
            //command_doc.validate_against_dtd( _spooler->_dtd );
            _spooler->_schema.validate( command_doc );
        }

        execute_2( command_doc.documentElement() );
    }
    catch( const _com_error& com_error ) { throw_com_error( com_error, "DOM/XML" ); }

    // Eigentlich nur für einige möglicherweise langlaufende <show_xxx>-Kommandos nötig, z.B. <show_state>, <show_history> (mit Datenbank)
    if( !_spooler->check_is_active() )  _spooler->cmd_terminate_after_error( Z_FUNCTION, command_doc.xml( scheduler_character_encoding ) );
}

//---------------------------------------------------------------------Command_processor::execute_2

void Command_processor::execute_2( const xml::Element_ptr& element )
{
    xml::Element_ptr e = element;

    if( e.nodeName_is( "spooler" ) ) 
    {
        xml::Node_ptr n = e.firstChild(); 
        while( n  &&  n.nodeType() != xml::ELEMENT_NODE )  n = n.nextSibling();
        e = n;
    }

    if( e )
    {
        if( e.nodeName_is( "commands" )  ||  e.nodeName_is( "command" )  ||  e.nodeName_is( "cluster_member_command" ) )
        {
            execute_commands( e );
        }
        else
        {
            xml::Element_ptr response_element = execute_command( e );
            
            if( !response_element  &&  !_response )  z::throw_xc( "SCHEDULER-353", e.nodeName() );
        }
        
        if( e != element )  // In einer Verschachtelung von <spooler>?
        {
            xml::Node_ptr n = e.nextSibling(); 
            while( n  &&  n.nodeType() != xml::ELEMENT_NODE )  n = n.nextSibling();
            e = n;
            if( e )  z::throw_xc( "SCHEDULER-319", e.nodeName() ); 
        }
    }
}

//--------------------------------------------------------------Command_processor::execute_commands

void Command_processor::execute_commands( const xml::Element_ptr& commands_element )
{
    DOM_FOR_EACH_ELEMENT( commands_element, node )
    {
        xml::Element_ptr response_element = execute_command( node );
        if( !response_element )  z::throw_xc( "SCHEDULER-353", node.nodeName() );
    }
}

//------------------------------------------------------------------Command_processor::begin_answer

void Command_processor::begin_answer()
{
    string now = Time::now().as_string();

    if( !_answer )
    {
        _answer.create();
        _answer.appendChild( _answer.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"" + scheduler_character_encoding + "\"" ) );
        _answer.appendChild( _answer.createElement( "spooler" ) );

        xml::Element_ptr answer_element = _answer.documentElement().appendChild( _answer.createElement( "answer" ) );
        answer_element.setAttribute( "time", now );
    }
}

//--------------------------------------------------------Command_processor::append_error_to_answer

void Command_processor::append_error_to_answer( const exception& x )
{
    _error = x;
    if( _answer  &&  _answer.documentElement()  &&  _answer.documentElement().firstChild() ) 
        append_error_element( _answer.documentElement().firstChild(), x );
}

//--------------------------------------------------------Command_processor::append_error_to_answer

void Command_processor::append_error_to_answer( const Xc& x )
{
    _error = x;
    if( _answer  &&  _answer.documentElement()  &&  _answer.documentElement().firstChild() ) 
        append_error_element( _answer.documentElement().firstChild(), x );
}

//---------------------------------------------------------------Command_response::Command_response

//Command_response::Command_response()
//{
//}

//--------------------------------------------------------------------------Command_response::close

//void Command_response::close()
//{
//    Xml_response::close();
//}

//--------------------------------------------------------Command_response::begin_standard_response

void Command_response::begin_standard_response()
{
    write( "<spooler><answer time=\"" );
    write( Time::now().as_string() );
    write( "\">" );
}

//----------------------------------------------------------Command_response::end_standard_response

void Command_response::end_standard_response()
{
    write( "</answer></spooler>" );
}

//-----------------------------------File_buffered_command_response::File_buffered_command_response

File_buffered_command_response::File_buffered_command_response()
: 
    _zero_(this+1), 
    _buffer_size(recommended_response_block_size)
{
    _buffer.reserve( _buffer_size );
}

//----------------------------------File_buffered_command_response::~File_buffered_command_response
    
File_buffered_command_response::~File_buffered_command_response()
{
}

//------------------------------------------------------------File_buffered_command_response::close
                                                                                                
void File_buffered_command_response::close()
{
    if( _state == s_ready  &&  _buffer == "" )  _state = s_finished;
    _close = true;
}

//------------------------------------------------------------File_buffered_command_response::flush

void File_buffered_command_response::flush()
{
    if( _state == s_ready  &&  _buffer == "" )  _state = s_finished;
    _close = true;
}

//--------------------------------------------------File_buffered_command_response::async_continue_

bool File_buffered_command_response::async_continue_( Continue_flags )
{
    z::throw_xc( Z_FUNCTION );
    return false;
}

//------------------------------------------------------------File_buffered_command_response::write

void File_buffered_command_response::write( const io::Char_sequence& seq )
{
    if( _close )
    {
        Z_LOG( "*** " << Z_FUNCTION << "  closed: " << seq << "\n" );
        return;
    }


    if( seq.length() == 0 )  return;

    if( _state != s_congested )
    {
        if( _buffer.length() == 0  ||  _buffer.length() + seq.length() <= _buffer_size )
        {
            _buffer.append( seq.ptr(), seq.length() );
            signal_new_data();  // Mit Senden beginnen
        }
        else
        {
            _state = s_congested;
        }
    }

    if( _state == s_congested )
    {
        if( !_congestion_file.opened() )  
        {
            _congestion_file.open_temporary( File::open_unlink );
            _congestion_file.print( _buffer );
            _buffer = "";
        }
        
        if( _last_seek_for_read )
        {
            _congestion_file.seek( _congestion_file_write_position );
            _last_seek_for_read = false;
        }

        _congestion_file.print( seq );
        
        _congestion_file_write_position += seq.length();
    }
}

//---------------------------------------------------------File_buffered_command_response::get_part

string File_buffered_command_response::get_part()
{
    string result;

    switch( _state )
    {
        case s_ready:
        {
            if( _buffer == "" ) 
            {
                // Leeren String zurückgeben bedeutet, dass noch keine neuen Daten da sind
            }
            else
            {
                result = _buffer;
                _buffer = "";
            }

            break;
        }

        case s_congested:
        {
            if( !_last_seek_for_read )
            {
                _congestion_file.seek( _congestion_file_read_position );
                _last_seek_for_read = true;
            }

            result = _congestion_file.read_string( _buffer_size );

            _congestion_file_read_position += result.length();
            
            if( _congestion_file_read_position == _congestion_file_write_position )
            {
                _congestion_file.seek( 0 );
                _congestion_file.truncate( 0 );
                _congestion_file_read_position = 0;
                _congestion_file_write_position = 0;

                _state = s_ready;
            }

            break;
        }

        case s_finished:
            break;

        default:
            z::throw_xc( Z_FUNCTION, _state );
    }

    if( _close  &&  _state == s_ready  &&  _buffer == "" )  _state = s_finished;

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
