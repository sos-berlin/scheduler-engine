// $Id$
// §851: Weitere Log-Ausgaben zum Scheduler-Start eingebaut
/*
    Hier sind implementiert

    Job
*/



#include "spooler.h"
#include "../zschimmer/z_sql.h"
#include "../kram/sleep.h"

#ifndef Z_WINDOWS
#   include <signal.h>
#   include <sys/signal.h>
#   include <sys/wait.h>
#endif

#define THREAD_LOCK_DUMMY( x )


namespace sos {
namespace spooler {

using namespace zschimmer::sql;


const int    max_task_time_out             = 365*24*3600;
const double directory_watcher_intervall   = 1.0;              // Nur für Unix (Windows gibt ein asynchrones Signal)

//----------------------------------------------------------------------Job::Delay_after_error::set
/*
void Job::Delay_after_error::set( int error_steps, const Time& delay )
{
    Map::iterator last = _map.rbegin();
    _map[ error_steps ] = delay;
}
*/
//-----------------------------------------------------------------------------------------Job::Job

Job::Job( Spooler* spooler )
: 
    _zero_(this+1),
    _spooler(spooler),
    _module(spooler,_log),
    _task_queue(this),
    _history(this),
    _lock( "Job" )
{
    init_run_time();
    _log            = Z_NEW( Prefix_log( spooler ) );
    _next_time      = latter_day;
    _directory_watcher_next_time = latter_day;
    _priority       = 1;
    _default_params = new Com_variable_set;
    _task_timeout   = latter_day;
    _idle_timeout   = latter_day;
    _max_tasks      = 1;

    _process_environment = new Com_variable_set();

#ifndef Z_WINDOWS
        _process_environment->_ignore_case = false;
#endif
}

//----------------------------------------------------------------------------------------Job::~Job

Job::~Job()
{
    try
    {
        close();
    }
    catch( exception& x ) { _log->warn( x.what() ); }
}

//-------------------------------------------------------------------------------------Job::set_dom

void Job::set_dom( const xml::Element_ptr& element, const Time& xml_mod_time )
{
    THREAD_LOCK_DUMMY( _lock )
    {
        bool order;

        _name       = element.     getAttribute( "name"         , _name       );
        _temporary  = element.bool_getAttribute( "temporary"    , _temporary  );
        _priority   = element. int_getAttribute( "priority"     , _priority   );
        _title      = element.     getAttribute( "title"        , _title      );
        _log_append = element.bool_getAttribute( "log_append"   , _log_append );
        order       = element.bool_getAttribute( "order"        );
        _module._process_class_name 
                    = element.     getAttribute( "process_class", _module._process_class_name );
        _max_tasks  = element.uint_getAttribute( "tasks"        , _max_tasks );
        string t    = element.     getAttribute( "timeout"      );
        if( t != "" )  
        {
            _task_timeout = time::time_from_string( t );
            if( _task_timeout > max_task_time_out )  _task_timeout = max_task_time_out;   // Begrenzen, damit's beim Addieren mit now() keinen Überlauf gibt
        }

        t           = element.     getAttribute( "idle_timeout"    );
        if( t != "" )  
        {
            _idle_timeout = time::time_from_string( t );
            if( _idle_timeout > max_task_time_out )  _idle_timeout = max_task_time_out;   // Begrenzen, damit's beim Addieren mit now() keinen Überlauf gibt
        }

        if( order )
        {
            if( _temporary )  throw_xc( "SCHEDULER-155" );
            if( element.getAttributeNode( "priority" ) )  throw_xc( "SCHEDULER-165" );
            _order_queue = new Order_queue( this, _log );
        }


        string text;

        text = element.getAttribute( "output_level" );
        if( !text.empty() )  _output_level = as_int( text );

        //for( time::Holiday_set::iterator it = _spooler->_run_time->_holidays.begin(); it != _spooler->_run_time->_holidays.end(); it++ )
        //    _run_time->_holidays.insert( *it );
        

        DOM_FOR_EACH_ELEMENT( element, e )
        {
            if( e.nodeName_is( "description" ) )
            {
                try { _description = text_from_xml_with_include( e, xml_mod_time, _spooler->include_path() ); }
                catch( const exception& x  ) { _spooler->_log.error( x.what() );  _description = x.what(); }
                catch( const _com_error& x ) { string d = bstr_as_string(x.Description()); _spooler->_log.error(d);  _description = d; }
            }
            else
            if( e.nodeName_is( "object_set" ) )  _object_set_descr = SOS_NEW( Object_set_descr( e ) );
            else
            if( e.nodeName_is( "params"     ) )  _default_params->set_dom( e );  
            else
            if( e.nodeName_is( "script"     ) )  
            {
                _module.set_dom_without_source( e );
                _module_xml_document  = e.ownerDocument();
                _module_xml_element   = e;
                _module_xml_mod_time  = xml_mod_time;

                _process_filename     = "";
                _process_param        = "";
                _process_log_filename = "";
            }
            else
            if( e.nodeName_is( "process"    ) )
            {
                _module_xml_document  = NULL;
                _module_xml_element   = NULL;

                _process_filename     = e.     getAttribute( "file"         , _process_filename      );
                _process_param        = e.     getAttribute( "param"        , _process_param         );
                _process_log_filename = e.     getAttribute( "log_file"     , _process_log_filename  );
                _process_ignore_error = e.bool_getAttribute( "ignore_error" , _process_ignore_error  );
                _process_ignore_signal= e.bool_getAttribute( "ignore_signal", _process_ignore_signal );

                DOM_FOR_EACH_ELEMENT( e, ee )
                {
                    if( ee.nodeName_is( "environment" ) )
                    {
                        DOM_FOR_EACH_ELEMENT( ee, eee )
                        {
                            if( eee.nodeName_is( "variable" ) ) 
                            {
                                _process_environment->set_var( eee.getAttribute( "name" ), 
                                                               subst_env( eee.getAttribute( "value" ), _process_environment ) );
                            }
                        }
                    }
                }
            }
            else
            if( e.nodeName_is( "start_when_directory_changed" ) )
            {
                start_when_directory_changed( e.getAttribute( "directory" ), e.getAttribute( "regex" ) );
            }
            else
            if( e.nodeName_is( "run_time" ) &&  !_spooler->_manual )  set_run_time( e );
        }

        if( !_run_time->set() )   _run_time->set_default();
        if( _spooler->_manual )  init_run_time(),  _run_time->set_default_days(),  _run_time->set_once();

        if( _object_set_descr )  _object_set_descr->_class = _spooler->get_object_set_class( _object_set_descr->_class_name );
    }
}

//---------------------------------------------------------------------------------------Job::init0
// Bei <add_jobs> von einem anderen Thread gerufen.

void Job::init0()
{
    LOGI( obj_name() << ".init0()\n" );

    if( _init0_called )  return;

    _state = s_none;

    _log->set_prefix( "Job  " + _name );       // Zwei Blanks, damit die Länge mit "Task " übereinstimmt
    _log->set_profile_section( profile_section() );
    _log->set_job( this );
    _log->set_title( obj_name() );

    _com_job  = new Com_job( this );
  //_com_log  = new Com_log( &_log );

    if( _module_xml_element )  read_script();
    if( _module.set() )  _module.init();

    _next_start_time = latter_day;

    set_state( s_pending );
    
    _init0_called = true;
}

//----------------------------------------------------------------------------------------Job::init
// Bei <add_jobs> von einem anderen Thread gerufen.

void Job::init()
{
    LOGI( obj_name() << ".init()\n" );

    if( !_init_called )
    {
        _history.open();

        _module_ptr = _object_set_descr? &_object_set_descr->_class->_module
                                       : &_module;

        if( !_spooler->log_directory().empty()  &&  _spooler->log_directory()[0] != '*' )
        {
            _log->set_append( _log_append );
            _log->set_filename( _spooler->log_directory() + "/job." + jobname_as_filename() + ".log" );      // Jobprotokoll
        }


        if( _spooler->_db->opened() )  load_tasks_from_db();

        _init_called = true;
    }
    
    init2();
}

//---------------------------------------------------------------------------------------Job::init2
// Bei <add_jobs> von einem anderen Thread gerufen.

void Job::init2()
{
    _delay_until   = 0;
    set_next_start_time( Time::now() );
}

//-------------------------------------------------------------------------------Job::init_run_time

void Job::init_run_time()
{
    _run_time = Z_NEW( Run_time( _spooler, Run_time::application_job ) );
}

//--------------------------------------------------------------------------------Job::set_run_time

void Job::set_run_time( const xml::Element_ptr& element )
{
    init_run_time();
    _run_time->set_holidays( _spooler->holidays() ), 
    _run_time->set_dom( element );

    _start_once    = _run_time->once();
    _period._begin = 0;
    _period._end   = 0;
    _next_single_start = latter_day;

    if( _state != s_none )  set_next_start_time( Time::now() );
}

//---------------------------------------------------------------------------------------Job::close

void Job::close()
{
    THREAD_LOCK_DUMMY( _lock )
    {
        clear_when_directory_changed();

        Z_FOR_EACH( Task_list, _running_tasks, t )
        {
            Task* task = *t;
            try
            {
                task->try_kill();
            }
            catch( const exception& x ) { LOG( *task << ".kill() => " << x.what() << "\n" ); }
        }

        Z_FOR_EACH( Task_list, _running_tasks, t )
        {
            Task* task = *t;
            task->close();
        }

        _running_tasks.clear();
        _task_queue.clear();

        Z_FOR_EACH( Module_instance_vector, _module_instances, m )
        {
            if( *m ) 
            {
                (*m)->close();
                *m = NULL;
            }
        }

        _log->close();
        _history.close();

        // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
        if( _com_job  )  _com_job->close(),         _com_job  = NULL;
      //if( _com_log  )  _com_log->close(),         _com_log  = NULL;
    }
}

//-------------------------------------------------------------------------Job::jobname_as_filename

string Job::jobname_as_filename()
{
    string filename = _name;

    for( int i = 0; i < filename.length(); i++ )
    {
        if( strchr(     "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
                    "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"
                    "<>:\"/\\|",
                    filename[i] ) )  filename[i] = '_';
    }

    return filename;
}

//-----------------------------------------------------------------------------Job::profile_section

string Job::profile_section() 
{
    return "Job " + _name;
}

//---------------------------------------------------------------------------Job::set_error_xc_only

void Job::set_error_xc_only( const Xc& x )
{
    THREAD_LOCK_DUMMY( _lock )
    {
        _error = x;
        _repeat = 0;
    }
}

//--------------------------------------------------------------------------------Job::set_error_xc

void Job::set_error_xc( const Xc& x )
{
    string msg; 
  //if( !_in_call.empty() )  msg = "In " + _in_call + "(): ";
    
    _log->error( msg + x.what() );

    set_error_xc_only( x );
}

//-----------------------------------------------------------------------------------Job::set_error

void Job::set_error( const exception& x )
{
    if( dynamic_cast< const zschimmer::Xc* >( &x ) ) 
    {
        set_error_xc( *(zschimmer::Xc*)&x );
    }
    else
    if( dynamic_cast< const Xc* >( &x ) ) 
    {
        set_error_xc( *(Xc*)&x );
    }
    else
    {
        Xc xc ( "SOS-2000", x.what(), exception_name(x).c_str() );
        set_error_xc( xc );
    }
}

//--------------------------------------------------------------------------------------Job::signal

void Job::signal( const string& signal_name )
{ 
    THREAD_LOCK_DUMMY( _lock )  _next_time = 0;

    _spooler->signal( signal_name ); 
}

//---------------------------------------------------------------------------------Job::create_task
// Wird auch vom Kommunikations-Thread für <start_job> gerufen!
// create_task() nicht mit gesperrten _lock rufen, denn get_id() in DB blockieren!

Sos_ptr<Task> Job::create_task( const ptr<spooler_com::Ivariable_set>& params, const string& name, const Time& start_at, int id )
{
    Sos_ptr<Task> task;

    if( !_process_filename.empty() )   task = SOS_NEW( Process_task   ( this ) );
    else
  //if( _object_set_descr          )   task = SOS_NEW( Object_set_task( this ) );
  //else                             
                                       task = SOS_NEW( Job_module_task( this ) );

    task->_id           = id;

    _default_params->Clone( (spooler_com::Ivariable_set**)task->_params.pp() );
    if( params )  task->_params->Merge( params );

    task->_name     = name;
    task->_start_at = start_at; 

    return task;
}

//---------------------------------------------------------------------------------Job::create_task
// Wird auch vom Kommunikations-Thread für <start_job> gerufen!
// create_task() nicht mit gesperrten _lock rufen, denn get_id() in DB blockieren!

Sos_ptr<Task> Job::create_task( const ptr<spooler_com::Ivariable_set>& params, const string& name, const Time& start_at )
{
    return create_task( params, name, start_at, _spooler->_db->get_task_id() );
}

//--------------------------------------------------------------------------Job::load_tasks_from_db

void Job::load_tasks_from_db()
{
    Time now = Time::now();

    Transaction ta ( _spooler->_db );

    Any_file sel ( "-in " + _spooler->_db->db_name() + 
                    "select \"TASK_ID\", \"ENQUEUE_TIME\", \"START_AT_TIME\"" //, length(\"PARAMETERS\") parlen " +
                    "  from " + quoted_string( ucase( _spooler->_tasks_tablename ), '"', '"' ) +
                    "  where \"SPOOLER_ID\"=" + quoted_string( _spooler->id_for_db(), '\'', '\'' ) +
                       " and \"JOB_NAME\"=" + quoted_string( _name, '\'', '\'' ) +
                    "  order by \"TASK_ID\"" );
    
    while( !sel.eof() )
    {
        Record record = sel.get_record();

        int                     task_id    = record.as_int   ( "task_id" );
        Time                    start_at;
        ptr<Com_variable_set>   parameters = new Com_variable_set;

        start_at.set_datetime( record.as_string( "start_at_time" ) );

        //if( !record.null( "parlen" )  &&  record.as_int( "parlen" ) > 0 )
        {
            string parameters_xml = file_as_string( _spooler->_db->db_name() + " -table=" + _spooler->_tasks_tablename + " -clob='parameters'"
                                                                               " where \"TASK_ID\"=" + as_string( task_id ) );
            if( !parameters_xml.empty() )  parameters->set_xml( parameters_xml );
        }

        _log->info( "Zu startende Task aus Datenbank geladen: id=" + as_string(task_id) + " start_at=" + start_at.as_string() );


        Sos_ptr<Task> task = create_task( +parameters, "", start_at, task_id );
        
        task->_is_in_db     = true;
        task->_let_run      = true;
        task->_enqueue_time.set_datetime( record.as_string( "enqueue_time" ) );

        if( !start_at  &&  !_run_time->period_follows( now ) ) 
        {
            try{ throw_xc( "SCHEDULER-143" ); } catch( const exception& x ) { _log->warn( x.what() ); }
        }

        _task_queue.enqueue_task( task );
    }
}

//--------------------------------------------------------------------Job::Task_queue::enqueue_task

void Job::Task_queue::enqueue_task( const Sos_ptr<Task>& task )
{
    if( !task->_enqueue_time )  task->_enqueue_time = Time::now();

    while(1)
    {
        try
        {
            if( !task->_is_in_db  &&  _spooler->_db->opened() )
            {
                Transaction ta ( _spooler->_db );
                //task->_history.enqueue();

                Insert_stmt insert ( &_spooler->_db->_db_descr );
                insert.set_table_name( _spooler->_tasks_tablename );

                insert             [ "TASK_ID"       ] = task->_id;
                insert             [ "JOB_NAME"      ] = task->_job->_name;
                insert             [ "SPOOLER_ID"    ] = _spooler->id_for_db();
                insert.set_datetime( "ENQUEUE_TIME"  ,   task->_enqueue_time.as_string( Time::without_ms ) );

                if( task->_start_at )
                insert.set_datetime( "START_AT_TIME" ,   task->_start_at.as_string( Time::without_ms ) );

                _spooler->_db->execute( insert );

                if( task->has_parameters() )
                {
                    Any_file blob;
                    blob.open( "-out " + _spooler->_db->db_name() + " -table=" + _spooler->_tasks_tablename + " -clob='parameters'"
                            " where \"TASK_ID\"=" + as_string( task->_id ) );
                    blob.put( xml_as_string( task->parameters_as_dom() ) );
                    blob.close();
                }

                ta.commit();

                task->_is_in_db = true;
            }
            break;
        }
        catch( const exception& x )
        {
            _spooler->_db->try_reopen_after_error( x );
        }
    }


    Queue::iterator it = _queue.begin();  // _queue nach _start_at geordnet halten
    while( it != _queue.end()  &&  (*it)->_start_at <= task->_start_at )  it++;
    _queue.insert( it, task );
}

//-------------------------------------------------------------Job::Task_queue::remove_task_from_db

void Job::Task_queue::remove_task_from_db( int task_id )
{
    while(1)
    {
        try
        {
            if( _spooler->_db->opened() )
            {
                Transaction ta ( _spooler->_db );

                _spooler->_db->execute( "DELETE from " + uquoted_name( _spooler->_tasks_tablename ) +
                                        "  where \"TASK_ID\"=" + as_string( task_id ) );
                ta.commit();
            }

            break;
        }
        catch( const exception& x )
        {
            _spooler->_db->try_reopen_after_error( x );
        }
    }
}

//---------------------------------------------------------------------Job::Task_queue::remove_task

bool Job::Task_queue::remove_task( int task_id, Why_remove )
{
    bool result = false;

    for( Queue::iterator it = _queue.begin(); it != _queue.end(); it++ )
    {
        Task* task = *it;
        if( task->_id == task_id )
        {
            bool remove_from_db = task->_is_in_db;
            _queue.erase( it );
            task = NULL;

            if( remove_from_db )  remove_task_from_db( task_id );

            result = true;
            break;
        }
    }

    return result;
}

//-----------------------------------------------------Job::Task_queue::has_task_waiting_for_period

bool Job::Task_queue::has_task_waiting_for_period()
{
    for( Task_queue::iterator it = _queue.begin(); it != _queue.end(); it++ )
    {
        Task* task = *it;
        if( !task->_start_at )  return true;
    }

    return false;
}

//--------------------------------------------------------------Job::Task_queue::next_at_start_time

Time Job::Task_queue::next_at_start_time( Time now )
{
    for( Task_queue::iterator it = _queue.begin(); it != _queue.end(); it++ )
    {
        Task* task = *it;
        if( task->_start_at )  return task->_start_at;
    }

    return latter_day;
}

//-------------------------------------------------------------------------Job::get_task_from_queue

Sos_ptr<Task> Job::get_task_from_queue( Time now )
{
    Sos_ptr<Task> task;

    if( _state == s_read_error )  return NULL;
    if( _state == s_error      )  return NULL;

    THREAD_LOCK_DUMMY( _lock )
    {
        if( _task_queue.empty() )     return NULL;

        {
            bool                 in_period = is_in_period(now);
            Task_queue::iterator it        = _task_queue.begin();
            
            for( ; it != _task_queue.end(); it++ )
            {
                task = *it;
                if(  task->_start_at  &&  task->_start_at <= now )  break;        // Task mit Startzeitpunkt
                if( !task->_start_at  &&  in_period              )  break;        // Task ohne Startzeitpunkt
            }

            if( it == _task_queue.end() )  return NULL;
        }
    }

    return task;
}

//-------------------------------------------------------------------------Job::remove_running_task

void Job::remove_running_task( Task* task )
{
    THREAD_LOCK_DUMMY( _lock )  
    {
        Task_list::iterator t = _running_tasks.begin();
        while( t != _running_tasks.end() )
        {
            if( *t == task )  t = _running_tasks.erase( t );
                        else  t++;
        }

        if( _running_tasks.empty() )
        {
            if( _state != s_stopped )
            {
                if( _state == s_stopping )  
                {
                    set_state( s_stopped );
                }
                else
                {
                    set_state( s_pending );
                }
            }

            set_next_start_time( Time::now(), true );
        }
    }
}

//---------------------------------------------------------------------------------------Job::start
/*
Sos_ptr<Task> Job::start( const ptr<spooler_com::Ivariable_set>& params, const string& task_name, Time start_at, bool log )
{
    Sos_ptr<Task> result;
    THREAD_LOCK_DUMMY( _lock )  result = start_without_lock( params, task_name, start_at, log );
    return result;
}
*/
//---------------------------------------------------------------------------------------Job::start
// start() und create_task() nicht mit gesperrten _lock rufen, denn get_id() in DB blockieren!

Sos_ptr<Task> Job::start( const ptr<spooler_com::Ivariable_set>& params, const string& task_name, Time start_at, bool log )
{
    Time now = Time::now();

    THREAD_LOCK_DUMMY( _lock )
    {
        if( log && _spooler->_debug )  _log->debug( "start(at=" + start_at.as_string() + ( task_name == ""? "" : ",name=\"" + task_name + '"' ) + ")" );

        switch( _state )
        {
            case s_read_error:  throw_xc( "SCHEDULER-132", name(), _error? _error->what() : "" );
            
            case s_error:       throw_xc( "SCHEDULER-204", _name, _error.what() );

            case s_stopped:     set_state( s_pending );
                                break;

            default: ;
        }

        if( !start_at  &&  !_run_time->period_follows( now ) )   throw_xc( "SCHEDULER-143" );
    }

    Sos_ptr<Task> task = create_task( params, task_name, start_at );
    task->_let_run = true;

    _task_queue.enqueue_task( task );
    calculate_next_time( now );

    signal( "start job" );

    return task;
}

//---------------------------------------------------------------------------------Job::read_script

bool Job::read_script()
{
    THREAD_LOCK_DUMMY( _lock )
    {
        try
        {
            _module.set_dom_source_only( _module_xml_element, _module_xml_mod_time, include_path() );
        }
        catch( const exception& x ) 
        { 
            set_error(x);  
        //_close_engine = true;  
            set_state( s_read_error );  
            return false; 
        }
    }

    return true;
}

//----------------------------------------------------------------------------------------Job::stop

void Job::stop( bool end_all_tasks )
{
    THREAD_LOCK_DUMMY( _lock )
    {
        if( end_all_tasks )
        {
            Z_FOR_EACH( Task_list, _running_tasks, t )
            {
                Task* task = *t;
                task->cmd_end();
            }
        }

        set_state( _running_tasks.size() > 0? s_stopping : s_stopped );

        clear_when_directory_changed();
    }
}

//--------------------------------------------------------------------------------------Job::reread

void Job::reread()
{
    _log->info( "Skript wird erneut gelesen (<include> wird erneut ausgeführt)" );
    read_script();
}

//---------------------------------------------------------------------------Job::execute_state_cmd

bool Job::execute_state_cmd()
{
    bool something_done = false;

    //THREAD_LOCK( _lock )   // create_task() nicht mit gesperrten _lock rufen, denn get_id() in DB blockieren.
    {
        if( _state_cmd )
        {
            switch( _state_cmd )
            {
                case sc_stop:       if( _state != s_stopping
                                     && _state != s_stopped  
                                     && _state != s_read_error )  stop( true ),                something_done = true;
                                    break;

                case sc_unstop:     if( _state == s_stopping
                                     || _state == s_stopped
                                     || _state == s_error      )  set_state( s_pending ),      something_done = true,  set_next_start_time( Time::now() );
                                    break;

                case sc_end:        if( _state == s_running 
                                   //|| _state == s_suspended
                                                               )                               something_done = true;
                                    set_state( s_running );
                                    THREAD_LOCK_DUMMY( _lock )  Z_FOR_EACH( Task_list, _running_tasks, t )  (*t)->cmd_end();
                                    break;

                case sc_suspend:    
                {
                    if( _state == s_running )
                    {
                        THREAD_LOCK_DUMMY( _lock )
                        {
                            Z_FOR_EACH( Task_list, _running_tasks, t ) 
                            {
                                Task* task = *t;
                                if( task->_state == Task::s_running 
                                 || task->_state == Task::s_running_delayed
                                 || task->_state == Task::s_running_waiting_for_order )  task->set_state( Task::s_suspended );
                            }

                            //set_state( s_suspended );
                            something_done = true;
                        }
                    }
                    break;
                }

                case sc_continue:   
                {
                    //if( _state == s_suspended )
                    {
                        THREAD_LOCK_DUMMY( _lock )
                        {
                            Z_FOR_EACH( Task_list, _running_tasks, t ) 
                            {
                                Task* task = *t;
                                if( task->_state == Task::s_suspended 
                                 || task->_state == Task::s_running_delayed
                                 || task->_state == Task::s_running_waiting_for_order )  task->set_state( Task::s_running );
                            }
                            set_state( s_running );
                            something_done = true;
                        }
                    }
                    break;
                }

                case sc_wake:
                {
                    //if( _state == s_suspended
                    // || _state == s_running_delayed )  set_state( s_running ), something_done = true;

                    if( _state == s_pending
                     || _state == s_stopped )
                    {
                        set_state( s_pending );

                        Time now = Time::now();
                        Sos_ptr<Task> task = create_task( NULL, "", 0 );      // create_task() nicht mit gesperrten _lock rufen, denn get_id() in DB blockieren.
                        
                        task->_cause = cause_wake;
                        task->_let_run = true;
                        task->attach_to_a_thread();   // Es gibt zZ nur einen Thread
                    }
                    break;
                }

                default: ;
            }

            _state_cmd = sc_none;
        }
    }

    return something_done;
}

//----------------------------------------------------------------Job::start_when_directory_changed

void Job::start_when_directory_changed( const string& directory_name, const string& filename_pattern )
{
    THREAD_LOCK_DUMMY( _lock )
    {
        _log->debug( "start_when_directory_changed \"" + directory_name + "\", \"" + filename_pattern + "\"" );

        for( Directory_watcher_list::iterator it = _directory_watcher_list.begin(); it != _directory_watcher_list.end(); it++ )
        {
            if( (*it)->directory()        == directory_name 
             && (*it)->filename_pattern() == filename_pattern )  
            {
#               ifdef Z_WINDOWS
                    // Windows: Überwachung erneuern
                    // Wenn das Verzeichnis bereits überwacht war, aber inzwischen gelöscht, und das noch nicht bemerkt worden ist
                    // (weil Spooler_thread::wait vor lauter Jobaktivität nicht gerufen wurde), dann ist es besser, die Überwachung 
                    // hier zu erneuern. Besonders, wenn das Verzeichnis wieder angelegt ist.

                    _directory_watcher_list.erase( it );
                    break;
#               else
                    (*it)->renew();
                    return;   // Unix: Alles in Ordnung
#               endif
            }
        }

        ptr<Directory_watcher> dw = Z_NEW( Directory_watcher( _log ) );

        dw->watch_directory( directory_name, filename_pattern );
        dw->set_name( "job(\"" + _name + "\").start_when_directory_changed(\"" + directory_name + "\")" );
        _directory_watcher_list.push_back( dw );
        dw->add_to( &_spooler->_wait_handles );

        _directory_watcher_next_time = 0;
        calculate_next_time();
    }
}

//----------------------------------------------------------------Job::clear_when_directory_changed

void Job::clear_when_directory_changed()
{
    THREAD_LOCK_DUMMY( _lock )
    {
        if( !_directory_watcher_list.empty() )  _log->debug( "clear_when_directory_changed" );

        _directory_watcher_list.clear();

        _directory_watcher_next_time = latter_day;
    }
}

//-------------------------------------------------------------------------------Job::select_period

void Job::select_period( Time now )
{
    if( now >= _period.end() )       // Periode abgelaufen?
    {
        THREAD_LOCK_DUMMY( _lock )  _period = _run_time->next_period(now);  

        if( _period.begin() != latter_day )
        {
            string rep; if( _period._repeat != latter_day )  rep = _period._repeat.as_string();
            _log->debug( "Nächste Periode ist <period begin=\"" + _period.begin().as_string() + "\" end=\"" + _period.end().as_string() + "\" repeat=\"" + rep + "\">" );
        }
        else 
            _log->debug( "Keine weitere Periode" );
    }
}

//--------------------------------------------------------------------------------Job::is_in_period

bool Job::is_in_period( Time now )
{
    return now >= _delay_until  &&  now >= _period.begin()  &&  now < _period.end();
}

//-------------------------------------------------------------------------Job::set_next_start_time

void Job::set_next_start_time( Time now, bool repeat )
{
    THREAD_LOCK_DUMMY( _lock )
    {
        select_period( now );

        Time   next_start_time = latter_day;  //_next_start_time;
        string msg;

        _next_single_start = latter_day;

        if( _delay_until )
        {
            next_start_time = _period.next_try( _delay_until );
            if( _spooler->_debug )  msg = "Wiederholung wegen delay_after_error: " + next_start_time.as_string();
        }
        else
        if( order_controlled() ) 
        {
            next_start_time = latter_day;
        }
        else
        if( _state == s_pending )
        {
            if( !_period.is_in_time( _next_start_time ) )
            {
                if( !_repeat )  _next_single_start = _run_time->next_single_start( now );

                if( _start_once  ||  !repeat && _period._repeat < latter_day )
                {
                //select_period();
                    if( _period.begin() > now )
                    {
                        next_start_time = _period.begin();
                        if( _spooler->_debug )  msg = "Erster Start zu Beginn der Periode " + next_start_time.as_string();
                    }
                    else
                    {
                        next_start_time = now;
                    }
                }
                else
                if( repeat )
                {
                    if( _repeat > 0 )       // spooler_task.repeat
                    {
                        next_start_time = _period.next_try( now + _repeat );
                        if( _spooler->_debug )  msg = "Wiederholung wegen spooler_job.repeat=" + as_string(_repeat) + ": " + next_start_time.as_string();
                        _repeat = 0;
                    }
                    else
                    if( _period.repeat() < latter_day )
                    {
                        next_start_time = now + _period.repeat();

                        if( _spooler->_debug && next_start_time != latter_day )  msg = "Nächste Wiederholung wegen <period repeat=\"" + as_string((double)_period._repeat) + "\">: " + next_start_time.as_string();

                        if( next_start_time >= _period.end() )
                        {
                            Period next_period = _run_time->next_period( _period.end() );
                            if( _period.end() == next_period.begin()  &&  _period.repeat() == next_period.repeat() )
                            {
                                if( _spooler->_debug )  msg += " (in der anschließenden Periode)";
                            }
                            else
                            {
                                next_start_time = latter_day;
                                if( _spooler->_debug )  msg = "Nächste Startzeit wird bestimmt zu Beginn der nächsten Periode " + next_period.begin().as_string();
                            }
                        }
                    }
                }
            }
        }
        else
        {
            next_start_time = latter_day;
        }

        if( _spooler->_debug )
        {
            if( _next_single_start < next_start_time )  msg = "Nächster single_start " + _next_single_start.as_string();
            if( !msg.empty() )  _log->debug( msg );
        }

        _next_start_time = next_start_time;
        calculate_next_time( now );
    }
}

//-------------------------------------------------------------------------Job::calculate_next_time
// Für Spooler_thread

void Job::calculate_next_time( Time now )
{
    if( _state == s_none )  return;


    THREAD_LOCK_DUMMY( _lock )
    {
        Time next_time = latter_day;

        if( !_waiting_for_process )
        {
            if( _state == s_pending  
             || _state == s_running   &&  _running_tasks.size() < _max_tasks )
            {
                bool in_period = is_in_period(now);

                if( _start_once && in_period ) 
                {
                    next_time = now;
                }
                else
                {
                    // Minimum von _start_at für _next_time berücksichtigen
                    Task_queue::iterator it = _task_queue.begin();  
                    while( it != _task_queue.end() )
                    {
                        if( (*it)->_start_at )  break;   // Startzeit angegeben?
                        if( in_period        )  break;   // Ohne Startzeit und Periode ist aktiv?
                        it++;
                    }

                    if( it != _task_queue.end()  &&  next_time > (*it)->_start_at )  next_time = (*it)->_start_at;

                    if( next_time > _next_start_time   )  next_time = _next_start_time;
                    if( next_time > _next_single_start )  next_time = _next_single_start;
                }
            }

            if( _order_queue )
            {
                Time next_order_time = _order_queue->next_time();
                if( next_time > next_order_time )  next_time = next_order_time;
            }


#           ifdef Z_UNIX
                if( next_time > _directory_watcher_next_time )  next_time = _directory_watcher_next_time;
#           endif
        }
         
        if( next_time > _period.end() )  next_time = _period.end();          // Das ist, wenn die Periode weder repeat noch single_start hat, also keinen automatischen Start

        _next_time = next_time;
    }
}

//-------------------------------------------------------------------Job::notify_a_process_is_idle

void Job::notify_a_process_is_idle()
{
    _waiting_for_process_try_again = true;
    signal( "A process is idle" );
}

//--------------------------------------------------------Job::remove_waiting_job_from_process_list

void Job::remove_waiting_job_from_process_list()
{
    if( _waiting_for_process )
    {
        _waiting_for_process = false;
        _module._process_class->remove_waiting_job( this );
    }

    _waiting_for_process_try_again = false;
}

//----------------------------------------------------------------------check_for_changed_directory

void Job::check_for_changed_directory( const Time& now )
{
#   ifdef Z_UNIX
        if( now < _directory_watcher_next_time )  return;
#   endif


    //Z_LOG2( "joacim", "Job::task_to_start(): Verzeichnisüberwachung _directory_watcher_next_time=" << _directory_watcher_next_time << ", now=" << now << "\n" );
    _directory_watcher_next_time = now + directory_watcher_intervall;

    Directory_watcher_list::iterator it = _directory_watcher_list.begin();
    while( it != _directory_watcher_list.end() )
    {
        (*it)->has_changed();                        // has_changed() für Unix (und seit 22.3.04 für Windows, siehe dort).
        if( (*it)->signaled_then_reset() )        
        {
            _directory_changed = true;

            if( !_changed_directories.empty() )  _changed_directories += ";";
            _changed_directories += (*it)->directory();
            
            if( !(*it)->valid() )
            {
                it = _directory_watcher_list.erase( it );  // Folge eines Fehlers, s. Directory_watcher::set_signal
                continue;
            }
        }

        it++;
    }
}

//-------------------------------------------------------------------------------Job::task_to_start

Sos_ptr<Task> Job::task_to_start()
{
    if( _spooler->state() == Spooler::s_stopping
     || _spooler->state() == Spooler::s_stopping_let_run )  return NULL;

    Time            now   = Time::now();
    Start_cause     cause = cause_none;
    Sos_ptr<Task>   task  = NULL;
    ptr<Order>      order;
    string          changed_directories;
    string          log_line;

    task = get_task_from_queue( now );
    if( task )  cause = task->_start_at? cause_queue_at : cause_queue;
        
    if( _state == s_pending  &&  now >= _next_single_start )  
    {
                                           cause = cause_period_single,                         log_line += "Task startet wegen <period single_start=\"...\">\n";
    }
    else
    if( is_in_period(now) )
    {
        if( _state == s_pending )
        {
            if( _start_once )              cause = cause_period_once,                           log_line += "Task startet wegen <run_time once=\"yes\">\n";
            else
            if( now >= _next_start_time )  
                if( _delay_until && now >= _delay_until )
                                           cause = cause_delay_after_error,                     log_line += "Task startet wegen delay_after_error\n";
                                      else cause = cause_period_repeat,                         log_line += "Task startet, weil Job-Startzeit erreicht: " + _next_start_time.as_string();

            if( _directory_changed  )      cause = cause_directory,                             log_line += "Task startet wegen eines Ereignisses für Verzeichnis " + _changed_directories;
        }

        if( !cause  &&  _order_queue )
        {
            order = _order_queue->first_order( now );
            if( order )
            {
                bool there_is_another_task_ready = false;
                FOR_EACH( Task_list, _running_tasks, t )
                    if( (*t)->state() == Task::s_running_waiting_for_order 
                        || (*t)->state() == Task::s_suspended                 )  { there_is_another_task_ready = true; break; }

                if( there_is_another_task_ready )  order = NULL;  // Soll sich doch die bereits laufende Task um den Auftrag kümmern!
            }
        }
    }


    if( cause || order )     // Es soll also eine Task gestartet werden.
    {
        // Ist denn ein Prozess verfügbar?

        if( _module._process_class  &&  !_module._process_class->process_available( this ) )
        {
            if( !_waiting_for_process  ||  _waiting_for_process_try_again )
            {
                if( !_waiting_for_process )
                {
                    _module._process_class->enqueue_waiting_job( this );
                    _waiting_for_process = true;
                }

                _waiting_for_process_try_again = false;
                _spooler->try_to_free_process( this, _module._process_class, now );     // Beendet eine Task in s_running_waiting_for_order
            }

            cause = cause_none;   // Wir können die Task nicht starten, denn kein Prozess ist verfügbar
            order = NULL;
        }
        else
        {
            remove_waiting_job_from_process_list();
        }

        if( order )
        {
            order = order_queue()->get_order_for_processing( now );  // Jetzt aus der Warteschlange nehmen und nicht verlieren!
            if( order )  // Ist der Auftrag noch da? (Muss bei Ein-Thread-Betrieb immer da sein!)
            {
                cause = cause_order;
                log_line += "Task startet wegen Auftrag " + order->obj_name();
            }
        }

        // order jetzt nicht verlieren! Der Auftrag hängt allein in der Variablen order!

        if( cause )
        {
            if( !log_line.empty() )  _log->debug( log_line );

            if( task )
            {
                //remove_from_task_queue( task, log_none );
                _task_queue.remove_task( task->id(), Task_queue::w_task_started );
            }
            else
            {
                task = create_task( NULL, "", 0 );     // create_task() nicht mit gesperrten _lock rufen, denn get_id() in DB blockieren.

                task->set_order( order );
                task->_cause = cause;
                task->_let_run |= ( cause == cause_period_single );
            }

            task->_changed_directories = _changed_directories;
            _changed_directories = "";
            _directory_changed = false;

            if( now >= _next_single_start )  _next_single_start = latter_day;  // Vorsichtshalber, 26.9.03
        }
    }
    else
    {
        // Keine Task zu starten

        if( _waiting_for_process )                 
        {
            bool notify = _waiting_for_process_try_again;                           // Sind wir mit notify_a_process_is_idle() benachrichtigt worden?
            remove_waiting_job_from_process_list();
            //test 18.5.04 calculate_next_time( now );
            if( notify )  _module._process_class->notify_a_process_is_idle();       // Dieser Job braucht den Prozess nicht mehr. Also nächsten Job benachrichtigen!
        }
    }

    if( task )  _start_once = false;

    return cause? task : NULL;
}

//--------------------------------------------------------------------------------Job::do_something

bool Job::do_something()
{
  //Z_DEBUG_ONLY( _log->debug9( "do_something() state=" + state_name() ); )

    bool something_done     = false;       
    Time now                = Time::now();

    check_for_changed_directory( now );         // Hier prüfen, damit Signal zurückgesetzt wird


    if( _state == s_read_error )  return false;
    if( _state == s_error      )  return false;

    try
    {
        try
        {
            Time next_time_at_begin = _next_time;

            if( _state )  
            {
                something_done = execute_state_cmd();

                if( _reread )  _reread = false,  reread(),  something_done = true;

                if( now > _period.end() )
                {
                    select_period();
                    if( !_period.is_in_time( _next_start_time ) )  set_next_start_time( now );
                }

                if( _state == s_pending 
                 || _state == s_running  &&  _running_tasks.size() < _max_tasks )
                {
                    if( !_waiting_for_process  ||  _waiting_for_process_try_again  ||  _module._process_class->process_available( this ) )    // Optimierung
                    {
                        Sos_ptr<Task> task = task_to_start();
                        if( task )
                        {
                            THREAD_LOCK_DUMMY( _lock )
                            {
                                _log->open();           // Jobprotokoll, nur wirksam, wenn set_filename() gerufen, s. Job::init().

                                reset_error();
                                _repeat = 0;
                                _delay_until = 0;

                                _running_tasks.push_back( task );
                                set_state( s_running );

                                _next_start_time = latter_day;
                                calculate_next_time();

                                task->attach_to_a_thread();
                                task->do_something();           // Damit die Task den Prozess startet und die Prozessklasse davon weiß
                            }

                            something_done = true;
                        }
                    }
                }
            }


            if( !something_done  &&  _next_time <= now )    // Obwohl _next_time erreicht, ist nichts getan?
            {
                calculate_next_time();

                if( _next_time <= now )
                {
                    LOG( obj_name() << ".do_something()  Nichts getan. state=" << state_name() << ", _next_time=" << _next_time << ", wird verzögert\n" );
                    _next_time = Time::now() + 1;
                }
                else
                {
                    Z_LOG2( "scheduler.nothing_done", obj_name() << ".do_something()  Nichts getan. state=" << state_name() << ", _next_time war " << next_time_at_begin << "\n" );
                }
            }
        }
        catch( const _com_error& x )  { throw_com_error( x ); }
    }
  //catch( Stop_scheduler_exception& ) { throw; }
    catch( const exception&  x ) { set_error( x );  set_job_error( x.what() );  sos_sleep(5); }     // Bremsen, falls sich der Fehler sofort wiederholt

    return something_done;
}

//-------------------------------------------------------------------------------Job::set_job_error

void Job::set_job_error( const string& what )
{
    set_state( s_error );

    _spooler->send_error_email( what, "Scheduler: Der Job " + _name + " ist nach dem Fehler\n" +
                                      what + "\n"
                                      "in den Fehlerzustand versetzt worden. Keine Task wird mehr gestartet." );
}

//-----------------------------------------------------------------------------------Job::set_state

void Job::set_state( State new_state )
{ 
    THREAD_LOCK_DUMMY( _lock )  
    {
        if( new_state == _state )  return;

        if( new_state == s_pending  &&  !_delay_until )  reset_error();      // Bei delay_after_error Fehler stehen lassen

        _state = new_state;

        if( _state == s_stopped 
         || _state == s_read_error
         || _state == s_error      )  _next_start_time = _next_time = latter_day;

        //if( _spooler->_debug )
        {
            if( new_state == s_stopping
             || new_state == s_stopped
             || new_state == s_read_error
             || new_state == s_error      )  _log->info  ( "state=" + state_name() ); 
                                       else  _log->debug9( "state=" + state_name() );
        }

        if( _waiting_for_process  &&  ( _state != s_pending  ||  _state != s_running ) )
        {
            remove_waiting_job_from_process_list();
        }
    }
}

//-------------------------------------------------------------------------------Job::set_state_cmd
// Anderer Thread (wird auch vom Kommunikations-Thread gerufen)

void Job::set_state_cmd( State_cmd cmd )
{ 
    bool ok = false;

    //THREAD_LOCK_DUMMY( _lock )          start() (create_task(), get_task_id()) nicht mit gesperrtem _lock rufen, weil DB blockieren kann!
    {
        switch( cmd )
        {
            case sc_stop:       ok = true; 
                                _state_cmd = cmd;
                                signal( state_cmd_name(cmd) );
                                break;

            case sc_unstop:     ok = _state == s_stopped;       if( !ok )  return;
                                _state_cmd = cmd;
                                signal( state_cmd_name(cmd) );
                                break;

            case sc_start:      {
                                    start( NULL, "", Time::now(), true );
                                    break;
                                }

            case sc_wake:       _state_cmd = cmd;
                                signal( state_cmd_name(cmd) );
                                break;

            case sc_end:        ok = true; // _state == s_running || _state == s_running_delayed || _state == s_running_waiting_for_order || _state == s_suspended;  if( !ok )  return;
                                _state_cmd = cmd;
                                break;

            case sc_suspend:    ok = true; //_state == s_running || _state == s_running_delayed || _state == s_running_waiting_for_order;   if( !ok )  return;
                                _state_cmd = cmd;
                                break;

            case sc_continue:   ok = true; //_state == s_suspended || _state == s_running_delayed;  if( !ok )  return;
                                _state_cmd = cmd;
                                signal( state_cmd_name(cmd) );
                                break;

            case sc_reread:     _reread = true, ok = true;  
                                signal( state_cmd_name(cmd) );
                                break;

            default:            ok = false;
        }
    }
}

//-----------------------------------------------------------------------------------Job::job_state
// Anderer Thread

string Job::job_state()
{
    string st;

    THREAD_LOCK_DUMMY( _lock )
    {
        st = "state=" + state_name();
    }

    return st;
}

//--------------------------------------------------------------------------------Job::include_path

string Job::include_path() const
{ 
    return _spooler->include_path(); 
}

//----------------------------------------------------------------------------------Job::state_name

string Job::state_name( State state )
{
    switch( state )
    {
        case s_stopping:        return "stopping";
        case s_stopped:         return "stopped";
        case s_read_error:      return "read_error";
        case s_error:           return "error";
        case s_pending:         return "pending";
        case s_running:         return "running";
      //case s_suspended:       return "suspended";
        default:                return as_string( (int)state );
    }
}

//------------------------------------------------------------------------------------Job::as_state

Job::State Job::as_state( const string& name )
{
    State state = (State)( s__max - 1 );

    while( state )
    {
        if( state_name(state) == name )  return state;
        state = (State)( state - 1 );
    }

    if( !name.empty() )  throw_xc( "SCHEDULER-110", name );
    return s_none;
}

//--------------------------------------------------------------------------------Job::as_state_cmd

Job::State_cmd Job::as_state_cmd( const string& name )
{
    State_cmd cmd = (State_cmd)( sc__max - 1 );

    while( cmd )
    {
        if( state_cmd_name(cmd) == name )  return cmd;
        cmd = (State_cmd)( cmd - 1 );
    }

    if( !name.empty() )  throw_xc( "SCHEDULER-106", name );
    return sc_none;
}

//------------------------------------------------------------------------------Job::state_cmd_name

string Job::state_cmd_name( Job::State_cmd cmd )
{
    switch( cmd )
    {
        case Job::sc_stop:     return "stop";
        case Job::sc_unstop:   return "unstop";
        case Job::sc_start:    return "start";
        case Job::sc_wake:     return "wake";
        case Job::sc_end:      return "end";
        case Job::sc_suspend:  return "suspend";
        case Job::sc_continue: return "continue";
        case Job::sc_reread:   return "reread";
        default:               return as_string( (int)cmd );
    }
}

//---------------------------------------------------------------Job::get_delay_order_after_setback

Time Job::get_delay_order_after_setback( int setback_count )
{
    Time delay = 0;

    FOR_EACH( Delay_order_after_setback, _delay_order_after_setback, it )  
    {
        if( setback_count >= it->first )  delay = it->second;
    }

    return delay;
}

//---------------------------------------------------------------------------------Job::dom_element
// Anderer Thread

xml::Element_ptr Job::dom_element( const xml::Document_ptr& document, const Show_what& show, Job_chain* which_job_chain )
{
    xml::Element_ptr job_element = document.createElement( "job" );

    THREAD_LOCK_DUMMY( _lock )
    {
        job_element.setAttribute( "job"       , _name                   );
        job_element.setAttribute( "state"     , state_name()            );

        if( _waiting_for_process )
        job_element.setAttribute( "waiting_for_process", _waiting_for_process? "yes" : "no" );

        if( !_title.empty() )
        job_element.setAttribute( "title"     , _title                  );

        job_element.setAttribute( "all_steps" , _step_count             );

        job_element.setAttribute( "all_tasks" , _tasks_count            );

        if( !_state_text.empty() )
        job_element.setAttribute( "state_text", _state_text             );

        job_element.setAttribute( "log_file"  , _log->filename()         );
        job_element.setAttribute( "order"     , _order_queue? "yes" : "no" );
        job_element.setAttribute( "tasks"     , _max_tasks              );


        if( _description != "" )  job_element.setAttribute( "has_description", "yes" );

        if( _state_cmd         )  job_element.setAttribute( "cmd"    , state_cmd_name()  );

        //if( _next_start_time < latter_day )
        //job_element.setAttribute( "next_start_time", _next_start_time.as_string() );

        if( _state == s_pending )
        {
            // Versuchen, nächste Startzeit herauszubekommen
            Period p             = _period;  
            int    i             = 100;      // Anzahl Perioden, die wir probieren
            Time   next          = _next_start_time;
            Time   time          = Time::now();
            Time   next_at_start = _task_queue.next_at_start_time( time );
            
            if( next == latter_day )
            {
                //p = _run_time->next_period( p.end() );
                next = p.begin();
                if( p.end() != latter_day )  time = p.end();

                while( i-- ) {          
                    if( p.has_start()  ||  _task_queue.has_task_waiting_for_period() )  break;
                    p = _run_time->next_period( time, time::wss_next_period_or_single_start );
                    next = p.begin();
                    if( next == latter_day        )  break;
                    if( next > next_at_start      )  break;
                    if( next > _next_single_start )  break;
                    time = p.end();
                }
                
                if( i < 0 )  next = latter_day;
            }

            if( next > _next_single_start )  next = _next_single_start;
            if( next > next_at_start      )  next = next_at_start;
            if( _order_queue )  next = min( next, _order_queue->next_time() );
            if( next < latter_day )  job_element.setAttribute( "next_start_time", next.as_string() );
        }

        if( show & show_run_time )  job_element.appendChild( _run_time->dom_element( document ) );

        dom_append_nl( job_element );
        xml::Element_ptr tasks_element = document.createElement( "tasks" );
        tasks_element.setAttribute( "count", (int)_running_tasks.size() );
        Z_FOR_EACH( Task_list, _running_tasks, t )  tasks_element.appendChild( (*t)->dom_element( document, show ) ), dom_append_nl( tasks_element );
        job_element.appendChild( tasks_element );
      //dom_append_nl( job_element );


        if( show & show_description )  dom_append_text_element( job_element, "description", _description );

        xml::Element_ptr queue_element = document.createElement( "queued_tasks" );
        queue_element.setAttribute( "length", as_string( _task_queue.size() ) );
        dom_append_nl( queue_element );
        job_element.appendChild( queue_element );

        if( (show & show_task_queue)  &&  !_task_queue.empty() )
        {
            FOR_EACH( Task_queue, _task_queue, it )
            {
                Task*            task                = *it;
                xml::Element_ptr queued_task_element = document.createElement( "queued_task" );
                
                queued_task_element.setAttribute( "task"    , task->id() );
                queued_task_element.setAttribute( "id"      , task->id() );                         // veraltet
                queued_task_element.setAttribute( "enqueued", task->_enqueue_time.as_string() );
                queued_task_element.setAttribute( "name"    , task->_name );
                
                if( task->_start_at )
                    queued_task_element.setAttribute( "start_at", task->_start_at.as_string() );
                
                if( task->has_parameters() )  queued_task_element.appendChild( task->_params->dom_element( document, "params", "param" ) );

                queue_element.appendChild( queued_task_element );
                dom_append_nl( queue_element );
            }
        }

        if( show & show_task_history )
        {
            job_element.appendChild( _history.read_tail( document, -1, -show._max_task_history, show, true ) );
        }

        if( _order_queue )
        {
            Show_what modified_show = show;
            if( modified_show | show_job_orders )  modified_show |= show_orders;

            job_element.appendChild( _order_queue->dom_element( document, modified_show, which_job_chain ) );
        }

        if( _error       )  append_error_element( job_element, _error );

        job_element.appendChild( _log->dom_element( document, show ) );
    }

    return job_element;
}

//---------------------------------------------------------------------------------kill_queued_task

void Job::kill_queued_task( int task_id )
{
    bool ok = _task_queue.remove_task( task_id, Task_queue::w_task_killed );

    if( ok ) 
    {
        Time old_next_time = _next_time;
        calculate_next_time();
        if( _next_time != old_next_time )  signal( "task killed" );
    }
}

//-----------------------------------------------------------------------------------Job::kill_task

void Job::kill_task( int id, bool immediately )
{
    THREAD_LOCK_DUMMY( _lock )
    {
        //Task* task = NULL;

        Z_FOR_EACH( Task_list, _running_tasks, t )
        {
            if( (*t)->_id == id )  
            { 
                (*t)->cmd_end( immediately );       // Ruft kill_queued_task()
                return;
            }
        }

        kill_queued_task( id );
    }
}

//-------------------------------------------------------------------------------Job::signal_object
// Anderer Thread

void Job::signal_object( const string& object_set_class_name, const Level& level )
{
    THREAD_LOCK_DUMMY( _lock )
    {
        if( _state == Job::s_pending
         && _object_set_descr
         && _object_set_descr->_class->_name == object_set_class_name 
         && _object_set_descr->_level_interval.is_in_interval( level ) )
        {
            //start_without_lock( NULL, object_set_class_name );
            //_event->signal( "Object_set " + object_set_class_name );
            //_thread->signal( obj_name() + ", Object_set " + object_set_class_name );
        }
    }
}

//----------------------------------------------------------------------Job::create_module_instance

ptr<Module_instance> Job::create_module_instance()
{
    ptr<Module_instance>  result;

    THREAD_LOCK_DUMMY( _lock )
    {
        if( _state == s_read_error )  throw_xc( "SCHEDULER-190" );
        if( _state == s_error      )  throw_xc( "SCHEDULER-204", _name, _error.what() );

        result = _module_ptr->create_instance();

        result->set_job_name( name() ); 
        result->set_log( _log );
    }

    return result;
}

//--------------------------------------------------------------------Job::get_free_module_instance
/*
Module_instance* Job::get_free_module_instance( Task* task )
{
    THREAD_LOCK_DUMMY( _lock )
    {
        Z_FOR_EACH( Module_instance_vector, _module_instances, m )
        {
            if( !*m )  *m = create_module_instance();


/ * Erstmal auskommentiert, 23.8.03
            if( !(*m)->_task ) 
            { 
                (*m)->_task = task; 
                return *m;
            }
* /
        }
    }

    throw_xc( "get_free_module_instance" );
}

//---------------------------------------------------------------------Job::release_module_instance

void Job::release_module_instance( Module_instance* module_instance )
{
    THREAD_LOCK_DUMMY( _lock )
    {
        Z_FOR_EACH( Module_instance_vector, _module_instances, m )
        {
            if( *m == module_instance )
            {
                *m = NULL; 
                break;
            }
        }
    }

    throw_xc( "release_module_instance" );
}
*/

} //namespace spooler
} //namespace sos
