// $Id$
/*
    Hier ist implementiert

    Command_processor
*/


#include "spooler.h"
#include "spooler_version.h"
#include "../file/anyfile.h"
#include "../zschimmer/z_sql.h"

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
namespace spooler {

using namespace std;

//--------------------------------------------------------------------------dom_append_text_element

void dom_append_text_element( const xml::Element_ptr& element, const char* element_name, const string& text )
{
    xml::Node_ptr     text_node = element.ownerDocument().createTextNode( text );
    xml::Element_ptr  e         = element.appendChild( element.ownerDocument().createElement( element_name ) );

    e.appendChild( text_node );
}

//------------------------------------------------------------------------------------dom_append_nl

void dom_append_nl( const xml::Element_ptr& )
{
    //indent ersetzt diese Newlines.    element.appendChild( element.ownerDocument().createTextNode( "\n" ) );
}

//-----------------------------------------------------------------------------create_error_element

xml::Element_ptr create_error_element( const xml::Document_ptr& document, const Xc_copy& x )
{
    xml::Element_ptr e = document.createElement( "ERROR" );

    //timeb  tm;     // Ob die Sommerzeitverschiebung bei der Fehlerzeit berücksichtigt wird, hängt von der _aktuellen_ Zeit ab.
    //ftime( &tm );  // Nicht schön, aber es funktioniert, weil der Spooler sowieso nicht während der Zeitumstellung laufen soll.
    //e.setAttribute( "time", Sos_optional_date_time( (time_t)x.time() - timezone - ( tm.dstflag? _dstbias : 0 ) ).as_string() );
    e.setAttribute( "time", Sos_optional_date_time( (time_t)Time::now() ).as_string() );

    if( !empty( x->name() )          )  e.setAttribute( "class" , x->name()          );

    e.setAttribute( "code", x->code() );
    e.setAttribute( "text", remove_password( x->what() ) );
    
    if( !empty( x->_pos.filename() ) )  e.setAttribute( "source", x->_pos.filename() );
    if( x->_pos._line >= 0           )  e.setAttribute( "line"  , as_string( x->_pos._line + 1 ) );
    if( x->_pos._col  >= 0           )  e.setAttribute( "col"   , as_string( x->_pos._col + 1  ) );

    return e;
}

//-----------------------------------------------------------------------------append_error_element

void append_error_element( const xml::Element_ptr& element, const Xc_copy& x )
{
    element.appendChild( create_error_element( element.ownerDocument(), x ) );
}

//-------------------------------------------------------------Command_processor::Command_processor

Command_processor::Command_processor( Spooler* spooler )
: 
    _zero_(this+1),
    _spooler(spooler)
{
    set_host( NULL );
    _spooler->_executing_command = true;
}

//------------------------------------------------------------Command_processor::~Command_processor

Command_processor::~Command_processor()
{
    _spooler->_executing_command = false;
}

//----------------------------------------------------------------Command_processor::execute_config

xml::Element_ptr Command_processor::execute_config( const xml::Element_ptr& config_element, const Time& xml_mod_time )
{
    if( _security_level < Security::seclev_all )  throw_xc( "SCHEDULER-121" );

    if( !config_element.nodeName_is( "config" ) )  throw_xc( "SCHEDULER-113", config_element.nodeName() );

    string spooler_id = config_element.getAttribute( "spooler_id" );
    if( spooler_id.empty()  ||  spooler_id == _spooler->id()  ||  _spooler->_manual )
    {
        if( _load_config_immediately )  _spooler->load_config( config_element, xml_mod_time, _source_filename );
                                  else  _spooler->cmd_load_config( config_element, xml_mod_time, _source_filename );
    }

    return _answer.createElement( "ok" );
}

//-------------------------------------------------------------Command_processor::execute_show_jobs

xml::Element_ptr Command_processor::execute_show_jobs( const Show_what& show )
{
    if( _security_level < Security::seclev_info )  throw_xc( "SCHEDULER-121" );

    return _spooler->jobs_as_xml( _answer, show );
}

//----------------------------------------------------------Command_processor::execute_show_threads
/*
xml::Element_ptr Command_processor::execute_show_threads( const Show_what& show )
{
    if( _security_level < Security::seclev_info )  throw_xc( "SCHEDULER-121" );

    return _spooler->threads_as_xml( _answer, show );
}
*/
//--------------------------------------------------Command_processor::execute_show_process_classes

xml::Element_ptr Command_processor::execute_show_process_classes( const Show_what& show )
{
    if( _security_level < Security::seclev_info )  throw_xc( "SCHEDULER-121" );

    return _spooler->process_classes_as_dom( _answer, show );
}

//------------------------------------------------------------Command_processor::execute_show_state

xml::Element_ptr Command_processor::execute_show_state( const xml::Element_ptr& element, const Show_what& show_ )
{
    if( _security_level < Security::seclev_info )  throw_xc( "SCHEDULER-121" );

    Show_what show = show_;
    if( show & show_all_ )  show |= Show_what_enum( show_task_queue | show_description );

    xml::Element_ptr state_element = _answer.createElement( "state" );
 
    state_element.setAttribute( "time"                 , Sos_optional_date_time::now().as_string() );   // Veraltet (<answer> hat time).
    state_element.setAttribute( "id"                   , _spooler->id() );
    state_element.setAttribute( "spooler_id"           , _spooler->id() );
    state_element.setAttribute( "spooler_running_since", Sos_optional_date_time( (time_t)_spooler->start_time() ).as_string() );
    state_element.setAttribute( "state"                , _spooler->state_name() );
    state_element.setAttribute( "log_file"             , _spooler->_base_log.filename() );
    state_element.setAttribute( "version"              , VER_PRODUCTVERSION_STR );
    state_element.setAttribute( "pid"                  , _spooler->_pid );

    if( _spooler->_db )
    {
        THREAD_LOCK( _spooler->_lock )
        {
            state_element.setAttribute( "db"                   , trim( remove_password( _spooler->_db->db_name() ) ) );

            if( _spooler->_db->is_waiting() )
                state_element.setAttribute( "db_waiting", "yes" );

            if( _spooler->_db->error() != "" )
                state_element.setAttribute( "db_error", trim( _spooler->_db->error() ) );
        }
    }

    double cpu_time = get_cpu_time();
    char buffer [30];
    sprintf( buffer, "%-.3lf", cpu_time ); 
#   ifdef Z_WINDOWS
        state_element.setAttribute( "cpu_time"             , buffer );
#   else
        LOG( "Command_processor::execute_show_state() cpu_time=" << cpu_time << "\n" );
#   endif

    state_element.setAttribute( "loop"                 , _spooler->_loop_counter );
    state_element.setAttribute( "waits"                , _spooler->_wait_counter );

    state_element.appendChild( execute_show_jobs( show ) );
  //state_element.appendChild( execute_show_threads( show ) );
    state_element.appendChild( execute_show_process_classes( show ) );
    state_element.appendChild( execute_show_job_chains( element, show ) );

    {
        xml::Element_ptr subprocesses_element = _answer.createElement( "subprocesses" );
        for( int i = 0; i < NO_OF( _spooler->_pids ); i++ )
        {
            int pid = _spooler->_pids[ i ];
            if( pid )
            {
                xml::Element_ptr subprocess_element = _answer.createElement( "subprocess" );
                subprocess_element.setAttribute( "pid", pid );

                subprocesses_element.appendChild( subprocess_element );
            }
        }
        
        state_element.appendChild( subprocesses_element );
    }

    return state_element;
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
    if( abs(*next) > max_n )  *next = sgn(*next) * max_n,  _spooler->_log.warn( "Max. " + as_string(max_n) + " Historiensätze werden gelesen" );
}

//----------------------------------------------------------Command_processor::execute_show_history

xml::Element_ptr Command_processor::execute_show_history( const xml::Element_ptr& element, const Show_what& show_ )
{
    if( _security_level < Security::seclev_info )  throw_xc( "SCHEDULER-121" );

    Show_what show = show_;
    if( show & show_all_ )  show |= show_log;

    string job_name = element.getAttribute( "job" );

    int id, next;
    get_id_and_next( element, &id, &next );
    
    Sos_ptr<Job> job = _spooler->get_job( job_name );

    return job->read_history( _answer, id, next, show );
}

//----------------------------------------------------Command_processor::execute_show_order_history
/*
xml::Element_ptr Command_processor::execute_show_order_history( const xml::Element_ptr& element, const Show_what& show )
{
    if( _security_level < Security::seclev_info )  throw_xc( "SCHEDULER-121" );

    if( show & show_all_ )  show = Show_what_enum( show | show_log );

    int id, next;
    get_id_and_prev( element, &id, &next );
    
    Sos_ptr<Job_chain> job_chain = _spooler->job_chain( element.getAttribute( "job_chain" ) );

    return job_chain->read_order_history( _answer, id, next, show );
}
*/
//-------------------------------------------------------------Command_processor::abort_immediately
/*
void Command_processor::abort_immediately( int exit_code )
{
#   ifdef Z_WINDOWS

        TerminateProcess( GetCurrentProcess(), exit_code );  // _exit() lässt noch Delphi-Code ausführen.

#    else
        //kill( 0, SIGKILL );   // Das killt auch den neuen Spooler.  signal is sent to every process in the process group of the current process.
        kill( _spooler->_pid, SIGKILL );
        _exit( exit_code );

#   endif
}
*/
//--------------------------------------------------------Command_processor::execute_modify_spooler

xml::Element_ptr Command_processor::execute_modify_spooler( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_all )  throw_xc( "SCHEDULER-121" );

