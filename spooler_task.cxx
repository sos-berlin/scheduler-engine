// $Id: spooler_task.cxx,v 1.165 2003/08/02 20:23:36 jz Exp $
/*
    Hier sind implementiert

    Spooler_object
    Object_set
    Task
*/


/*
    ÄNDERUNGEN DES VERHALTENS, SEIT MEHRERE TASKS EINES JOBS ERLAUBT SIND

    Job::stop(): Tasks laufen zu Ende, neue Tasks nicht erlaubt. Zustände s_stopping und s_stopped

    Job::reread(): Skript wird neu gelesen, gilt aber erst für neue Tasks. Laufende Tasks werden nicht beendet.

    Task::stop(): Task wird gestoppt, Job::stop() wird gerufen

    Job::reset_error() gestrichen. Wann wird der Fehler in Job zurückgesetzt? Gibt es überhaupt einen Fehler in Job? Nur im Zustand s_stopped?

    <task> -> <job><tasks><task>...
    


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

static const string spooler_init_name       = "spooler_init()Z";
static const string spooler_open_name       = "spooler_open()Z";
static const string spooler_close_name      = "spooler_close()V";
static const string spooler_get_name        = "spooler_get";
static const string spooler_process_name    = "spooler_process()Z";
static const string spooler_level_name      = "spooler_level";
static const string spooler_on_error_name   = "spooler_on_error()V";
static const string spooler_on_success_name = "spooler_on_success()V";

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

Spooler_thread* Object_set::thread() const
{ 
    return _task->_job->_thread; 
}

//-----------------------------------------------------------------------------------------Job::Job

Job::Job( Spooler_thread* thread )
: 
    _zero_(this+1),
    _thread(thread),
    _spooler(thread->_spooler),
    _log(thread->_spooler),
    _module(thread->_spooler,&_log),
    _history(this)
{
    _next_time = latter_day;
    _priority  = 1;
    _default_params = new Com_variable_set;
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

        _name             = element.     getAttribute( "name" );
        _temporary        = element.bool_getAttribute( "temporary"  , _temporary  );
        _priority         = element. int_getAttribute( "priority"   , _priority   );
        _title            = element.     getAttribute( "title"      , _title      );
        _log_append       = element.bool_getAttribute( "log_append" , _log_append );
        order             = element.bool_getAttribute( "order"      );
        

        if( order )
        {
            if( _temporary )  throw_xc( "SPOOLER-155" );
            if( element.getAttributeNode( "priority" ) )  throw_xc( "SPOOLER-165" );
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
                                                 _process_filename     = e.getAttribute( "file" ),
                                                 _process_param        = e.getAttribute( "param" ),
                                                 _process_log_filename = e.getAttribute( "log_file" );
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

    _log.set_prefix( obj_name() );
    _log.set_profile_section( profile_section() );
    _log.set_job( this );

    _com_job  = new Com_job( this );
  //_com_log  = new Com_log( &_log );
  //_com_task = new Com_task();

    set_state( s_pending );
}

//----------------------------------------------------------------------------------------Job::init
// Bei <add_jobs> von einem anderen Thread gerufen.

void Job::init()
{
    _history.open();

    if( _module_xml_element )  read_script();

    _module_ptr = _object_set_descr? &_object_set_descr->_class->_module
                                   : &_module;
/*
    if( _process_filename.empty() )
    {
        _module_instance = _module_ptr->create_instance();
        _module_instance->set_title( "Job " + _name );
    }
*/

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

    set_next_start_time();
}

//---------------------------------------------------------------------------------------Job::close

void Job::close()
{
    //close_task();

    clear_when_directory_changed();

    //close_engine();
    //_module_instance = NULL;

    Z_FOR_EACH( Task_list, _running_tasks, t )
    {
        Task* task = *t;
        task->close();
    }

    _running_tasks.clear();


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
  //if( _com_task )  _com_task->set_task(NULL), _com_task = NULL;
  //if( _com_log  )  _com_log->close(),         _com_log  = NULL;

    _task_queue.clear();
}

//----------------------------------------------------------------------------------Job::close_task
/*
void Job::close_task()
{
    if( _task )  _task->close();

    THREAD_LOCK( _lock )
    {
        _task = NULL; 
    }

    if( _close_engine  ||  _module_ptr && _module_ptr->_reuse == Module::reuse_task ) 
    {
        close_engine();
        _close_engine = false;
    }
}
*/
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

