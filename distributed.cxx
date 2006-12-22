// $Id$

#include "spooler.h"
#include "../kram/msec.h"

#ifdef Z_WINDOWS
#   include <process.h>
#endif

using namespace zschimmer;

namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------------------------

//const time_t                    accepted_clock_difference       = Z_NDEBUG_DEBUG(  5,  2 );     // Die Uhren sollten noch besser übereinstimmen! ntp verwenden!
//const time_t                    warned_clock_difference         = Z_NDEBUG_DEBUG(  1,  1 ); 
const int                       heart_beat_period                       = Z_NDEBUG_DEBUG( 60, 20 );
const int                       max_processing_time                     = Z_NDEBUG_DEBUG( 10,  3 );     // Zeit, die gebraucht wird, um den Herzschlag auszuführen
const int                       active_heart_beat_minimum_check_period  = heart_beat_period / 2;
const int                       active_heart_beat_maximum_check_period  = heart_beat_period + max_processing_time + 1;
const int                       trauerfrist                             = 12*3600;                      // Trauerzeit, nach der Mitgliedssätze gelöscht werden
const int                       database_commit_visible_time            = 10;                           // Zeit, die die Datenbank braucht, um nach Commit Daten für anderen Client sichtbar zu machen.
const int                       precedence_check_period                 = active_heart_beat_maximum_check_period;

//const int                       Distributed_scheduler::min_precedence   = 0;
//const int                       Distributed_scheduler::max_precedence   = 9999;

//---------------------------------------------------------------------------------------Heart_beat

struct Heart_beat : Async_operation, Scheduler_object
{
                                Heart_beat                  ( Distributed_scheduler* );


    // Async_operation
    bool                        async_finished_             () const;
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );
    void                        set_alarm                   ();


    // Scheduler_operation
    Prefix_log*                 log                         ()                                      { return _log; }


    Fill_zero                  _zero_;
    Distributed_scheduler*     _distributed_scheduler;
    Prefix_log*                _log;
};

//-------------------------------------------------------------------------------------------------

struct Other_scheduler 
{
                                Other_scheduler             ()                                      : _zero_(this+1) {}

    Fill_zero                  _zero_;
    string                     _member_id;
    time_t                     _last_heart_beat_db;
    time_t                     _next_heart_beat_db;
    time_t                     _last_heart_beat_detected;
    bool                       _scheduler_993_logged;
    //time_t                     _clock_difference;
    //bool                       _clock_difference_checked;
    //bool                       _checking_clock_difference;
};

//----------------------------------------------------------------------Exclusive_scheduler_watchdog

struct Exclusive_scheduler_watchdog : Async_operation, Scheduler_object
{
                                Exclusive_scheduler_watchdog ( Distributed_scheduler* );


    // Async_operation
    bool                        async_finished_             () const;
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );
    void                        set_alarm                   ();


    // Scheduler_operation
    Prefix_log*                 log                         ()                                      { return _log; }


    void                        try_to_become_exclusive     ();
    bool                        check_exclusive_schedulers_heart_beat( Transaction* );
    void                        calculate_next_check_time   ();
  //void                        check_clock_difference      ( time_t last_heart_beat, time_t now );
    bool                        mark_as_exclusive           ();
    bool                        set_exclusive               ();
    bool                        check_backup_precedence     ();
    string                      read_distributed_scheduler_session_id();
    bool                        restart_when_active_scheduler_has_started();

    Fill_zero                  _zero_;
    Distributed_scheduler*     _distributed_scheduler;
    Other_scheduler            _other_scheduler;
    time_t                     _next_check_time;
    bool                       _is_scheduler_up;            // Scheduler ist nicht ordentlich beendet worden
    bool                       _announced_to_become_exclusive; // Wenn eine Meldung ausgegeben oder ein Datensatz zu ändern versucht wurde 
    time_t                     _next_precedence_check;
    string                     _distributed_scheduler_session_id;
  //time_t                     _set_exclusive_until;           // Nur für Access (kennt keine Sperre): Aktivierung bis dann (now+database_commit_visible_time) verzögern, dann nochmal prüfen
    Prefix_log*                _log;
};


//----------------------------------------------------------------------------my_string_from_time_t

static string my_string_from_time_t( time_t time )
{
    return string_gmt_from_time_t( time ) + " UTC";
}

//---------------------------------------------------------------------------Heart_beat::Heart_beat

Heart_beat::Heart_beat( Distributed_scheduler* m ) 
:
    _zero_(this+1),
    Scheduler_object( m->_spooler, this, Scheduler_object::type_heart_beat ),
    _distributed_scheduler(m),
    _log(m->_log)
{
}

//----------------------------------------------------------------------Heart_beat::async_finished_
    
bool Heart_beat::async_finished_() const
{ 
    return false;
}

//--------------------------------------------------------------------Heart_beat::async_state_text_
    
string Heart_beat::async_state_text_() const
{
    return "Heart_beat";
}

//----------------------------------------------------------------------Heart_beat::async_continue_

bool Heart_beat::async_continue_( Continue_flags )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );


    if( !db()->opened() )
    {
        _distributed_scheduler->async_wake();    // Datenbank ist geschlossen worden
        return true;
    }


    try
    {
        bool ok = _distributed_scheduler->_has_exclusiveness? _distributed_scheduler->do_exclusive_heart_beat( (Transaction*)NULL )
                                                            : _distributed_scheduler->do_a_heart_beat( (Transaction*)NULL, false );
        if( ok )
        {
            // Wir sind weiterhin aktiv
            set_alarm();
        }
        else
        {
            // Wir sind nicht mehr aktiv, Operation beenden!
            _distributed_scheduler->async_wake();
        }
    }
    catch( exception& )
    {
        _distributed_scheduler->async_wake();
        throw;
    }
        
    return true;
}

//----------------------------------------------------------------------------Heart_beat::set_alarm

void Heart_beat::set_alarm()
{
    set_async_next_gmtime( _distributed_scheduler->_next_heart_beat );   //last_heart_beat() + heart_beat_period );
}

//---------------------------------------Exclusive_scheduler_watchdog::Exclusive_scheduler_watchdog

Exclusive_scheduler_watchdog::Exclusive_scheduler_watchdog( Distributed_scheduler* m ) 
:
    _zero_(this+1),
    Scheduler_object( m->_spooler, this, Scheduler_object::type_exclusive_scheduler_watchdog ),
    _distributed_scheduler(m),
    _log(m->_log)
{
}

//----------------------------------------------------Exclusive_scheduler_watchdog::async_finished_
    
bool Exclusive_scheduler_watchdog::async_finished_() const
{ 
    return false;
}

//--------------------------------------------------Exclusive_scheduler_watchdog::async_state_text_

string Exclusive_scheduler_watchdog::async_state_text_() const
{
    S result;

    result << "Exclusive_scheduler_watchdog";

    //if( _set_exclusive_until )
    //{
    //    result << " (becoming exclusive at " << my_string_from_time_t( _set_exclusive_until) << ")";
    //}
    //else
    //{
    //    result << " (inexclusive)";
    //}

    return result;
}

//----------------------------------------------------Exclusive_scheduler_watchdog::async_continue_