    string cmd = element.getAttribute( "cmd" );
  //if( !cmd.empty() )
    {
        if( cmd == "pause"                 )  _spooler->cmd_pause();
        else
        if( cmd == "continue"              )  _spooler->cmd_continue();
        else
        if( cmd == "stop"                  )  _spooler->cmd_stop();
        else
        if( cmd == "reload"                )  _spooler->cmd_reload();
        else
        if( cmd == "terminate"             )  _spooler->cmd_terminate();
        else
        if( cmd == "terminate_and_restart" )  _spooler->cmd_terminate_and_restart();
        else
        if( cmd == "let_run_terminate_and_restart" )  _spooler->cmd_let_run_terminate_and_restart();
        else
        if( cmd == "abort_immediately"     )  _spooler->abort_immediately();  //abort_immediately();
        else
        if( cmd == "abort_immediately_and_restart" )  _spooler->abort_immediately( true ); //){ try{ spooler_restart( NULL, _spooler->is_service() ); } catch(...) {}; abort_immediately(); }
        else
      //if( cmd == "new_log"               )  _spooler->cmd_new_log();
      //else
            throw_xc( "SCHEDULER-105", cmd );
    }
    
    return _answer.createElement( "ok" );
}

//-------------------------------------------------------------Command_processor::execute_terminate