//--------------------------------------------------------------------------------------Job::signal

void Job::signal( const string& signal_name )
{ 
    _next_time = 0;  
    _thread->signal( signal_name ); 
}

//---------------------------------------------------------------------------Job::set_error_xc_only

void Job::set_error_xc_only( const Xc& x )
{
    THREAD_LOCK( _lock )  _error = x;
}

//--------------------------------------------------------------------------------Job::set_error_xc

void Job::set_error_xc( const Xc& x )
{
    string msg; 
  //if( !_in_call.empty() )  msg = "In " + _in_call + "(): ";
    
    _log.error( msg + x.what() );

    set_error_xc_only( x );

    _repeat = 0;
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

//---------------------------------------------------------------------------------Job::create_task

Sos_ptr<Task> Job::create_task( const ptr<spooler_com::Ivariable_set>& params, const string& name, Time start_at )
{
    Sos_ptr<Task> task;

    if( !_process_filename.empty() )   task = SOS_NEW( Process_task( _spooler, this ) );
    else
    if( _object_set_descr          )   task = SOS_NEW( Object_set_task( _spooler, this ) );
    else                             
                                       task = SOS_NEW( Job_module_task( _spooler, this ) );

    Time now = Time::now();
    task->_enqueue_time = now;
    task->_id           = _spooler->_db->get_task_id();

    _default_params->Clone( (spooler_com::Ivariable_set**)task->_params.pp() );
    if( params )   task->_params->merge( params );

    task->_name         = name;
    task->_start_at     = start_at; 
    
    THREAD_LOCK( _lock )
    {
        Task_queue::iterator it = _task_queue.begin();  // _task_queue nach _start_at geordnet halten
        while( it != _task_queue.end()  &&  (*it)->_start_at <= task->_start_at )  it++;
        _task_queue.insert( it, task );

        calculate_next_time( now );
    }

    return task;
}

//--------------------------------------------------------------------------------Job::dequeue_task

Sos_ptr<Task> Job::dequeue_task( Time now )
{
    Sos_ptr<Task> task;

    if( _state == s_read_error )  return NULL;

    THREAD_LOCK( _lock )
    {
        if( _task_queue.empty() )     return NULL;

        bool                 in_period = is_in_period(now);
        Task_queue::iterator it        = _task_queue.begin();
        
        for( ; it != _task_queue.end(); it++ )
        {
            task = *it;
            if(  task->_start_at  &&  task->_start_at <= now )  break;        // Task mit Startzeitpunkt
            if( !task->_start_at  &&  in_period              )  break;        // Task ohne Startzeitpunkt
        }

        if( it == _task_queue.end() )  return NULL;

        _running_tasks.push_back( task );
        it = _task_queue.erase( it );

        reset_error();
        _repeat = 0;

        if( _module_ptr->_reuse == Module::reuse_job )  
        {
            //_close_engine = false;
            //_com_task->set_task( _task );
        }

        task->set_state( Task::s_start_task );
    }

    return task;
}

//----------------------------------------------------------------------Job::remove_from_task_queue

void Job::remove_from_task_queue( Task* task )
{
    THREAD_LOCK( _lock )
    FOR_EACH( Task_queue, _task_queue, it )  
    {
        if( +*it == task )  
        {
            _log( task->obj_name() + " aus der Warteschlange entfernt" );
            it = _task_queue.erase( it );
            break;
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
        case s_read_error:  throw_xc( "SPOOLER-132", name(), _error? _error->what() : "" );
        case s_stopped:     set_state( s_pending );
        default: ;
    }

    if( !start_at  &&  !_run_time.period_follows( Time::now() ) )   throw_xc( "SPOOLER-143" );

    Sos_ptr<Task> task = create_task( params, task_name, start_at );
    task->_let_run = true;

    _thread->signal( "start job" );

    return task;
}

//---------------------------------------------------------------------------------Job::read_script

bool Job::read_script()
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

    return true;
}

//-----------------------------------------------------------------------------------------Job::end
/*
void Job::end()
{
    if( !_state )  return;

    set_mail_defaults();      // Vor spooler_on_error() bzw. spooler_on_success(); eMail wird in finish() verschickt


    if( _state == s_suspended  
     || _state == s_running_delayed 
     || _state == s_running_waiting_for_order )  set_state( s_running );
    
    if( _state == s_start_task
     || _state == s_starting
     || _state == s_running  
     || _state == s_running_process )  
    {
        if( _task )  _task->end();
    }
    
    close_task();

    if( _state != s_stopped )  set_state( s_ended );
}
*/
//----------------------------------------------------------------------------------------Job::stop

void Job::stop( bool end_all_tasks )
{
    if( _state != s_stopping  &&  _state != s_stopped )
    {
/*
        try 
        {
            if( _task )  _task->do_stop();
        }
        catch( const exception& x )  { set_error(x); }
*/
    }


    if( end_all_tasks )
    {
        Z_FOR_EACH( Task_list, _running_tasks, t )
        {
            Task* task = *t;
            task->cmd_end();
        }
    }

    set_state( _running_tasks.size() > 0? s_stopping : s_stopped );

  //end();
  //close_engine();

    // Kein Signal mehr soll kommen, wenn Job gestoppt:
    //_event.close();
    clear_when_directory_changed();
}

//--------------------------------------------------------------------------------------Job::reread

void Job::reread()
{
    //_close_engine = true;
    //close_task();

    _log( "Skript wird erneut gelesen (<include> wird erneut ausgeführt)" );
    read_script();

    //set_state( s_pending );
    //init2();
}

//---------------------------------------------------------------------------Job::execute_state_cmd

bool Job::execute_state_cmd()
{
    bool something_done = false;

    //THREAD_LOCK( _lock )
    {
        if( _state_cmd )
        {
            switch( _state_cmd )
            {
                case sc_stop:       if( _state != s_stopping
                                     && _state != s_stopped  
                                     && _state != s_read_error )  stop( true ),                something_done = true;
                                    break;

                case sc_unstop:     if( _state == s_stopped    )  set_state( s_pending ),      something_done = true,  set_next_start_time();
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
                                        create_task( NULL, "", now );
                                        Sos_ptr<Task> task = dequeue_task( now );
                                        task->_cause = cause_wake;
                                        task->_let_run = true;
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
        dw->add_to( &_thread->_wait_handles );
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

//-------------------------------------------------------------------------------Job::calculate_next_time
// Für Spooler_thread

void Job::calculate_next_time( Time now )
{
    THREAD_LOCK( _lock )
    {
        Time next_time = latter_day;

        if( _state != s_stopped )
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

                if( _state != s_stopping  &&  _spooler->state() != Spooler::s_stopping_let_run )
                {
                    if( next_time > _next_start_time   )  next_time = _next_start_time;
                    if( next_time > _period.end()      )  next_time = _period.end();          // Das ist, wenn die Periode weder repeat noch single_start hat, also keinen automatischen Start
                    if( next_time > _next_single_start )  next_time = _next_single_start;
                }

                // Gesammelte eMail senden, wenn collected_max erreicht:
                Time log_time = _log.collect_end();
                if( log_time > now  &&  next_time > log_time )  next_time = log_time;


                Z_FOR_EACH( Task_list, _running_tasks, t )
                {
                    Task* task = *t;
                    switch( task->_state )
                    {
                        case Task::s_running_delayed:
                            next_time = min( next_time, task->_next_spooler_process );
                            break;

                        case Task::s_running_waiting_for_order:
                            next_time = min( next_time, _order_queue->next_time() );
                            next_time = min( next_time, _period.end() );                // Thread am Ende der Run_time wecken, damit Task beendet werden kann
                            break;

                        default: ;
                    }
                }
            }
        }

        _next_time = next_time;
    }
}

//-------------------------------------------------------------------------------Job::set_next_time
/*
void Job::set_next_time( Time time )
{
    THREAD_LOCK( _lock )
    {
        _next_time = min( _next_time, time );
    }
}
*/
//-------------------------------------------------------------------------------Job::select_period

void Job::select_period( Time now )
{
    if( now >= _period.end() )       // Periode abgelaufen?
    {
        _period = _run_time.next_period(now);  

        if( _period.begin() != latter_day )
        {
            string rep; if( _period._repeat != latter_day )  rep = _period._repeat.as_string();
            _log.debug( "Nächste Periode ist <period begin=\"" + _period.begin().as_string() + "\" end=\"" + _period.end().as_string() + "\" repeat=\"" + rep + "\">" );
        }
        else 
            _log.debug( "Keine weitere Periode" );

        if( _period.has_start() )
        {
            _next_start_time = max( now, _period.begin() );
            //_log.debug( "Nächster Start " + _next_start_time.as_string() );
        }
        else
            _next_start_time = latter_day;
    }

    calculate_next_time( now );
}

//--------------------------------------------------------------------------------Job::is_in_period

bool Job::is_in_period( Time now )
{
    return now >= _delay_until  &&  now >= _period.begin()  &&  now < _period.end();
}

//-------------------------------------------------------------------------Job::set_next_start_time

void Job::set_next_start_time( Time now )
{
    string msg;

    if( _delay_until )
    {
        _next_start_time = _period.next_try( _delay_until );
        if( _spooler->_debug )  msg = "Wiederholung wegen delay_after_error: " + _next_start_time.as_string();
    }
    else
    if( _repeat > 0 )       // spooler_task.repeat
    {
        _next_start_time = _period.next_try( now + _repeat );
        if( _spooler->_debug )  msg = "Wiederholung wegen spooler_job.repeat=" + as_string(_repeat) + ": " + _next_start_time.as_string();
        _repeat = 0;
    }
    else
    {
        //31.5.03 _next_start_time = _period.next_try( now );
        _next_start_time = _period.next_try( now + _period.repeat() );
        if( _spooler->_debug && _next_start_time != latter_day )  msg = "Nächste Wiederholung wegen <period repeat=\"" + as_string((double)_period._repeat) + "\">: " + _next_start_time.as_string();

        _next_single_start = _run_time.next_single_start( now );
        if( _spooler->_debug && _next_single_start < _next_start_time )  msg = "Nächster single_start " + _next_single_start.as_string();
    }

    if( _next_start_time > _period.end()  ||  _next_start_time == latter_day ) 
    {
        if( now < _period.begin() )     // Nächste Periode hat noch nicht begonnen?
        {
            _next_start_time = _period.begin();
            if( _spooler->_debug )  msg = "Nächster Start zu Beginn der Periode: " + _next_start_time.as_string();
        }
        else  
        if( now >= _period.end() )
        {
            select_period( now );
        }
    }

    if( !msg.empty() )  _log.debug( msg );

    calculate_next_time( now );
}

//-------------------------------------------------------------------------------Job::task_to_start

Task* Job::task_to_start()
{
    Time        now   = Time::now();
    Start_cause cause = cause_none;
    Task*       dequeued_task = NULL;

    if( _spooler->state() == Spooler::s_stopping_let_run )  return NULL;

    if( _state == s_pending )
    {
        dequeued_task = dequeue_task(now);
        if( dequeued_task )  dequeued_task->_cause = dequeued_task->_start_at? cause_queue_at : cause_queue;
            
        if( now >= _next_single_start )  cause = cause_period_single;     
                                   else  select_period(now);

        if( cause                      // Auf weitere Anlässe prüfen und diese protokollieren
            || is_in_period(now) )
        {
            THREAD_LOCK( _lock )
            {
                if( _start_once )              cause = cause_period_once,  _start_once = false,     _log.debug( "Task startet wegen <run_time once=\"yes\">" );
                                                                            
                if( now >= _next_start_time )  cause = cause_period_repeat,                         _log.debug( "Task startet, weil Job-Startzeit erreicht: " + _next_start_time.as_string() );

                for( Directory_watcher_list::iterator it = _directory_watcher_list.begin(); it != _directory_watcher_list.end(); it++ )
                {
                    if( (*it)->signaled_then_reset() )
                    {
                        cause = cause_directory;
                        _log.debug( "Task startet wegen eines Ereignisses für Verzeichnis " + (*it)->directory() );
                        if( !(*it)->valid() )  it = _directory_watcher_list.erase( it ),  it--;  // Folge eines Fehlers, s. Directory_watcher::set_signal
                    }
                }

                if( !cause && _order_queue )
                {
                    ptr<Order> order = _order_queue->first_order( now );
                    if( order )                cause = cause_order,                                 _log.debug( "Task startet wegen Auftrag " + order->obj_name() );
                }
                                                                                    
                if( !dequeued_task && cause )
                {
                    create_task( NULL, "", now );
                    dequeued_task = dequeue_task( now );
                    dequeued_task->_cause = cause;
                    dequeued_task->_let_run |= ( cause == cause_period_single );
                }
            }
        }
    }

    return dequeued_task;
}

//--------------------------------------------------------------------------------Job::do_something

bool Job::do_something()
{
    Z_DEBUG_ONLY( _log.debug9( "do_something() state=" + state_name() ); )

    if( !_state )  return false;

    bool something_done = false;       
    bool ok             = true;
    bool do_a_step      = false;

    something_done = execute_state_cmd();

  //if( _reread  &&  !_task )  _reread = false,  reread(),  something_done = true;
    if( _reread )  _reread = false,  reread(),  something_done = true;

    if( _state == s_pending )  
    {
        Task* task = task_to_start();
        if( task )
        {
            _log.open();           // Jobprotokoll, nur wirksam, wenn set_filename() gerufen, s. Job::init().
            _delay_until = 0;
          //_last_task_step_count = 0;

            set_state( s_running );
        }
    }


    Z_FOR_EACH( Task_list, _running_tasks, t )
    {
        Task* task = *t;

        something_done |= task->do_something();

        switch( task->_state )
        {
            case Task::s_running_delayed:
                //set_next_time( task->_next_spooler_process );
                calculate_next_time( Time::now() );
                break;

            case Task::s_running_waiting_for_order:
                //set_next_time( min( _period.end(), _order_queue->next_time() ) );     // Thread am Ende der Run_time wecken, damit Task beendet werden kann
                calculate_next_time( Time::now() );
                break;

            case Task::s_ended:
            {
                THREAD_LOCK( _lock )  t = _running_tasks.erase( t ), t--;

                if( _running_tasks.empty() )
                {
                    if( _state == s_stopping )  
                    {
                        set_state( s_stopped );
                    }
                    else
                    {
                        set_next_start_time();
                        set_state( s_pending );
                    }
                }

                break;
            }
        }
    }

/*
    // Wenn nichts zu tun ist, dann raus. Der Job soll nicht wegen eines alten Fehlers nachträglich gestoppt werden (s.u.)
    if( _state == s_pending    )  goto ENDE;
  //if( _state == s_suspended  )  goto ENDE;
    if( _state == s_stopped    )  goto ENDE;
    if( _state == s_read_error )  goto ENDE;
  //if( _state == s_running_process  &&  !((Process_task*)+_task)->signaled() )  goto ENDE;

ENDE:
*/
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
                                _thread->signal( state_cmd_name(cmd) );
                                break;

            case sc_unstop:     ok = _state == s_stopped;       if( !ok )  return;
                                _state_cmd = cmd;
                                _thread->signal( state_cmd_name(cmd) );
                                break;

            case sc_start:      {
                                    THREAD_LOCK( _lock )  start_without_lock( NULL, "", Time::now(), true );
                                    break;
                                }

            case sc_wake:       _state_cmd = cmd;
                                _thread->signal( state_cmd_name(cmd) );
                                break;

            case sc_end:        ok = true; // _state == s_running || _state == s_running_delayed || _state == s_running_waiting_for_order || _state == s_suspended;  if( !ok )  return;
                                _state_cmd = cmd;
                                break;

            case sc_suspend:    ok = true; //_state == s_running || _state == s_running_delayed || _state == s_running_waiting_for_order;   if( !ok )  return;
                                _state_cmd = cmd;
                                break;

            case sc_continue:   ok = true; //_state == s_suspended || _state == s_running_delayed;  if( !ok )  return;
                                _state_cmd = cmd;
                                _thread->signal( state_cmd_name(cmd) );
                                break;

            case sc_reread:     _reread = true, ok = true;  
                                _thread->signal( state_cmd_name(cmd) );
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
    return _thread->include_path(); 
}

//----------------------------------------------------------------------------------Job::state_name

string Job::state_name( State state )
{
    switch( state )
    {
        case s_stopping:        return "stopping";
        case s_stopped:         return "stopped";
        case s_read_error:      return "read_error";
      //case s_load_error:      return "load_error";
      //case s_loaded:          return "loaded";
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

    if( !name.empty() )  throw_xc( "SPOOLER-110", name );
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

    if( !name.empty() )  throw_xc( "SPOOLER-106", name );
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
        job_element.setAttribute( "title"     , _title                  );
        job_element.setAttribute( "all_steps" , _step_count             );
        job_element.setAttribute( "state_text", _state_text             );
        job_element.setAttribute( "log_file"  , _log.filename()         );
        job_element.setAttribute( "order"     , _order_queue? "yes" : "no" );
        
        if( _state_cmd        )  job_element.setAttribute( "cmd"    , state_cmd_name()  );

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


        xml::Element_ptr tasks_element = document.createElement( "tasks" );
        Z_FOR_EACH( Task_list, _running_tasks, t )  tasks_element.appendChild( (*t)->dom( document, show ) ), dom_append_nl( tasks_element );
        job_element.appendChild( tasks_element );


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
                    
                    if( _next_time != old_next_time )  _thread->signal( "task killed" );
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

//--------------------------------------------------------------------Job::get_free_module_instance

Module_instance* Job::get_free_module_instance( Task* task )
{
    Z_FOR_EACH( Module_instance_vector, _module_instances, m )
    {
        if( !*m )
        {
            *m = _module_ptr->create_instance();
        }

        if( !(*m)->_task ) 
        { 
            (*m)->_task = task; 
            return *m;
        }
    }

    throw_xc( "get_free_module_instance" );
}

//---------------------------------------------------------------------Job::release_module_instance

void Job::release_module_instance( Module_instance* module_instance )
{
    Z_FOR_EACH( Module_instance_vector, _module_instances, m )
    {
        if( *m == module_instance )
        {
            *m = NULL; 
            break;
        }
    }

    throw_xc( "release_module_instance" );
}

//---------------------------------------------------------------------------------------Task::Task

Task::Task( Spooler* spooler, const Sos_ptr<Job>& job )    
: 
    _zero_(this+1), 
    _spooler(spooler), 
    _job(job),
    _log(spooler),
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

//---------------------------------------------------------------------------------Task::set_in_call

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

    _job->_repeat = 0;
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

void Task::set_state( State new_state )
{ 
    THREAD_LOCK( _lock )  
    {
        if( new_state == _state )  return;

        if( new_state == s_running )  InterlockedIncrement( &_job->_running_tasks_count ),  InterlockedIncrement( &_job->_thread->_running_tasks_count );
        if( _state    == s_running )  InterlockedDecrement( &_job->_running_tasks_count ),  InterlockedDecrement( &_job->_thread->_running_tasks_count );
      //if( new_state == s_running  ||  new_state == s_start_task )  InterlockedIncrement( &_job->_running_tasks_count ),  InterlockedIncrement( &_job->_thread->_running_tasks_count );
      //if( _state    == s_running  ||  _state    == s_start_task )  InterlockedDecrement( &_job->_running_tasks_count ),  InterlockedDecrement( &_job->_thread->_running_tasks_count );

        if( new_state != s_running_delayed )  _next_spooler_process = 0;

        _state = new_state;


        if( _spooler->_debug )
        {
            if( new_state == s_starting ) 
            {
                string msg = state_name();
                if( _start_at )  msg += " (at=" + _start_at.as_string() + ")";
                _log.debug( "state=" + msg );
            }
            else
            if( new_state == s_ended )  _log.debug( "state=" + state_name() ); 
                                  else  _log.debug9( "state=" + state_name() );
        }
    }
}

//---------------------------------------------------------------------------------Task::state_name

string Task::state_name( State state )
{
    switch( state )
    {
      //case s_stopped:         return "stopped";
      //case s_read_error:      return "read_error";
      //case s_load_error:      return "load_error";
      //case s_loaded:          return "loaded";
      //case s_pending:         return "pending";
        case s_start_task:      return "start_task";
        case s_starting:        return "starting";
        case s_running:         return "running";
        case s_running_delayed: return "running_delayed";
        case s_running_waiting_for_order: return "running_waiting_for_order";
        case s_running_process: return "running_process";
        case s_suspended:       return "suspended";
        case s_ending:          return "ending";
        case s_ended:           return "ended";
        default:                return as_string( (int)state );
    }
}

//---------------------------------------------------------------------------------Task::task_state
// Anderer Thread

string Task::task_state()
{
    string st;

    THREAD_LOCK( _lock )
    {
        st = "state=" + state_name();
        if( !_in_call.empty() )  st += " in " + _in_call + "()";
    }

    return st;
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

//-------------------------------------------------------------------------------Task::do_something

bool Task::do_something()
{
    Z_DEBUG_ONLY( _log.debug9( "do_something() state=" + state_name() ); )

    bool something_done = false;
    bool ok             = true;
    bool do_a_step      = false;


    // Wenn nichts zu tun ist, dann raus. Der Job soll nicht wegen eines alten Fehlers nachträglich gestoppt werden (s.u.)
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
            if( !_order )  _order = _job->_order_queue->get_order_for_processing( now, this );

            if( _order )  set_state( s_running );            // Auftrag da? Dann Task weiterlaufen lassen (Ende der Run_time wird noch geprüft)
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
             || _state == s_running_process )  end(), something_done = true;

            if( has_error()  &&  _job->_repeat == 0  &&  _job->_delay_after_error.empty() )  stop(), something_done = true;

            finish();
        }


        if( _state == s_running  &&  _next_spooler_process )
        {
            set_state( s_running_delayed );
        }

        if( _state == s_running  &&  _job->_order_queue  &&  !_job->_order_queue->has_order( now ) )
        {
            set_state( s_running_waiting_for_order );  
        }

        if( _state == s_ended )                                                                     // TASK BEENDET
        {
            if( _job->_temporary  &&  _job->_repeat == 0 )  
            {
                stop();   // _temporary && s_stopped ==> spooler_thread.cxx entfernt den Job
                finish();
            }

            something_done = true;
        }
    }


ENDE:
    if( _state != s_running  
     && _state != s_running_delayed
     && _state != s_running_waiting_for_order
     && _state != s_running_process  
     && _state != s_suspended        )  send_collected_log();

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

    string body = Sos_optional_date_time::now().as_string() + "\n\nJob " + _job->_name + "  " + _job->_title + "\n";
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
            InterlockedIncrement( &_job->_thread->_task_count );
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


    // Bei mehreren aufeinanderfolgenden Fehlern Wiederholung verzögern?

    if( has_error() )
    {
        InterlockedIncrement( &_job->_error_steps );
    
        if( !_job->_repeat )   // spooler_task.repeat hat Vorrang
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
        THREAD_LOCK( _job->_lock )
        {
            if( _job->_order_queue )
            {
                if( !_order )  _order = _job->_order_queue->get_order_for_processing( Time::now(), this );
                if( !_order )  return true;

                _log.set_order_log( &_order->_log );
            }
        }

        result = do_step();

        if( _next_spooler_process )  result = true;

        if( has_step_count()  ||  _step_count == 0 )        // Bei Process_task nur einen Schritt zählen
        {
            InterlockedIncrement( &_job->_thread->_step_count );
            InterlockedIncrement( &_job->_step_count );
            _step_count++;
          //_job->_last_task_step_count = _step_count;
        }


        if( _order )
        {
            _order->postprocessing( result );
            _log.set_order_log( NULL );
            _order = NULL;
            result = true;
        }
    }
    catch( const exception& x ) { _job->set_error(x); result = false; }

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
    Sos_ptr<Job> job = _job;        // this wird vielleicht gelöscht

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
    if( my_thread_id == _job->_thread->thread_id() )  throw_xc( "SPOOLER-125" );     // Deadlock

    Spooler_thread* calling_thread = _spooler->thread_by_thread_id( my_thread_id );
    if( calling_thread &&  !calling_thread->_free_threading                                      )  throw_xc( "SPOOLER-131" );

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
        module_instance->add_obj( (IDispatch*)_job->_thread->_com_thread, "spooler_thread" );
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


  //set_state( s_loaded );

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

bool Job_module_task::do_step()
{
    //if( !_module_instance->_has_spooler_process )  return false;
    if( !_module_instance->name_exists( spooler_process_name ) )  return false;

    bool ok = false;

    switch( _call_state )
    {
        case c_none:            
            _module_instance->call_async( spooler_process_name );
            break;

        case c_spooler_process: 
            ok = check_result( _module_instance->call_wait() );
            break;

        default:
            throw_xc( "call_state", _call_state );
    }

  //Module_instance::In_call in_call ( this, spooler_process_name, _order? "Auftrag " + _order->obj_name() : "" );
    bool ok = check_result( _module_instance->call( spooler_process_name ) );
    in_call.set_result( ok );
    return ok;
}

//-----------------------------------------------------------------------Process_task::Process_task

Process_task::Process_task( Spooler* sp, const Sos_ptr<Job>& j ) 
: 
    Task(sp,j),
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
    _process_handle.add_to( &_job->_thread->_wait_handles );

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
    // waitpid() rufen, falls noch nicht geschehen (um Zombie zu schließen)

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

      //LOG( "Prozess läuft noch\n" );

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
        signal( SIGCHLD, SIG_DFL );                 // Java verändert das Signal-Verhalten, so dass waitpid() ohne diesen Aufruf versagte.
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