bool Exclusive_scheduler_watchdog::async_continue_( Continue_flags )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );


    if( !db()->opened() )
    {
        _distributed_scheduler->async_wake();    // Datenbank ist geschlossen worden
        return true;
    }

    try
    {
        try_to_become_exclusive();

        if( !_distributed_scheduler->_has_exclusiveness  &&  _is_scheduler_up )
        {
            restart_when_active_scheduler_has_started();
        }
    }
    catch( exception& )
    {
        _distributed_scheduler->async_wake();
        throw;
    }

    if( _distributed_scheduler->has_exclusiveness() )
    {
        _distributed_scheduler->async_wake();    // Wir sind aktives Mitglied geworden, Exclusive_scheduler_watchdog beenden
    }
    else
    {
        set_alarm();                        // Wir sind weiterhin inaktives Mitglied
    }

    return true;
}

//----------------------------------------------------------Exclusive_scheduler_watchdog::set_alarm

void Exclusive_scheduler_watchdog::set_alarm()
{
    if( _distributed_scheduler->_db_next_heart_beat < _next_check_time )
    {
        Z_LOG2( "scheduler.distributed", __FUNCTION__ << "  Next heart beat at " << my_string_from_time_t( _distributed_scheduler->_db_next_heart_beat ) << "\n" );
        set_async_next_gmtime( _distributed_scheduler->_db_next_heart_beat );
    }
    else
    {
        Z_LOG2( "scheduler.distributed", __FUNCTION__ << "  Next check at " << my_string_from_time_t( _next_check_time ) << "\n" );
        set_async_next_gmtime( _next_check_time );
    }
}

//--------------------------------------------Exclusive_scheduler_watchdog::try_to_become_exclusive

void Exclusive_scheduler_watchdog::try_to_become_exclusive()
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( !_distributed_scheduler->_has_exclusiveness );


    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        _distributed_scheduler->do_a_heart_beat( &ta, false );     // Der Herzschlag ist ein Nebeneffekt von try_to_become_exclusive()

        bool none_has_exclusiveness = false;

        //if( !_set_exclusive_until )
        //{
        //    {
        //        Transaction ta ( db() );

                none_has_exclusiveness = check_exclusive_schedulers_heart_beat( &ta );   // Ruft auch do_a_heart_beat()

                ta.commit( __FUNCTION__ );
            //}

            if( none_has_exclusiveness )
            {
                if( _is_scheduler_up )
                {
                    bool has_precedence = check_backup_precedence();
                    if( has_precedence )
                    {
                        _announced_to_become_exclusive = true;

                        bool ok = mark_as_exclusive();
                        if( ok )  set_exclusive();
                    }
                }
            }
            else
            {
                _next_precedence_check = 0;
            }


        //}
        //else
        //{
        //    bool ok = _distributed_scheduler->do_a_heart_beat( NULL, true );
        //    if( !ok ) 
        //    {
        //        _set_exclusive_until = 0;
        //    }
        //}

        //if( _set_exclusive_until  &&  _set_exclusive_until <= ::time(NULL) )
        //{
        //    _set_exclusive_until = 0;
        //    _announced_to_become_exclusive = true; 
        //
        //    set_exclusive();
        //}
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ) ); }


    if( _announced_to_become_exclusive  &&  !_distributed_scheduler->_has_exclusiveness )  //&&  !_set_exclusive_until )
    {
        _announced_to_become_exclusive = false;
        _distributed_scheduler->show_active_schedulers( (Transaction*)NULL );
    }
}

//------------------------------Exclusive_scheduler_watchdog::check_exclusive_schedulers_heart_beat

bool Exclusive_scheduler_watchdog::check_exclusive_schedulers_heart_beat( Transaction* ta )
{
    // Rollback, wenn Funktion false liefert!

    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( ta );


    time_t now1            = ::time(NULL);      // Vor dem Datenbankzugriff
    bool   none_has_exclusiveness  = false;


    // Exklusiven Scheduler lesen

    Any_file select = ta->open_result_set( S() << 
                 "select `scheduler_member_id`, `last_heart_beat`, `next_heart_beat` "
                 " from " << _spooler->_members_tablename << 
                    "  where `scheduler_id`=" << sql::quoted( _spooler->id_for_db() ) <<
                      " and `exclusive` is not null",
                 __FUNCTION__ );

    if( select.eof() )
    {
        //other_member_id = _distributed_scheduler->empty_member_id();

        if( _other_scheduler._member_id != _distributed_scheduler->empty_member_id() )
        {
            _log->info( message_string( "SCHEDULER-805" ) );
            _other_scheduler = Other_scheduler();
            _other_scheduler._member_id = _distributed_scheduler->empty_member_id();
        }

        none_has_exclusiveness = true;  // Kein Scheduler ist exklusiv, 
        _is_scheduler_up = false;       // aber der Scheduler ist auch nicht hochgefahren (Satz mit empty_member_id() fehlt)
    }
    else
    {
        _is_scheduler_up = true;

        bool   other_member_timed_out = false;
        Record record                 = select.get_record();

        string other_member_id = record.as_string( 0 );

        if( other_member_id != _other_scheduler._member_id )
        {
            if( other_member_id == _distributed_scheduler->empty_member_id() )  
            {
                _log->info( message_string( "SCHEDULER-805" ) );
            }
            else 
            {
                _log->info( message_string( "SCHEDULER-801", other_member_id ) );
                _distributed_scheduler->show_active_schedulers( ta );
            }

            _other_scheduler = Other_scheduler();
            _other_scheduler._member_id                = other_member_id;
            //_other_scheduler._last_heart_beat_detected = ::time(NULL);

            //_log->info( message_string( "SCHEDULER-995", other_member_id, ::time(NULL) - last_heart_beat, other_http_url ) );
        }


        if( _other_scheduler._member_id == _distributed_scheduler->empty_member_id() )
        {
            none_has_exclusiveness = true;
        }
        else
        {
            time_t last_heart_beat = record.as_int64( 1 );                         //(time_t)( record.as_double( 1 ) + 0.5 );
            time_t next_heart_beat = record.null( 2 )? 0 : record.as_int64( 2 );   //(time_t)( record.as_double( 2 ) + 0.5 );

            //if( last_heart_beat > now1 )
            //{
            //    time_t diff = last_heart_beat - now1;
            //    
            //    if( diff >= warned_clock_difference + 1 )
            //        _log->warn( message_string( "SCHEDULER-364", diff, other_member_id ) );
            //}
            bool warned = false;

            if( last_heart_beat == _other_scheduler._last_heart_beat_db )   // Kein neuer Herzschlag?
            {
                if( _other_scheduler._last_heart_beat_detected + heart_beat_period + max_processing_time/2 < now1 )
                {
                    _log->warn( message_string( "SCHEDULER-994", 
                                                    other_member_id, 
                                                    my_string_from_time_t( _other_scheduler._last_heart_beat_detected ), 
                                                    now1 - _other_scheduler._last_heart_beat_detected ) );
                    warned = true;

                    if( _other_scheduler._last_heart_beat_detected + heart_beat_period + max_processing_time < now1 )  
                        other_member_timed_out = true;
                }
            }
            else
            {
                _other_scheduler._last_heart_beat_detected = ::time(NULL);
            }

            if( !other_member_timed_out  &&  !warned  &&  !_other_scheduler._scheduler_993_logged 
             && next_heart_beat + max_processing_time < now1 )
            {
                _other_scheduler._scheduler_993_logged = true;
                _log->warn( message_string( "SCHEDULER-993", 
                                                other_member_id, 
                                                my_string_from_time_t( next_heart_beat ),
                                                now1 - next_heart_beat ) );

                //if( next_heart_beat + heart_beat_delay < now1 )  other_member_timed_out = true;
            }

            //if( !other_member_timed_out  &&  last_heart_beat + heart_beat_period + heart_beat_delay/2 < now1 )
            //{
            //    _log->warn( message_string( "SCHEDULER-994", 
            //                                    other_member_id, 
            //                                    my_string_from_time_t( next_heart_beat ), 
            //                                    now1 - ( last_heart_beat + heart_beat_period ) ) );

            //    //if( last_heart_beat + heart_beat_period + heart_beat_delay < now1 )  other_member_timed_out = true;
            //}
            
            //if( !other_member_timed_out  &&  _other_scheduler._checking_clock_difference  &&  last_heart_beat == _other_scheduler._next_heart_beat_db )
            //{
            //    check_clock_difference( last_heart_beat, now1 );
            //}

            if( other_member_timed_out )  none_has_exclusiveness = true;
                                    else  _is_scheduler_up = true;  // Bei Herzschlag eines aktiven Schedulers kann er nicht heruntergefahren sein

            _other_scheduler._last_heart_beat_db = last_heart_beat;
            _other_scheduler._next_heart_beat_db = next_heart_beat;
        }
    }

    calculate_next_check_time();

    return none_has_exclusiveness;
}

