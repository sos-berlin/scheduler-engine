// $Id: spooler_job.cxx,v 1.40 2003/10/19 19:59:02 jz Exp $
/*
    Hier sind implementiert

    Job
*/


#include "spooler.h"

#ifndef Z_WINDOWS
#   include <signal.h>
#   include <sys/signal.h>
#   include <sys/wait.h>
#endif


namespace sos {
namespace spooler {


const int max_task_time_out = 7*24*3600;

//---------------------------------------------------------------------------------start_cause_name

string start_cause_name( Start_cause cause )
{
    switch( cause )
    {
        case cause_none               : return "none";
        case cause_period_once        : return "period_once";
        case cause_period_single      : return "period_single";
        case cause_period_repeat      : return "period_repeat";
        case cause_job_repeat         : return "job_repeat";
        case cause_queue              : return "queue";
        case cause_queue_at           : return "queue_at";
        case cause_directory          : return "directory";
        case cause_signal             : return "signal";
        case cause_delay_after_error  : return "delay_after_error";
        case cause_order              : return "order";
        case cause_wake               : return "wake";
        default                       : return as_string( (int)cause );
    }
}

//-----------------------------------------------------------------------------------------Job::Job

Job::Job( Spooler* spooler )
: 
    _zero_(this+1),
    _spooler(spooler),
    _log(spooler),
    _module(spooler,&_log),
    _history(this)
{
    _next_time = latter_day;
    _priority  = 1;
    _default_params = new Com_variable_set;
    _task_timeout = latter_day;
}

//----------------------------------------------------------------------------------------Job::~Job

Job::~Job()
{
    close();
}

//-------------------------------------------------------------------------------------Job::set_dom

void Job::set_dom( const xml::Element_ptr& element, const Time& xml_mod_time )
{
    THREAD_LOCK( _lock )
    {
        bool order;

        _name             = element.     getAttribute( "name"       );
        _temporary        = element.bool_getAttribute( "temporary"  , _temporary  );
        _priority         = element. int_getAttribute( "priority"   , _priority   );
        _title            = element.     getAttribute( "title"      , _title      );
        _log_append       = element.bool_getAttribute( "log_append" , _log_append );
        order             = element.bool_getAttribute( "order"      );
        _max_tasks        = element.uint_getAttribute( "tasks"      , 1 );
        string t          = element.     getAttribute( "timeout"    );
        if( t != "" )  
        {
            _task_timeout = time::time_from_string( t );
            if( _task_timeout > max_task_time_out )  _task_timeout = max_task_time_out;   // Begrenzen, damit's beim Addieren mit now() keinen Überlauf gibt
        }

        if( order )
        {
            if( _temporary )  throw_xc( "SCHEDULER-155" );
            if( element.getAttributeNode( "priority" ) )  throw_xc( "SCHEDULER-165" );
            _order_queue = new Order_queue( this, &_log );
        }


        string text;

        text = element.getAttribute( "output_level" );
        if( !text.empty() )  _output_level = as_int( text );

        //for( time::Holiday_set::iterator it = _spooler->_run_time._holidays.begin(); it != _spooler->_run_time._holidays.end(); it++ )
        //    _run_time._holidays.insert( *it );
        

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
                _module._use_process_class = _spooler->has_process_classes();

                _module.set_dom_without_source( e );
                _module_xml_document  = e.ownerDocument();
                _module_xml_element   = e;
                _module_xml_mod_time  = xml_mod_time;

                _process_filename     = "";
                _process_param        = "";
                _process_log_filename = "";
            }
            else
            if( e.nodeName_is( "process"    ) )  _module_xml_document  = NULL,
                                                 _module_xml_element   = NULL,
                                                 _process_filename     = e.     getAttribute( "file" ),
                                                 _process_param        = e.     getAttribute( "param" ),
                                                 _process_log_filename = e.     getAttribute( "log_file" ),
                                                 _process_ingore_error = e.bool_getAttribute( "ignore_error" );
            else
            if( e.nodeName_is( "run_time" ) &&  !_spooler->_manual )  _run_time = Run_time(), 
                                                                      _run_time.set_holidays( _spooler->holidays() ), 
                                                                      _run_time.set_dom( e );
        }