xml::Element_ptr Command_processor::execute_terminate( const xml::Element_ptr& )
{
    if( _security_level < Security::seclev_all )  throw_xc( "SCHEDULER-121" );

    _spooler->cmd_terminate();
    
    return _answer.createElement( "ok" );
}
//--------------------------------------------------------------Command_processor::execute_show_job

xml::Element_ptr Command_processor::execute_show_job( const xml::Element_ptr& element, const Show_what& show_ )
{
    if( _security_level < Security::seclev_info )  throw_xc( "SCHEDULER-121" );

    Show_what show = show_;
    if( show & show_all_ )  show |= show_description | show_task_queue | show_orders;

    return _spooler->get_job( element.getAttribute( "job" ) ) -> dom( _answer, show );
}

//------------------------------------------------------------Command_processor::execute_modify_job

xml::Element_ptr Command_processor::execute_modify_job( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_all )  throw_xc( "SCHEDULER-121" );

    string job_name = element.getAttribute( "job" );
    string cmd_name = element.getAttribute( "cmd" );

    Job::State_cmd cmd = cmd_name.empty()? Job::sc_none 
                                         : Job::as_state_cmd( cmd_name );

    xml::Element_ptr jobs_element = _answer.createElement( "jobs" );

    Job* job = _spooler->get_job( job_name );

    if( cmd )  job->set_state_cmd( cmd );
    
    return jobs_element;
}

//-------------------------------------------------------------Command_processor::execute_show_task

xml::Element_ptr Command_processor::execute_show_task( const xml::Element_ptr& element, const Show_what& show )
{
    if( _security_level < Security::seclev_info )  throw_xc( "SCHEDULER-121" );

    int task_id = element.int_getAttribute( "id" );

    Sos_ptr<Task> task = _spooler->get_task_or_null( task_id );
    if( task )
    {
        return task->dom( _answer, show );
    }
    else
    {
        return _spooler->_db->read_task( _answer, task_id, show );
    }
}

//-------------------------------------------------------------Command_processor::execute_kill_task

xml::Element_ptr Command_processor::execute_kill_task( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_all )  throw_xc( "SCHEDULER-121" );

    int    id          = element. int_getAttribute( "id" );
    string job_name    = element.     getAttribute( "job" );              // Hilfsweise
    bool   immediately = element.bool_getAttribute( "immediately", false );
    

    _spooler->get_job( job_name )->kill_task( id, immediately );
    
    return _answer.createElement( "ok" );
}

//-------------------------------------------------------------Command_processor::execute_start_job

xml::Element_ptr Command_processor::execute_start_job( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_all )  throw_xc( "SCHEDULER-121" );

    string job_name        = element.getAttribute( "job"   );
    string task_name       = element.getAttribute( "name"  );
    string after_str       = element.getAttribute( "after" );
    string at_str          = element.getAttribute( "at"    );

    Time start_at;

    if( !after_str.empty() )  start_at = Time::now() + Time( as_int( after_str ) );

    if( at_str == ""       )  at_str = "now";
    if( at_str == "period" )  start_at = 0;                                     // start="period" => start_at = 0 (sobald eine Periode es zulässt)
                        else  start_at = (Sos_optional_date_time) at_str;       // 

    ptr<Com_variable_set> pars = new Com_variable_set;

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "params" ) )  { pars->set_dom( e );  break; }
    }

    Sos_ptr<Task> task = _spooler->get_job( job_name )->start( ptr<spooler_com::Ivariable_set>(pars), task_name, start_at, true );

    return _answer.createElement( "ok" );
}

