// $Id: spooler_task.cxx,v 1.76 2002/04/05 22:14:39 jz Exp $
/*
    Hier sind implementiert

    Spooler_object
    Object_set
    Task
*/



#include "../kram/sos.h"
#include "spooler.h"


namespace sos {
namespace spooler {

//--------------------------------------------------------------------------------------------const

static const char spooler_init_name      [] = "spooler_init";
static const char spooler_open_name      [] = "spooler_open";
static const char spooler_close_name     [] = "spooler_close";
static const char spooler_get_name       [] = "spooler_get";
static const char spooler_process_name   [] = "spooler_process";
static const char spooler_level_name     [] = "spooler_level";
static const char spooler_on_error_name  [] = "spooler_on_error";
static const char spooler_on_success_name[] = "spooler_on_success";

//----------------------------------------------------------------------------Spooler_object::level

Level Spooler_object::level()
{
    CComVariant level = com_property_get( _idispatch, spooler_level_name );
    level.ChangeType( VT_INT );

    return level.intVal;
}

//--------------------------------------------------------------------------Spooler_object::process

void Spooler_object::process( Level output_level )
{
    com_call( _idispatch, spooler_process_name, output_level );
}

//---------------------------------------------------------------------------Object_set::Object_set

Object_set::Object_set( Spooler* spooler, Task* task, const Sos_ptr<Object_set_descr>& descr ) 
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
    CComVariant object_set_vt;

    if( _class->_object_interface )
    {
        Job::In_call( _task, "spooler_make_object_set" );
        object_set_vt = _task->_job->_script_instance.call( "spooler_make_object_set" );

        if( object_set_vt.vt != VT_DISPATCH 
         || object_set_vt.pdispVal == NULL  )  throw_xc( "SPOOLER-103", _object_set_descr->_class_name );

        _idispatch = object_set_vt.pdispVal;
    }
    else
    {
        _idispatch = _task->_job->_script_instance._script_site->dispatch();
    }

    if( com_name_exists( _idispatch, spooler_open_name ) ) 
    {
        Job::In_call in_call ( _task, spooler_open_name );
        ok = check_result( com_call( _idispatch, spooler_open_name ) );
        in_call.set_result( ok );
    }
    else  
        ok = true;

    return ok && !_task->_job->has_error();
}

//--------------------------------------------------------------------------------Object_set::close

void Object_set::close()
{
    if( com_name_exists( _idispatch, spooler_close_name ) )  
    {
        Job::In_call in_call ( _task, spooler_close_name );
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
        Job::In_call in_call ( _task, spooler_get_name );
        CComVariant obj = com_call( _idispatch, spooler_get_name );

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

        if( _task->_job->has_error() )  return false;       // spooler_task.error() gerufen?

        Job::In_call in_call ( _task, spooler_process_name );
        object.process( result_level );

        return true;
    }
    else
    {
        Job::In_call in_call ( _task, spooler_process_name );
        bool result = check_result( _task->_job->_script_instance.call( spooler_process_name, result_level ) );
        in_call.set_result( result );
        return result;
    }
}

//-------------------------------------------------------------------------------Object_set::thread

Thread* Object_set::thread() const
{ 
    return _task->_job->_thread; 
}

//----------------------------------------------------------------------------Job::In_call::In_call

Job::In_call::In_call( Job* job, const string& name ) 
: 
    _job(job),
    _name(name),
    _result_set(false)
{ 
    _job->set_in_call(name); 
    LOG( *job << '.' << name << "() begin\n" );

    _ASSERTE( _CrtCheckMemory( ) );
}

//----------------------------------------------------------------------------Job::In_call::In_call

Job::In_call::In_call( Task* task, const string& name ) 
: 
    _job(task->job()),
    _name(name),
    _result_set(false)
{ 
    _job->set_in_call(name); 
    LOG( *task->job() << '.' << name << "() begin\n" );

    _ASSERTE( _CrtCheckMemory( ) );
}

//---------------------------------------------------------------------------Job::In_call::~In_call

Job::In_call::~In_call()
{ 
    _job->set_in_call( "" ); 

    if( log_ptr )
    {
        *log_ptr << *_job << '.' << _name << "() end";
        if( _result_set )  *log_ptr << "  result=" << ( _result? "true" : "false" );
        *log_ptr << '\n';
    }

    _ASSERTE( _CrtCheckMemory( ) );
}

//-----------------------------------------------------------------------------------------Job::Job

Job::Job( Thread* thread )
: 
    _zero_(this+1),
    _thread(thread),
    _spooler(thread->_spooler),
    _log(thread->_spooler),
    _script(thread->_spooler),
    _script_instance(&_log),
    _history(this)
{
    _next_time = latter_day;
}

//----------------------------------------------------------------------------------------Job::~Job

Job::~Job()
{
    close();
}

//---------------------------------------------------------------------------------------Job::close

void Job::close()
{
    close_task();

    _log.close();
    _event.close();
    clear_when_directory_changed();

    close_engine();

    _history.close();

    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_job  )  _com_job->close(),         _com_job  = NULL;
    if( _com_task )  _com_task->set_task(NULL), _com_task = NULL;
    if( _com_log  )  _com_log->close(),         _com_log  = NULL;

    _task_queue.clear();
}

