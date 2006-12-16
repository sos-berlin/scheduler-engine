// $Id$

#include "spooler.h"
#include "../kram/msec.h"

#ifdef Z_WINDOWS
#   include <process.h>
#endif

using namespace zschimmer;

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

//const time_t                    accepted_clock_difference       = Z_NDEBUG_DEBUG(  5,  2 );     // Die Uhren sollten noch besser übereinstimmen! ntp verwenden!
//const time_t                    warned_clock_difference         = Z_NDEBUG_DEBUG(  1,  1 ); 
const time_t                    heart_beat_period                       = Z_NDEBUG_DEBUG( 60, 20 );
const time_t                    max_heart_beat_processing_time          = Z_NDEBUG_DEBUG( 10,  3 );     // Zeit, die gebraucht wird, um den Herzschlag auszuführen
const time_t                    active_heart_beat_minimum_check_period  = heart_beat_period / 2;
const time_t                    trauerfrist                             = 12*3600;                      // Trauerzeit, nach der Mitgliedssätze gelöscht werden
const time_t                    database_commit_visible_time            = 10;                           // Zeit, die die Datenbank braucht, um nach Commit Daten für anderen Client sichtbar zu machen.
const int                       Scheduler_member::max_precedence        = 9999;

//---------------------------------------------------------------------------------------Heart_beat

struct Heart_beat : Async_operation, Scheduler_object
{
                                Heart_beat                  ( Scheduler_member* );


    // Async_operation
    bool                        async_finished_             () const;
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );
    void                        set_alarm                   ();


    // Scheduler_operation
    Prefix_log*                 log                         ()                                      { return _log; }


    Fill_zero                  _zero_;
    Scheduler_member*          _scheduler_member;
    Prefix_log*                _log;
    //time_t                     _db_last_heart_beat;
    //time_t                     _db_next_heart_beat;
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
                                Exclusive_scheduler_watchdog ( Scheduler_member* );


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


    Fill_zero                  _zero_;
    Scheduler_member*          _scheduler_member;
    Other_scheduler            _other_scheduler;
    time_t                     _next_check_time;
    bool                       _is_scheduler_up;            // Scheduler ist nicht ordentlich beendet worden
    bool                       _announced_to_become_exclusive; // Wenn eine Meldung ausgegeben oder ein Datensatz zu ändern versucht wurde 
    //time_t                     _set_exclusive_until;           // Nur für Access (kennt keine Sperre): Aktivierung bis dann (now+database_commit_visible_time) verzögern, dann nochmal prüfen
    Prefix_log*                _log;
};


//----------------------------------------------------------------------------my_string_from_time_t

static string my_string_from_time_t( time_t time )
{
    return string_gmt_from_time_t( time ) + " UTC";
}

//---------------------------------------------------------------------------Heart_beat::Heart_beat

Heart_beat::Heart_beat( Scheduler_member* m ) 
:
    _zero_(this+1),
    Scheduler_object( m->_spooler, this, Scheduler_object::type_heart_beat ),
    _scheduler_member(m),
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
        _scheduler_member->async_wake();    // Datenbank ist geschlossen worden
        return true;
    }


    try
    {
        bool ok = _scheduler_member->_has_exclusiveness? _scheduler_member->do_exclusive_heart_beat( (Transaction*)NULL )
                                                       : _scheduler_member->do_a_heart_beat( (Transaction*)NULL, false );
        if( ok )
        {
            // Wir sind weiterhin aktiv
            set_alarm();
        }
        else
        {
            // Wir sind nicht mehr aktiv, Operation beenden!
            _scheduler_member->async_wake();
        }
    }
    catch( exception& )
    {
        _scheduler_member->async_wake();
        throw;
    }
        
    return true;
}

//----------------------------------------------------------------------------Heart_beat::set_alarm

void Heart_beat::set_alarm()
{
    set_async_next_gmtime( _scheduler_member->_next_heart_beat );   //last_heart_beat() + heart_beat_period );
}

//-----------------------------------------Exclusive_scheduler_watchdog::Exclusive_scheduler_watchdog

Exclusive_scheduler_watchdog::Exclusive_scheduler_watchdog( Scheduler_member* m ) 
:
    _zero_(this+1),
    Scheduler_object( m->_spooler, this, Scheduler_object::type_exclusive_scheduler_watchdog ),
    _scheduler_member(m),
    _log(m->_log)
{
}