//---------------------------------------------------------Command_processor::execute_signal_object

xml::Element_ptr Command_processor::execute_signal_object( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_signal )  throw_xc( "SCHEDULER-121" );

    string class_name = element.getAttribute( "class" );
    Level  level      = as_int( element.getAttribute( "level" ) );

    xml::Element_ptr jobs_element = _answer.createElement( "tasks" );

    _spooler->signal_object( class_name, level );
    
    return _answer.createElement( "ok" );
}

//--------------------------------------------------------------Command_processor::execute_add_jobs

xml::Element_ptr Command_processor::execute_add_jobs( const xml::Element_ptr& add_jobs_element )
{
    if( _security_level < Security::seclev_all )  throw_xc( "SCHEDULER-121" );

    //ptr<Spooler_thread> thread = _spooler->get_thread( add_jobs_element.getAttribute( "thread" ) );
    _spooler->cmd_add_jobs( add_jobs_element );

    return _answer.createElement( "ok" );
}

//-------------------------------------------------------Command_processor::execute_show_job_chains

xml::Element_ptr Command_processor::execute_show_job_chains( const xml::Element_ptr&, const Show_what& show_ )
{
    if( _security_level < Security::seclev_info )  throw_xc( "SCHEDULER-121" );

    Show_what show = show_;
    if( show & show_all_ )  show |= show | show_description | show_orders;

    return _spooler->xml_from_job_chains( _answer, show );
}

//------------------------------------------------------------Command_processor::execute_show_order

xml::Element_ptr Command_processor::execute_show_order( const xml::Element_ptr& show_order_element, const Show_what& show_ )
{
    if( _security_level < Security::seclev_info )  throw_xc( "SCHEDULER-121" );

    Show_what show = show_;
    if( show == show_all_ )  show = Show_what( show_standard );

    string    job_chain_name = show_order_element.getAttribute( "job_chain" );
    Order::Id id             = show_order_element.getAttribute( "order"     );
    string    id_string      = string_from_variant( id );

    ptr<Job_chain> job_chain = _spooler->job_chain( job_chain_name );
    ptr<Order>     order     = job_chain->order_or_null( id );

    if( order )
    {
        return order->dom( _answer, show );
    }
    else
    {
        if( !_spooler->_db->opened() )  goto NO_ORDER;
    
        string history_id;

        {
            Any_file sel ( "-in " + _spooler->_db->db_name() + 
                           " select max(\"HISTORY_ID\") as history_id_max "
                           " from " + sql::uquoted_name( _spooler->_order_history_tablename ) +
                           " where \"SPOOLER_ID\"=" + sql::quoted( _spooler->id_for_db() ) + 
                            " and \"JOB_CHAIN\"="   + sql::quoted( job_chain_name ) +
                            " and \"ORDER_ID\"="    + sql::quoted( id_string ) );

            if( sel.eof() )  goto NO_ORDER;

            history_id = sel.get_record().as_string( "history_id_max" );
            if( history_id == "" )  goto NO_ORDER;
        }

        {
            Any_file sel ( "-in " + _spooler->_db->db_name() + "-max-length=32K "
                           "select \"ORDER_ID\" as \"ID\", \"START_TIME\", \"TITLE\", \"STATE\", \"STATE_TEXT\""
                           " from " + sql::uquoted_name( _spooler->_order_history_tablename ) +
                           " where \"HISTORY_ID\"=" + history_id );

            Record record = sel.get_record();

            //order = Z_NEW( Order( _spooler, sel.get_record() );
            order = new Order( _spooler );
            order->set_id        ( record.as_string( "id"         ) );
            order->set_state     ( record.as_string( "state"      ) );
            order->set_state_text( record.as_string( "state_text" ) );
            order->set_title     ( record.as_string( "title"      ) );
          //order->set_priority  ( record.as_int   ( "priority"   ) );
        }

        string log = file_as_string( GZIP_AUTO + _spooler->_db->db_name() + " -table=" + sql::uquoted_name( _spooler->_order_history_tablename ) + " -blob=\"LOG\"" 
                                     " where \"HISTORY_ID\"=" + history_id );

        return order->dom( _answer, show, &log );
    }


NO_ORDER:
    throw_xc( "SCHEDULER-162", id_string, job_chain_name );
}

//-------------------------------------------------------------Command_processor::execute_add_order