//--------------------------------------------------------------------------------Job::close_engine

void Job::close_engine()
{
    _com_task = new Com_task();

    try 
    {
        _script_instance.close();
    }
    catch( const Xc& x        ) { set_error(x); }
    catch( const exception& x ) { set_error(x); }
}

//----------------------------------------------------------------------------------Job::close_task

void Job::close_task()
{
    if( _task )  _task->close();

    THREAD_LOCK( _lock )
    {
        _task = NULL; 
      //if( _state != s_stopped )  set_state( s_ended );
    }

    //if( _load_error  ||  _script_ptr  &&  _script_ptr->_reuse == Script::reuse_task )  close_engine();
    if( _close_engine  || _script_ptr  &&  _script_ptr->_reuse == Script::reuse_task ) 
    {
        close_engine();
        _close_engine = false;
    }
}

//----------------------------------------------------------------------------------------Job::init
// Bei <add_jobs> von einem anderen Thread gerufen.

void Job::init()
{
    _state = s_none;

    _log.set_prefix( obj_name() );
    _log.set_profile_section( profile_section() );
    _log.set_jobname( _name );

    _event.set_name( obj_name() );
    _event.add_to( &_thread->_wait_handles );

    _com_job  = new Com_job( this );
    _com_log  = new Com_log( &_log );
    _com_task = new Com_task();

    _history.open();

    set_state( s_pending );

    if( _script_xml_element )  read_script();

    _script_ptr = _object_set_descr? &_object_set_descr->_class->_script
                                   : &_script;


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
    _start_once = _run_time.once();
    _delay_until = 0;
    _period._begin = 0;
    _period._end = 0;

    select_period();
}