//-----------------------------------------------------Exclusive_scheduler_watchdog::async_finished_
    
bool Exclusive_scheduler_watchdog::async_finished_() const
{ 
    return false;
}

//---------------------------------------------------Exclusive_scheduler_watchdog::async_state_text_

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

//-----------------------------------------------------Exclusive_scheduler_watchdog::async_continue_

bool Exclusive_scheduler_watchdog::async_continue_( Continue_flags )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );


    if( !db()->opened() )
    {
        _scheduler_member->async_wake();    // Datenbank ist geschlossen worden
        return true;
    }

    try
    {
        try_to_become_exclusive();
    }
    catch( exception& )
    {
        _scheduler_member->async_wake();
        throw;
    }

    if( _scheduler_member->has_exclusiveness() )
    {
        _scheduler_member->async_wake();    // Wir sind aktives Mitglied geworden, Exclusive_scheduler_watchdog beenden
    }
    else
    {
        set_alarm();                        // Wir sind weiterhin inaktives Mitglied
    }

    return true;
}

//-----------------------------------------------------------Exclusive_scheduler_watchdog::set_alarm

void Exclusive_scheduler_watchdog::set_alarm()
{
    if( _scheduler_member->_db_next_heart_beat < _next_check_time )
    {
        Z_LOG2( "scheduler.distributed", __FUNCTION__ << "  Next heart beat at " << my_string_from_time_t( _scheduler_member->_db_next_heart_beat ) << "\n" );
        set_async_next_gmtime( _scheduler_member->_db_next_heart_beat );
    }
    else
    {
        Z_LOG2( "scheduler.distributed", __FUNCTION__ << "  Next check at " << my_string_from_time_t( _next_check_time ) << "\n" );
        set_async_next_gmtime( _next_check_time );
    }
}

//------------------------------------------------Exclusive_scheduler_watchdog::try_to_become_exclusive

void Exclusive_scheduler_watchdog::try_to_become_exclusive()
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( !_scheduler_member->_has_exclusiveness );


    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        _scheduler_member->do_a_heart_beat( &ta, false );     // Der Herzschlag ist ein Nebeneffekt von try_to_become_exclusive()

        bool none_has_exclusiveness = false;

        //if( !_set_exclusive_until )
        //{
        //    {
        //        Transaction ta ( db() );

                none_has_exclusiveness = check_exclusive_schedulers_heart_beat( &ta );   // Ruft auch do_a_heart_beat()

                ta.commit( __FUNCTION__ );
            //}

            if( none_has_exclusiveness  &&  _is_scheduler_up )
            {
                _announced_to_become_exclusive = true;
                bool ok = mark_as_exclusive();
                if( ok )  set_exclusive();
            }

        //}
        //else
        //{
        //    bool ok = _scheduler_member->do_a_heart_beat( NULL, true );
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
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", x ) ); }


    if( _announced_to_become_exclusive  &&  !_scheduler_member->_has_exclusiveness )  //&&  !_set_exclusive_until )
    {
        _announced_to_become_exclusive = false;
        _scheduler_member->show_active_schedulers( (Transaction*)NULL );
    }
}