xml::Element_ptr Command_processor::execute_add_order( const xml::Element_ptr& add_order_element )
{
    if( _security_level < Security::seclev_all )  throw_xc( "SCHEDULER-121" );

    string priority       = add_order_element.getAttribute( "priority"  );
    string id             = add_order_element.getAttribute( "id"        );
    string title          = add_order_element.getAttribute( "title"     );
    string job_name       = add_order_element.getAttribute( "job"       );
    string job_chain_name = add_order_element.getAttribute( "job_chain" );
    string state_name     = add_order_element.getAttribute( "state"     );

    ptr<Order> order = new Order( _spooler );

    if( priority   != "" )  order->set_priority( as_int(priority) );
    if( id         != "" )  order->set_id      ( id.c_str() );
                            order->set_title   ( title );
    if( state_name != "" )  order->set_state   ( state_name.c_str() );


    DOM_FOR_EACH_ELEMENT( add_order_element, e )  
    {
        if( e.nodeName_is( "params" ) )
        { 
            ptr<Com_variable_set> pars = new Com_variable_set;
            pars->set_dom( e );  
            order->set_payload( Variant( (IDispatch*)pars ) );
            break; 
        }
    }


    if( job_name != "" )  order->add_to_job( job_name );
                    else  order->add_to_job_chain( _spooler->job_chain( job_chain_name ) );

    return _answer.createElement( "ok" );
}

//-----------------------------------------xml::Element_ptr Command_processor::execute_modify_order

xml::Element_ptr Command_processor::execute_modify_order( const xml::Element_ptr& modify_order_element )
{
    if( _security_level < Security::seclev_all )  throw_xc( "SCHEDULER-121" );

    string    job_chain_name = modify_order_element.getAttribute( "job_chain" );
    Order::Id id             = modify_order_element.getAttribute( "order"     );
    string    priority       = modify_order_element.getAttribute( "priority"  );

    ptr<Job_chain> job_chain = _spooler->job_chain( job_chain_name );
    ptr<Order>     order     = job_chain->order( id );

    if( priority != "" )  order->set_priority( as_int( priority ) );

    return _answer.createElement( "ok" );
}

//---------------------------------------------------------------Command_processor::execute_command

xml::Element_ptr Command_processor::execute_command( const xml::Element_ptr& element, const Time& xml_mod_time )
{
    Show_what show;

    string max_orders = element.getAttribute( "max_orders" );
    if( max_orders != "" )  show._max_orders = as_int( max_orders );

    string what = element.getAttribute( "what" );

    const char* p = what.c_str();  // Bsp: "all"  "orders,description"  "task_queue,orders,description,"
    while( *p )
    {
        if( string_equals_prefix_then_skip( &p, "all"              ) )  show |= show_all_;
        else
        if( string_equals_prefix_then_skip( &p, "task_queue"       ) )  show |= show_task_queue;
        else
        if( string_equals_prefix_then_skip( &p, "orders"           ) )  show |= show_orders;
        else
        if( string_equals_prefix_then_skip( &p, "job_chain_orders" ) )  show |= show_job_chain_orders;
        else
        if( string_equals_prefix_then_skip( &p, "job_orders"       ) )  show |= show_job_orders;
        else
        if( string_equals_prefix_then_skip( &p, "description"      ) )  show |= show_description;
        else
        if( string_equals_prefix_then_skip( &p, "log"              ) )  show |= show_log;
        else
        if( string_equals_prefix_then_skip( &p, "standard"         ) )  ;
        else
            throw_xc( "SCHEDULER-164", what );

        if( *p == 0 )  break;
        if( *p != ',' )  throw_xc( "SCHEDULER-164", what );
        p++;
    }

    if( element.nodeName_is( "show_state"       ) 
     || element.nodeName_is( "s"                ) )  return execute_show_state( element, show );
    else
    if( element.nodeName_is( "show_history"     ) )  return execute_show_history( element, show );
    else
    if( element.nodeName_is( "modify_spooler"   ) )  return execute_modify_spooler( element );
    else
    if( element.nodeName_is( "terminate"        ) )  return execute_terminate( element );
    else
    if( element.nodeName_is( "modify_job"       ) )  return execute_modify_job( element );
    else
    if( element.nodeName_is( "show_job"         ) )  return execute_show_job( element, show );
    else
    if( element.nodeName_is( "start_job"        ) )  return execute_start_job( element );
    else
    if( element.nodeName_is( "show_task"        ) )  return execute_show_task( element, show );
    else
    if( element.nodeName_is( "kill_task"        ) )  return execute_kill_task( element );
    else
    if( element.nodeName_is( "add_jobs"         ) )  return execute_add_jobs( element );
    else
    if( element.nodeName_is( "signal_object"    ) )  return execute_signal_object( element );
    else
    if( element.nodeName_is( "config"           ) )  return execute_config( element, xml_mod_time );
    else
    if( element.nodeName_is( "show_job_chains"  ) )  return execute_show_job_chains( element, show );
    else
    if( element.nodeName_is( "show_order"       ) )  return execute_show_order( element, show );
    else
    if( element.nodeName_is( "add_order"        ) )  return execute_add_order( element );     // in spooler_order.cxx
    else
    if( element.nodeName_is( "modify_order"     ) )  return execute_modify_order( element );
  //else
  //if( element.nodeName_is( "show_order_history" ) )  return execute_show_order_history( element, show );
    else
    {
        throw_xc( "SCHEDULER-105", element.nodeName() ); return xml::Element_ptr();
    }
}