//------------------------------------------Exclusive_scheduler_watchdog::calculate_next_check_time

void Exclusive_scheduler_watchdog::calculate_next_check_time()
{
    time_t now   = ::time(NULL);

    _next_check_time = now + active_heart_beat_maximum_check_period;

    //if( _set_exclusive_until  &&  _next_check_time > _set_exclusive_until )
    //{
    //    _next_check_time = _set_exclusive_until;
    //    extra_log << ", warten, bis Datenbank Änderungen für alle sichtbar gemacht hat";
    //}
    //else
    {
        time_t delay          = max_processing_time + 1;   // Erst in der folgenden Sekunde prüfen
        time_t new_next_check = _other_scheduler._next_heart_beat_db + delay + 1;

        if( new_next_check - now >= active_heart_beat_minimum_check_period  &&  new_next_check < _next_check_time )
        {
            time_t diff = new_next_check - _next_check_time;
            if( abs(diff) > 2 )  Z_LOG2( "scheduler.distributed", __FUNCTION__ << "  Synchronized with _next_heart_beat_db=" << my_string_from_time_t( _other_scheduler._next_heart_beat_db ) << ": " << diff << "s\n" );
            _next_check_time = new_next_check;
        }
    }
}

//--------------------------------------------Exclusive_scheduler_watchdog::check_backup_precedence

bool Exclusive_scheduler_watchdog::check_backup_precedence()
{
    bool result = false;

    //if( _distributed_scheduler->backup_precedence() == Distributed_scheduler::max_precedence )        Überflüssig
    //{
    //    result = true;
    //}
    //else
    if( !_next_precedence_check  ||  _next_precedence_check <= ::time(NULL) )
    {
        try
        {
            Transaction ta ( db() );

            bool     higher_precedence = false;
            time_t   now               = ::time(NULL);
            Any_file result_set        = ta.open_result_set
                ( 
                    S() << "select `precedence`, `scheduler_member_id`, `http_url`  from " << _spooler->_members_tablename << 
                       "  where `precedence`>" << _distributed_scheduler->backup_precedence() <<
                         "  and `active` is null " 
                         "  and `exclusive` is null " 
                         "  and `next_heart_beat`>=" << ( now - ( heart_beat_period + max_processing_time ) ) <<    // Vorrang funktioniert nur, 
                         "  and `next_heart_beat`<=" << ( now + ( heart_beat_period + max_processing_time ) ),      // wenn die Uhren übereinstimmen
                    __FUNCTION__
                );
            
            while( !result_set.eof() )
            {
                Record record = result_set.get_record();
                _log->info( message_string( "SCHEDULER-814", record.as_string(0), record.as_string(1), record.as_string(2) ) );
                higher_precedence = true;
            }

            if( higher_precedence  &&  !_next_precedence_check )
            {
                _next_precedence_check = ::time(NULL) + precedence_check_period;
            }
            else
            {
                //if( precedence_count )  _log->info( message_string( "SCHEDULER-814", "ignored" ) );
                _next_precedence_check = 0;
                result = true;
            }
        }
        catch( exception& x )  { _log->error( S() << x.what() << ", while checking backup precendence" ); }
    }
    else
    {
        Z_LOG2( "scheduler.distributed", __FUNCTION__ << "  continuing waiting...\n" );
    }


    return result;
}

//--------------------------------------------------Exclusive_scheduler_watchdog::mark_as_exclusive

bool Exclusive_scheduler_watchdog::mark_as_exclusive()
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( !_distributed_scheduler->_has_exclusiveness );

    _distributed_scheduler->calculate_next_heart_beat();

    bool   ok;
    time_t now                    = ::time(NULL);
    time_t new_db_last_heart_beat = now;
    time_t new_db_next_heart_beat = _distributed_scheduler->_next_heart_beat;


    _log->debug( message_string( "SCHEDULER-803" ) );  // "This Scheduler becomes exclusive"

    try
    {
        Transaction ta ( db() );

        _distributed_scheduler->lock_member_records( &ta, _other_scheduler._member_id, _distributed_scheduler->member_id() );


        // Bisher aktiven Scheduler inaktivieren

        sql::Update_stmt update ( ta.database_descriptor(), _spooler->_members_tablename );
    
        update[ "active"    ] = sql::null_value;
        update[ "exclusive" ] = sql::null_value;
        update.and_where_condition( "scheduler_member_id", _other_scheduler._member_id );
        update.and_where_condition( "exclusive"          , 1 );
        
        if( _other_scheduler._member_id != _distributed_scheduler->empty_member_id() )
        {
            update.and_where_condition( "last_heart_beat"    , _other_scheduler._last_heart_beat_db );
            update.and_where_condition( "next_heart_beat"    , _other_scheduler._next_heart_beat_db );
        }

        ok = ta.try_execute_single( update, __FUNCTION__ );

        if( ok )
        {
            sql::Update_stmt update ( ta.database_descriptor(), _spooler->_members_tablename );
        
            update[ "active"          ] = 1;
            update[ "exclusive"       ] = 1;
            update[ "last_heart_beat" ] = new_db_last_heart_beat;
            update[ "next_heart_beat" ] = new_db_next_heart_beat;
            update.and_where_condition( "scheduler_member_id", _distributed_scheduler->member_id() );
            update.and_where_condition( "active"             , sql::null_value );
            update.and_where_condition( "exclusive"          , sql::null_value );

            ok = ta.try_execute_single( update, __FUNCTION__ );
        }

        ta.commit( __FUNCTION__ );
    }
    catch( exception& x )   // Bei optimistischer Sperrung kann es eine Exception geben
    { 
        ok = false;
        _log->debug3( S() << x.what() << ", in " << __FUNCTION__ );
    }

    if( ok )
    {
        _distributed_scheduler->_db_last_heart_beat = new_db_last_heart_beat;
        _distributed_scheduler->_db_next_heart_beat = new_db_next_heart_beat;

        //_set_exclusive_until = ::time(NULL);
        //if( db_mode == use_commit_visible_time )  _set_exclusive_until += database_commit_visible_time + 1;     // Nachfolgende Sekunde
    }

    return ok;
}