//-------------------------------------------------------------------------------Job::select_period
/*ALT
void Job::select_period( Time now )
{
    if( now > _period.end() )
    {
        _period = _run_time.next_period(now);  _log.debug( "Nächste Periode beginnt " + _period.begin().as_string() );

        _next_start_time = _period.has_start()? max( now, _period.begin() )
                                              : latter_day;
 
        if( _next_start_time != latter_day )
        {
            _log.debug( "Nächster Start " + _next_start_time.as_string() );
            _next_time = _next_start_time;
        }
        else
        {
            _next_time = min( _next_start_time, _period.begin() );
        }
    }
    else
    if( _next_time == latter_day
     || _next_time < now         )
    {
        _next_time = _next_time < _next_start_time? _next_time = _next_start_time
                                                  : _run_time.next_period(_period.end()).begin();
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

    set_next_time( now );
}

//--------------------------------------------------------------------------------Job::is_in_period

bool Job::is_in_period( Time now )
{
    //select_period( now );
    return now >= _delay_until  &&  now >= _period.begin()  &&  now < _period.end();
}

//-------------------------------------------------------------------------------Job::set_next_time
// Für Spooler_thread

void Job::set_next_time( Time now )
{
    _next_time = _next_start_time;

    // Minimum von _start_at für _next_time berücksichtigen
    Task_queue::iterator it = _task_queue.begin();  
    while( it != _task_queue.end()  &&  !(*it)->_start_at )  it++;
    if( it != _task_queue.end()  &&  _next_time > (*it)->_start_at )  _next_time = (*it)->_start_at;

    if( _next_time == latter_day )  // Das ist, wenn die Periode weder repeat noch single_start hat, also keinen automatischen Start
    {
        _next_time = _period.end();
    }

    // Gesammelte eMail senden`, wenn collected_max erreicht:
    Time log_time = _log.collect_end();
    if( log_time > now  &&  _next_time > log_time )  _next_time = log_time;
}

//-------------------------------------------------------------------------Job::set_next_start_time

void Job::set_next_start_time( Time now )
{
    if( _delay_until )
    {
        _next_start_time = _delay_until;
        _log.debug( "Wiederholung wegen delay_after_error: " + _next_start_time.as_string() );
    }
    else
    if( _repeat > 0 )       // spooler_task.repeat
    {
        _next_start_time = now + _repeat;
        _log.debug( "Wiederholung wegen spooler_job.repeat=" + as_string(_repeat) + ": " + _next_start_time.as_string() );
        _repeat = 0;
    }
    else
    {
        _next_start_time = _period.next_try( now );
        if( _next_start_time != latter_day )  _log.debug( "Nächste Wiederholung wegen <period repeat=\"" + as_string((double)_period._repeat) + "\">: " + _next_start_time.as_string() );
    }

    if( _next_start_time > _period.end()  ||  _next_start_time == latter_day ) 
    {
        if( now < _period.begin() )     // Nächste Periode hat noch nicht begonnen?
        {
            _next_start_time = _period.begin();
            _log.debug( "Nächster Start zu Beginn der Periode: " + _next_start_time.as_string() );
        }
        else  
        if( now >= _period.end() )
        {
            select_period( now );
        }
    }

    set_next_time( now );
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

//---------------------------------------------------------------------------------Job::create_task

Sos_ptr<Task> Job::create_task( const CComPtr<spooler_com::Ivariable_set>& params, const string& name, Time start_at )
{
    Sos_ptr<Task> task;

    //_log.debug( "create_task" );

    if( !_process_filename.empty() )   task = SOS_NEW( Process_task( _spooler, this ) );
    else
    if( _object_set_descr          )   task = SOS_NEW( Object_set_task( _spooler, this ) );
    else                             
                                       task = SOS_NEW( Job_script_task( _spooler, this ) );

    Time now = Time::now();
    task->_enqueue_time = now;
    task->_params       = params? params : new Com_variable_set;
    task->_name         = name;
    task->_start_at     = start_at; 
    
    //if( task->_start_at < _next_time )  _next_time = start_at;
    
    Task_queue::iterator it = _task_queue.begin();  // _task_queue nach _start_at geordnet halten
    while( it != _task_queue.end()  &&  (*it)->_start_at <= task->_start_at )  it++;
    _task_queue.insert( it, task );

    set_next_time( now );

    return task;
}

//--------------------------------------------------------------------------------Job::dequeue_task

bool Job::dequeue_task( Time now )
{
    if( _state == s_read_error )  return false;
    if( _task_queue.empty() )     return false;

    THREAD_LOCK( _lock )
    {
        bool                 in_period = is_in_period(now);
        Task_queue::iterator it        = _task_queue.begin();
        
        for( ; it != _task_queue.end(); it++ )
        {
            Task* t = *it;
            if(  t->_start_at  &&  t->_start_at <= now )  break;        // Task mit Startzeitpunkt
            if( !t->_start_at  &&  in_period           )  break;        // Task ohne Startzeitpunkt
        }

        if( it == _task_queue.end() )  return false;

        _task = *it;
        it = _task_queue.erase( it );

        reset_error();
        _close_engine = false;
        _repeat = 0;

        _com_task->set_task( _task );
        set_state( s_start_task );
    }

    return true;
}

//----------------------------------------------------------------------Job::remove_from_task_queue

void Job::remove_from_task_queue( Task* task )
{
    //_next_start_at = latter_day;

    FOR_EACH( Task_queue, _task_queue, it )  
    {
        if( +*it == task )  
        {
            _log( task->obj_name() + " aus der Warteschlange entfernt" );
            it = _task_queue.erase( it );
            break;
        }
        else
        {
            //if( _next_start_at > (*it)->_start_at )  _next_start_at = (*it)->_start_at;
        }
    }
}

//---------------------------------------------------------------------------------------Job::start

void Job::start( const CComPtr<spooler_com::Ivariable_set>& params, const string& task_name, Time start_at )
{
    THREAD_LOCK_LOG( _lock, "Job::start" )  start_without_lock( params, task_name, start_at );
}

//---------------------------------------------------------------------------------------Job::start

Sos_ptr<Task> Job::start_without_lock( const CComPtr<spooler_com::Ivariable_set>& params, const string& task_name, Time start_at )
{
    switch( _state )
    {
        case s_read_error:  throw_xc( "SPOOLER-132", name(), _error? _error->what() : "" );
     
      //case s_load_error:
        case s_stopped:     set_state( s_pending );
        
        default: ;
    }

    Sos_ptr<Task> task = create_task( params, task_name, start_at );
    
    task->_let_run = true;

    if( GetCurrentThreadId() != thread()->_thread_id )  _thread->signal( "start job" );

    return task;
}

//----------------------------------------------------------------Job::start_when_directory_changed

void Job::start_when_directory_changed( const string& directory_name, const string& filename_pattern )
{
    THREAD_LOCK( _lock )
    {
        _log.debug( "start_when_directory_changed \"" + directory_name + "\", \"" + filename_pattern + "\"" );

#       ifdef SYSTEM_WIN

            for( Directory_watcher_list::iterator it = _directory_watcher_list.begin(); it != _directory_watcher_list.end(); it++ )
            {
                if( (*it)->directory()        == directory_name 
                 && (*it)->filename_pattern() == filename_pattern )  it = _directory_watcher_list.erase( it );   // Überwachung erneuern
                // Wenn das Verzeichnis bereits überwacht war, aber inzwischen gelöscht, und das noch nicht bemerkt worden ist
                // (weil Spooler_thread::wait vor lauter Jobaktivität nicht gerufen wurde), dann ist es besser, die Überwachung 
                // hier zu erneuern. Besonders, wenn das Verzeichnis wieder angelegt ist.
            }


            Sos_ptr<Directory_watcher> dw = SOS_NEW( Directory_watcher( &_log ) );

            dw->watch_directory( directory_name, filename_pattern );
            dw->set_name( "job(\"" + _name + "\").start_when_directory_changed(\"" + directory_name + "\")" );
            _directory_watcher_list.push_back( dw );
            dw->add_to( &_thread->_wait_handles );

#        else

            throw_xc( "SPOOLER-112", "Job::start_when_directory_changed" );

#       endif
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

//---------------------------------------------------------------------------------Job::read_script

bool Job::read_script()
{
    try
    {
        _script.set_xml( _script_xml_element, include_path() );
    }
    catch( const Xc& x        ) { set_error(x);  _close_engine = true;  set_state( s_read_error );  return false; }
    catch( const exception& x ) { set_error(x);  _close_engine = true;  set_state( s_read_error );  return false; }

    return true;
}

//----------------------------------------------------------------------------------------Job::load

bool Job::load()
{
    _script_instance.init( _script_ptr->_language );

    _script_instance.add_obj( (IDispatch*)_spooler->_com_spooler, "spooler"        );
    _script_instance.add_obj( (IDispatch*)_thread->_com_thread  , "spooler_thread" );
    _script_instance.add_obj( (IDispatch*)_com_log              , "spooler_log"    );
    _script_instance.add_obj( (IDispatch*)_com_job              , "spooler_job"    );
    _script_instance.add_obj( (IDispatch*)_com_task             , "spooler_task"   );

    try
    {
        _script_instance.load( *_script_ptr );
        _script_instance.start();
    }
    catch( const Xc& x        ) { set_error(x);  _close_engine = true;  return false; }
    catch( const exception& x ) { set_error(x);  _close_engine = true;  return false; }


    {
        In_call in_call ( this, spooler_init_name );
        bool ok = check_result( _script_instance.call_if_exists( spooler_init_name ) );
        in_call.set_result( ok );
        if( !ok || has_error() )  { _close_engine = true;  return false; }
    }

    _has_spooler_process = _script_instance.name_exists( spooler_process_name );

    set_state( s_loaded );
    return true;
}

//-----------------------------------------------------------------------------------------Job::end

void Job::end()
{
    if( !_state )  return;

    set_mail_defaults();      // Vor spooler_on_error() bzw. spooler_on_success(); eMail wird in finish() verschickt


    if( _state == s_suspended )  set_state( s_running );
    
    if( _state == s_starting
     || _state == s_running  
     || _state == s_running_process )  
    {
        if( _task )  _task->end();
        _history.end();
    }

    close_task();

    if( _state != s_stopped )  set_state( s_ended );
}

//----------------------------------------------------------------------------------------Job::stop

void Job::stop()
{
    //_log.msg( "stop" );

    if( _state != s_stopped  &&  _task )
    {
        try 
        {
            _task->do_stop();
        }
        catch( const Xc& x        )  { set_error(x); }
        catch( const exception& x )  { set_error(x); }
    }

    set_state( s_stopped );

    end();
    close_engine();

    // Kein Signal mehr soll kommen, wenn Job gestoppt:
    //_event.close();
    clear_when_directory_changed();
}

//--------------------------------------------------------------------------------------Job::finish

void Job::finish()
{
    // eMail versenden

    try
    {
        //if( _log.mail_on_success() && !has_error()  
        //||  _log.mail_on_error()   &&  has_error() )  _log.send();

        _log.send( has_error()? -1 : _process_ok? +1 : 0 );

        //_log.close();
    }
    catch( const Xc& x         ) { _log.warn(x.what()); }  //{ set_error(x); }
    catch( const exception& x  ) { _log.warn(x.what()); }  //{ set_error(x); }
    catch( const _com_error& x ) { _log.warn(bstr_as_string(x.Description())); }  //{ set_error(x); }
}

//----------------------------------------------------------------------------Job::interrupt_script

void Job::interrupt_script()
{
    if( _script_instance )
    {
        _log.warn( "Skript wird abgebrochen" );
        _script_instance.interrupt();
    }
}

//--------------------------------------------------------------------------------------Job::reread

void Job::reread()
{
    close_task();
    close_engine();

    _log( "Skript wird erneut gelesen (<include> wird erneut ausgeführt)" );
    read_script();

    set_state( s_pending );
    init2();
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
                case sc_stop:       if( _state != s_stopped  
                                     && _state != s_read_error )  stop(), finish(),            something_done = true;
                                    break;

                case sc_unstop:     if( _state == s_stopped    )  set_state( s_pending ),      something_done = true;
                                    break;

                case sc_end:        if( _state == s_running    )  end(), finish(),             something_done = true;
                                    break;

                case sc_suspend:    if( _state == s_running    )  set_state( s_suspended ),    something_done = true;
                                    break;

                case sc_continue:   if( _state == s_suspended  )  set_state( s_running ),      something_done = true;
                                    break;

                default: ;
            }

            _state_cmd = sc_none;
        }
    }

    return something_done;
}

//-------------------------------------------------------------------------------Job::task_to_start

void Job::task_to_start()
{
    Time now      = Time::now();
    bool dequeued = false;
    bool ok       = false;
    Start_cause cause = cause_none;

    if( _state == s_pending  )
    {
        dequeued = dequeue_task(now);
        if( dequeued )  _task->_cause = _task->_start_at? cause_queue_at : cause_queue;

        
        if( _period._single_start  &&  now >= _next_start_time )  
        {
            cause = cause_period_single;     
            //_log.debug( "Task startet wegen single_start=" + _period.begin().as_string() );
        }
        else
            select_period(now);

        if( cause                      // Auf weitere Anlässe prüfen und diese protokollieren
         || is_in_period(now) )
        {
            THREAD_LOCK( _lock )
            {
                if( _start_once )              cause = cause_period_once,  _start_once = false,  _log.debug( "Task startet wegen <run_time once=\"yes\">" );
                                                                              
                if( _event.signaled() )        cause = cause_signal,                             _log.debug( "Task startet wegen " + _event.as_string() );
                                                                                      
                if( now >= _next_start_time )  cause = cause_period_repeat,                      _log.debug( "Task startet, weil Job-Startzeit erreicht: " + _next_start_time.as_string() );

                for( Directory_watcher_list::iterator it = _directory_watcher_list.begin(); it != _directory_watcher_list.end(); it++ )
                {
                    if( (*it)->signaled_then_reset() )
                    {
                        cause = cause_directory;
                        _log.debug( "Task startet wegen eines Ereignisses für Verzeichnis " + (*it)->directory() );

                        if( !(*it)->handle() )  it = _directory_watcher_list.erase( it );  // Folge eines Fehlers, s. Directory_watcher::set_signal
                    }
                }

                if( !dequeued && cause )
                {
                    create_task( NULL, "", now );
                    dequeue_task( now );
                    _task->_cause = cause;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------Job::do_something

bool Job::do_something()
{
#   ifdef DEBUG
        _log.debug9( "do_something() state=" + state_name() );
#   endif

    if( !_state )  return false;

    Start_cause cause          = cause_none;
    bool        something_done = false;
    bool        ok             = true;
    bool        do_a_step      = false;

    something_done = execute_state_cmd();

    if( _reread  &&  !_task )  _reread = false,  reread(),  something_done = true;

    if( _state == s_pending )  task_to_start();

    // Wenn nichts zu tun ist, dann raus. Der Job soll nicht wegen eines alten Fehlers nachträglich gestoppt werden (s.u.)
    if( _state == s_pending    )  goto ENDE;
    if( _state == s_suspended  )  goto ENDE;
    if( _state == s_stopped    )  goto ENDE;
    if( _state == s_read_error )  goto ENDE;


    if( _state == s_start_task )    // SPOOLER_INIT, SPOOLER_OPEN
    {
        _event.reset();

        ok = _task->start();
        if( ok )  do_a_step = true;
        something_done = true;
        
        if( _state == s_read_error )
        {
            close_task();
            goto ENDE;
        }
    }

    if( ( _state == s_running || _state == s_running_process )  &&  ok  &&  !has_error() )        // SPOOLER_PROCESS
    {
        Time now;
        bool call_step = do_a_step | _task->_let_run;
        if( !call_step )  now = Time::now(), call_step = _period.is_in_time( now );
        if( !call_step )   // Period abgelaufen?
        {
            call_step = _task->_let_run  ||  ( select_period(now), is_in_period(now) );
        }

        if( call_step ) 
        {
            ok = _task->step(); 
            something_done = true;
        }
        else
        {
            ok = false;
            _log( "Laufzeitperiode ist abgelaufen, Task wird beendet" );
        }
    }

    if( !ok || has_error() )        // SPOOLER_CLOSE, SPOOLER_ON_SUCCESS, SPOOLER_ON_ERROR
    {
        //if( _spooler->_debug )  LOG( "spooler_process() lieferte " << ok << ", Fehler=" << _error << '\n' );      // Problem bei Uwe, 20.2.02

        if( _state == s_starting        // Bei Fehler in spooler_init()
         || _state == s_running 
         || _state == s_running_process )  end(), something_done = true;

        if( _state != s_stopped  &&  has_error()  &&  _repeat == 0  &&  _delay_after_error.empty() )  stop(), something_done = true;

        finish();
    }


    if( _state == s_ended ) 
    {
        if( _temporary && _repeat == 0 )  
        {
            stop();   // _temporary && s_stopped ==> spooler_thread.cxx entfernt den Job
            finish();
        }
        else
        {
            //bool dequeued = dequeue_task();

            //if( !dequeued )
            {
                set_next_start_time();
                set_state( s_pending );
            }
        }

        something_done = true;
    }


ENDE:
    if( _state != s_running  &&  _state != s_running_process  &&  _state != s_suspended )  send_collected_log();

    return something_done;
}

//---------------------------------------------------------------------------Job::set_mail_defaults

void Job::send_collected_log()
{
    try
    {
        _log.send( +2 );
    }
    catch( const Xc& x         ) { _spooler->_log.error(x.what()); }
    catch( const exception&  x ) { _spooler->_log.error(x.what()); }
    catch( const _com_error& x ) { _spooler->_log.error(bstr_as_string(x.Description())); }
}

//---------------------------------------------------------------------------Job::set_mail_defaults

void Job::set_mail_defaults()
{
    bool is_error = has_error();

    _log.set_mail_from_name( profile_section() );

    char hostname[200];
    if( gethostname( hostname, sizeof hostname ) == SOCKET_ERROR )  hostname[0] = '\0';
    string body = Sos_optional_date_time::now().as_string() + "\n\nJob " + _name + "  " + _title + "\n"
                  "Spooler -id=" + _spooler->id() + "  host=" + hostname + "\n\n";

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

//--------------------------------------------------------------------------------Job::set_error_xc

void Job::set_error_xc( const Xc& x )
{
    string msg; 
    if( !_in_call.empty() )  msg = "In " + _in_call + "(): ";
    
    _log.error( msg + x.what() );

    THREAD_LOCK( _lock )  _error = x;

    _repeat = 0;
}

//-----------------------------------------------------------------------------------Job::set_error

void Job::set_error( const exception& x )
{
    Xc xc ( "SOS-2000", x.what(), exception_name(x) );
    set_error_xc( xc );
}

//-----------------------------------------------------------------------------------Job::set_error
#ifdef SYSTEM_HAS_COM

void Job::set_error( const _com_error& x )
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
//-----------------------------------------------------------------------------------Job::set_state

void Job::set_state( State new_state )
{ 
    THREAD_LOCK( _lock )  
    {
        if( new_state == _state )  return;

        if( new_state == s_running  ||  new_state == s_start_task )  _thread->_running_tasks_count++;
        if( _state    == s_running  ||  _state    == s_start_task )  _thread->_running_tasks_count--;

        if( new_state == s_pending )  reset_error();

        _state = new_state;


        if( _spooler->_debug )
        {
            if( new_state == s_starting ) 
            {
                string msg = state_name();
                if( _task && _task->_start_at )  msg += " (at=" + _task->_start_at.as_string() + ")";
                _log.debug( msg );
            }
            else
            if( new_state == s_ended    ) _log.debug( state_name() ); 
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
                                    THREAD_LOCK_LOG( _lock, "Job::set_state_cmd" )  start_without_lock( NULL, "", Time::now() );
                                    break;
                                }

            case sc_wake:       wake();            
                                _state_cmd = cmd;
                                _thread->signal( state_cmd_name(cmd) );
                                break;

            case sc_end:        ok = _state == s_running;       if( !ok )  return;
                                _state_cmd = cmd;
                                break;

            case sc_suspend:    ok = _state == s_running;       if( !ok )  return;
                                _state_cmd = cmd;
                                break;

            case sc_continue:   ok = _state == s_suspended;     if( !ok )  return;
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
        if( !_in_call.empty() )  st += " in " + _in_call + "()";
    }

    return st;
}

//---------------------------------------------------------------------------------Job::include_path

string Job::include_path() const
{ 
    return _thread->include_path(); 
}

//----------------------------------------------------------------------------------Job::set_in_call

void Job::set_in_call( const string& name )
{
    THREAD_LOCK( _lock )
    {
        _in_call = name;
        if( _spooler->_debug  &&  !name.empty() )  _log.debug( name + "()" );
    }
}

//----------------------------------------------------------------------------------Job::state_name

string Job::state_name( State state )
{
    switch( state )
    {
        case s_stopped:         return "stopped";
        case s_read_error:      return "read_error";
      //case s_load_error:      return "load_error";
        case s_loaded:          return "loaded";
        case s_pending:         return "pending";
        case s_start_task:      return "start_task";
        case s_starting:        return "starting";
        case s_running:         return "running";
        case s_running_process: return "running_process";
        case s_suspended:       return "suspended";
        case s_ending:          return "ending";
        case s_ended:           return "ended";
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

//-----------------------------------------------------------------------------------------Job::xml
// Anderer Thread

xml::Element_ptr Job::xml( xml::Document_ptr document, bool show_all )
{
    xml::Element_ptr job_element = document->createElement( "task" );

    THREAD_LOCK( _lock )
    {
        job_element->setAttribute( "job"       , as_dom_string( _name ) );
        job_element->setAttribute( "state"     , as_dom_string( state_name() ) );
        job_element->setAttribute( "title"     , as_dom_string( _title ) );
        job_element->setAttribute( "steps"     , as_dom_string( _step_count ) );
        job_element->setAttribute( "state_text", as_dom_string( _state_text ) );
        job_element->setAttribute( "log_file"  , as_dom_string( _log.filename() ) );
        


        if( !_in_call.empty() )  job_element->setAttribute( "calling", as_dom_string( _in_call ) );

        if( _state_cmd )  job_element->setAttribute( "cmd", as_dom_string( state_cmd_name() ) );

        if( _state == s_pending )
        {
            // Versuchen, nächste Startzeit herauszubekommen
            Period p    = _period;  
            int    i    = 100;      // Anzahl Perioden, die wir probieren
            Time   next;
            if( _next_start_time != latter_day )  next = _next_start_time;
                                            else  p = _run_time.next_period( p.end() ),  next = p.begin();
            while( i-- ) {          
                if( p.has_start() )  break;
                p = _run_time.next_period( p.end() );
                next = p.begin();
            }

            if( i > 0  &&  next != latter_day )  job_element->setAttribute( "next_start_time", as_dom_string( next.as_string() ) );
        }

        if( _task )
      //THREAD_LOCK( _task->_lock )
        {
            job_element->setAttribute( "running_since", as_dom_string( _task->_running_since.as_string() ) );
            //job_element->setAttribute( "steps"        , as_dom_string( as_string( _task->_step_count ) ) );
        }
        

        if( show_all )  dom_append_text_element( job_element, "description", _description );

        if( !_task_queue.empty() )
        {
            xml::Element_ptr queue_element = document->createElement( "queued_tasks" );
            dom_append_nl( queue_element );

            FOR_EACH( Task_queue, _task_queue, it )
            {
                xml::Element_ptr queued_task_element = document->createElement( "queued_task" );
                queued_task_element->setAttribute( "enqueued", as_dom_string( (*it)->_enqueue_time.as_string() ) );
                queued_task_element->setAttribute( "name"    , as_dom_string( (*it)->_name ) );
                if( (*it)->_start_at )
                    queued_task_element->setAttribute( "start_at", as_dom_string( (*it)->_start_at.as_string() ) );

                queue_element->appendChild( queued_task_element );
                dom_append_nl( queue_element );
            }

            job_element->appendChild( queue_element );
        }

        if( _error )  append_error_element( job_element, _error );
    }

    return job_element;
}

//-------------------------------------------------------------------------------Job::signal_object
// Anderer Thread

void Job::signal_object( const string& object_set_class_name, const Level& level )
{
    THREAD_LOCK_LOG( _lock, "Job::signal_object" )
    {
        if( _state == Job::s_pending
         && _object_set_descr
         && _object_set_descr->_class->_name == object_set_class_name 
         && _object_set_descr->_level_interval.is_in_interval( level ) )
        {
            //start_without_lock( NULL, object_set_class_name );
            _event.signal( "Object_set " + object_set_class_name );
            //_thread->signal( obj_name() + ", Object_set " + object_set_class_name );
        }
    }
}

//---------------------------------------------------------------------------------------Task::Task

Task::Task( Spooler* spooler, const Sos_ptr<Job>& job )    
: 
    _zero_(this+1), 
    _spooler(spooler), 
    _job(job)
{
    _let_run = _job->_period.let_run();
}

//---------------------------------------------------------------------------------------Task::Task
// Kann von anderem Thread gerufen werden, wenn der noch eine COM-Referenz hat

Task::~Task()    
{
    try{ close(); } catch(const Xc&) {}
}

//--------------------------------------------------------------------------------------Task::close
// Kann von anderem Thread gerufen werden, wenn der noch eine COM-Referenz hat

void Task::close()
{
    if( _closed )  return;
    
    {
        THREAD_LOCK( _job->_lock )  
            if( _job->_com_task )  
                if( _job->_com_task->task() == this )      // spooler_task?
                    _job->_com_task->set_task(NULL);
    }

    do_close();

    // Alle, die mit wait_until_terminated() auf diese Task warten, wecken:
    THREAD_LOCK( _terminated_events_lock )  
    {
        FOR_EACH( vector<Event*>, _terminated_events, it )  (*it)->signal( "close task" );
        _terminated_events.clear();
    }

    _closed = true;
}

//-------------------------------------------------------------------------------Task::set_start_at
/*
void Task::set_start_at( Time time )
{ 
    _start_at = time; 
    
    if( _start_at < _job->_next_time )  _job->_next_time = time;
}
*/
//--------------------------------------------------------------------------------------Task::start