//------------------------------------------------------------------------------------xml_as_string

string xml_as_string( const xml::Document_ptr& document, bool indent )
{
    try 
    {
        return document.xml( indent );
    }
    catch( const exception&  ) { return "<?xml version=\"1.0\"?><ERROR/>"; }
    catch( const _com_error& ) { return "<?xml version=\"1.0\"?><ERROR/>"; }
}

//-------------------------------------------------------------------Command_processor::execute_http

ptr<Http_response> Command_processor::execute_http( Http_request* http_request )
{
    string        path                    = http_request->_path;
    string        response_body;
    string        response_content_type;
    int           http_status_code        = 0;
    string        error_text;
    string const  show_log_request        = "/show_log?";

    try
    {
        if( _security_level < Security::seclev_info )  throw_xc( "SCHEDULER-121" );

        if( path.find( ".." ) != string::npos )  throw_xc( "SCHEDULER-214" );
        if( path.find( ":" )  != string::npos )  throw_xc( "SCHEDULER-214" );

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

                response_body = execute( xml, Time::now(), true );
                response_content_type = "text/xml";
            }
            else
            if( string_ends_with( path, "?" ) )
            {
                if( string_ends_with( path, show_log_request ) )
                {
                    ptr<Prefix_log> log;

                    if( http_request->has_parameter( "job"   ) )
                    {
                        log = _spooler->get_job( http_request->parameter( "job" ) )->_log;
                    }
                    else
                    if( http_request->has_parameter( "task"  ) )
                    {
                        int           task_id = as_int( http_request->parameter( "task" ) );
                        Sos_ptr<Task> task    = _spooler->get_task_or_null( task_id );

                        if( task )
                            log = task->log();
                        else
                        {
                            xml::Element_ptr task_element = _spooler->_db->read_task( _answer, task_id, show_log );
                            DOM_FOR_EACH_ELEMENT( task_element, e )
                            {
                                if( e.nodeName_is( "log" ) )
                                {
                                    S title;  title << "Task " << task_id;
                                    //TODO Log wird im Speicher gehalten! Besser: In Datei schreiben, vielleicht sogar Task und Log anlegen
                                    ptr<Http_response> response = Z_NEW( Http_response( http_request, Z_NEW( Html_chunk_reader( Z_NEW( String_chunk_reader( e.nodeValue() ) ), title ) ), "text/html" ) );
                                    return +response;
                                }
                            }
                        }
                    }
                    else
                    if( http_request->has_parameter( "order" ) )
                    {
                        string     job_chain_name = http_request->parameter( "job_chain" );
                        string     order_id       = http_request->parameter( "order" );
                        ptr<Order> order          = _spooler->job_chain( job_chain_name )->order_or_null( order_id );

                        if( order )
                        {
                            log = order->_log;
                        }
                        else
                        {
                            Any_file sel ( "-in " + _spooler->_db->db_name() + 
                                           " select max(\"HISTORY_ID\") as history_id_max "
                                           " from " + sql::uquoted_name( _spooler->_order_history_tablename ) +
                                           " where \"SPOOLER_ID\"=" + sql::quoted( _spooler->id_for_db() ) + 
                                             " and \"JOB_CHAIN\"="  + sql::quoted( job_chain_name ) +
                                             " and \"ORDER_ID\"="   + sql::quoted( order_id ) );

                            if( !sel.eof() )
                            {
                                string history_id = sel.get_record().as_string( "history_id_max" );
                                if( history_id != "" )
                                {
                                    string log_text = file_as_string( GZIP_AUTO + _spooler->_db->db_name() + " -table=" + sql::uquoted_name( _spooler->_order_history_tablename ) + " -blob=\"LOG\"" 
                                                                    " where \"HISTORY_ID\"=" + history_id );
                                    string title = "Auftrag " + order_id;
                                    //TODO Log wird im Speicher gehalten! Besser: In Datei schreiben, vielleicht sogar Order und Log anlegen
                                    ptr<Http_response> response = Z_NEW( Http_response( http_request, Z_NEW( Html_chunk_reader( Z_NEW( String_chunk_reader( log_text ) ), title ) ), "text/html" ) );
                                    return +response;
                                }
                            }
                        }
                    }
                    else
                    {
                        log = &_spooler->_log;  // Hauptprotokoll
                    }

                    if( log )
                    {
                        ptr<Http_response> response = Z_NEW( Http_response( http_request, Z_NEW( Html_chunk_reader( Z_NEW( Log_chunk_reader( log ) ), log->title() ) ), "text/html" ) );
                        return +response;
                    }
                }
                else
                if( string_ends_with( path, "/job_description?" ) )
                {
                    Job* job = _spooler->get_job( http_request->parameter( "job" ) );;
                    
                    if( job->_description == "" ) 
                    {
                        http_status_code = 404;
                        error_text = "Der Job hat keine Beschreibung";
                    }
                    else
                    {
                        response_body = "<html><head><title>Scheduler-Job " + job->name() + "</title>";
                        response_body += "<style type='text/css'> @import 'scheduler.css'; @import 'custom.css';</style>";
                        response_body += "<body id='job_description'>";
                        response_body += job->_description;
                        response_body += "</body></html>";

                        response_content_type = "text/html";
                    }
                }
                else
                    throw_xc( "SCHEDULER-216", path );
            }
            else
            {
                if( _spooler->_html_directory.empty() )  throw_xc( "SCHEDULER-212" );
                if( !string_begins_with( path, "/" ) )  path = "/" + path;

                if( filename_of_path( path ).find( '.' ) == string::npos )      // Kein Punkt: Es muss ein Verzeichnis sein!
                {
                    if( !string_ends_with( path, "/" )  &&  isalnum( (uint)*path.rbegin() ) )  // '?' am Ende führt zum erneuten GET mit demselben Pfad
                    {
                        // (Man könnte hier noch prüfen, ob's wirklich ein Verzeichnis ist.)
                        // Der Browser soll dem Verzeichnisnamen einen Schräger anhängen und das als Basisadresse für weitere Anfragen verwenden.
                        // http://localhost:6310/jz ==> http://localhost:6310/jz/, http://localhost:6310/jz/details.html
                        // Ohne diesen Mechanismus würde http://localhost:6310/details.html, also das Oberverzeichnis gelesen

                        ptr<Http_response> response = Z_NEW( Http_response( http_request, Z_NEW( String_chunk_reader( "" ) ), "" ) );
                        response->set_status( 301, "Slash appended" );
                        response->set_header_field( "Location", "http://" + http_request->header_field( "host" ) + path + "/" );
                        return +response;
                    }

                    path += "index.html";
                }

                string extension = extension_of_path( path );
             
                if( extension == "html"  
                 || extension == "htm"  )  response_content_type = "text/html";
                else
                if( extension == "xml"  )  response_content_type = "text/xml";
                else
                if( extension == "xsl"  )  response_content_type = "text/xml";  // wie xslt?      "text/xsl";
                else
                if( extension == "xslt" )  response_content_type = "text/xml";  // Firefox und Netscape verlangen text/xml!      "text/xslt";
                else
                if( extension == "js"   )  response_content_type = "text/javascript";
                else
                if( extension == "css"  )  response_content_type = "text/css";


                string filename = path;

                if( string_begins_with( filename, "/doc/" )
                 && !file_exists( _spooler->_html_directory + "/doc" ) )
               //&&  file_exists( _spooler->_html_directory + "/../doc" ) )  überflüssig
                {
                    filename = "/.." + filename;
                }

                filename = _spooler->_html_directory + filename;
/*
                struct stat st;
                memset( &st, 0, sizeof st );
                int err = stat( filename.c_str(), &st );
                if( !err  &&  stat.st_mode & S_IFDIR )
*/
                response_body = zschimmer::string_from_file( filename );
            }
        }
        else
        if( http_request->_http_cmd == "POST" )
        {
            response_body = execute( http_request->_body, Time::now(), true );
            response_content_type = "text/xml";
        }


        if( response_body.empty() )
        {
            response_body = execute( "<show_state what=\"all,orders\"/>", Time::now(), true );
            response_content_type = "text/xml";
        }
    }
    catch( const exception& x )
    {
        _spooler->log().debug( "Fehler beim HTTP-Aufruf " + http_request->_http_cmd + " " + path + ": " + x.what() );

        http_status_code = 404;
        error_text = x.what();
/*
        response_body = "<html><head><title>Scheduler</title></head><body>";
        
        for( int i = 0; i < error_text.length(); i++ )
        {
            switch( char c = error_text[ i ] )
            {
                case '<' : response_body += "&lt;";   break;
                case '&' : response_body += "&amp;";  break;
                case '\r': break;
                case '\n': response_body += "<br/>";  break;
                default  : response_body += c;
            }
        }
        
        response_body += "<br/><p>Siehe auch das Hauptprotokoll des Schedulers</p></body></html>";
*/
    }

    ptr<Http_response> response = Z_NEW( Http_response( http_request, Z_NEW( String_chunk_reader( response_body ) ), response_content_type ) );
    if( http_status_code )  response->set_status( http_status_code, error_text );
    //if( path != http_request->_path )  response->set_header_field( "Content-Location", "http://" + http_request->header_field( "host" ) + path );
    return +response;
