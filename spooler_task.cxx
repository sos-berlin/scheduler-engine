// $Id: spooler_task.cxx,v 1.169 2003/08/14 11:01:14 jz Exp $
/*
    Hier sind implementiert

    Spooler_object
    Object_set
    Task
*/




#include "spooler.h"

#ifndef Z_WINDOWS
#   include <signal.h>
#   include <sys/signal.h>
#   include <sys/wait.h>
#endif


namespace sos {
namespace spooler {

//--------------------------------------------------------------------------------------------const

static const string spooler_get_name        = "spooler_get";
static const string spooler_level_name      = "spooler_level";

//----------------------------------------------------------------------------Spooler_object::level

Level Spooler_object::level()
{
    Variant level = com_property_get( _idispatch, spooler_level_name );
    level.ChangeType( VT_INT );

    return level.intVal;
}

//--------------------------------------------------------------------------Spooler_object::process

void Spooler_object::process( Level output_level )
{
    com_call( _idispatch, spooler_process_name, output_level );
}

//---------------------------------------------------------------------------Object_set::Object_set

Object_set::Object_set( Spooler* spooler, Module_task* task, const Sos_ptr<Object_set_descr>& descr ) 
: 
    _zero_(this+1),
    _spooler(spooler),
    _task(task),
    _object_set_descr(descr),
    _class(descr->_class)
{
}

//--------------------------------------------------------------------------Object_set::~Object_set

Object_set::~Object_set()
{
    _idispatch = NULL;
}

//---------------------------------------------------------------------------------Object_set::open

bool Object_set::open()
{
    bool        ok;
    Variant object_set_vt;

    if( _class->_object_interface )
    {
        Module_instance::In_call( _task, "spooler_make_object_set" );
        object_set_vt = _task->_module_instance->call( "spooler_make_object_set" );

        if( object_set_vt.vt != VT_DISPATCH 
         || object_set_vt.pdispVal == NULL  )  throw_xc( "SPOOLER-103", _object_set_descr->_class_name );

        _idispatch = object_set_vt.pdispVal;
    }
    else
    {
        _idispatch = _task->_module_instance->dispatch();
    }

    if( com_name_exists( _idispatch, spooler_open_name ) ) 
    {
        Module_instance::In_call in_call ( _task, spooler_open_name );
        ok = check_result( com_call( _idispatch, spooler_open_name ) );
        in_call.set_result( ok );
    }
    else  
        ok = true;

    return ok && !_task->has_error();
}

//--------------------------------------------------------------------------------Object_set::close

void Object_set::close()
{
    if( com_name_exists( _idispatch, spooler_close_name ) )  
    {
        Module_instance::In_call in_call ( _task, spooler_close_name );
        com_call( _idispatch, spooler_close_name );
    }

    _idispatch = NULL;
}

//----------------------------------------------------------------------------------Object_set::get

Spooler_object Object_set::get()
{
    Spooler_object object;

    while(1)
    {
        Module_instance::In_call in_call ( _task, spooler_get_name );
        Variant obj = com_call( _idispatch, spooler_get_name );

        if( obj.vt == VT_EMPTY    )  return Spooler_object(NULL);
        if( obj.vt != VT_DISPATCH
         || obj.pdispVal == NULL  )  throw_xc( "SPOOLER-102", _object_set_descr->_class_name );
    
        object = obj.pdispVal;

        if( obj.pdispVal == NULL )  break;  // EOF
        if( _object_set_descr->_level_interval.is_in_interval( object.level() ) )  break;

        //_log.info( "Objekt-Level " + as_string( object.level() ) + " ist nicht im Intervall" );
    }

    return object;
}

//---------------------------------------------------------------------------------Object_set::step

bool Object_set::step( Level result_level )
{
  //if( eof() )  return false;

    if( _class->_object_interface )
    {
        Spooler_object object = get();
        if( object.is_null() )  return false;

        if( _task->has_error() )  return false;       // spooler_task.error() gerufen?

        Module_instance::In_call in_call ( _task, spooler_process_name );
        object.process( result_level );

        return true;
    }
    else
    {
        Module_instance::In_call in_call ( _task, spooler_process_name );
        bool result = check_result( _task->_module_instance->call( spooler_process_name, result_level ) );
        in_call.set_result( result );
        return result;
    }
}

//-------------------------------------------------------------------------------Object_set::thread
/*
Spooler_thread* Object_set::thread() const
{ 
    return _task->_job->_thread; 
}
*/
//---------------------------------------------------------------------------------------Task::Task

Task::Task( Job* job )    
: 
    _zero_(this+1), 
    _spooler(job->_spooler), 
    _job(job),
    _log(job->_spooler),
    _history(&job->_history,this)
{
    _let_run = _job->_period.let_run();

    _log.set_job( _job );
    _log.set_task( this );
}

//--------------------------------------------------------------------------------------Task::~Task
// Kann von anderem Thread gerufen werden, wenn der noch eine COM-Referenz hat

Task::~Task()    
{
    try
    { 
        close(); 
    } 
    catch( const exception& x ) { _log.warn( x.what() ); }
}

//--------------------------------------------------------------------------------------Task::close
// Kann von anderem Thread gerufen werden, wenn der noch eine COM-Referenz hat

void Task::close()
{
    if( _closed )  return;
    
    _history.end();

    try
    {
        do_stop();
    }
    catch( const exception& x ) { _log.warn( x.what() ); }

    do_close();
/*
    THREAD_LOCK( _job->_lock )  
        if( _job->_com_task )  
            if( _job->_com_task->task() == this )      // spooler_task?
                _job->_com_task->set_task(NULL);
*/

    // Alle, die mit wait_until_terminated() auf diese Task warten, wecken:
    THREAD_LOCK( _terminated_events_lock )  
    {
        FOR_EACH( vector<Event*>, _terminated_events, it )  (*it)->signal( "close task" );
        _terminated_events.clear();
    }

    _closed = true;
}

//----------------------------------------------------------------------------------------Task::dom

xml::Element_ptr Task::dom( const xml::Document_ptr& document, Show_what show )
{
    xml::Element_ptr task_element = document.createElement( "task" );

    THREAD_LOCK( _lock )
    {
        task_element.setAttribute( "running_since"   , _running_since.as_string() );

        if( _state == s_running  &&  _last_process_start_time )
        task_element.setAttribute( "in_process_since", _last_process_start_time.as_string() );

        task_element.setAttribute( "steps"           , _step_count );
        task_element.setAttribute( "id"              , _id );

        if( !_in_call.empty() )  
        task_element.setAttribute( "calling"         , _in_call );

        task_element.setAttribute( "log_file"        , _log.filename() );

        if( Module_task* t = dynamic_cast<Module_task*>( this ) )
        {
            if( t->_module_instance )  
            {
                int pid = t->_module_instance->pid();
                if( pid ) task_element.setAttribute( "pid", pid );       // separate_process="yes", Remote_module_instance_proxy
            }
        }

        if( _order )  dom_append_nl( task_element ),  task_element.appendChild( _order->dom( document, show ) );
        if( _error )  dom_append_nl( task_element ),  append_error_element( task_element, _error );
    }

    return task_element;
}

//-------------------------------------------------------------------------------Task::enter_thread

void Task::enter_thread( Spooler_thread* thread )
{ 
    set_state( s_start_task );

    _thread = thread;  
    thread->add_task( this ); 
}

//-------------------------------------------------------------------------------Task::leave_thread

void Task::leave_thread()
{ 
    _thread->remove_task( this );  
    _thread = NULL; 
}

//-------------------------------------------------------------------------Task::attach_to_a_thread

void Task::attach_to_a_thread()
{
    assert( current_thread_id() == _spooler->thread_id() );

    enter_thread( _spooler->select_thread_for_task( this ) );
}

//--------------------------------------------------------------------------------Task::set_in_call

void Task::set_in_call( const string& name, const string& extra )
{
    THREAD_LOCK( _lock )
    {
        _in_call = name;

        if( _spooler->_debug  &&  !name.empty() )  
        {
            _log.debug( name + "()  " + extra );
        }
    }
}

//--------------------------------------------------------------------------Task::set_error_xc_only

void Task::set_error_xc_only( const Xc& x )
{
    THREAD_LOCK( _lock )  _error = x;
    _job->set_error_xc_only( x );
}

//-------------------------------------------------------------------------------Task::set_error_xc

void Task::set_error_xc( const Xc& x )
{
    string msg; 
    if( !_in_call.empty() )  msg = "In " + _in_call + "(): ";
    
    _log.error( msg + x.what() );

    set_error_xc_only( x );
}

//----------------------------------------------------------------------------------Task::set_error

void Task::set_error( const exception& x )
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

//----------------------------------------------------------------------------------Task::set_error
#ifdef SYSTEM_HAS_COM

void Task::set_error( const _com_error& x )
{
    try
    {
        throw_mswin_error( x.Error(), x.Description() );
    }
    catch( const Xc& x )
    {
        set_error_xc( x );
    }
}

#endif

//----------------------------------------------------------------------------------Task::set_state

void Task::set_state( State new_state, const Time& next_time )
{ 
    THREAD_LOCK( _lock )  
    {
        if( new_state == _state  &&  next_time == _next_time )  return;

        if( new_state == s_running )  _job->increment_running_tasks(),  _thread->increment_running_tasks();
        if( _state    == s_running )  _job->decrement_running_tasks(),  _thread->decrement_running_tasks();
      //if( new_state == s_running  ||  new_state == s_start_task )  InterlockedIncrement( &_job->_running_tasks_count ),  InterlockedIncrement( &_job->_thread->_running_tasks_count );
      //if( _state    == s_running  ||  _state    == s_start_task )  InterlockedDecrement( &_job->_running_tasks_count ),  InterlockedDecrement( &_job->_thread->_running_tasks_count );

        if( new_state != s_running_delayed )  _next_spooler_process = 0;

        _state = new_state;
        _next_time = next_time;

        if( _spooler->_debug )
        {
            string msg = "state=" + state_name();
            if( _next_time )  msg += " (" + _next_time.as_string() + ")";
            if( new_state == s_starting  &&  _start_at )  msg += " (at=" + _start_at.as_string() + ")";

            if( new_state == s_ended )  _log.debug ( msg ); 
                                  else  _log.debug9( msg );
        }

    }
}

//---------------------------------------------------------------------------------Task::state_name

string Task::state_name( State state )
{
    switch( state )
    {
        case s_start_task:                  return "start_task";
        case s_starting:                    return "starting";
        case s_running:                     return "running";
        case s_running_delayed:             return "running_delayed";
        case s_running_waiting_for_order:   return "running_waiting_for_order";
        case s_running_process:             return "running_process";
        case s_suspended:                   return "suspended";
        case s_ending:                      return "ending";
        case s_ended:                       return "ended";
        default:                            return as_string( (int)state );
    }
}

//-------------------------------------------------------------------------------------Task::state
/* Anderer Thread

string Task::state()
{
    string st;

    THREAD_LOCK( _lock )
    {
        st = "state=" + state_name();
        if( !_in_call.empty() )  st += " in " + _in_call + "()";
    }

    return st;
}
*/
//-------------------------------------------------------------------------------------Task::signal

void Task::signal( const string& signal_name )
{ 
    set_next_time( 0 );

    _thread->signal( signal_name ); 
}

//-------------------------------------------------------------------------------------Task::finish

void Task::finish()
{
    // eMail versenden

    try
    {
        if( !_spooler->_manual )  _log.send( has_error()? -1 : _step_count );
        clear_mail();
    }
    catch( const exception& x  ) { _log.warn( x.what() ); }
    catch( const _com_error& x ) { _log.warn( bstr_as_string(x.Description()) ); }  
}

//------------------------------------------------------------------------------Task::set_next_time

void Task::set_next_time( const Time& next_time )
{
    THREAD_LOCK( _lock )  _next_time = next_time;
}

//-------------------------------------------------------------------------------Task::do_something

bool Task::do_something()
{
    Z_DEBUG_ONLY( _log.debug9( "do_something() state=" + state_name() ); )

    bool something_done = false;
    bool ok             = true;
    bool do_a_step      = false;


    // Wenn nichts zu tun ist, dann raus. Der Job soll nicht wegen eines alten Fehlers nachtr‰glich gestoppt werden (s.u.)
    if( _state == s_suspended  )  goto ENDE;
    if( _state == s_running_process  &&  !((Process_task*)+this)->signaled() )  goto ENDE;


    // HISTORIE und _end
    if( _state == s_start_task 
     || _state == s_running 
     || _state == s_running_delayed 
     || _state == s_running_waiting_for_order 
     || _state == s_running_process )          
    {
        if( _step_count == _job->_history.min_steps() )  _history.start();
        if( _end )  ok = false;
    }


    if( _state == s_start_task )                                                                // SPOOLER_INIT, SPOOLER_OPEN
    {
        ok = start();
        if( ok )  do_a_step = true;
        something_done = true;
        
        if( _job->_state == Job::s_read_error )
        {
            close();
            goto ENDE;
        }
    }


    // SPOOLER_PROCESS()
    {
        Time now = Time::now();

        if( _state == s_running_delayed  &&  now >= _next_spooler_process )
        {
            _next_spooler_process = 0;
            set_state( s_running );
        }

        if( _state == s_running_waiting_for_order )
        {
            if( !_order )  _order = _job->order_queue()->get_order_for_processing( now, this );

            if( _order )  set_state( s_running );            // Auftrag da? Dann Task weiterlaufen lassen (Ende der Run_time wird noch gepr¸ft)
                    else  ok &= _job->_period.is_in_time( now );   // Run_time abgelaufen? Dann Task beenden
        }

        if( ( _state == s_running || _state == s_running_process )  &&  ok  &&  !has_error() )      // SPOOLER_PROCESS
        {
            bool call_step = do_a_step | _let_run;
            if( !call_step )  call_step = _job->_period.is_in_time( now );
            if( !call_step )   // Period abgelaufen?
            {
                call_step = _let_run  ||  ( _job->select_period(now), _job->is_in_period(now) );
            }

            if( call_step ) 
            {
                _last_process_start_time = now;
                ok = step(); 
                something_done = true;
            }
            else
            {
                ok = false;
                _log( "Laufzeitperiode ist abgelaufen, Task wird beendet" );
            }
        }


        if( !ok || has_error() )                                                                    // SPOOLER_CLOSE
        {                                                                                           // SPOOLER_ON_SUCCESS, SPOOLER_ON_ERROR
            if( has_error() )  _history.start();

            if( _state == s_start_task
             || _state == s_starting        // Bei Fehler in spooler_init()
             || _state == s_running 
             || _state == s_running_delayed
             || _state == s_running_waiting_for_order
             || _state == s_running_process           )
            {
                end();
                something_done = true;
            }

            if( has_error()  &&  _job->repeat() == 0  &&  _job->_delay_after_error.empty() )
            {
                stop(); 
                something_done = true;
            }

            finish();
        }


        // _next_time wird hier ge‰ndert

        if( _state == s_running  &&  _next_spooler_process )
        {
            set_state( s_running_delayed, _next_spooler_process );
        }

        if( _state == s_running  &&  _job->order_queue()  &&  !_job->order_queue()->has_order( now ) )
        {
            set_state( s_running_waiting_for_order, latter_day );  
        }

        if( _state == s_running_waiting_for_order )
        {
            set_next_time( _job->order_queue()->next_time() );
        }


        if( _state == s_ended )                                                                     // TASK BEENDET
        {
            if( _job->_temporary  &&  _job->repeat() == 0 )  
            {
                stop();   // _temporary && s_stopped ==> spooler_thread.cxx entfernt den Job
                finish();
            }

            // Gesammelte eMail senden, wenn collected_max erreicht:
            Time log_time = _log.collect_end();
            if( log_time > Time::now()  &&  _next_time > log_time )  set_next_time( log_time );

            something_done = true;
        }

        THREAD_LOCK( _lock )
        {
            if( _next_time )  set_next_time( min( _next_time, _job->_period.end() ) );                      // Am Ende der Run_time wecken, damit die Task beendet werden kann
        }
    }


ENDE:
    if( _state != s_running  
     && _state != s_running_delayed
     && _state != s_running_waiting_for_order
     && _state != s_running_process  
     && _state != s_suspended                 )  send_collected_log();


    return something_done;
}

//-------------------------------------------------------------------------Task::send_collected_log

void Task::send_collected_log()
{
    try
    {
        _log.send( -2 );
    }
    catch( const exception&  x ) { _spooler->_log.error( x.what() ); }
    catch( const _com_error& x ) { _spooler->_log.error( bstr_as_string(x.Description()) ); }
}

//--------------------------------------------------------------------------Task::set_mail_defaults

void Task::set_mail_defaults()
{
    bool is_error = has_error();

    _log.set_mail_from_name( _job->profile_section() );

    string body = Sos_optional_date_time::now().as_string() + "\n\nJob " + _job->name() + "  " + _job->title() + "\n";
    body += "Task-Id " + as_string(id()) + ", " + as_string(_step_count) + " Schritte\n";
    body += "Spooler -id=" + _spooler->id() + "  host=" + _spooler->_hostname + "\n\n";

    if( !is_error )
    {
        _log.set_mail_subject( "Job " + _name + " gelungen" );
    }
    else
    {
        string errmsg = _error? _error->what() : _log.highest_msg();
        _log.set_mail_subject( string("FEHLER ") + errmsg, is_error );
    
        body += errmsg + "\n\n";
    }

    _log.set_mail_body( body + "Das Jobprotokoll liegt dieser Nachricht bei.", is_error );
}

//---------------------------------------------------------------------------------Task::clear_mail

void Task::clear_mail()
{
    _log.set_mail_from_name( "" );
    _log.set_mail_subject( "", true );
    _log.set_mail_body( "", true );
}

//--------------------------------------------------------------------------------------Task::start

bool Task::start()
{
    try 
    {
        if( !_spooler->log_directory().empty()  &&  _spooler->log_directory()[0] != '*' )
        {
            _log.set_filename( _spooler->log_directory() + "/task." + _job->jobname_as_filename() + ". " + as_string(_id) + ".log" );      // Task-Protokoll
        }


        THREAD_LOCK( _job->_lock )
        {
            set_state( s_starting );
            _thread->count_task();
            reset_error();
        }

        _running_since = Time::now();

        if( !loaded() )  
        {
            bool ok = do_load();
            if( !ok || has_error() )  return false;
        }

        bool ok = do_start();
        if( !ok || has_error() )  return false;

        _opened = true;
    }
    catch( const exception& x ) { set_error(x); }

    return !has_error();
}

//-----------------------------------------------------------------------------------------Task::end

void Task::end()
{
    set_state( s_ending );

    if( _opened )  
    {
        try
        {
            do_end();
        }
        catch( const exception& x ) { set_error(x); }
    }

    on_error_on_success();

    _opened = false;
    
    close();


    // Bei mehreren aufeinanderfolgenden Fehlern Wiederholung verzˆgern?

    if( has_error() )
    {
        InterlockedIncrement( &_job->_error_steps );
    
        if( !_job->repeat() )   // spooler_task.repeat hat Vorrang
        {
            Time delay = 0;
            FOR_EACH( Job::Delay_after_error, _job->_delay_after_error, it )  
                if( _job->_error_steps >= it->first )  delay = it->second;
            _job->_delay_until = delay? Time::now() + delay : 0;
        }
    }
    else
    {
       _job->_error_steps = 0;
    }

    leave_thread();

    set_state( s_ended );
}

//----------------------------------------------------------------------------------------Task::stop

void Task::stop()
{
    _job->stop( false );
}

//-------------------------------------------------------------------------Task::on_error_on_success

void Task::on_error_on_success()
{
    if( has_error() )
    {
        // spooler_on_error() wird auch gerufen, wenn spooler_init() einen Fehler hatte

        if( !_on_error_called )
        {
            _on_error_called = true;

            try
            {
                do_on_error();
            }
            catch( const exception& x ) { _log.error( string(spooler_on_error_name) + ": " + x.what() ); }
        }
    }
    else
    if( _opened )   
    {
        // spooler_on_success() wird nicht gerufen, wenn spooler_init() false lieferte

        try
        {
            do_on_success();
        }
        catch( const exception& x ) { set_error(x); }
    }
}

//----------------------------------------------------------------------------------------Task::step

bool Task::step()
{
    bool result;


    try 
    {
        if( _job->order_queue() )
        {
            if( !_order )  _order = _job->order_queue()->get_order_for_processing( Time::now(), this );
            if( !_order )  return true;

            _log.set_order_log( &_order->_log );
        }

        result = do_step();

        if( _next_spooler_process )  result = true;

        if( has_step_count()  ||  _step_count == 0 )        // Bei Process_task nur einen Schritt z‰hlen
        {
            _thread->count_step();
            _job->count_step();
            _step_count++;
        }


        if( _order )
        {
            _order->postprocessing( result );
            _log.set_order_log( NULL );
            _order = NULL;
            result = true;
        }
    }
    catch( const exception& x ) 
    { 
        _job->set_error(x); 
        result = false; 
    }


    if( _order )  // Nach Fehler
    {
        _order->processing_error();
        _log.set_order_log( NULL );
        _order = NULL;
    }

    return result;
}

//------------------------------------------------------------------------------------Task::cmd_end
/*
// Anderer Thread

void Task::cmd_end()
{
    Sos_ptr<Job> job = _job;        // this wird vielleicht gelˆscht

    THREAD_LOCK( job->_lock )
    {
        //THREAD_LOCK( _lock )
        {
            if( +job->_task == this )  job->set_state_cmd( Job::sc_end );
                                 else  job->remove_from_task_queue( this );
        }
    }
}
*/
//----------------------------------------------------------------------Task::wait_until_terminated
// Anderer Thread

bool Task::wait_until_terminated( double wait_time )
{
    Thread_id my_thread_id = current_thread_id();
    if( my_thread_id == _thread->thread_id() )  throw_xc( "SPOOLER-125" );     // Deadlock

    Spooler_thread* calling_thread = _spooler->thread_by_thread_id( my_thread_id );
  //if( calling_thread &&  !calling_thread->_free_threading )  throw_xc( "SPOOLER-131" );

    Event event ( obj_name() + " wait_until_terminated" );

    THREAD_LOCK( _terminated_events_lock )  _terminated_events.push_back( &event );

    bool result = event.wait( wait_time );

    THREAD_LOCK( _terminated_events_lock )  _terminated_events.pop_back();

    return result;
}

//----------------------------------------------------------------------------------Task::set_cause

void Task::set_cause( Start_cause cause )
{
    if( !_cause )  _cause = cause;
}

//-----------------------------------------------------------------------------Task::has_parameters

bool Task::has_parameters()
{
    int n = 0;
    _params->get_count( &n );
    return n != 0;
}

//--------------------------------------------------------------------------Task::set_history_field

void Task::set_history_field( const string& name, const Variant& value )
{
  //if( !_job->its_current_task(this) )  throw_xc( "SPOOLER-138" );

    _history.set_extra_field( name, value );
}

//--------------------------------------------------------------------------------Module_task::load

bool Module_task::do_load()
{
    ptr<Module_instance> module_instance;
    bool                 is_new = false;


    if( _job->_module_ptr->_reuse == Module::reuse_job )
    {
        module_instance = _job->get_free_module_instance( this );
    }
    else
    {
        module_instance = _job->_module_ptr->create_instance();
        is_new = true;
        _close_engine = true;
    }


    if( !module_instance->loaded() )
    {
        module_instance->init();
        module_instance->add_obj( (IDispatch*)_spooler->_com_spooler    , "spooler"        );
      //module_instance->add_obj( (IDispatch*)_job->_thread->_com_thread, "spooler_thread" );
        module_instance->add_obj( (IDispatch*)_job->_com_job            , "spooler_job"    );
        module_instance->add_obj( (IDispatch*)module_instance->_com_task, "spooler_task"   );
        module_instance->add_obj( (IDispatch*)module_instance->_com_log , "spooler_log"    );
    }


    module_instance->set_title( obj_name() );
    module_instance->_com_task->set_task( this );
    module_instance->_com_log->set_log( &_log );

    _module_instance = module_instance;


    if( !_module_instance->loaded() )
    {
        try
        {
            module_instance->load();
            module_instance->start();
        }
        catch( const exception& x ) { set_error(x);  _close_engine = true;  return false; }
    }

    if( is_new )
    {
        if( module_instance->name_exists( spooler_init_name ) )
        {
            Module_instance::In_call in_call ( this, spooler_init_name );
            bool ok = check_result( module_instance->call( spooler_init_name ) );
            in_call.set_result( ok );
            if( !ok || has_error() )  { _close_engine = true;  return false; }
        }
    }

    //_has_spooler_process = _module_instance->name_exists( spooler_process_name );

    return true;
}

//----------------------------------------------------------------------------Module_task::do_close

void Module_task::do_close()
{
    if( _module_instance )  
    { 
        if( _close_engine ) 
        {
            close_engine();
            _close_engine = false;
    
            if( _job->_module_ptr->_reuse == Module::reuse_job )
            {
                _job->release_module_instance( _module_instance );
            }
        }

      //if( _module_instance )  
        {
            _module_instance->_com_task->set_task( NULL );
            _module_instance->_com_log->set_log( NULL );
          //if( _module_instance->_com_log  )  _module_instance->_com_log->close(), _module_instance->_com_log  = NULL;
        }

        _module_instance = NULL;
    }
}

//------------------------------------------------------------------------Module_task::close_engine

void Module_task::close_engine()
{
    try 
    {
        //_log.debug3( "close scripting engine" );
        //if( _module_instance )  
        {
            _module_instance->close();
            _module_instance->_com_task = new Com_task();
            //_module_instance = NULL;
        }
    }
    catch( const exception& x ) { set_error(x); }
}

//-----------------------------------------------------------------------Module_task::do_on_success

void Module_task::do_on_success()
{
    if( _module_instance->loaded()  &&  _module_instance->name_exists( spooler_on_success_name ) )
    {
        Module_instance::In_call in_call ( this, spooler_on_success_name );
        _module_instance->call( spooler_on_success_name );
    }
}

//-------------------------------------------------------------------------Module_task::do_on_error

void Module_task::do_on_error()
{
    if( _module_instance->loaded()  &&  _module_instance->name_exists( spooler_on_error_name ) )
    {
        Module_instance::In_call in_call ( this, spooler_on_error_name );
        _module_instance->call( spooler_on_error_name );
    }
}

//------------------------------------------------------------------------Object_set_task::do_close
// Kann von anderem Thread gerufen werden, wenn der noch eine COM-Referenz hat

void Object_set_task::do_close()
{
    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_object_set )  _com_object_set->clear();
}

//-------------------------------------------------------------------------Object_set_task::do_load

bool Object_set_task::do_load()
{
    bool ok = true;

    if( !_object_set ) 
    {
        _object_set = SOS_NEW( Object_set( _spooler, this, _job->_object_set_descr ) );
        _com_object_set = new Com_object_set( _object_set );
    }

    if( !_module_instance->loaded() )
    {
        //ok = _module_instance->load();
        //if( !ok || _job->has_error() )  return false;
        throw_xc( "Object_set_task::do_load" );  //ok = do_load();
        if( !ok || has_error() )  return false;
    }

    return ok;
}

//------------------------------------------------------------------------Object_set_task::do_start

bool Object_set_task::do_start()
{
    set_state( s_running );

    return _object_set->open();
}

//--------------------------------------------------------------------------Object_set_task::do_end

void Object_set_task::do_end()
{
    _object_set->close();
    _object_set = NULL;
}

//-------------------------------------------------------------------------Object_set_task::do_step

bool Object_set_task::do_step()
{
    return _object_set->step( _job->_output_level );
}

//-------------------------------------------------------------------------Job_module_task::do_load
/*
bool Job_module_task::do_load()
{
    return _module_instance->load();
}
*/
//------------------------------------------------------------------------Job_module_task::do_start

bool Job_module_task::do_start()
{
    bool ok;

    set_state( s_running );

    if( _module_instance->name_exists( spooler_open_name ) )
    {
        Module_instance::In_call in_call ( this, spooler_open_name );

        ok = check_result( _module_instance->call( spooler_open_name ) );
        in_call.set_result( ok );
        if( !ok )  return false;
    }

    return true;
}

//--------------------------------------------------------------------------Job_module_task::do_end

void Job_module_task::do_end()
{
    if( _module_instance->name_exists( spooler_close_name ) )
    {
        Module_instance::In_call in_call ( this, spooler_close_name );
        _module_instance->call( spooler_close_name );
    }
}

//-------------------------------------------------------------------------Job_module_task::do_step

bool Job_module_task::do_step()
{
    //if( !_module_instance->_has_spooler_process )  return false;
    if( !_module_instance->name_exists( spooler_process_name ) )  return false;

    Module_instance::In_call in_call ( this, spooler_process_name, _order? "Auftrag " + _order->obj_name() : "" );
    bool ok = check_result( _module_instance->call( spooler_process_name ) );
    in_call.set_result( ok );
    return ok;
}

//-------------------------------------------------------------------------Job_module_task::do_step
/*
bool Job_module_task::do_step()
{
    //if( !_module_instance->_has_spooler_process )  return false;
    if( !_module_instance->name_exists( spooler_process_name ) )  return false;

    bool ok = false;

    switch( _call_state )
    {
        case c_none:            
            _module_instance->step__start();
            break;

        case c_step: 
            ok = check_result( _module_instance->step__end() );
            break;

        default:
            throw_xc( "call_state", _call_state );
    }

  //Module_instance::In_call in_call ( this, spooler_process_name, _order? "Auftrag " + _order->obj_name() : "" );
  //bool ok = check_result( _module_instance->call( spooler_process_name ) );
  //in_call.set_result( ok );
  //return ok;
}
*/
//-----------------------------------------------------------------------Process_task::Process_task

Process_task::Process_task( Job* job ) 
: 
    Task(job),
    _zero_(this+1)

#   ifdef Z_WINDOWS
        ,_process_handle( "process_handle" ) 
#   endif
{
}

//---------------------------------------------------------------------------Process_task::do_close

void Process_task::do_close()
{
     _process_handle.close();
}

//----------------------------------------------------------------------------Process_task::do_start
#ifdef Z_WINDOWS

bool Process_task::do_start()
{
    PROCESS_INFORMATION process_info; 
    STARTUPINFO         startup_info; 
    BOOL                ok;

    memset( &process_info, 0, sizeof process_info );

    memset( &startup_info, 0, sizeof startup_info );
    startup_info.cb = sizeof startup_info; 

    string command_line = _job->_process_filename;
    if( !_job->_process_param.empty() )  command_line += " " + _job->_process_param;

    for( int i = 1;; i++ )
    {
        string nr = as_string(i);
        Variant vt;
        HRESULT hr;

        hr = _params->get_var( Bstr(nr), &vt );
        if( FAILED(hr) )  throw_ole( hr, "Variable_set.var", nr.c_str() );

        if( vt.vt == VT_EMPTY )  break;

        hr = vt.ChangeType( VT_BSTR );
        if( FAILED(hr) )  throw_ole( hr, "VariantChangeType", nr.c_str() );

        string param = bstr_as_string( vt.bstrVal );
        if( param.find_first_of(' ') != string::npos )  param = quoted_string( param, '"', '"' );  // Ist Verdoppeln richtig?

        command_line += " " + param;
    }


    ok = CreateProcess( _job->_process_filename.c_str(),  // application name
                        (char*)command_line.c_str(),      // command line 
                        NULL,                       // process security attributes 
                        NULL,                       // primary thread security attributes 
                        FALSE,                      // handles are inherited?
                        0,                          // creation flags 
                        NULL,                       // use parent's environment 
                        NULL,                       // use parent's current directory 
                        &startup_info,              // STARTUPINFO pointer 
                        &process_info );            // receives PROCESS_INFORMATION 
    if( !ok )  throw_mswin_error( "CreateProcess", _job->_process_filename.c_str() );

    CloseHandle( process_info.hThread );

    _process_id = process_info.dwProcessId;
    _process_handle.set_handle( process_info.hProcess );
    _process_handle.set_name( "Process " + _job->_process_filename );
    _process_handle.add_to( &_thread->_wait_handles );

    set_state( s_running_process );
    return true;
}

//----------------------------------------------------------------------------Process_task::do_stop

void Process_task::do_stop()
{
    if( _process_handle )
    {
        _log.warn( "Prozess wird abgebrochen" );

        LOG( "TerminateProcess(" <<  _process_handle << ",999)\n" );
        BOOL ok = TerminateProcess( _process_handle, 999 );
        if( !ok )  throw_mswin_error( "TerminateProcess" );
    }
}

//-----------------------------------------------------------------------------Process_task::do_end

void Process_task::do_end()
{
    DWORD exit_code;

    BOOL ok = GetExitCodeProcess( _process_handle, &exit_code );
    if( !ok )  throw_mswin_error( "GetExitCodeProcess" );

    _process_handle.close();
    _result = (int)exit_code;

    _log.log_file( _job->_process_log_filename ); 

    if( exit_code )  throw_xc( "SPOOLER-126", exit_code );
}

//---------------------------------------------------------------------------Process_task::signaled

bool Process_task::signaled()
{
    return !_process_handle.signaled();
}

#endif
//----------------------------------------------------------------------------Process_task::do_step

bool Process_task::do_step()
{
    return !signaled();
}

//-----------------------------------------------------------------------------Process_event::close
#ifndef Z_WINDOWS

void Process_event::close()
{
    // waitpid() rufen, falls noch nicht geschehen (um Zombie zu schlieﬂen)

    if( _pid )
    {
        int status = 0;

        if( log_ptr )  *log_ptr << "waitpid(" << _pid << ")  ";

        int ret = waitpid( _pid, &status, WNOHANG | WUNTRACED );    // WUNTRACED: "which means to also return for children which are stopped, and whose status has not been reported."

        if( log_ptr )  if( ret == -1 )  *log_ptr << "ERRNO-" << errno << "  " << strerror(errno) << endl;
                                  else  *log_ptr << endl;

        _pid = 0;
    }
}

//------------------------------------------------------------------------------Process_event::wait

bool Process_event::wait( double seconds )
{
    if( !_pid )  return true;

    while(1)
    {
        int status = 0;

      //if( log_ptr )  *log_ptr << "waitpid(" << _pid << ")  ";

        int ret = waitpid( _pid, &status, WNOHANG | WUNTRACED );    // WUNTRACED: "which means to also return for children which are stopped, and whose status has not been reported."
        if( ret == -1 )  
        { 
            LOG( "waitpid(" << _pid << ") ==> ERRNO-" << errno << "  " << strerror(errno) << "\n" );
            //int pid = _pid; 
            _pid = 0; 
            //throw_errno( errno, "waitpid", as_string(pid).c_str() ); 
            set_signal();
            return true;
        }

        if( ret == _pid )
        {
            if( WIFEXITED( status ) )
            {
                _process_exit_code = WEXITSTATUS( status );
              //LOG( "exit_code=" << _process_exit_code << "\n" );
                _pid = 0;
                set_signal();
                return true;
            }

            if( WIFSIGNALED( status ) )
            {
                _process_signaled = WTERMSIG( status );
              //LOG( "signal=" << _process_exit_code << "\n" );
                _pid = 0;
                set_signal();
                return true;
            }
        }

      //LOG( "Prozess l‰uft noch\n" );

        if( seconds < 0.0001 )  break;

        double w = min( 0.1, seconds );
        seconds -= w;

        struct timespec t;
        t.tv_sec = (time_t)floor( w );
        t.tv_nsec = (time_t)max( 0.0, ( w - floor( w ) ) * 1e9 );

        int err = nanosleep( &t, NULL );
        if( err )
        {
            if( errno == EINTR )  return true;
            throw_errno( errno, "nanosleep" );
        }
    }

    return false;
}

#endif
//----------------------------------------------------------------------------Process_task::do_start
#ifndef Z_WINDOWS

bool Process_task::do_start()
{
    vector<string> string_args;

    string_args.push_back( _job->_process_filename );   // argv[0]
    if( _job->_process_param != "" )  string_args.push_back( _job->_process_param );

    for( int i = 1;; i++ )
    {
        string nr = as_string(i);
        Variant vt;
        HRESULT hr;

        hr = _params->get_var( Bstr(nr), &vt );
        if( FAILED(hr) )  throw_ole( hr, "Variable_set.var", nr.c_str() );

        if( vt.vt == VT_EMPTY )  break;

        string_args.push_back( string_from_variant( vt ) );
    }


#   ifndef SYSTEM_WINDOWS
        LOG( "signal(SIGCHLD,SIG_DFL)\n" );
        signal( SIGCHLD, SIG_DFL );                 // Java ver‰ndert das Signal-Verhalten, so dass waitpid() ohne diesen Aufruf versagte.
#   endif

    if( log_ptr )  *log_ptr << "fork()  ";
    int pid = fork();

    switch( pid )
    {
        case -1:    
            throw_errno( errno, "fork" );
        
        case 0:
        {
            char** args = new char* [ string_args.size() + 1 ];
            int    i;

            for( i = 0; i < string_args.size(); i++ )  args[i] = (char*)string_args[i].c_str();
            args[i] = NULL;

            execvp( _job->_process_filename.c_str(), args );

            fprintf( stderr, "ERRNO-%d  %s, bei execlp(\"%s\")\n", errno, strerror(errno), _job->_process_filename.c_str() );
            _exit( errno? errno : 250 );  // Wie melden wir den Fehler an den rufenden Prozess?
        }

        default:
            LOG( "pid=" << pid << "\n" );
            _process_handle._pid = pid;
            break;
    }


    set_state( s_running_process );

    _process_handle.set_name( "Process " + _job->_process_filename );
    _process_handle.add_to( &_job->_thread->_wait_handles );

    return true;
}

//----------------------------------------------------------------------------Process_task::do_stop

void Process_task::do_stop()
{
    if( _process_handle._pid )
    {
        _log.warn( "Prozess wird abgebrochen" );

        LOG( "kill(" << _process_handle._pid << ",SIGTERM)\n" );
        int err = kill( _process_handle._pid, SIGTERM );
        if( err )  throw_errno( errno, "killpid" );

        //? _process_handle._pid = 0;
    }
}

//-----------------------------------------------------------------------------Process_task::do_end

void Process_task::do_end()
{
    if( _process_handle._pid )
    {
        do_step();      // waitpid() sollte schon gerufen sein. 

        if( _process_handle._pid )   throw_xc( "SPOOLER-179", _process_handle._pid );       // Sollte nicht passieren (ein Zombie wird stehen bleiben)
    }

    _process_handle.close();

    if( _process_handle._process_signaled )  throw_xc( "SPOOLER-181", _process_handle._process_signaled );

    _result = (int)_process_handle._process_exit_code;

    _log.log_file( _job->_process_log_filename ); 

    if( _process_handle._process_exit_code )  throw_xc( "SPOOLER-126", _process_handle._process_exit_code );
}

//---------------------------------------------------------------------------Process_task::signaled

bool Process_task::signaled()
{
    _process_handle.wait( 0 );
    return _process_handle.signaled();
}

#endif

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