//----------------------------------------------Exclusive_scheduler_watchdog::check_clock_difference

//void Exclusive_scheduler_watchdog::check_clock_difference( time_t last_heart_beat, time_t now )
//{
//    _other_scheduler._clock_difference         = last_heart_beat - now;
//    _other_scheduler._clock_difference_checked = true;
//    
//    time_t own_delay = ::time(NULL) - _next_check_time;
//
//    if( abs( _other_scheduler._clock_difference ) < own_delay + warned_clock_difference + 1 )
//    {
//        _log->info( message_string( "SCHEDULER-804", _other_scheduler._member_id ) );
//    }
//    else
//    {
//        _log->warn( message_string( "SCHEDULER-364", _other_scheduler._clock_difference, _other_scheduler._member_id ) );
//    }
//
//    _other_scheduler._checking_clock_difference = false;
//}

//------------------------------------------------------Exclusive_scheduler_watchdog::set_exclusive

bool Exclusive_scheduler_watchdog::set_exclusive()
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( !_distributed_scheduler->_has_exclusiveness );

    bool ok;

    try
    {
        Transaction ta ( db() );

        ok = _distributed_scheduler->check_database_integrity( &ta );
        if( ok )
        {
            ok = _distributed_scheduler->do_exclusive_heart_beat( &ta );
        }

        ta.commit( __FUNCTION__ );
    }
    catch( exception& x ) 
    {
        ok = false;
        _log->debug3( S() << x.what() << ", in " << __FUNCTION__ );
    }

    if( ok )
    {
        _other_scheduler._member_id = _distributed_scheduler->member_id();

        _distributed_scheduler->_is_active         = true;
        _distributed_scheduler->_has_exclusiveness = true;

        _log->info( message_string( "SCHEDULER-806" ) );
    }

    return ok;
}

//--------------------------Exclusive_scheduler_watchdog::restart_when_active_scheduler_has_started

bool Exclusive_scheduler_watchdog::restart_when_active_scheduler_has_started()
{
    bool result = false;

    if( _is_scheduler_up )
    {
        string current_id = read_distributed_scheduler_session_id();

        if( current_id != ""  &&  current_id != _distributed_scheduler_session_id )
        {
            _distributed_scheduler_session_id = current_id;

            _log->info( message_string( "SCHEDULER-818" ) );
            bool restart = true;
            bool shutdown = false;
            _spooler->cmd_terminate( restart, INT_MAX, shutdown );
            result = true;
        }
    }

    return result;
}

//------------------------------Exclusive_scheduler_watchdog::read_distributed_scheduler_session_id

string Exclusive_scheduler_watchdog::read_distributed_scheduler_session_id()
{
    S result;

    if( _is_scheduler_up )
    {
        if( db()  &&  db()->opened() )
        for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ )
        try
        {
            Any_file select = ta.open_result_set( 
                                S() << "select `host`, `pid`, `running_since`"
                                       "  from " << _spooler->_members_tablename <<
                                      "  where `scheduler_member_id`=" + sql::quoted( _distributed_scheduler->empty_member_id()  ),
                                __FUNCTION__ );

            if( !select.eof() )
            {
                Record record = select.get_record();

                result << record.as_string( "host" ) << "." << 
                          record.as_string( "pid" ) << "." << 
                          record.as_string( "running_since" );
            }
        }
        catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ) ); }
    }

    return result;
}

//------------------------------------------------static Distributed_scheduler::string_from_command

string Distributed_scheduler::string_from_command( Command command )
{
    switch( command )
    {
        case cmd_none                 : return "";
        case cmd_terminate            : return "terminate";
        case cmd_terminate_and_restart: return "terminate_and_restart";
        default:                        return as_string( (int)command );
    }
}

//------------------------------------------------static Distributed_scheduler::command_from_string

Distributed_scheduler::Command Distributed_scheduler::command_from_string( const string& command )
{
    if( command == ""                      )  return cmd_none;
    if( command == "terminate"             )  return cmd_terminate;
    if( command == "terminate_and_restart" )  return cmd_terminate_and_restart;
    return cmd_none;
}

//-----------------------------------------------------Distributed_scheduler::Distributed_scheduler

Distributed_scheduler::Distributed_scheduler( Spooler* spooler )
:
    Scheduler_object( spooler, this, type_scheduler_member ),
    _zero_(this+1)
    //_backup_precedence( max_precedence )
{
}

//----------------------------------------------------Distributed_scheduler::~Distributed_scheduler
    
Distributed_scheduler::~Distributed_scheduler()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler.distributed", "ERROR in " << __FUNCTION__ << ": " << x.what() << "\n" ); }
}

//---------------------------------------------------------------------Distributed_scheduler::close

void Distributed_scheduler::close()
{
    if( !_closed )
    {
        Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

        close_operations();
        set_async_manager( NULL );

        if( member_id() != "" )
        try
        {
            Transaction ta ( db() );

            mark_as_inactive( &ta, true );    // Member-Satz löschen

            delete_old_member_records( &ta );

            ta.commit( __FUNCTION__ );
        }
        catch( exception& x )
        {
            if( _log )  _log->warn( S() << x.what() << ", in " << __FUNCTION__ );
        }

        _closed = true;
    }
}

//------------------------------------------------------------------Distributed_scheduler::shutdown

void Distributed_scheduler::shutdown()
{
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

    if( db()  &&  db()->opened()  )
    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        mark_as_inactive( &ta, true, true );  // Member-Satz und leeren Member-Satz löschen

        ta.commit( __FUNCTION__ );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ) ); }

    close();
}

//-------------------------------------------------Distributed_scheduler::calculate_next_heart_beat

void Distributed_scheduler::calculate_next_heart_beat()
{
    _next_heart_beat = ::time(NULL) + heart_beat_period;
}

//-------------------------------------------------------Distributed_scheduler::lock_member_records

void Distributed_scheduler::lock_member_records( Transaction* ta, const string& member1_id, const string& member2_id )
{
    assert( ta );

    S sql;
    sql << "select `scheduler_member_id`  from " << _spooler->_members_tablename << " %update_lock"
           "  where `scheduler_member_id`"
           " in (" << sql::quoted( member1_id ) << "," 
                   << sql::quoted( member2_id ) << ")";
    //sql << "select `scheduler_member_id`  from " << _spooler->_members_tablename
    //if( db()->lock_syntax() == db_lock_with_updlock  )  sql << "  WITH(UPDLOCK)";
    //sql << "  where `scheduler_member_id`"
    //    << " in (" << sql::quoted( member1_id ) << "," 
    //               << sql::quoted( member2_id ) << ")";
    //if( db()->lock_syntax() == db_lock_for_update )  sql << " FOR UPDATE";

    ta->set_transaction_written();
    ta->open_result_set( sql, __FUNCTION__ );
}