bool Task::start()
{
    try 
    {
        //_job->_log.close();
        _job->_log.open();           // Jobprotokoll, nur wirksam, wenn set_filename() gerufen, s. Job::init().

        THREAD_LOCK( _job->_lock )
        {
            _id = _spooler->_db.get_id();

            _job->set_state( Job::s_starting );
            _job->reset_error();
            _job->_delay_until = 0;

            _job->_thread->_task_count++;
          //_job->_step_count = 0;
            _job->_process_ok = false;
            _running_since = Time::now();

            _job->_history.start();
        }

        if( !loaded() )  
        {
            bool ok = do_load();
            if( !ok || _job->has_error() )  return false;
        }

        bool ok = do_start();
        if( !ok || _job->has_error() )  return false;
        _opened = true;
    }
    catch( const Xc& x        ) { _job->set_error(x); }
    catch( const exception& x ) { _job->set_error(x); }

    return !_job->has_error();
}

//-----------------------------------------------------------------------------------------Task::end

void Task::end()
{
    _job->set_state( Job::s_ending );

    if( _opened )  
    {
        try
        {
            do_end();
        }
        catch( const Xc& x        ) { _job->set_error(x); }
        catch( const exception& x ) { _job->set_error(x); }

        _opened = false;
    }

    on_error_on_success();
    close();


    // Bei mehreren aufeinanderfolgenden Fehlern Wiederholung verzögern?

    if( _job->has_error() )
    {
        _job->_error_steps++;
    
        if( !_job->_repeat )   // spooler_task.repeat hat Vorrang
        {
            Time delay = 0;
            FOR_EACH( Job::Delay_after_error, _job->_delay_after_error, it )  
                if( _job->_error_steps >= it->first )  delay = it->second;
            _job->_delay_until = Time::now() + delay;
        }
    }
    else
    {
       _job->_error_steps = 0;
    }
}