//-------------------------------Exclusive_scheduler_watchdog::check_exclusive_schedulers_heart_beat

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
        //other_member_id = _scheduler_member->empty_member_id();

        if( _other_scheduler._member_id != _scheduler_member->empty_member_id() )
        {
            _log->info( message_string( "SCHEDULER-805" ) );
            _other_scheduler = Other_scheduler();
            _other_scheduler._member_id = _scheduler_member->empty_member_id();
        }

        none_has_exclusiveness = true;   // Kein Scheduler ist exklusiv, 
        _is_scheduler_up = false;   // aber der Scheduler ist auch nicht hochgefahren (Satz mit empty_member_id() fehlt)
    }
    else
    {
        _is_scheduler_up = true;

        bool   other_member_timed_out = false;
        Record record                 = select.get_record();

        string other_member_id = record.as_string( 0 );

        if( other_member_id != _other_scheduler._member_id )
        {
            if( other_member_id == _scheduler_member->empty_member_id() )  _log->info( message_string( "SCHEDULER-805" ) );
            else _log->info( message_string( "SCHEDULER-801", other_member_id ) );

            _other_scheduler = Other_scheduler();
            _other_scheduler._member_id                = other_member_id;
            //_other_scheduler._last_heart_beat_detected = ::time(NULL);

            //_log->info( message_string( "SCHEDULER-995", other_member_id, ::time(NULL) - last_heart_beat, other_http_url ) );
            _scheduler_member->show_active_schedulers( ta );
        }


        if( _other_scheduler._member_id == _scheduler_member->empty_member_id() )
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
                if( _other_scheduler._last_heart_beat_detected + heart_beat_period + max_heart_beat_processing_time/2 < now1 )
                {
                    _log->warn( message_string( "SCHEDULER-994", 
                                                    other_member_id, 
                                                    my_string_from_time_t( _other_scheduler._last_heart_beat_detected ), 
                                                    now1 - _other_scheduler._last_heart_beat_detected ) );
                    warned = true;

                    if( _other_scheduler._last_heart_beat_detected + heart_beat_period + max_heart_beat_processing_time < now1 )  
                        other_member_timed_out = true;
                }
            }
            else
            {
                _other_scheduler._last_heart_beat_detected = ::time(NULL);
            }

            if( !other_member_timed_out  &&  !warned  &&  !_other_scheduler._scheduler_993_logged 
             && next_heart_beat + max_heart_beat_processing_time < now1 )
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

//-------------------------------------------Exclusive_scheduler_watchdog::calculate_next_check_time

void Exclusive_scheduler_watchdog::calculate_next_check_time()
{
    time_t now   = ::time(NULL);
    time_t delay = max_heart_beat_processing_time + 1;   // Erst in der folgenden Sekunde prüfen

    _next_check_time = now + heart_beat_period + delay;

    //if( _set_exclusive_until  &&  _next_check_time > _set_exclusive_until )
    //{
    //    _next_check_time = _set_exclusive_until;
    //    extra_log << ", warten, bis Datenbank Änderungen für alle sichtbar gemacht hat";
    //}
    //else
    {
        time_t new_next_check = _other_scheduler._next_heart_beat_db + delay;
        if( new_next_check - now >= active_heart_beat_minimum_check_period  &&  new_next_check < _next_check_time )
        {
            time_t diff = new_next_check - _next_check_time;
            if( abs(diff) > 2 )  Z_LOG2( "scheduler.distributed", __FUNCTION__ << "  Synchronized with _next_heart_beat_db=" << my_string_from_time_t( _other_scheduler._next_heart_beat_db ) << ": " << diff << "s\n" );
            _next_check_time = new_next_check;
        }
    }
}

//------------------------------------------------------Exclusive_scheduler_watchdog::mark_as_exclusive