//-------------------------------------------------Distributed_scheduler::delete_old_member_records

void Distributed_scheduler::delete_old_member_records( Transaction* ta )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( ta );

    ta->execute
    ( 
        S() << "DELETE from " << _spooler->_members_tablename << 
                "  where `scheduler_id`= " << sql::quoted( _spooler->id_for_db() ) <<
                   " and `active` is null"
                   " and `next_heart_beat`<" << ( ::time(NULL) - trauerfrist ), 
        S() << "Deleting inactive records with a heart beat more then " << trauerfrist << "s ago"
    );

    if( int record_count = ta->record_count() )
    {
        Z_LOG2( "scheduler.distributed", record_count << " veraltete Sätze aus " << _spooler->_members_tablename << " gelöscht\n" );
    }
}

//---------------------------------------------------------------------Distributed_scheduler::start

bool Distributed_scheduler::start()
{
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

    assert( !_heart_beat );
    assert( !_exclusive_scheduler_watchdog );
    assert( !_is_backup || _demand_exclusiveness );

    if( !db()->opened() )  z::throw_xc( "SCHEDULER-357" ); 
    if( db()->lock_syntax() == db_lock_none )  z::throw_xc( "SCHEDULER-359", db()->dbms_name() );

    if( _scheduler_member_id == "" )  make_scheduler_member_id();
    check_member_id();

    create_table_when_needed();


    bool scheduler_up = check_empty_member_record();    // "Scheduler ist hochgefahren"

    {
        Transaction ta ( db() );

        bool db_broken = !check_database_integrity( &ta );
        if( db_broken )  z::throw_xc( "SCHEDULER-364", __FUNCTION__ );

        delete_old_member_records( &ta );
        insert_member_record( &ta );
        ta.commit( __FUNCTION__ );
    }


    if( _demand_exclusiveness )
    {
        _exclusive_scheduler_watchdog = Z_NEW( Exclusive_scheduler_watchdog( this ) );
        _exclusive_scheduler_watchdog->try_to_become_exclusive();   // Stellt sofort _is_scheduler_up fest
        _exclusive_scheduler_watchdog->_is_scheduler_up = scheduler_up;
        _exclusive_scheduler_watchdog->_distributed_scheduler_session_id = _exclusive_scheduler_watchdog->read_distributed_scheduler_session_id();

        if( _has_exclusiveness )  _exclusive_scheduler_watchdog = NULL;
    }
    else
    {
        _is_active = true;
        //set_active();
    }


    set_async_manager( _spooler->_connection_manager );
    start_operations();

    return true;
}

//------------------------------------------------Distributed_scheduler::wait_until_is_scheduler_up

bool Distributed_scheduler::wait_until_is_scheduler_up()
{
    _log->info( message_string( "SCHEDULER-800" ) );

    while( !_spooler->is_termination_state_cmd()  &&  !is_scheduler_up() )
    {
        _spooler->simple_wait_step();
        async_check_exception();
    }

    bool ok = is_scheduler_up();

    if( ok )  assert_database_integrity( __FUNCTION__ );

    return ok;
}

//------------------------------------------------------Distributed_scheduler::wait_until_is_active

bool Distributed_scheduler::wait_until_is_active()
{
    if( !is_scheduler_up() ) _log->info( message_string( "SCHEDULER-800" ) );
        //else  _log->info( message_string( __FUNCTION__ ) );

    bool was_scheduler_up = is_scheduler_up();

    while( !_spooler->is_termination_state_cmd()  &&  !_is_active )  
    {
        if( was_scheduler_up  &&  !is_scheduler_up() ) 
        {
            _log->info( message_string( "SCHEDULER-802" ) );
            was_scheduler_up = false; 
        }

        _spooler->simple_wait_step();
        async_check_exception();
    }

    bool ok = _is_active;

    if( ok )  assert_database_integrity( __FUNCTION__ );

    return ok;
}

//----------------------------------------------Distributed_scheduler::wait_until_has_exclusiveness

bool Distributed_scheduler::wait_until_has_exclusiveness()
{
    wait_until_is_active();

    if( exclusive_member_id() != "" )  _log->info( message_string( "SCHEDULER-801", exclusive_member_id() ) );

    while( !_spooler->is_termination_state_cmd()  &&  !has_exclusiveness() )  _spooler->simple_wait_step();

    bool ok = has_exclusiveness();

    if( ok )
    {
        Transaction ta ( db() );
        if( !check_database_integrity( &ta ) )  z::throw_xc( "SCHEDULER-364", __FUNCTION__ );
    }

    return ok;
}

//--------------------------------------------------Distributed_scheduler::create_table_when_needed

void Distributed_scheduler::create_table_when_needed()
{
    Transaction ta ( db() );

    db()->create_table_when_needed( &ta, _spooler->_members_tablename,
            "`scheduler_member_id`"    " varchar(100) not null, "
            "`scheduler_id`"           " varchar(100) not null, "
            "`version`"                " varchar(100) not null, "
            "`running_since`"          " datetime, "
            "`precedence`"             " integer, "
            "`last_heart_beat`"        " integer, "     //numeric(14,3) not null, "
            "`next_heart_beat`"        " integer, "     //numeric(14,3), "
            "`active`"                 " boolean, "                     // null oder 1 (not null)
            "`exclusive`"              " boolean, "                     // null oder 1 (not null
            "`command`"                " varchar(100), "
            "`host`"                   " varchar(100) not null, "
            "`udp_port`"               " integer, "
            "`tcp_port`"               " integer, "
            "`pid`"                    " varchar(20)  not null, "
            "`http_url`"               " varchar(100), "
            "primary key( `scheduler_member_id` )" );

    ta.commit( __FUNCTION__ );
}

//----------------------------------------------------------Distributed_scheduler::start_operations

void Distributed_scheduler::start_operations()
{
    if( _demand_exclusiveness  &&  !has_exclusiveness() )
    {
        if( !_exclusive_scheduler_watchdog )
            _exclusive_scheduler_watchdog = Z_NEW( Exclusive_scheduler_watchdog( this ) );

        _exclusive_scheduler_watchdog->set_alarm();
        _exclusive_scheduler_watchdog->set_async_manager( _spooler->_connection_manager );
    }
    else
    if( _is_active )
    {
        assert( !_exclusive_scheduler_watchdog );

        if( !_heart_beat )
            _heart_beat = Z_NEW( Heart_beat( this ) );

        _heart_beat->set_alarm();
        _heart_beat->set_async_manager( _spooler->_connection_manager );
    }
    else
        z::throw_xc( __FUNCTION__ );
}

//----------------------------------------------------------Distributed_scheduler::close_operations

void Distributed_scheduler::close_operations()
{
    if( _heart_beat ) 
    {
        _heart_beat->set_async_manager( NULL );
        _heart_beat = NULL;
    }

    if( _exclusive_scheduler_watchdog )
    {
        _exclusive_scheduler_watchdog->set_async_child( NULL );
        _exclusive_scheduler_watchdog = NULL;
    }
}