        if( !_run_time.set() )   _run_time.set_default();
        if( _spooler->_manual )  _run_time = Run_time(),  _run_time.set_default_days(),  _run_time.set_once();

        if( _object_set_descr )  _object_set_descr->_class = _spooler->get_object_set_class( _object_set_descr->_class_name );
    }
}

//---------------------------------------------------------------------------------------Job::init0
// Bei <add_jobs> von einem anderen Thread gerufen.

void Job::init0()
{
    _state = s_none;

    _log.set_prefix( "Job  " + _name );       // Zwei Blanks, damit die Länge mit "Task " übereinstimmt
    _log.set_profile_section( profile_section() );
    _log.set_job( this );

    _com_job  = new Com_job( this );
  //_com_log  = new Com_log( &_log );

    _next_start_time = latter_day;

    set_state( s_pending );

    if( _module_xml_element )  read_script();
}

//----------------------------------------------------------------------------------------Job::init
// Bei <add_jobs> von einem anderen Thread gerufen.

void Job::init()
{
    _history.open();

    _module_ptr = _object_set_descr? &_object_set_descr->_class->_module
                                   : &_module;

    if( !_spooler->log_directory().empty()  &&  _spooler->log_directory()[0] != '*' )
    {
        _log.set_append( _log_append );
        _log.set_filename( _spooler->log_directory() + "/job." + jobname_as_filename() + ".log" );      // Jobprotokoll
    }


    init2();
}

//----------------------------------------------------------------------------------------Job::init
// Bei <add_jobs> von einem anderen Thread gerufen.

void Job::init2()
{
    _start_once    = _run_time.once();
    _delay_until   = 0;
    _period._begin = 0;
    _period._end   = 0;
    _next_single_start = latter_day;

    Time now = Time::now();

    //select_period( now );
    set_next_start_time( now );
}

//---------------------------------------------------------------------------------------Job::close

void Job::close()
{
    THREAD_LOCK( _lock )
    {
        clear_when_directory_changed();

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

        _log.close();
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
    THREAD_LOCK( _lock )
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
    
    _log.error( msg + x.what() );

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
        Xc xc ( "SOS-2000", x.what(), exception_name(x) );
        set_error_xc( xc );
    }
}

//--------------------------------------------------------------------------------------Job::signal

void Job::signal( const string& signal_name )
{ 
    THREAD_LOCK( _lock )  _next_time = 0;

    _spooler->signal( signal_name ); 
}

//---------------------------------------------------------------------------------Job::create_task

Sos_ptr<Task> Job::create_task( const ptr<spooler_com::Ivariable_set>& params, const string& name, Time start_at )
{
    Sos_ptr<Task> task;

    if( !_process_filename.empty() )   task = SOS_NEW( Process_task   ( this ) );
    else
  //if( _object_set_descr          )   task = SOS_NEW( Object_set_task( this ) );
  //else                             
                                       task = SOS_NEW( Job_module_task( this ) );

    Time now = Time::now();
    task->_enqueue_time = now;
    task->_id           = _spooler->_db->get_task_id();

    _default_params->Clone( (spooler_com::Ivariable_set**)task->_params.pp() );
    if( params )  task->_params->merge( params );

    task->_name     = name;
    task->_start_at = start_at; 

    return task;
}

//--------------------------------------------------------------------------------Job::enqueue_task

void Job::enqueue_task( const Sos_ptr<Task>& task )
{
    THREAD_LOCK( _lock )
    {
        Task_queue::iterator it = _task_queue.begin();  // _task_queue nach _start_at geordnet halten
        while( it != _task_queue.end()  &&  (*it)->_start_at <= task->_start_at )  it++;
        _task_queue.insert( it, task );

        calculate_next_time( Time::now() );
    }
}

//-------------------------------------------------------------------------Job::get_task_from_queue

