// $Id: spooler.cxx,v 1.13 2001/01/07 16:35:18 jz Exp $




/*
    WAS FEHLT?

    Listen etc. sperren bei execute()

*/

#include "../kram/sos.h"
#include "../kram/sosprof.h"
#include "../kram/sosopt.h"
#include "../kram/sleep.h"
#include "../file/anyfile.h"
#include "spooler.h"


namespace sos {

extern const Bool _dll = false;

namespace spooler {

using namespace std;

//---------------------------------------------------------------------------------------------now

Time now() 
{
    return time(NULL) - _timezone;
}

//----------------------------------------------------------------------------Script_instance::load

void Script_instance::load()
{
    _script_site = new Script_site;
    _script_site->_engine_name = _script->_language;
    _script_site->init_engine();

    _script_site->parse( _script->_text );
}

//---------------------------------------------------------------------------Script_instance::close

void Script_instance::close()
{
    _script_site->close_engine();
    _script_site = NULL;
}

//----------------------------------------------------------------------------Script_instance::call

CComVariant Script_instance::call( const char* name )
{
    return _script_site->call( name );
}

//----------------------------------------------------------------------------Spooler_object::level

Level Spooler_object::level()
{
    CComVariant level = com_property_get( _dispatch, "level" );
    level.ChangeType( VT_INT );

    return level.intVal;
}

//--------------------------------------------------------------------------Spooler_object::process

void Spooler_object::process( Level output_level )
{
    com_call( _dispatch, "process", output_level );
}

//---------------------------------------------------------------------------------Object_set::open

void Object_set::open()
{
    _script_instance.load();

    CComVariant object_set_vt;
/*
    if( stricmp( _script_instance._script->_language.c_str(), "PerlScript" ) == 0 )
    {
        object_set_vt = _script_site->com_property_get( "object_set" );
    }
    else
*/
    {
        object_set_vt = _script_instance.call( "spooler_make_object_set" );
    }

    if( object_set_vt.vt != VT_DISPATCH 
     || object_set_vt.pdispVal == NULL  )  throw_xc( "SPOOLER-103", _object_set_descr->_class_name );
    _dispatch = object_set_vt.pdispVal;

    com_property_put( _dispatch, "spooler_low_level" , _object_set_descr->_level_interval._low_level );
    com_property_put( _dispatch, "spooler_high_level", _object_set_descr->_level_interval._high_level );
    com_property_put( _dispatch, "spooler_param"     , _spooler->_object_set_param.c_str() );

    com_call( _dispatch, "spooler_open" );
}

//--------------------------------------------------------------------------------Object_set::close

void Object_set::close()
{
    com_call( _dispatch, "spooler_close" );

    _script_instance.close();
}

//----------------------------------------------------------------------------------Object_set::get

Spooler_object Object_set::get()
{
    Spooler_object object;

    while(1)
    {
        CComVariant obj = com_call( _dispatch, "spooler_get" );

        if( obj.vt == VT_EMPTY    )  return Spooler_object(NULL);
        if( obj.vt != VT_DISPATCH
         || obj.pdispVal == NULL  )  throw_xc( "SPOOLER-102", _object_set_descr->_class_name );
        
        object = obj.pdispVal;

        if( obj.pdispVal == NULL )  break;  // EOF
        if( _object_set_descr->_level_interval.is_in_interval( object.level() ) )  break;
    }

    return object;
}

//---------------------------------------------------------------------------Weekday_set::next_date

Time Weekday_set::next_date( Time tim )
{
    int day_no = tim / (24*60*60);

    int weekday = ( day_no + 4 ) % 7;

    for( int i = weekday; i < weekday+7; i++ )
    {
        if( _days[ i % 7 ] )  return day_no * (24*60*60);
        day_no++;
    }

    return latter_day;
}

//--------------------------------------------------------------------------Monthday_set::next_date

Time Monthday_set::next_date( Time tim )
{
    int                     day_no = tim / (24*60*60);
    Sos_optional_date_time  date   = tim;

    for( int i = 0; i < 31; i++ )
    {
        if( _days[ date.day() ] )  return day_no * (24*60*60);
        day_no++;
        date.add_days(1);
    }

    return latter_day;
}

//----------------------------------------------------------------------------Ultimo_set::next_date

Time Ultimo_set::next_date( Time tim )
{
/*
    int         day_no = tim / (24*60*60);
    struct tm*  tm = localtime(tim);
    Sos_date    date;

    date.assign_date( 1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday );

    for( int i = 0; i < 31; i++ )
    {
        if( _days[ date.day() ] )  return day_no * (24*60*60);
        day_no++;
        date.add_days(1);
    }
*/
    return latter_day;
}

//---------------------------------------------------------------------------------Run_time::check

void Run_time::check()
{
    if( _begin_time_of_day < 0                )  throw_xc( "SPOOLER-104" );
    if( _begin_time_of_day > _end_time_of_day )  throw_xc( "SPOOLER-104" );
    if( _end_time_of_day > 24*60*60           )  throw_xc( "SPOOLER-104" );
}

//----------------------------------------------------------------------------------Run_time::next

Time Run_time::next( Time tim_par )
{
    // Bei der Umschaltung von Winter- auf Sommerzeit fehlt eine Stunde!

    Time tim = tim_par;

    time_t time_only = (time_t)tim % (24*60*60);
    if( time_only > _end_time_of_day )  tim += 24*60*60;

    tim -= time_only;


    Time next;
 
    while(1)
    {
        next = latter_day;

        FOR_EACH( set<time_t>, _date_set, it )
        {
            if( *it < next  &&  *it >= tim )  next = *it;
        }

        next = min( next, _weekday_set .next_date( tim ) );
        next = min( next, _monthday_set.next_date( tim ) );
        next = min( next, _ultimo_set  .next_date( tim ) );

        if( _holiday_set.find( next ) == _holiday_set.end() )  break;

        tim += (24*60*60);
    }

    _next_start_time = next + _begin_time_of_day;
    _next_end_time = _next_start_time + ( _end_time_of_day - _begin_time_of_day );

    if( _next_start_time < tim_par )
    {
        //_next_start_time += int( (tim_par-_next_start_time) / _retry_period + _retry_period - 0.01 ) * _retry_period;
        _next_start_time = tim_par;
    }

    return _next_start_time;
}

//-----------------------------------------------------------------------------------------Task::Task

Task::Task( Spooler* spooler, const Sos_ptr<Job>& job )    
: 
    _zero_(this+1), 
    _spooler(spooler), 
    _job(job),
    _script_instance(&job->_script)
{
    set_new_start_time();
}

//--------------------------------------------------------------------------Task::set_new_start_time

void Task::set_new_start_time()
{
    _next_start_time = _job->_run_time.next();
}

//---------------------------------------------------------------------------------------Task::error

void Task::error( const Xc& x)
{
    cerr << "Job " << _job->_name << ": " << x << '\n';

    _error = x;

    _running = false;
    _stopped = true;        // Nicht wieder starten

    _object_set = NULL;
}

//---------------------------------------------------------------------------------Task::start_error

void Task::start_error( const Xc& x )
{
    error( x );
}

//-----------------------------------------------------------------------------------Task::end_error

void Task::end_error( const Xc& x )
{
    error( x );
}

//----------------------------------------------------------------------------------Task::step_error

void Task::step_error( const Xc& x )
{
    error( x );
}

//---------------------------------------------------------------------------------------Task::start

bool Task::start()
{
    cerr << "Job " << _job->_name << " start\n";

    _running_since = now();

    if( _job->_object_set_descr )  _object_set = SOS_NEW( Object_set( _spooler, _job->_object_set_descr ) );

    try 
    {
        if( _object_set ) 
        {
            _object_set->open();
        }

        if( !_job->_script.empty() )
        {
            _script_instance.load();
        }

        _running = true;
        _spooler->_running_jobs_count++;

        _next_start_time = max( _next_start_time + _job->_run_time._retry_period, now() );
        if( now() >= _job->_run_time._next_end_time )  set_new_start_time();
    }
    catch( const Xc& x ) { start_error(x); return false; }

    return true;
}

//-----------------------------------------------------------------------------------------Task::end

void Task::end()
{
    cerr << "Job " << _job->_name << " end\n";

    try
    {
        if( _object_set )
        {
            _object_set->close();
        }

        if( !_job->_script.empty() ) 
        {
            _script_instance.close();
        }
    }
    catch( const Xc& x ) { end_error(x); }

    _spooler->_running_jobs_count--;
    _running = false;
}

//----------------------------------------------------------------------------------------Task::step

bool Task::step()
{
    cerr << "Task " << _job->_name << " step\n";

    Spooler_object object;

    try 
    {
        if( !_job->_script.empty() ) 
        {
            CComVariant result = _script_instance.call( "step" );
            result.ChangeType( VT_BOOL );
            if( !V_BOOL( &result ) )  return false;
        }
        else
        if( _object_set )
        {
            Spooler_object object = _object_set->get();

            if( object.is_null() )  return false;

            object.process( _job->_output_level );
        }
        else 
            return false; //?

        _step_count++;
    }
    catch( const Xc& x ) { step_error(x); return false; }

    return true;
}

//------------------------------------------------------------------------------------Spooler::step

void Spooler::step()
{
    Thread_semaphore::Guard guard = &_semaphore;


    FOR_EACH( Task_list, _task_list, it )
    {
        Time  nw   = now();
        Task* task = *it;

        if( !task->_running )
        {
            if( !task->_stopped  &&  now() >= task->_next_start_time )
            {
                task->start();
            }
        }

        if( task->_running )
        {
            if( task->_stop || nw > task->_job->_run_time._next_end_time )   // Bei _next_start_time == _next_end_time wenigstens ein step()
            {
                task->end();
                if( task->_stop )  task->_stop = false, task->_stopped = true;
            }
            else
            {
                bool ok = task->step();
                if( !ok )  task->end();
            }
        }
    }
}

//----------------------------------------------------------------------------------Task::cmd_start

void Task::cmd_start()
{ 
    if( !_running ) 
    { 
        _stopped = false; 
        _error = NULL; 
        _spooler->cmd_wake(); 
    } 
}

//------------------------------------------------------------------------------------Spooler::load

void Spooler::load()
{
    cerr << "Spooler::load\n";

    tzset();

    {
        Thread_semaphore::Guard guard = &_semaphore;

        load_xml();
    }
}

//-----------------------------------------------------------------------------------Spooler::start

void Spooler::start()
{
    cerr << "Spooler::start\n";

    Thread_semaphore::Guard guard = &_semaphore;

    FOR_EACH( Job_list, _job_list, it )
    {
        Sos_ptr<Task> task = SOS_NEW( Task( this, *it ) );

        _task_list.push_back( task );
    }

    _spooler_start_time = now();

    _comm_channel.start_thread();
}

//------------------------------------------------------------------------------------Spooler::wait

void Spooler::wait()
{
    tzset();

    if( _running_jobs_count == 0 )
    {
        Time  next_start_time = latter_day;
        Task* next_task = NULL;

        if( _paused )
        {
            cerr << "Spooler paused\n";
        }
        else
        {
            Thread_semaphore::Guard guard = &_semaphore;

            next_start_time = latter_day;
            FOR_EACH( Task_list, _task_list, it ) 
            {
                Task* task = *it;
                if( !task->_running & !task->_stopped ) 
                {
                    if( next_start_time > (*it)->_next_start_time )  next_task = *it, next_start_time = next_task->_next_start_time;
                }
            }

        }

        _sleep = true;  // Das muss in einen kritischen Abschnitt!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        Time wait_time = next_start_time - now();
        if( wait_time > 0 ) 
        {
            if( next_task )  cerr << "Job " << next_task->_job->_name << ": Nächster Start " << Sos_optional_date_time( next_start_time ) << '\n';
                       else  cerr << "Kein Job zu starten\n";

            while( _sleep  &&  wait_time > 0 )
            {
                double sleep_time = 1.0;
                sos_sleep( min( sleep_time, wait_time ) );
                wait_time -= sleep_time;
            }

            _sleep = false;
        }

        tzset();
    }
}

//-------------------------------------------------------------------------------------Spooler::run

void Spooler::run()
{
    cerr << "Spooler::run\n";

    while(1)
    {
        if( _pause     )  _pause = false, _paused = true;
        if( _stop      )  stop();
        if( _terminate )  break;
        if( _restart   )  restart();

        if( !_paused )  step();

        wait();
    }
}

//------------------------------------------------------------------------------------Spooler::stop

void Spooler::stop()
{
    cerr << "Spooler::stop\n";
    _stop = false;

    {
        FOR_EACH( Task_list, _task_list, it ) 
        {
            Task* task = *it;
        
            if( task->_running )  task->end();

            it = _task_list.erase( it );
        }
    }

    _object_set_class_list.clear();
    _job_list.clear();
}

//---------------------------------------------------------------------------------Spooler::restart

void Spooler::restart()
{
    cerr << "Spooler::restart\n";

    _restart = false;
    stop();
    load();
    start();
}

//-----------------------------------------------------------------------------Spooler::cmd_restart

void Spooler::cmd_restart()
{
    _restart = true;
    cmd_wake();
}

//--------------------------------------------------------------------------------Spooler::cmd_stop

void Spooler::cmd_stop()
{
    _stop = true;
    cmd_wake();
}

//---------------------------------------------------------------------------Spooler::cmd_terminate

void Spooler::cmd_terminate()
{
    _terminate = true;
    cmd_stop();
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler

//-----------------------------------------------------------------------------------------sos_main

int sos_main( int argc, char** argv )
{
    string config_filename  = read_profile_string( "factory.ini", "spooler", "config" );
    string log_filename     = read_profile_string( "factory.ini", "spooler", "log-file" );
    string spooler_id       = read_profile_string( "factory.ini", "spooler", "spooler-id" );
    string object_set_param = read_profile_string( "factory.ini", "spooler", "object-set-param" );

    for( Sos_option_iterator opt ( argc, argv ); !opt.end(); opt.next() )
    {
        if( opt.with_value( "config"           ) )  config_filename = opt.value();
        else
        if( opt.with_value( "log-file"         ) )  log_filename = opt.value();
        else
        if( opt.with_value( "spooler-id"       ) )  spooler_id = opt.value();
        else
        if( opt.with_value( "object-set-param" ) )  object_set_param = opt.value();
        else
            throw_sos_option_error( opt );
    }


    HRESULT hr = CoInitialize(NULL);
    if( FAILED(hr) )  throw_ole( hr, "CoInitialize" );

    {
        spooler::Spooler spooler;
        
        spooler._config_filename  = config_filename;
        spooler._log_filename     = log_filename;
        spooler._spooler_id       = spooler_id;
        spooler._object_set_param = object_set_param;

        spooler.load();
        spooler.start();
        spooler.run();
    }

    CoUninitialize();
    return 0;
}


} //namespace sos