bool Exclusive_scheduler_watchdog::mark_as_exclusive()
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( !_scheduler_member->_has_exclusiveness );

    _scheduler_member->calculate_next_heart_beat();

    bool   ok;
    time_t now                    = ::time(NULL);
    time_t new_db_last_heart_beat = now;
    time_t new_db_next_heart_beat = _scheduler_member->_next_heart_beat;


    _log->info( message_string( "SCHEDULER-803" ) );  // "This Scheduler becomes exclusive"

    try
    {
        Transaction ta ( db() );

        _scheduler_member->lock_member_records( &ta, _other_scheduler._member_id, _scheduler_member->member_id() );


        // Bisher aktiven Scheduler inaktivieren

        sql::Update_stmt update ( &db()->_db_descr, _spooler->_members_tablename );
    
        update[ "active"    ] = sql::null_value;
        update[ "exclusive" ] = sql::null_value;
        update.and_where_condition( "scheduler_member_id", _other_scheduler._member_id );
        update.and_where_condition( "exclusive"          , 1 );
        
        if( _other_scheduler._member_id != _scheduler_member->empty_member_id() )
        {
            update.and_where_condition( "last_heart_beat"    , _other_scheduler._last_heart_beat_db );
            update.and_where_condition( "next_heart_beat"    , _other_scheduler._next_heart_beat_db );
        }

        ok = ta.try_execute_single( update, __FUNCTION__ );

        if( ok )
        {
            sql::Update_stmt update ( &db()->_db_descr, _spooler->_members_tablename );
        
            update[ "active"          ] = 1;
            update[ "exclusive"       ] = 1;
            update[ "last_heart_beat" ] = new_db_last_heart_beat;
            update[ "next_heart_beat" ] = new_db_next_heart_beat;
            update.and_where_condition( "scheduler_member_id", _scheduler_member->member_id() );
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
        _scheduler_member->_db_last_heart_beat = new_db_last_heart_beat;
        _scheduler_member->_db_next_heart_beat = new_db_next_heart_beat;

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

//----------------------------------------------------------Exclusive_scheduler_watchdog::set_exclusive

bool Exclusive_scheduler_watchdog::set_exclusive()
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( !_scheduler_member->_has_exclusiveness );

    bool ok;

    try
    {
        Transaction ta ( db() );

        ok = _scheduler_member->check_database_integrity( &ta );
        if( ok )
        {
            ok = _scheduler_member->do_exclusive_heart_beat( &ta );
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
        _other_scheduler._member_id = _scheduler_member->member_id();

        _scheduler_member->_is_active         = true;
        _scheduler_member->_has_exclusiveness = true;

        _log->info( message_string( "SCHEDULER-807" ) );
    }

    return ok;
}

//-----------------------------------------------------static Scheduler_member::string_from_command

string Scheduler_member::string_from_command( Command command )
{
    switch( command )
    {
        case cmd_none                 : return "";
        case cmd_terminate            : return "terminate";
        case cmd_terminate_and_restart: return "terminate_and_restart";
        default:                        return as_string( (int)command );
    }
}

//-----------------------------------------------------static Scheduler_member::command_from_string

Scheduler_member::Command Scheduler_member::command_from_string( const string& command )
{
    if( command == ""                      )  return cmd_none;
    if( command == "terminate"             )  return cmd_terminate;
    if( command == "terminate_and_restart" )  return cmd_terminate_and_restart;
    return cmd_none;
}

//---------------------------------------------------------------Scheduler_member::Scheduler_member

Scheduler_member::Scheduler_member( Spooler* spooler )
:
    Scheduler_object( spooler, this, type_scheduler_member ),
    _zero_(this+1)
{
    _log = Z_NEW( Prefix_log( spooler, obj_name() ) );
}

//--------------------------------------------------------------Scheduler_member::~Scheduler_member
    
Scheduler_member::~Scheduler_member()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler.distributed", "ERROR in " << __FUNCTION__ << ": " << x.what() << "\n" ); }
}

//--------------------------------------------------------------------------Scheduler_member::close

void Scheduler_member::close()
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

//-----------------------------------------------------------------------Scheduler_member::shutdown

void Scheduler_member::shutdown()
{
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

    if( db()  &&  db()->opened()  )
    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        mark_as_inactive( &ta, true, true );  // Member-Satz und leeren Member-Satz löschen

        ta.commit( __FUNCTION__ );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", x ) ); }

    close();
}

//------------------------------------------------------Scheduler_member::calculate_next_heart_beat

void Scheduler_member::calculate_next_heart_beat()
{
    _next_heart_beat = ::time(NULL) + heart_beat_period;
}

//------------------------------------------------------------Scheduler_member::lock_member_records

void Scheduler_member::lock_member_records( Transaction* ta, const string& member1_id, const string& member2_id )
{
    assert( ta );

    S sql;
    sql << "select `scheduler_member_id`  from " << _spooler->_members_tablename;
    if( db()->lock_syntax() == db_lock_with_updlock  )  sql << "  WITH(UPDLOCK)";
    sql << "  where `scheduler_member_id`"
        << " in (" << sql::quoted( member1_id ) << "," 
                   << sql::quoted( member2_id ) << ")";
    if( db()->lock_syntax() == db_lock_for_update )  sql << " FOR UPDATE";

    ta->set_transaction_written();
    ta->open_result_set( sql, __FUNCTION__ );
}

//------------------------------------------------------Scheduler_member::delete_old_member_records

void Scheduler_member::delete_old_member_records( Transaction* ta )
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

//--------------------------------------------------------------------------Scheduler_member::start

bool Scheduler_member::start()
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

//-----------------------------------------------------Scheduler_member::wait_until_is_scheduler_up

bool Scheduler_member::wait_until_is_scheduler_up()
{
    _log->info( message_string( "SCHEDULER-800" ) );

    while( !_spooler->is_termination_state_cmd()  &&  !is_scheduler_up() )
    {
        _spooler->simple_wait_step();
    }

    bool ok = is_scheduler_up();

    if( ok )  assert_database_integrity( __FUNCTION__ );

    return ok;
}

//-----------------------------------------------------------Scheduler_member::wait_until_is_active

bool Scheduler_member::wait_until_is_active()
{
    if( !is_scheduler_up() ) _log->info( message_string( "SCHEDULER-800" ) );
        //else  _log->info( message_string( __FUNCTION__ ) );

    bool was_scheduler_up = is_scheduler_up();

    while( !_spooler->is_termination_state_cmd() && !is_active() )  
    {
        if( was_scheduler_up  &&  !is_scheduler_up() ) 
        {
            _log->info( message_string( "SCHEDULER-802" ) );
            was_scheduler_up = false; 
        }

        _spooler->simple_wait_step();
    }

    bool ok = is_active();

    if( ok )  assert_database_integrity( __FUNCTION__ );

    return ok;
}

//--------------------------------------------------------Scheduler_member::wait_until_has_exclusiveness

bool Scheduler_member::wait_until_has_exclusiveness()
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

//-------------------------------------------------------Scheduler_member::create_table_when_needed

void Scheduler_member::create_table_when_needed()
{
    Transaction ta ( db() );

    db()->create_table_when_needed( &ta, _spooler->_members_tablename,
            "`scheduler_member_id`"    " varchar(100) not null, "
            "`scheduler_id`"           " varchar(100) not null, "
            "`version`"                " varchar(100) not null, "
            "`running_since`"          " datetime, "
            "`precedence`"             " numeric(4) not null, "
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

//---------------------------------------------------------------Scheduler_member::start_operations

void Scheduler_member::start_operations()
{
    if( _demand_exclusiveness && !has_exclusiveness() )
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

//---------------------------------------------------------------Scheduler_member::close_operations

void Scheduler_member::close_operations()
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

//----------------------------------------------------------------Scheduler_member::async_continue_

bool Scheduler_member::async_continue_( Continue_flags )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    if( _heart_beat                   )  _heart_beat                  ->async_check_exception( "Error in Scheduler member operation" );
    if( _exclusive_scheduler_watchdog )  _exclusive_scheduler_watchdog->async_check_exception( "Error in Scheduler member operation" );

    if( _has_exclusiveness  &&  _exclusive_scheduler_watchdog )
    {
        _exclusive_scheduler_watchdog->set_async_manager( NULL );
        _exclusive_scheduler_watchdog = NULL;
    }

    if( !_has_exclusiveness  &&  !_heart_beat )
    {
        _heart_beat->set_async_manager( NULL );
        start_operations();
    }

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

//-----------------------------------------------------------Scheduler_member::do_exclusive_heart_beat

bool Scheduler_member::do_exclusive_heart_beat( Transaction* outer_transaction )
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
    
//----------------------------------------------------------------Scheduler_member::do_a_heart_beat

bool Scheduler_member::do_a_heart_beat( Transaction* outer_transaction )
{
    return do_a_heart_beat( outer_transaction, has_exclusiveness() );
}

//-----------------------------------------------------------------Scheduler_member::do_a_heart_beat

bool Scheduler_member::do_a_heart_beat( Transaction* outer_transaction, bool db_record_marked_exclusive )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    time_t old_next_heart_beat = _db_next_heart_beat;
    bool   ok                  = false;
    bool   was_late            = false;


    calculate_next_heart_beat();

    if( !db()->opened() )  return false;


  AGAIN:
    for( Retry_transaction ta ( db(), outer_transaction ); ta.enter_loop(); ta++ ) try
    {
        ok = true;
        if( _spooler->_db_check_integrity )  ok = check_database_integrity( &ta );

        if( ok )  ok = try_to_heartbeat_member_record( &ta, db_record_marked_exclusive );

        ta.commit( __FUNCTION__ );


        time_t now = ::time(NULL);
        if( !was_late  &&  now - old_next_heart_beat > max_heart_beat_processing_time )    // Herzschlag noch in der Frist?
        {
            was_late = true;
            _log->warn( message_string( "SCHEDULER-996", my_string_from_time_t( old_next_heart_beat ), now - old_next_heart_beat ) );
            if( !outer_transaction )   goto AGAIN;  // Noch einmal in neuer Transaktion, um zu prüfen, ob uns jemand die Aktivität genommen hat.
        }
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", x ) ); }

    return ok;
}
    
//-------------------------------------------------Scheduler_member::try_to_heartbeat_member_record

bool Scheduler_member::try_to_heartbeat_member_record( Transaction* ta, bool db_record_marked_exclusive )
{
    calculate_next_heart_beat();

    time_t new_db_last_heart_beat = ::time(NULL);
    time_t new_db_next_heart_beat = _next_heart_beat;

    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "  new_db_last_heart_beat=" << new_db_last_heart_beat << " (" << my_string_from_time_t( new_db_last_heart_beat ) << "), "
                                                        "new_db_next_heart_beat=" << new_db_next_heart_beat << " (" << my_string_from_time_t( new_db_next_heart_beat ) << ")\n" );

    assert( ta );


    sql::Update_stmt update ( &db()->_db_descr, _spooler->_members_tablename );
    
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

//----------------------------------------------------------------Scheduler_member::execute_command

void Scheduler_member::execute_command( Command command )
{
    _log->info( message_string( "SCHEDULER-811", _heart_beat_command_string ) );

    switch( command )
    {
        case cmd_terminate            : _spooler->cmd_terminate( false, INT_MAX, false );  break;
        case cmd_terminate_and_restart: _spooler->cmd_terminate( true , INT_MAX, false );  break;
        default: ;
    }
}

//------------------------------------------------------Scheduler_member::assert_database_integrity

void Scheduler_member::assert_database_integrity( const string& message_text )
{
    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        if( !check_database_integrity( &ta ) )  z::throw_xc( "SCHEDULER-364", message_text );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", x ) ); }
}

//-------------------------------------------------------Scheduler_member::check_database_integrity

bool Scheduler_member::check_database_integrity( Transaction* ta )
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

//-----------------------------------------------------Scheduler_member::check_empty_member_record

bool Scheduler_member::check_empty_member_record()
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
            sql::Insert_stmt stmt ( &db()->_db_descr, _spooler->_members_tablename );

            stmt[ "scheduler_member_id" ] = empty_member_id();
            stmt[ "precedence"          ] = 0;
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

//-----------------------------------------------------------Scheduler_member::insert_member_record

void Scheduler_member::insert_member_record( Transaction* ta )
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

    sql::Insert_stmt insert ( &db()->_db_descr, _spooler->_members_tablename );
    
    insert[ "scheduler_member_id" ] = member_id();
    insert[ "precedence"          ] = max_precedence;
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

//---------------------------------------------------------------Scheduler_member::mark_as_inactive

void Scheduler_member::mark_as_inactive( Transaction* ta, bool delete_inactive_record, bool delete_new_active_record )
{
    assert( ta );
    assert( !delete_new_active_record || delete_inactive_record );
    assert( _is_active || delete_inactive_record );


    lock_member_records( ta, member_id(), empty_member_id() );

    sql::Update_stmt update ( &db()->_db_descr, _spooler->_members_tablename );

    update[ "active"    ] = sql::null_value;
    update[ "exclusive" ] = sql::null_value;
    update.and_where_condition( "scheduler_member_id", member_id() );
    update.and_where_condition( "active"             , _is_active   ? sql::Value( 1 ) : sql::null_value );
    update.and_where_condition( "exclusive"          , _has_exclusiveness? sql::Value( 1 ) : sql::null_value );
  //update.and_where_condition( "last_heart_beat"    , _db_last_heart_beat );
  //update.and_where_condition( "next_heart_beat"    , _db_next_heart_beat );

    bool ok = ta->try_execute_single( delete_inactive_record? update.make_delete_stmt() 
                                                            : update.make_update_stmt(), __FUNCTION__ );


    if( ok && _is_active )
    {
        sql::Update_stmt update ( &db()->_db_descr, _spooler->_members_tablename );
    
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

//--------------------------------------------------------------Scheduler_member::async_state_text_

string Scheduler_member::async_state_text_() const
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

//---------------------------------Scheduler_member::set_command_for_all_inactive_schedulers_but_me

void Scheduler_member::set_command_for_all_inactive_schedulers_but_me( Transaction* ta, Command command )
{
    set_command_for_all_schedulers_but_me( ta, "`exclusive` is null", command );
}

//------------------------------------------Scheduler_member::set_command_for_all_schedulers_but_me

void Scheduler_member::set_command_for_all_schedulers_but_me( Transaction* ta, Command command )
{
    set_command_for_all_schedulers_but_me( ta, "", command );
}

//------------------------------------------Scheduler_member::set_command_for_all_schedulers_but_me

void Scheduler_member::set_command_for_all_schedulers_but_me( Transaction* outer_transaction, const string& where, Command command )
{
    for( Retry_transaction ta ( db(), outer_transaction ); ta.enter_loop(); ta++ ) try
    {
        sql::Update_stmt update ( &db()->_db_descr, _spooler->_members_tablename );
     
        update[ "command" ] = string_from_command( command );
        update.and_where_condition( "scheduler_id", _spooler->id_for_db() );
        update.add_where( " and `scheduler_member_id`<>" + sql::quoted( member_id() ) );
        if( where != "" )  update.add_where( " and " + where );

        ta.execute( update, __FUNCTION__ );
        ta.commit( __FUNCTION__ );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", x ) ); }
}

//---------------------------------------------------------Scheduler_member::show_active_schedulers

void Scheduler_member::show_active_schedulers( Transaction* outer_transaction )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    if( db()  &&  db()->opened() )
    for( Retry_transaction ta ( db(), outer_transaction ); ta.enter_loop(); ta++ )
    try
    {
        bool found = false;

        Any_file select = ta.open_result_set( 
                            S() << "select `scheduler_member_id`, `last_heart_beat`, `http_url`, `host`, `pid`"
                                   "  from " << _spooler->_members_tablename <<
                                  "  where `scheduler_id`=" + sql::quoted( _spooler->id_for_db() ) <<
                                     " and `active` is not null",
                            __FUNCTION__ );

        while( !select.eof() )
        {
            Record record = select.get_record();
            found = true;

            string other_member_id  = record.as_string( 0 );
            if( other_member_id == empty_member_id() )
            {
                _log->info( message_string( "SCHEDULER-809" ) );
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
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", x ) ); }
}

//------------------------------------------Scheduler_member::scheduler_up_variable_name

//string Scheduler_member::scheduler_up_variable_name()
//{
//    return "scheduler/" + _spooler->id_for_db() + "/up";
//}

//----------------------------------------------------------------Scheduler_member::empty_member_id

string Scheduler_member::empty_member_id()
{ 
    return _spooler->id_for_db(); 
}

//------------------------------------------------------------------Scheduler_member::set_member_id
    
void Scheduler_member::set_member_id( const string& member_id )
{
    assert( !_heart_beat  &&  !_exclusive_scheduler_watchdog );

    _scheduler_member_id = member_id;
    check_member_id();

    _log->set_prefix( obj_name() );
}

//---------------------------------------------------------------Scheduler_member::exclusive_member_id

string Scheduler_member::exclusive_member_id()
{
    if( _exclusive_scheduler_watchdog )
    {
        if( _exclusive_scheduler_watchdog->_other_scheduler._member_id == _spooler->id_for_db() )  return "";
        return _exclusive_scheduler_watchdog->_other_scheduler._member_id;
    }

    if( _has_exclusiveness )  return member_id();

    return "";
}

//----------------------------------------------------------------Scheduler_member::is_scheduler_up

bool Scheduler_member::is_scheduler_up()
{ 
    return _is_active  ||  _exclusive_scheduler_watchdog  &&  _exclusive_scheduler_watchdog->_is_scheduler_up; 
}

//----------------------------------------------------------------Scheduler_member::check_member_id

void Scheduler_member::check_member_id()
{
    string prefix = _spooler->id_for_db() + "/";
    if( !string_begins_with( member_id(), prefix )  ||  member_id() == prefix )  z::throw_xc( "SCHEDULER-358", member_id(), prefix );
}

//-------------------------------------------------------Scheduler_member::make_scheduler_member_id

void Scheduler_member::make_scheduler_member_id()
{
    set_member_id( S() << _spooler->id_for_db() 
                       << "/" << _spooler->_hostname << "." << _spooler->tcp_port() 
                       << "." << getpid() << "." << ( (uint64)(double_from_gmtime()*1000000)%1000000 ) );
}

//-----------------------------------------------------------------------Scheduler_member::obj_name

string Scheduler_member::obj_name() const
{ 
    return "Distributed_scheduler";   // + _scheduler_member_id;
} 

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