Sos_ptr<Task> Job::get_task_from_queue( Time now )
{
    Sos_ptr<Task> task;

    if( _state == s_read_error )  return NULL;

    THREAD_LOCK( _lock )
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

            //_task_queue.erase( it );
        }
    }

    return task;
}

//----------------------------------------------------------------------Job::remove_from_task_queue

void Job::remove_from_task_queue( Task* task, Log_level log_level )
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Task_queue, _task_queue, it )  
        {
            if( +*it == task )  
            {
                if( log_level > log_none )  _log.log( log_level, task->obj_name() + " aus der Warteschlange entfernt" );
                _task_queue.erase( it );
                break;
            }
        }
    }
}

//---------------------------------------------------------------------------------Job::remove_task

void Job::remove_running_task( Task* task )
{
    THREAD_LOCK( _lock )  
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

void Job::start( const ptr<spooler_com::Ivariable_set>& params, const string& task_name, Time start_at )
{
    THREAD_LOCK( _lock )  start_without_lock( params, task_name, start_at );
}

//---------------------------------------------------------------------------------------Job::start

Sos_ptr<Task> Job::start_without_lock( const ptr<spooler_com::Ivariable_set>& params, const string& task_name, Time start_at, bool log )
{
    if( log && _spooler->_debug )  _log.debug( "start(at=" + start_at.as_string() + ( task_name == ""? "" : ",name=\"" + task_name + '"' ) + ")" );

    switch( _state )
    {
        case s_read_error:  throw_xc( "SCHEDULER-132", name(), _error? _error->what() : "" );
        case s_stopped:     set_state( s_pending );
        default: ;
    }

    if( !start_at  &&  !_run_time.period_follows( Time::now() ) )   throw_xc( "SCHEDULER-143" );

    Sos_ptr<Task> task = create_task( params, task_name, start_at );
    task->_let_run = true;
    enqueue_task( task );

    signal( "start job" );

    return task;
}

//---------------------------------------------------------------------------------Job::read_script

bool Job::read_script()
{
    THREAD_LOCK( _lock )
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

//--------------------------------------------------------------------------------------Job::reread

void Job::reread()
{
    _log( "Skript wird erneut gelesen (<include> wird erneut ausgeführt)" );
    read_script();
}

//---------------------------------------------------------------------------Job::execute_state_cmd

bool Job::execute_state_cmd()
{
    bool something_done = false;

    THREAD_LOCK( _lock )
    {
        if( _state_cmd )
        {
            switch( _state_cmd )
            {
                case sc_stop:       if( _state != s_stopping
                                     && _state != s_stopped  
                                     && _state != s_read_error )  stop( true ),                something_done = true;
                                    break;

                case sc_unstop:     if( _state == s_stopped    )  set_state( s_pending ),      something_done = true,  set_next_start_time( Time::now() );
                                    break;

                case sc_end:        if( _state == s_running    )                               something_done = true;
                                    Z_FOR_EACH( Task_list, _running_tasks, t )  (*t)->cmd_end();
                                    break;

                case sc_suspend:    Z_FOR_EACH( Task_list, _running_tasks, t ) 
                                    {
                                        Task* task = *t;
                                        if( task->_state == Task::s_running 
                                         || task->_state == Task::s_running_delayed
                                         || task->_state == Task::s_running_waiting_for_order )  task->set_state( Task::s_suspended );
                                    }
                                    something_done = true;
                                    break;

                case sc_continue:   Z_FOR_EACH( Task_list, _running_tasks, t ) 
                                    {
                                        Task* task = *t;
                                        if( task->_state == Task::s_suspended 
                                         || task->_state == Task::s_running_delayed
                                         || task->_state == Task::s_running_waiting_for_order )  task->set_state( Task::s_running );
                                    }
                                    something_done = true;
                                    break;

                case sc_wake:       //if( _state == s_suspended
                                    // || _state == s_running_delayed )  set_state( s_running ), something_done = true;

                                    if( _state == s_pending
                                     || _state == s_stopped )
                                    {
                                        set_state( s_pending );

                                        Time now = Time::now();
                                        Sos_ptr<Task> task = create_task( NULL, "", now );
                                        
                                        task->_cause = cause_wake;
                                        task->_let_run = true;
                                        task->attach_to_a_thread();   // Es gibt zZ nur einen Thread
                                    }
                                    break;

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
    THREAD_LOCK( _lock )
    {
        _log.debug( "start_when_directory_changed \"" + directory_name + "\", \"" + filename_pattern + "\"" );

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

        ptr<Directory_watcher> dw = Z_NEW( Directory_watcher( &_log ) );

        dw->watch_directory( directory_name, filename_pattern );
        dw->set_name( "job(\"" + _name + "\").start_when_directory_changed(\"" + directory_name + "\")" );
        _directory_watcher_list.push_back( dw );
        dw->add_to( &_spooler->_wait_handles );
    }
}

//----------------------------------------------------------------Job::clear_when_directory_changed

void Job::clear_when_directory_changed()
{
    THREAD_LOCK( _lock )
    {
        if( !_directory_watcher_list.empty() )  _log.debug( "clear_when_directory_changed" );

        _directory_watcher_list.clear();
    }
}

//-------------------------------------------------------------------------------Job::select_period

void Job::select_period( Time now )
{
    if( now >= _period.end() )       // Periode abgelaufen?
    {
        THREAD_LOCK( _lock )  _period = _run_time.next_period(now);  

        if( _period.begin() != latter_day )
        {
            string rep; if( _period._repeat != latter_day )  rep = _period._repeat.as_string();
            _log.debug( "Nächste Periode ist <period begin=\"" + _period.begin().as_string() + "\" end=\"" + _period.end().as_string() + "\" repeat=\"" + rep + "\">" );
        }
        else 
            _log.debug( "Keine weitere Periode" );
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
            if( !_repeat )  _next_single_start = _run_time.next_single_start( now );

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
                        Period next_period = _run_time.next_period( _period.end() );
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
        if( !msg.empty() )  _log.debug( msg );
    }

    THREAD_LOCK( _lock )
    {
        _next_start_time = next_start_time;
        calculate_next_time( now );
    }
}

//-------------------------------------------------------------------------Job::calculate_next_time
// Für Spooler_thread

void Job::calculate_next_time( Time now )
{
    THREAD_LOCK( _lock )
    {
        Time next_time = latter_day;

        if( !_waiting_for_process )
        {
            if( _state == s_pending  
            || _state == s_running  &&  _running_tasks.size() < _max_tasks )
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

//-------------------------------------------------------------------------------Job::task_to_start

Sos_ptr<Task> Job::task_to_start()
{
    if( _spooler->state() == Spooler::s_stopping
     || _spooler->state() == Spooler::s_stopping_let_run )  return NULL;

    Time            now   = Time::now();
    Start_cause     cause = cause_none;
    Sos_ptr<Task>   task  = NULL;
    ptr<Order>      order;
    string          log_line;

    task = get_task_from_queue( now );
    if( task )  cause = task->_start_at? cause_queue_at : cause_queue;
        
    if( _state == s_pending && now >= _next_single_start )  cause = cause_period_single;     
                                                    //else  select_period(now);

    if( cause                      // Auf weitere Anlässe prüfen und diese protokollieren
     || is_in_period(now) )
    {
        THREAD_LOCK( _lock )
        {
            if( _state == s_pending )
            {
                if( _start_once )              cause = cause_period_once,  _start_once = false,     log_line += "Task startet wegen <run_time once=\"yes\">\n";
                else
                if( now >= _next_start_time )  cause = cause_period_repeat,                         log_line += "Task startet, weil Job-Startzeit erreicht: " + _next_start_time.as_string();
                                                                        
                Directory_watcher_list::iterator it = _directory_watcher_list.begin();
                while( it != _directory_watcher_list.end() )
                {
                    if( (*it)->signaled_then_reset() )
                    {
                        cause = cause_directory;
                        log_line += "Task startet wegen eines Ereignisses für Verzeichnis " + (*it)->directory();
                        
                        if( !(*it)->valid() )
                        {
                            it = _directory_watcher_list.erase( it );  // Folge eines Fehlers, s. Directory_watcher::set_signal
                            continue;
                        }
                    }

                    it++;
                }
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
            if( !log_line.empty() )  _log.debug( log_line );

            if( task )
            {
                remove_from_task_queue( task, log_none );
            }
            else
            {
                task = create_task( NULL, "", now );

                task->set_order( order );
                task->_cause = cause;
                task->_let_run |= ( cause == cause_period_single );
            }

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
            if( notify )  _module._process_class->notify_a_process_is_idle();       // Dieser Job braucht den Prozess nicht mehr. Also nächsten Job benachrichtigen!
        }
    }

    return cause? task : NULL;
}

//--------------------------------------------------------------------------------Job::do_something

bool Job::do_something()
{
  //Z_DEBUG_ONLY( _log.debug9( "do_something() state=" + state_name() ); )

    Time next_time_at_begin = _next_time;
    bool something_done     = false;       
    Time now                = Time::now();

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
                    THREAD_LOCK( _lock )
                    {
                        _log.open();           // Jobprotokoll, nur wirksam, wenn set_filename() gerufen, s. Job::init().

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
            LOG( obj_name() << ".do_something()  Nichts getan. state=" << state_name() << ", _next_time= " << _next_time << ", wird verzögert\n" );
            _next_time = Time::now() + 1;
        }
        else
        {
            Z_DEBUG_ONLY( LOG( obj_name() << ".do_something()  Nichts getan. state=" << state_name() << ", _next_time war " << next_time_at_begin << "\n" ); )
        }
    }


    return something_done;
}

//-----------------------------------------------------------------------------------Job::set_state

void Job::set_state( State new_state )
{ 
    THREAD_LOCK( _lock )  
    {
        if( new_state == _state )  return;

        if( new_state == s_pending )  reset_error();

        _state = new_state;

        if( _state == s_stopped )  _next_start_time = _next_time = latter_day;

        if( _spooler->_debug )
        {
            if( new_state == s_stopping
             || new_state == s_stopped  )  _log.info( "state=" + state_name() ); 
                                     else  _log.debug9( "state=" + state_name() );
        }

        if( _waiting_for_process  &&  ( _state != s_pending  ||  _state != s_running ) )
        {
            remove_waiting_job_from_process_list();
        }
    }
}

//-------------------------------------------------------------------------------Job::set_state_cmd
// Anderer Thread

void Job::set_state_cmd( State_cmd cmd )
{ 
    bool ok = false;

    THREAD_LOCK( _lock )
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
                                    THREAD_LOCK( _lock )  start_without_lock( NULL, "", Time::now(), true );
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

    THREAD_LOCK( _lock )
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
        case s_pending:         return "pending";
        case s_running:         return "running";
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

//-----------------------------------------------------------------------------------------Job::dom
// Anderer Thread

xml::Element_ptr Job::dom( const xml::Document_ptr& document, Show_what show, Job_chain* which_job_chain )
{
    xml::Element_ptr job_element = document.createElement( "job" );

    THREAD_LOCK( _lock )
    {
        job_element.setAttribute( "job"       , _name                   );
        job_element.setAttribute( "state"     , state_name()            );

        if( _waiting_for_process )
        job_element.setAttribute( "waiting_for_process", _waiting_for_process? "yes" : "no" );

        if( !_title.empty() )
        job_element.setAttribute( "title"     , _title                  );

        job_element.setAttribute( "all_steps" , _step_count             );

        if( !_state_text.empty() )
        job_element.setAttribute( "state_text", _state_text             );

        job_element.setAttribute( "log_file"  , _log.filename()         );
        job_element.setAttribute( "order"     , _order_queue? "yes" : "no" );
        
        if( _state_cmd        )  job_element.setAttribute( "cmd"    , state_cmd_name()  );

        //if( _next_start_time < latter_day )
        //job_element.setAttribute( "next_start_time", _next_start_time.as_string() );

        if( _state == s_pending )
        {
            // Versuchen, nächste Startzeit herauszubekommen
            Period p    = _period;  
            int    i    = 100;      // Anzahl Perioden, die wir probieren
            Time   next = _next_start_time;
            
            if( next == latter_day )
            {
                p = _run_time.next_period( p.end() );
                next = p.begin();

                while( i-- ) {          
                    if( p.has_start() )  break;
                    p = _run_time.next_period( p.end() );
                    next = p.begin();
                }
                
                if( i < 0 )  next = latter_day;
            }

            if( next > _next_single_start )  next = _next_single_start;
            if( next < latter_day )  job_element.setAttribute( "next_start_time", next.as_string() );
        }


        dom_append_nl( job_element );
        xml::Element_ptr tasks_element = document.createElement( "tasks" );
        Z_FOR_EACH( Task_list, _running_tasks, t )  tasks_element.appendChild( (*t)->dom( document, show ) ), dom_append_nl( tasks_element );
        job_element.appendChild( tasks_element );
      //dom_append_nl( job_element );


        if( show & show_description )  dom_append_text_element( job_element, "description", _description );

        if( (show & show_task_queue)  &&  !_task_queue.empty() )
        {
            xml::Element_ptr queue_element = document.createElement( "queued_tasks" );
            dom_append_nl( queue_element );

            FOR_EACH( Task_queue, _task_queue, it )
            {
                xml::Element_ptr queued_task_element = document.createElement( "queued_task" );
                queued_task_element.setAttribute( "id"      , (*it)->id() );
                queued_task_element.setAttribute( "enqueued", (*it)->_enqueue_time.as_string() );
                queued_task_element.setAttribute( "name"    , (*it)->_name );
                if( (*it)->_start_at )
                    queued_task_element.setAttribute( "start_at", (*it)->_start_at.as_string() );

                queue_element.appendChild( queued_task_element );
                dom_append_nl( queue_element );
            }

            job_element.appendChild( queue_element );
        }

        if( _order_queue             )  dom_append_nl( job_element ),  job_element.appendChild( _order_queue->dom( document, show, which_job_chain ) );
        if( _error                   )  dom_append_nl( job_element ),  append_error_element( job_element, _error );
    }

    return job_element;
}

//-----------------------------------------------------------------------------------Job::kill_task

void Job::kill_task( int id )
{
    THREAD_LOCK( _lock )
    {
        bool ok = false;

        Z_FOR_EACH( Task_list, _running_tasks, t )
        {
            Task* task = *t;
            if( task->_id == id )  { task->cmd_end();  ok = true;  break; }
        }

        if( !ok )
        {
            for( Task_queue::iterator it = _task_queue.begin(); it != _task_queue.end(); it++ )
            {
                if( (*it)->_id == id )  
                {
                    Time old_next_time = _next_time;

                    _task_queue.erase( it );

                    calculate_next_time();
                    
                    if( _next_time != old_next_time )  signal( "task killed" );
                    break;
                }
            }
        }
    }
}

//-------------------------------------------------------------------------------Job::signal_object
// Anderer Thread

void Job::signal_object( const string& object_set_class_name, const Level& level )
{
    THREAD_LOCK( _lock )
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

    THREAD_LOCK( _lock )
    {
        if( _state == s_read_error )  throw_xc( "SCHEDULER-190" );

        result = _module_ptr->create_instance();

        result->set_job_name( name() ); 
        result->set_log( &_log );
    }

    return result;
}

//--------------------------------------------------------------------Job::get_free_module_instance

Module_instance* Job::get_free_module_instance( Task* task )
{
    THREAD_LOCK( _lock )
    {
        Z_FOR_EACH( Module_instance_vector, _module_instances, m )
        {
            if( !*m )  *m = create_module_instance();


/* Erstmal auskommentiert, 23.8.03
            if( !(*m)->_task ) 
            { 
                (*m)->_task = task; 
                return *m;
            }
*/
        }
    }

    throw_xc( "get_free_module_instance" );
}

//---------------------------------------------------------------------Job::release_module_instance

void Job::release_module_instance( Module_instance* module_instance )
{
    THREAD_LOCK( _lock )
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

} //namespace spooler
} //namespace sos