//-----------------------------------------------------------Distributed_scheduler::async_continue_

bool Distributed_scheduler::async_continue_( Continue_flags )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    if( _heart_beat                   )  _heart_beat                  ->async_check_exception( "Error in Scheduler member operation" );
    if( _exclusive_scheduler_watchdog )  _exclusive_scheduler_watchdog->async_check_exception( "Error in Scheduler member operation" );

    if( _has_exclusiveness  &&  _exclusive_scheduler_watchdog )
    {
        _exclusive_scheduler_watchdog->set_async_manager( NULL );
        _exclusive_scheduler_watchdog = NULL;
    }

    if( !_has_exclusiveness  ||  !_heart_beat )
    {
        start_operations();
    }

    assert( _heart_beat );
    assert( _has_exclusiveness == !_exclusive_scheduler_watchdog );

    //if( !_is_exclusiveness_stolen )
    //{
    //    if( db()->opened() )
    //    {
    //        start_inexclusive_scheduler_watchdog();
    //    }
    //    else
    //    if( _is_backup )  z::throw_xc( "SCHEDULER-357" );
    //}

    return true;
}

//------------------------------------------------------Distributed_scheduler::do_exclusive_heart_beat

bool Distributed_scheduler::do_exclusive_heart_beat( Transaction* outer_transaction )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    bool ok = do_a_heart_beat( outer_transaction, true );

    if( !ok )
    {
        _has_exclusiveness       = false;
        _is_exclusiveness_stolen = true;
        _is_active               = false;

        _log->error( message_string( "SCHEDULER-372" ) );   // "Some other Scheduler member has become exclusive"
    }

    return ok;
}
    
//-----------------------------------------------------------Distributed_scheduler::do_a_heart_beat

bool Distributed_scheduler::do_a_heart_beat( Transaction* outer_transaction )
{
    return do_a_heart_beat( outer_transaction, has_exclusiveness() );
}

//------------------------------------------------------------Distributed_scheduler::do_a_heart_beat

bool Distributed_scheduler::do_a_heart_beat( Transaction* outer_transaction, bool db_record_marked_exclusive )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    time_t old_next_heart_beat = _db_next_heart_beat;
    bool   ok                  = false;
    bool   was_late            = false;


    calculate_next_heart_beat();

    if( !db()->opened() )  return false;


  //AGAIN:
    for( Retry_transaction ta ( db(), outer_transaction ); ta.enter_loop(); ta++ ) try
    {
        ok = true;
        if( _spooler->_db_check_integrity )  ok = check_database_integrity( &ta );

        if( ok )  ok = try_to_heartbeat_member_record( &ta, db_record_marked_exclusive );

        ta.commit( __FUNCTION__ );


        check_heart_beat_is_in_time( old_next_heart_beat );  // Verspätung wird nur gemeldet
        //bool is_in_time = 
        //if( !is_in_time  &&  !was_late )
        //{
        //    was_late = true;
        //    if( !outer_transaction )   goto AGAIN;  // Noch einmal in neuer Transaktion, um zu prüfen, ob uns jemand nach dem Commit die Aktivität genommen hat (unwahrscheinlich)
        //}

        //time_t now = ::time(NULL);
        //if( !was_late  &&  now - old_next_heart_beat > max_processing_time )    // Herzschlag noch in der Frist?
        //{
        //    was_late = true;
        //    _log->warn( message_string( "SCHEDULER-996", my_string_from_time_t( old_next_heart_beat ), now - old_next_heart_beat ) );
        //    if( !outer_transaction )   goto AGAIN;  // Noch einmal in neuer Transaktion, um zu prüfen, ob uns jemand die Aktivität genommen hat.
        //}
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ) ); }

    return ok;
}
    
//--------------------------------------------Distributed_scheduler::try_to_heartbeat_member_record

bool Distributed_scheduler::try_to_heartbeat_member_record( Transaction* ta, bool db_record_marked_exclusive )
{
    calculate_next_heart_beat();

    time_t new_db_last_heart_beat = ::time(NULL);
    time_t new_db_next_heart_beat = _next_heart_beat;

    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "  new_db_last_heart_beat=" << new_db_last_heart_beat << " (" << my_string_from_time_t( new_db_last_heart_beat ) << "), "
                                                        "new_db_next_heart_beat=" << new_db_next_heart_beat << " (" << my_string_from_time_t( new_db_next_heart_beat ) << ")\n" );

    assert( ta );


    sql::Update_stmt update ( ta->database_descriptor(), _spooler->_members_tablename );
    
    update[ "last_heart_beat" ] = new_db_last_heart_beat;
    update[ "next_heart_beat" ] = new_db_next_heart_beat;

    update.and_where_condition( "scheduler_member_id", member_id()      );
    update.and_where_condition( "last_heart_beat"    , _db_last_heart_beat );
    update.and_where_condition( "next_heart_beat"    , _db_next_heart_beat );
    update.and_where_condition( "exclusive"          , db_record_marked_exclusive? sql::Value(1) : sql::null_value );
    update.and_where_condition( "command"            , _heart_beat_command? sql::Value( string_from_command( _heart_beat_command ) ) 
                                                                          : sql::null_value );

    bool record_is_updated = ta->try_execute_single( update, __FUNCTION__ );

    if( !record_is_updated )
    {
        update.remove_where_condition( "command" );
        record_is_updated = ta->try_execute_single( update, __FUNCTION__ );

        if( record_is_updated )     // Neues Kommando
        {
            Any_file result_set = ta->open_result_set
            ( 
                S() << "select `command`"
                       "  from " << _spooler->_members_tablename << 
                       "  where `scheduler_member_id`=" << sql::quoted( member_id() ),
                __FUNCTION__
            );

            if( !result_set.eof() )
            {
                _heart_beat_command_string = result_set.get_record().as_string( 0 );
                _heart_beat_command        = command_from_string( _heart_beat_command_string) ;
                if( _heart_beat_command )  execute_command( _heart_beat_command );
            }
        }
    }

    if( record_is_updated )
    {
        _db_last_heart_beat = new_db_last_heart_beat;
        _db_next_heart_beat = new_db_next_heart_beat;
    }

    return record_is_updated;
}

//-----------------------------------------------Distributed_scheduler::check_heart_beat_is_in_time

bool Distributed_scheduler::check_heart_beat_is_in_time( time_t expected_next_heart_beat )
{
    bool   result;
    time_t now = ::time(NULL);
    
    if( now - expected_next_heart_beat <= max_processing_time )    
    {
        _late_heart_beat = 0;
        result = true;  // Herzschlag ist in der Frist
    }
    else
    {
        result = false;

        if( _late_heart_beat != expected_next_heart_beat )
        {
            _late_heart_beat = expected_next_heart_beat;
            _log->warn( message_string( "SCHEDULER-996", my_string_from_time_t( expected_next_heart_beat ), now - expected_next_heart_beat ) );
        }
    }

    return result;
}

//-----------------------------------------------------------Distributed_scheduler::execute_command

void Distributed_scheduler::execute_command( Command command )
{
    _log->info( message_string( "SCHEDULER-811", _heart_beat_command_string ) );

    switch( command )
    {
        case cmd_terminate            : _spooler->cmd_terminate( false, INT_MAX, false );  break;
        case cmd_terminate_and_restart: _spooler->cmd_terminate( true , INT_MAX, false );  break;
        default: ;
    }
}