//-------------------------------------------------------------------------Task::on_error_on_success

void Task::on_error_on_success()
{
    if( _job->has_error() )
    {
        // spooler_on_error() wird auch gerufen, wenn spooler_init() einen Fehler hatte

        if( !_on_error_called )
        {
            _on_error_called = true;

            try
            {
                do_on_error();
            }
            catch( const Xc& x        ) { _job->_log.error( string(spooler_on_error_name) + ": " + x.what() ); }
            catch( const exception& x ) { _job->_log.error( string(spooler_on_error_name) + ": " + x.what() ); }
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
        catch( const Xc& x        ) { _job->set_error(x); }
        catch( const exception& x ) { _job->set_error(x); }
    }
}

//----------------------------------------------------------------------------------------Task::step

bool Task::step()
{
    bool result;

    try 
    {
        result = do_step();

        _job->_thread->_step_count++;
        _job->_step_count++;
        _step_count++;
        _job->_process_ok |= result;
    }
    catch( const Xc& x        ) { _job->set_error(x); return false; }
    catch( const exception& x ) { _job->set_error(x); return false; }

    return result;
}

//------------------------------------------------------------------------------------Task::cmd_end
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

//----------------------------------------------------------------------Task::wait_until_terminated
// Anderer Thread

bool Task::wait_until_terminated( double wait_time )
{
    Thread_id my_thread_id = GetCurrentThreadId();
    if( my_thread_id == _job->_thread->_thread_id )  throw_xc( "SPOOLER-125" );     // Deadlock

    Thread* calling_thread = _spooler->thread_by_thread_id( my_thread_id );
    if( calling_thread &&  !calling_thread->_free_threading  &&  !_job->_thread->_free_threading )  throw_xc( "SPOOLER-131" );

    Event event ( obj_name() + " wait_until_terminated" );
    int   i = 0;
    
    THREAD_LOCK( _terminated_events_lock ) 
    {
        i = _terminated_events.size();
        _terminated_events.push_back( &event );
    }

    bool result = event.wait( wait_time );

    { THREAD_LOCK( _terminated_events_lock )  _terminated_events.erase( &_terminated_events[i] ); }

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

//--------------------------------------------------------------------------Task::parameters_as_dom

xml::Document_ptr Task::parameters_as_dom()
{
    xml::Document_ptr result;
    _params->get_dom( &result );
    return result;
}

//--------------------------------------------------------------------------Task::set_history_field

void Task::set_history_field( const string& name, const CComVariant& value )
{
    if( !_job->its_current_task(this) )  throw_xc( "SPOOLER-138" );

    _job->_history.set_extra_field( name, value );
}

//-----------------------------------------------------------------------Script_task::do_on_success

void Script_task::do_on_success()
{
    if( _job->_script_instance.loaded() )
    {
        Job::In_call in_call ( this, spooler_on_success_name );
        _job->_script_instance.call_if_exists( spooler_on_success_name );
    }
}

//-------------------------------------------------------------------------Script_task::do_on_error

void Script_task::do_on_error()
{
    //if( _job->_script_instance.loaded() )
    {
        Job::In_call in_call ( this, spooler_on_error_name );
        _job->_script_instance.call_if_exists( spooler_on_error_name );
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
    bool ok;

    if( !_object_set ) 
    {
        _object_set = SOS_NEW( Object_set( _spooler, this, _job->_object_set_descr ) );
        _com_object_set = new Com_object_set( _object_set );
    }

    if( !_job->_script_instance.loaded() )
    {
        ok = _job->load();
        if( !ok || _job->has_error() )  return false;
    }

    return ok;
}

//------------------------------------------------------------------------Object_set_task::do_start

bool Object_set_task::do_start()
{
    _job->set_state( Job::s_running );

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

//-------------------------------------------------------------------------Job_script_task::do_load

bool Job_script_task::do_load()
{
    return _job->load();
}

//------------------------------------------------------------------------Job_script_task::do_start

bool Job_script_task::do_start()
{
    bool ok;

    if( !_job->_script_instance.loaded() )
    {
        ok = _job->load();
        if( !ok || _job->has_error() )  return false;
    }

    _job->set_state( Job::s_running );

    {
        Job::In_call in_call ( this, spooler_open_name );

        ok = check_result( _job->_script_instance.call_if_exists( spooler_open_name ) );
        in_call.set_result( ok );
        if( !ok )  return false;
    }

    return true;
}

//--------------------------------------------------------------------------Job_script_task::do_end

void Job_script_task::do_end()
{
    Job::In_call in_call ( this, spooler_close_name );
    _job->_script_instance.call_if_exists( spooler_close_name );
}

//-------------------------------------------------------------------------Job_script_task::do_step

bool Job_script_task::do_step()
{
    if( !_job->_has_spooler_process )  return false;

    Job::In_call in_call ( this, spooler_process_name );
    bool ok = check_result( _job->_script_instance.call( spooler_process_name ) );
    in_call.set_result( ok );
    return ok;
}

//----------------------------------------------------------------------------Process_task::do_start

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
        string      nr_str  = as_string( i );
        CComBSTR    nr_bstr;
        CComVariant vt;
        HRESULT     hr;

        nr_bstr.Attach( SysAllocString_string( nr_str ) );

        hr = _params->get_var( nr_bstr, &vt );
        if( FAILED(hr) )  throw_ole( hr, "Variable_set.var", nr_str.c_str() );

        if( vt.vt == VT_EMPTY )  break;

        hr = vt.ChangeType( VT_BSTR );
        if( FAILED(hr) )  throw_ole( hr, "VariantChangeType", nr_str.c_str() );

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
    _process_handle = process_info.hProcess;
    _process_handle.set_name( "Process " + _job->_process_filename );
    _process_handle.add_to( &_job->_thread->_wait_handles );

    _job->set_state( Job::s_running_process );
    return true;
}

//----------------------------------------------------------------------------Process_task::do_stop

void Process_task::do_stop()
{
    if( _process_handle )
    {
        _job->_log.warn( "Prozess wird abgebrochen" );

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
    if( exit_code )  throw_xc( "SPOOLER-126", exit_code );
}

//----------------------------------------------------------------------------Process_task::do_step

bool Process_task::do_step()
{
    return !_process_handle.signaled();
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