/*
    time_t      t;
    char        time_text[26];

    ::time( &t );
    memset( time_text, 0, sizeof time_text );

#   ifdef Z_WINDOWS
        strcpy( time_text, asctime( gmtime( &t ) ) );
#    else
        struct tm  tm;
        asctime_r( gmtime_r( &t, &tm ), time_text );
#   endif
    
    time_text[24] = '\0';

    string response = "HTTP/1.1 200 OK\r\n"
                      "Content-Type: " + response_content_type + "\r\n"
                      "Transfer-Encoding: chunked\r\n"
                      "Date: " + string(time_text) + " GMT\r\n"
                      "Server: Scheduler " + string(VER_PRODUCTVERSION_STR) + "\r\n"
                      "Cache-Control: no-cache\r\n"
                      "\r\n";

    response += as_hex_string( (int)response_body.length() ) + "\r\n";
    return response + response_body + "\r\n0\r\n\r\n";
*/
}

//----------------------------------------------------------------------Command_processor::set_host

void Command_processor::set_host( Host* host )
{ 
    _host = host; 

    _security_level = _host? _spooler->security_level( *_host ) 
                            : Security::seclev_all;
}

//------------------------------------------------------------------------Command_processor::execute

string Command_processor::execute( const string& xml_text_par, const Time& xml_mod_time, bool indent )
{
    try 
    {
        _error = NULL;

        string xml_text = xml_text_par;
        if( strchr( xml_text.c_str(), '<' ) == NULL )  xml_text = "<" + xml_text + "/>";

        execute_2( xml_text, xml_mod_time );
    }
    catch( const Xc& x )
    {
        _error = x;
        append_error_element( _answer.documentElement().firstChild(), x );
    }
    catch( const exception& x )
    {
        _error = x;
        append_error_element( _answer.documentElement().firstChild(), x );
    }

  //return _answer.xml;  //Bei save wird die encoding belassen. Eigenschaft xml verwendet stets unicode, was wir nicht wollen.
    
    return xml_as_string( _answer, indent );
}