//-------------------------------------------------Distributed_scheduler::assert_database_integrity

void Distributed_scheduler::assert_database_integrity( const string& message_text )
{
    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        if( !check_database_integrity( &ta ) )  z::throw_xc( "SCHEDULER-364", message_text );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ) ); }
}

//--------------------------------------------------Distributed_scheduler::check_database_integrity

bool Distributed_scheduler::check_database_integrity( Transaction* ta )
{
    bool ok = false;

    S sql_stmt;
    sql_stmt << "select 1, count( `exclusive` )"
                "  from " << _spooler->_members_tablename <<
                "  where `scheduler_id`=" << sql::quoted( _spooler->id_for_db() ) <<
             "   union  "
              "  select 2, count(*)"
                "  from " << _spooler->_members_tablename <<
                "  where `scheduler_member_id`=" << sql::quoted( empty_member_id() );

    
    Any_file result = ta->open_result_set( sql_stmt, __FUNCTION__ );    // In _einem_ Snapshot abfragen

    int exclusive_count = 0;
    int empty_count     = 0;

    while( !result.eof() )
    {
        Record record = result.get_record();
        switch( record.as_int( 0 ) )
        {
            case 1: exclusive_count = record.as_int( 1 );  break;
            case 2: empty_count     = record.as_int( 1 );  break;
            default: assert((__FUNCTION__,0));
        }
    }


    ok = exclusive_count == 1 && empty_count == 1  ||
         exclusive_count == 0 && empty_count == 0;

    if( !ok )
    {
        _log->error( message_string( "SCHEDULER-371", S() << "exclusive_count=" << exclusive_count << " empty_count=" << empty_count ) );
        mark_as_inactive( ta );
    }

    return ok;
}

//------------------------------------------------Distributed_scheduler::check_empty_member_record

bool Distributed_scheduler::check_empty_member_record()
{
    bool   is_scheduler_up = false;
    time_t now             = ::time(NULL);
    Record record;

    try
    {
        Transaction ta ( db() );

        Any_file select = ta.open_result_set
        (
            S() << "select `exclusive`"
                   "  from " << _spooler->_members_tablename << 
                   "  where `scheduler_member_id`=" << sql::quoted( empty_member_id() ),
            __FUNCTION__
        );

        if( !select.eof() )
        {
            is_scheduler_up = true;
            record = select.get_record();
        }
        else
        if( !_is_backup )
        {
            sql::Insert_stmt stmt ( ta.database_descriptor(), _spooler->_members_tablename );

            stmt[ "scheduler_member_id" ] = empty_member_id();
          //stmt[ "precedence"          ] = 0;
          //stmt[ "last_heart_beat"     ] = Beide Felder NULL lassen, damit sie nicht als veraltete Einträge angesehen und gelöscht werden
          //stmt[ "next_heart_beat"     ] = 
            stmt[ "exclusive"           ] = 1;
            stmt[ "scheduler_id"        ] = _spooler->id_for_db();
            stmt[ "version"             ] = _spooler->_version;
            stmt[ "running_since"       ].set_datetime( string_local_from_time_t( now ) );
            stmt[ "host"                ] = _spooler->_hostname;
            stmt[ "pid"                 ] = getpid();
            stmt[ "http_url"            ] = _spooler->http_url();

            ta.execute( stmt, "The empty member record" );
            ta.commit( __FUNCTION__ );

            is_scheduler_up = true;
        }
    }
    catch( exception& )
    {
        Transaction ta ( db() );

        Any_file select = ta.open_result_set
        (
            S() << "select `exclusive`"
                   "  from " << _spooler->_members_tablename << 
                   "  where `scheduler_member_id`=" << sql::quoted( empty_member_id() ),
            __FUNCTION__
        );

        if( select.eof() )  throw;

        is_scheduler_up = true;
        record = select.get_record();
    }

    return is_scheduler_up;
}

//------------------------------------------------------Distributed_scheduler::insert_member_record

void Distributed_scheduler::insert_member_record( Transaction* ta )
{
    assert( ta );
    assert( !_is_active );


    calculate_next_heart_beat();

    time_t now                    = ::time(NULL);
    time_t new_db_last_heart_beat = now;
    time_t new_db_next_heart_beat = _next_heart_beat;

    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "  new_db_last_heart_beat=" << new_db_last_heart_beat << " (" << my_string_from_time_t( new_db_last_heart_beat) << "), "
                                                        "new_db_next_heart_beat=" << new_db_next_heart_beat << " (" << my_string_from_time_t( new_db_next_heart_beat) << ")\n" );


    // Darf nicht vorhanden sein:  db()->execute( S() << "DELETE from " << _spooler->_members_tablename << "  where `scheduler_member_id`=" << sql::quoted( member_id() ) );

    sql::Insert_stmt insert ( ta->database_descriptor(), _spooler->_members_tablename );
    
    insert[ "scheduler_member_id" ] = member_id();
    insert[ "precedence"          ] = _backup_precedence;
    insert[ "last_heart_beat"     ] = new_db_last_heart_beat;
    insert[ "next_heart_beat"     ] = new_db_next_heart_beat;
    insert[ "active"              ] = sql::null_value;
    insert[ "exclusive"           ] = sql::null_value;
    insert[ "scheduler_id"        ] = _spooler->id_for_db();
    insert[ "version"             ] = _spooler->_version;
    insert[ "running_since"       ].set_datetime( string_local_from_time_t( new_db_last_heart_beat ) );
    insert[ "host"                ] = _spooler->_hostname;
    
    if( _spooler->tcp_port() )
    insert[ "tcp_port"            ] = _spooler->tcp_port();

    if( _spooler->udp_port() )
    insert[ "udp_port"            ] = _spooler->udp_port();

    insert[ "pid"                 ] = getpid();
    insert[ "http_url"            ] = _spooler->http_url();

    ta->execute( insert );

    _db_last_heart_beat = new_db_last_heart_beat;
    _db_next_heart_beat = new_db_next_heart_beat;
}

//----------------------------------------------------------Distributed_scheduler::mark_as_inactive

void Distributed_scheduler::mark_as_inactive( Transaction* ta, bool delete_inactive_record, bool delete_new_active_record )
{
    assert( ta );
    assert( !delete_new_active_record || delete_inactive_record );
    assert( _is_active || delete_inactive_record );


    lock_member_records( ta, member_id(), empty_member_id() );

    sql::Update_stmt update ( ta->database_descriptor(), _spooler->_members_tablename );

    update[ "active"    ] = sql::null_value;
    update[ "exclusive" ] = sql::null_value;
    update.and_where_condition( "scheduler_member_id", member_id() );
    update.and_where_condition( "active"             , _is_active        ? sql::Value( 1 ) : sql::null_value );
    update.and_where_condition( "exclusive"          , _has_exclusiveness? sql::Value( 1 ) : sql::null_value );
  //update.and_where_condition( "last_heart_beat"    , _db_last_heart_beat );
  //update.and_where_condition( "next_heart_beat"    , _db_next_heart_beat );

    bool ok = ta->try_execute_single( delete_inactive_record? update.make_delete_stmt() 
                                                            : update.make_update_stmt(), __FUNCTION__ );


    if( ok && _is_active )
    {
        sql::Update_stmt update ( ta->database_descriptor(), _spooler->_members_tablename );
    
        update[ "active"    ] = 1;
        update[ "exclusive" ] = 1;
        update.and_where_condition( "scheduler_member_id", empty_member_id() );
        update.and_where_condition( "active"             , sql::null_value );
        update.and_where_condition( "exclusive"          , sql::null_value );

        ok = ta->try_execute_single( delete_new_active_record? update.make_delete_stmt() 
                                                             : update.make_update_stmt(), __FUNCTION__ );
        if( !ok )
        {
            _log->error( message_string( "SCHEDULER-371", "Update without effect: " + update.make_stmt() ) );
            show_active_schedulers( ta );
        }
    }

    _is_active = false;
    _has_exclusiveness = false;
}

//---------------------------------------------------------Distributed_scheduler::async_state_text_

string Distributed_scheduler::async_state_text_() const
{
    S result;

    result << obj_name();

    if( _has_exclusiveness )  result << " (exclusive)";
    else  
    if( _exclusive_scheduler_watchdog )
    {
        result << _exclusive_scheduler_watchdog->async_state_text();
    }

    return result;
}

//----------------------------Distributed_scheduler::set_command_for_all_inactive_schedulers_but_me

void Distributed_scheduler::set_command_for_all_inactive_schedulers_but_me( Transaction* ta, Command command )
{
    set_command_for_all_schedulers_but_me( ta, "`exclusive` is null", command );
}

//-------------------------------------Distributed_scheduler::set_command_for_all_schedulers_but_me

void Distributed_scheduler::set_command_for_all_schedulers_but_me( Transaction* ta, Command command )
{
    set_command_for_all_schedulers_but_me( ta, "", command );
}

//-------------------------------------Distributed_scheduler::set_command_for_all_schedulers_but_me

void Distributed_scheduler::set_command_for_all_schedulers_but_me( Transaction* outer_transaction, const string& where, Command command )
{
    for( Retry_transaction ta ( db(), outer_transaction ); ta.enter_loop(); ta++ ) try
    {
        sql::Update_stmt update ( ta.database_descriptor(), _spooler->_members_tablename );
     
        update[ "command" ] = string_from_command( command );
        update.and_where_condition( "scheduler_id", _spooler->id_for_db() );
        update.add_where( " and `scheduler_member_id`<>" + sql::quoted( member_id() ) );
        if( where != "" )  update.add_where( " and " + where );

        ta.execute( update, __FUNCTION__ );
        ta.commit( __FUNCTION__ );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ) ); }
}

//----------------------------------------------------Distributed_scheduler::show_active_schedulers

void Distributed_scheduler::show_active_schedulers( Transaction* outer_transaction )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    if( db()  &&  db()->opened() )
    for( Retry_transaction ta ( db(), outer_transaction ); ta.enter_loop(); ta++ )
    try
    {
        bool found = false;

        Any_file select = ta.open_result_set( 
                            S() << "select `scheduler_member_id`, `last_heart_beat`, `http_url`, `host`, `pid`, `running_since`"
                                   "  from " << _spooler->_members_tablename <<
                                  "  where `scheduler_id`=" + sql::quoted( _spooler->id_for_db() ) <<
                                     " and `active` is not null",
                            __FUNCTION__ );

        while( !select.eof() )
        {
            Record record = select.get_record();
            found = true;

            string running_since    = record.as_string( "running_since"       );
            string other_member_id  = record.as_string( "scheduler_member_id" );
            if( other_member_id == empty_member_id() )
            {
                _log->info( message_string( "SCHEDULER-809", running_since ) );
            }
            else
            {
                time_t last_heart_beat  = record.as_int64 ( 1 );
                string other_http_url   = record.as_string( 2 );
                string other_hostname   = record.as_string( 3 );
                string other_pid        = record.as_string( 4 );

                _log->info( message_string( "SCHEDULER-995", other_member_id, ::time(NULL) - last_heart_beat, other_http_url, other_pid ) );
            }
        }

        if( !found )  _log->info( message_string( "SCHEDULER-805" ) );

        ta.commit( __FUNCTION__ );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ) ); }
}

//-------------------------------------Distributed_scheduler::scheduler_up_variable_name

//string Distributed_scheduler::scheduler_up_variable_name()
//{
//    return "scheduler/" + _spooler->id_for_db() + "/up";
//}

//-----------------------------------------------------------Distributed_scheduler::empty_member_id

string Distributed_scheduler::empty_member_id()
{ 
    return _spooler->id_for_db(); 
}

//-------------------------------------------------------------Distributed_scheduler::set_member_id
    
void Distributed_scheduler::set_member_id( const string& member_id )
{
    assert( !_heart_beat  &&  !_exclusive_scheduler_watchdog );

    _scheduler_member_id = member_id;
    check_member_id();

    _log->set_prefix( obj_name() );
}

//-------------------------------------------------------Distributed_scheduler::exclusive_member_id

string Distributed_scheduler::exclusive_member_id()
{
    if( _exclusive_scheduler_watchdog )
    {
        if( _exclusive_scheduler_watchdog->_other_scheduler._member_id == _spooler->id_for_db() )  return "";
        return _exclusive_scheduler_watchdog->_other_scheduler._member_id;
    }

    if( _has_exclusiveness )  return member_id();

    return "";
}

//-----------------------------------------------------------Distributed_scheduler::is_scheduler_up

bool Distributed_scheduler::is_scheduler_up()
{ 
    return _is_active  ||  _exclusive_scheduler_watchdog  &&  _exclusive_scheduler_watchdog->_is_scheduler_up; 
}

//-----------------------------------------------------------Distributed_scheduler::check_is_active

bool Distributed_scheduler::check_is_active()
{
    if( _is_active )  
    {
        bool is_in_time = check_heart_beat_is_in_time( _next_heart_beat );
        if( !is_in_time  &&  !db()->is_in_transaction() )      // Nicht in laufender Transaction
        {
            _log->warn( message_string( "SCHEDULER-997" ) );
            do_a_heart_beat( (Transaction*)NULL );
        }
    }

    return _is_active;
}

//-----------------------------------------------------------Distributed_scheduler::check_member_id

void Distributed_scheduler::check_member_id()
{
    string prefix = _spooler->id_for_db() + "/";
    if( !string_begins_with( member_id(), prefix )  ||  member_id() == prefix )  z::throw_xc( "SCHEDULER-358", member_id(), prefix );
}

//--------------------------------------------------Distributed_scheduler::make_scheduler_member_id

void Distributed_scheduler::make_scheduler_member_id()
{
    set_member_id( S() << _spooler->id_for_db() 
                       << "/" << _spooler->_hostname << "." << _spooler->tcp_port() 
                       << "." << getpid() << "." << ( (uint64)(double_from_gmtime()*1000000)%1000000 ) );
}

//------------------------------------------------------------------Distributed_scheduler::obj_name

string Distributed_scheduler::obj_name() const
{ 
    return "Distributed_scheduler";   // + _scheduler_member_id;
} 

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