//------------------------------------------------------------------Command_processor::execute_file

void Command_processor::execute_file( const string& filename )
{
    _source_filename = filename;

    execute_2( string_from_file( filename ), modification_time_of_file( filename ) );
}

//----------------------------------------------------------------------Command_processor::execute_2

void Command_processor::execute_2( const string& xml_text, const Time& xml_mod_time )
{
    try 
    {
        Z_LOGI2( "scheduler.xml", "XML-Dokument wird gelesen ...\n" );

        //_answer = msxml::Document_ptr( __uuidof(msxml::DOMDocument30), NULL );
        _answer.create();
        _answer.appendChild( _answer.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"iso-8859-1\"" ) );
        _answer.appendChild( _answer.createElement( "spooler" ) );

        xml::Element_ptr answer_element = _answer.documentElement().appendChild( _answer.createElement( "answer" ) );
        answer_element.setAttribute( "time", Time::now().as_string() );

        xml::Document_ptr command_doc;
        command_doc.create();

        int ok = command_doc.try_load_xml( xml_text );
        if( !ok )
        {
            string text = command_doc.error_text();
            _spooler->_log.error( text );       // Log ist möglicherweise noch nicht geöffnet
            throw_xc( "XML-ERROR", text );
        }

        if( !_spooler->_dtd )  _spooler->_dtd.read( dtd_string );
        
        command_doc.validate_against_dtd( _spooler->_dtd );

/*
        xml::DocumentType_ptr doctype = command_doc->doctype;
        if( doctype )  command_doc->removeChild( doctype );

        doctype = command_doc->createDoc
*/
        xml::Element_ptr e = command_doc.documentElement();

        if( e.nodeName_is( "spooler" ) ) 
        {
            xml::Node_ptr n = e.firstChild(); 
            while( n  &&  n.nodeType() != xml::ELEMENT_NODE )  n = n.nextSibling();
            e = n;
        }

        if( e )
        {
            if( e.nodeName_is( "command" ) )
            {
                DOM_FOR_EACH_ELEMENT( e, node )
                //xml::NodeList_ptr node_list = e.childNodes();
                //for( int i = 0; i < node_list.length(); i++ )
                {
                    //xml::Node_ptr node = node_list.item(i);

                    answer_element.appendChild( execute_command( node, xml_mod_time ) );
                }
            }
            else
            {
                answer_element.appendChild( execute_command( e, xml_mod_time ) );
            }
        }

        Z_LOG2( "scheduler.xml", "XML-Dokument ist eingelesen\n" );

    }
    catch( const _com_error& com_error ) { throw_com_error( com_error, "DOM/XML" ); }
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
