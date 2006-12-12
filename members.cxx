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
const time_t                    active_heart_beat_period                = Z_NDEBUG_DEBUG( 60, 20 );
const time_t                    max_heart_beat_processing_time          = Z_NDEBUG_DEBUG( 10,  3 );     // Zeit, die gebraucht wird, um den Herzschlag auszuführen
const time_t                    active_heart_beat_minimum_check_period  = active_heart_beat_period / 2;
const time_t                    trauerfrist                             = 2*3600;                       // Trauerzeit, nach der Mitgliedssätze gelöscht werden
const time_t                    database_commit_visible_time            = 10;                           // Zeit, die die Datenbank braucht, um nach Commit Daten für anderen Client sichtbar zu machen.

//----------------------------------------------------------------------Active_scheduler_heart_beat

struct Active_scheduler_heart_beat : Async_operation, Scheduler_object
{
                                Active_scheduler_heart_beat ( Scheduler_member* );


    // Async_operation
    bool                        async_finished_             () const;
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );
    void                        set_alarm                   ();


    // Scheduler_operation
    Prefix_log*                 log                         ()                                      { return _log; }


    Spooler_db*                 db                          ()                                      { return _scheduler_member->db(); }


    Fill_zero                  _zero_;
    Scheduler_member*          _scheduler_member;
    Prefix_log*                _log;
    time_t                     _last_heart_beat;
    time_t                     _next_heart_beat;
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
    //time_t                     _clock_difference;
    //bool                       _clock_difference_checked;
    //bool                       _checking_clock_difference;
};

//----------------------------------------------------------------------Inactive_scheduler_watchdog

struct Inactive_scheduler_watchdog : Async_operation, Scheduler_object
{
                                Inactive_scheduler_watchdog ( Scheduler_member* );


    // Async_operation
    bool                        async_finished_             () const;
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );
    void                        set_alarm                   ();


    // Scheduler_operation
    Prefix_log*                 log                         ()                                      { return _log; }


    void                        try_to_become_active        ();
    bool                        check_for_inactivity        ( Transaction* );
  //void                        check_clock_difference      ( time_t last_heart_beat, time_t now );
    bool                        mark_as_active              ();
    bool                        set_active                  ();
    Spooler_db*                 db                          ()                                      { return _scheduler_member->db(); }


    Fill_zero                  _zero_;
    Scheduler_member*          _scheduler_member;
    Other_scheduler            _other_scheduler;
    time_t                     _next_check_time;
    bool                       _is_scheduler_up;            // Scheduler ist nicht ordentlich beendet worden
    bool                       _announced_to_become_active; // Wenn eine Meldung ausgegeben oder ein Datensatz zu ändern versucht wurde 
    //time_t                     _set_active_until;           // Nur für Access (kennt keine Sperre): Aktivierung bis dann (now+database_commit_visible_time) verzögern, dann nochmal prüfen
    Prefix_log*                _log;
};

//-----------------------------------------Active_scheduler_heart_beat::Active_scheduler_heart_beat

Active_scheduler_heart_beat::Active_scheduler_heart_beat( Scheduler_member* m ) 
:
    _zero_(this+1),
    Scheduler_object( m->_spooler, this, Scheduler_object::type_active_scheduler_heart_beat ),
    _scheduler_member(m),
    _log(m->_log)
{
}

//-------------------------------------------------------------------------------------------------

bool Active_scheduler_heart_beat::async_finished_() const
{ 
    return false;
}

//---------------------------------------------------Active_scheduler_heart_beat::async_state_text_

string Active_scheduler_heart_beat::async_state_text_() const
{
    return "Active_scheduler_heart_beat";
}

//-----------------------------------------------------Active_scheduler_heart_beat::async_continue_

bool Active_scheduler_heart_beat::async_continue_( Continue_flags )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    try
    {
        if( !db()->opened() )
        {
            _scheduler_member->async_wake();    // Datenbank ist geschlossen worden
            return true;
        }

        
        bool ok = _scheduler_member->do_heart_beat( NULL, true );
        if( ok )
        {
            // Wir sind weiterhin aktives Mitglied
            set_alarm();
        }
        else
        {
            // Wir sind nicht mehr aktives Mitglied, Operation beenden!
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

//-----------------------------------------------------------Active_scheduler_heart_beat::set_alarm

void Active_scheduler_heart_beat::set_alarm()
{
    set_async_next_gmtime( _scheduler_member->last_heart_beat() + active_heart_beat_period );
}

//-----------------------------------------Inactive_scheduler_watchdog::Inactive_scheduler_watchdog

Inactive_scheduler_watchdog::Inactive_scheduler_watchdog( Scheduler_member* m ) 
:
    _zero_(this+1),
    Scheduler_object( m->_spooler, this, Scheduler_object::type_inactive_scheduler_watchdog ),
    _scheduler_member(m),
    _log(m->_log)
{
}

//-----------------------------------------------------Inactive_scheduler_watchdog::async_finished_
    
bool Inactive_scheduler_watchdog::async_finished_() const
{ 
    return false;
}

//---------------------------------------------------Inactive_scheduler_watchdog::async_state_text_

string Inactive_scheduler_watchdog::async_state_text_() const
{
    S result;

    result << "Inactive_scheduler_watchdog";

    //if( _set_active_until )
    //{
    //    result << " (becoming active at " << string_gmt_from_time_t( _set_active_until) << ")";
    //}
    //else
    //{
    //    result << " (inactive)";
    //}

    return result;
}

//-----------------------------------------------------Inactive_scheduler_watchdog::async_continue_

bool Inactive_scheduler_watchdog::async_continue_( Continue_flags )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    try
    {
        if( !db()->opened() )
        {
            _scheduler_member->async_wake();    // Datenbank ist geschlossen worden
            return true;
        }

        try_to_become_active();

        if( _scheduler_member->is_active() )
        {
            // Wir sind aktives Mitglied geworden, Operation beenden und zu Active_scheduler_heart_beat wechseln!
            _scheduler_member->async_wake();
        }
        else
        {
            // Wir sind weiterhin inaktives Mitglied
            set_alarm();
        }
    }
    catch( exception& )
    {
        _scheduler_member->async_wake();
        throw;
    }

    return true;
}

//-----------------------------------------------------------Inactive_scheduler_watchdog::set_alarm

void Inactive_scheduler_watchdog::set_alarm()
{
    time_t now = ::time(NULL);
    S      extra_log;

    //if( !_other_scheduler._clock_difference_checked  &&  _other_scheduler._next_heart_beat_db )
    //{
    //    extra_log << ", checking clock difference ...";
    //    _other_scheduler._checking_clock_difference = true;
    //    _next_check_time = now + warned_clock_difference;
    //}
    //else
    {
        time_t delay = max_heart_beat_processing_time + 1;   // Erst in der folgenden Sekunde prüfen

        _next_check_time = now + active_heart_beat_period + delay;

        //if( _set_active_until  &&  _next_check_time > _set_active_until )
        //{
        //    _next_check_time = _set_active_until;
        //    extra_log << ", warten, bis Datenbank Änderungen für alle sichtbar gemacht hat";
        //}
        //else
        {
            time_t wait_until = _other_scheduler._next_heart_beat_db + delay;
            if( wait_until - now >= active_heart_beat_minimum_check_period  &&  wait_until < _next_check_time )
            {
                time_t new_next_check = wait_until;
                extra_log << ", watchdog synchronized with heart beat: " << ( new_next_check - _next_check_time  ) << "s";
                _next_check_time = new_next_check;
            }
        }
    }

    Z_LOG2( "scheduler.distributed", __FUNCTION__ << "  " << ( _next_check_time - now ) << "s" << extra_log << "\n" );

    set_async_next_gmtime( _next_check_time );
}

//------------------------------------------------Inactive_scheduler_watchdog::try_to_become_active

void Inactive_scheduler_watchdog::try_to_become_active()
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( !_scheduler_member->_is_active );
    if( !db()->opened() )  z::throw_xc( "SCHEDULER-357" );

    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        bool none_is_active = false;

        //if( !_set_active_until )
        //{
        //    {
        //        Transaction ta ( db() );

                none_is_active = check_for_inactivity( &ta );

                ta.commit();
            //}

            if( none_is_active  &&  _is_scheduler_up )
            {
                _announced_to_become_active = true;
                bool ok = mark_as_active();
                if( ok )  set_active();
            }

        //}
        //else
        //{
        //    bool ok = _scheduler_member->do_heart_beat( NULL, true );
        //    if( !ok ) 
        //    {
        //        _set_active_until = 0;
        //    }
        //}

        //if( _set_active_until  &&  _set_active_until <= ::time(NULL) )
        //{
        //    _set_active_until = 0;
        //    _announced_to_become_active = true; 
        //
        //    set_active();
        //}

        if( _announced_to_become_active  &&  !_scheduler_member->_is_active )  //&&  !_set_active_until )
        {
            _announced_to_become_active = false;
            _scheduler_member->show_active_scheduler();
        }
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", x ) ); }
}

//-----------------------------------------------Inactive_scheduler_watchdog::check_for_inactivity

bool Inactive_scheduler_watchdog::check_for_inactivity( Transaction* ta )
{
    // Rollback, wenn Funktion false liefert!

    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( ta );


    bool   none_is_active = false;
    string other_member_id;


    // Aktiven Scheduler lesen

    Any_file select = ta->open_result_set( S() << 
                 "select `scheduler_member_id`, `last_heart_beat`, `next_heart_beat` "
                 " from " << _spooler->_members_tablename << 
                     " where `scheduler_id`=" << sql::quoted( _spooler->id_for_db() ) <<
                      " and `is_active`<>0" );

    if( select.eof() )  // Keiner ist aktiv?
    {
        if( _other_scheduler._member_id != "" )
        {
            _log->info( message_string( "SCHEDULER-805" ) );
            _other_scheduler = Other_scheduler();
        }

        none_is_active = true;
        _is_scheduler_up = ta->get_variable_text( _scheduler_member->scheduler_up_variable_name() ) != "";
    }
    else
    {
        time_t now = ::time(NULL);

        Record record = select.get_record();
        if( !select.eof() )
        {
            _log->error( "MORE THAN ONE ACTIVE SCHEDULER" );
            _scheduler_member->show_active_scheduler( ta );
            return false;
        }


               other_member_id        = record.as_string( 0 );
        bool   other_member_timed_out = false;
        time_t last_heart_beat        = record.as_int64( 1 );                         //(time_t)( record.as_double( 1 ) + 0.5 );
        time_t next_heart_beat        = record.null( 2 )? 0 : record.as_int64( 2 );   //(time_t)( record.as_double( 2 ) + 0.5 );

        
        if( other_member_id != _other_scheduler._member_id )
        {
            _other_scheduler = Other_scheduler();
            _other_scheduler._member_id                = other_member_id;
            _other_scheduler._last_heart_beat_detected = now;

            //_log->info( message_string( "SCHEDULER-995", other_member_id, ::time(NULL) - last_heart_beat, other_http_url ) );
            _scheduler_member->show_active_scheduler( ta );
        }
        else
        {
            //if( last_heart_beat > now )
            //{
            //    time_t diff = last_heart_beat - now;
            //    
            //    if( diff >= warned_clock_difference + 1 )
            //        _log->warn( message_string( "SCHEDULER-364", diff, other_member_id ) );
            //}
            bool warned = false;

            if( last_heart_beat == _other_scheduler._last_heart_beat_db )   // Kein neuer Herzschlag?
            {
                if( _other_scheduler._last_heart_beat_detected + active_heart_beat_period + max_heart_beat_processing_time/2 < now )
                {
                    _log->warn( message_string( "SCHEDULER-994", 
                                                    other_member_id, 
                                                    string_gmt_from_time_t( _other_scheduler._last_heart_beat_detected ) + " UTC", 
                                                    now - _other_scheduler._last_heart_beat_detected ) );
                    warned = true;

                    if( _other_scheduler._last_heart_beat_detected + active_heart_beat_period + max_heart_beat_processing_time < now )  
                        other_member_timed_out = true;
                }
            }
            else
            {
                _other_scheduler._last_heart_beat_detected = now;
            }

            if( !other_member_timed_out  &&  !warned  &&  next_heart_beat + max_heart_beat_processing_time < now )
            {
                _log->warn( message_string( "SCHEDULER-993", 
                                                other_member_id, 
                                                string_gmt_from_time_t( next_heart_beat ) + " UTC",
                                                string_gmt_from_time_t( _other_scheduler._last_heart_beat_detected ) + " UTC",
                                                now - _other_scheduler._last_heart_beat_detected ) );

                //if( next_heart_beat + heart_beat_delay < now )  other_member_timed_out = true;
            }

            //if( !other_member_timed_out  &&  last_heart_beat + active_heart_beat_period + heart_beat_delay/2 < now )
            //{
            //    _log->warn( message_string( "SCHEDULER-994", 
            //                                    other_member_id, 
            //                                    string_gmt_from_time_t( next_heart_beat ) + " UTC", 
            //                                    now - ( last_heart_beat + active_heart_beat_period ) ) );

            //    //if( last_heart_beat + active_heart_beat_period + heart_beat_delay < now )  other_member_timed_out = true;
            //}
            
            //if( !other_member_timed_out  &&  _other_scheduler._checking_clock_difference  &&  last_heart_beat == _other_scheduler._next_heart_beat_db )
            //{
            //    check_clock_difference( last_heart_beat, now );
            //}

            if( other_member_timed_out )  none_is_active = true;
                                    else  _is_scheduler_up = true;  // Bei Herzschlag eines aktiven Schedulers kann er nicht heruntergefahren sein
        }

        _other_scheduler._last_heart_beat_db = last_heart_beat;
        _other_scheduler._next_heart_beat_db = next_heart_beat;
    }

    return none_is_active;
}

//------------------------------------------------------Inactive_scheduler_watchdog::mark_as_active

bool Inactive_scheduler_watchdog::mark_as_active()
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( !_scheduler_member->_is_active );

    ///// Vorher irgend ein SQL ausführen, weil hier nicht die Datenbank nach Abbruch neu geöffnet wird /////

    bool   ok;
    time_t now                 = ::time(NULL);
    time_t new_last_heart_beat = now;
    time_t new_next_heart_beat = new_last_heart_beat + active_heart_beat_period;


    _log->info( message_string( "SCHEDULER-803" ) );

    enum 
    { 
        use_for_update,         // Sätze mit "select for update" sperren
        use_with_updlock,       // SQL-Server: Sätze mit "select with(updlock)" sperren, für SQL Server
        use_commit_visible_time // Access: Nach use_commit_visible_time Erfolg prüfen
    } 
    db_mode = db()->dbms_kind() == dbms_sql_server? use_with_updlock 
              //db()->dbms_kind() == dbms_access    ? use_commit_visible_time
                                                  : use_for_update;

    try
    {
        Transaction ta ( db() );

        Any_file select_for_update;

        //if( db_mode != use_commit_visible_time )
        if( _other_scheduler._member_id != "" )
        {
            //db()->execute( "SET TRANSACTION ISOLATION LEVEL SERIALIZABLE" );
            S sql;
            sql << "select `is_active`  from " << _spooler->_members_tablename;
            if( db_mode == use_with_updlock  )  sql << "  with(updlock)";
            sql << " where `scheduler_member_id`";
            sql << " in (" << sql::quoted( _other_scheduler._member_id ) << "," 
                           << sql::quoted( _scheduler_member->member_id() ) << ")";
            if( db_mode == use_for_update )  sql << " for update";

            select_for_update = ta.open_result_set( sql );
        }

        if( _other_scheduler._member_id == "" )
        {
            // Wir sind der erste aktive Scheduler
            ok = true;
        }
        else
        {
            // Bisher aktiven Scheduler inaktivieren

            sql::Update_stmt update ( &db()->_db_descr, _spooler->_members_tablename );
        
            update[ "is_active" ] = 0;
            update.and_where_condition( "scheduler_member_id", _other_scheduler._member_id );
            update.and_where_condition( "is_active"          , 1 );
            ok = ta.try_execute_single( update );
        }

        if( ok )
        {
            sql::Update_stmt update ( &db()->_db_descr, _spooler->_members_tablename );
        
            update[ "is_active" ] = 1;
            update[ "last_heart_beat" ] = new_last_heart_beat;
            update[ "next_heart_beat" ] = new_next_heart_beat;
            update.and_where_condition( "scheduler_member_id", _scheduler_member->member_id() );
            ok = ta.try_execute_single( update );
        }

        ta.commit();
    }
    catch( exception& x )   // Bei optimistischer Sperrung kann es eine Exception geben
    { 
        ok = false;
        _log->debug3( S() << x.what() << ", in " << __FUNCTION__ );
    }

    if( ok )
    {
        _scheduler_member->_last_heart_beat = new_last_heart_beat;
        _scheduler_member->_next_heart_beat = new_next_heart_beat;

        //_set_active_until = ::time(NULL);
        //if( db_mode == use_commit_visible_time )  _set_active_until += database_commit_visible_time + 1;     // Nachfolgende Sekunde
    }

    return ok;
}

//----------------------------------------------Inactive_scheduler_watchdog::check_clock_difference

//void Inactive_scheduler_watchdog::check_clock_difference( time_t last_heart_beat, time_t now )
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

//----------------------------------------------------------Inactive_scheduler_watchdog::set_active

bool Inactive_scheduler_watchdog::set_active()
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( !_scheduler_member->_is_active );

    bool ok;

    try
    {
        Transaction ta ( db() );

        ok = _scheduler_member->check_database_consistency( &ta );
        if( ok )
        {
            ok = _scheduler_member->do_heart_beat( &ta, true );
        }

        ta.commit();
    }
    catch( exception& x ) 
    {
        ok = false;
        _log->debug3( S() << x.what() << ", in " << __FUNCTION__ );
    }

    if( ok )
    {
        _other_scheduler._member_id = _scheduler_member->member_id();
        _scheduler_member->_is_active = true;
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

        close_operation();
        set_async_manager( NULL );

        try
        {
            Transaction ta ( db() );

            delete_member_record( &ta );
            delete_old_member_records( &ta );

            ta.commit();
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

    if( db()  &&  db()->opened() && _is_active )
    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        ta.execute( S() << "DELETE from " << _spooler->_variables_tablename << 
                             "  where `name`=" << sql::quoted( scheduler_up_variable_name() ) );

        ta.commit();
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", x ) ); }

    close();
}

//-----------------------------------------------------------Scheduler_member::delete_member_record

void Scheduler_member::delete_member_record( Transaction* ta )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( ta );


    ta->execute( S() << "DELETE from " << _spooler->_members_tablename << 
                       "  where `scheduler_member_id`=" << sql::quoted( member_id() ) );

    // Wenn Where-Klause nicht zutrifft, sind wir zwischendurch inaktiv geworden
}

//------------------------------------------------------Scheduler_member::delete_old_member_records

void Scheduler_member::delete_old_member_records( Transaction* ta )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( ta );

    ta->execute( S() << "DELETE from " << _spooler->_members_tablename << 
                        "  where `scheduler_id`= " << sql::quoted( _spooler->id_for_db() ) <<
                           " and `next_heart_beat`<" << ( ::time(NULL) - trauerfrist ) );

    if( int record_count = ta->record_count() )
    {
        Z_LOG2( "scheduler.distributed", record_count << " alte Sätze aus " << _spooler->_members_tablename << " gelöscht\n" );
    }
}

//--------------------------------------------------------------------------Scheduler_member::start

bool Scheduler_member::start()
{
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

    assert( !_current_operation );
    assert( !_active_scheduler_heart_beat );
    assert( !_inactive_scheduler_watchdog );

    if( !db()->opened() )  z::throw_xc( "SCHEDULER-357" ); 

    create_table_when_needed();

    if( _scheduler_member_id == "" )  make_scheduler_member_id();
    check_member_id();


    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        bool is_scheduler_up;

        delete_old_member_records( &ta );

        insert_member_record( &ta );
        if( !_is_backup ) 
        {
            ta.set_variable( scheduler_up_variable_name(), "1" );
            is_scheduler_up = true;
        }
        else
            is_scheduler_up = ta.get_variable_text( scheduler_up_variable_name() ) != "";

        ta.commit();


        _inactive_scheduler_watchdog = Z_NEW( Inactive_scheduler_watchdog( this ) );
        _inactive_scheduler_watchdog->try_to_become_active();   // Stellt sofort _is_scheduler_up fest
        _inactive_scheduler_watchdog->_is_scheduler_up = is_scheduler_up;
        if( _is_active )  _inactive_scheduler_watchdog = NULL;
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", x ) ); }


    set_async_manager( _spooler->_connection_manager );
    start_operation();


    bool ok = true;

    if( _is_backup  &&  is_scheduler_up() ) 
    {
        ok = wait_until_started_up();
    }
    
    if( ok  &&  !is_active() )
    {
        ok = wait_until_active();
    }


    return ok;
}

//----------------------------------------------------------Scheduler_member::wait_until_started_up

bool Scheduler_member::wait_until_started_up()
{
    _log->info( message_string( "SCHEDULER-800" ) );

    while( _spooler->_state_cmd != Spooler::sc_terminate  &&  is_scheduler_up() ) 
    {
        _spooler->wait();
        _spooler->_connection_manager->async_continue();
        _spooler->run_check_ctrl_c();
    }

    return !is_scheduler_up();
}

//--------------------------------------------------------------Scheduler_member::wait_until_active

bool Scheduler_member::wait_until_active()
{
    if( active_member_id() == "" )  _log->info( message_string( "SCHEDULER-800" ) );
                              else  _log->info( message_string( "SCHEDULER-801", active_member_id() ) );

    bool was_scheduler_up = is_scheduler_up();

    while( _spooler->_state_cmd != Spooler::sc_terminate  &&  !is_active() )  
    {
        if( was_scheduler_up  &&  !is_scheduler_up() ) 
        {
            _log->info( message_string( "SCHEDULER-802" ) );
            was_scheduler_up = false; 
        }

        _spooler->wait();
        _spooler->_connection_manager->async_continue();
        _spooler->run_check_ctrl_c();
    }

    return is_active();
}

//-------------------------------------------------------Scheduler_member::create_table_when_needed

void Scheduler_member::create_table_when_needed()
{
    db()->create_table_when_needed( _spooler->_members_tablename,
            "`scheduler_member_id`"    " varchar(100) not null, "
            "`scheduler_id`"           " varchar(100) not null, "
            "`running_since`"          " datetime, "
            "`last_heart_beat`"        " integer, "     //numeric(14,3) not null, "
            "`next_heart_beat`"        " integer, "     //numeric(14,3), "
            "`is_active`"              " boolean not null, "
            "`command`"                " varchar(100), "
          //"`ip_address`"             " varchar(40), "
          //"`udp_port`"               " integer, "
          //"`tcp_port`"               " integer, "
            "`host`"                   " varchar(100) not null, "
            "`pid`"                    " varchar(20)  not null, "
            "`http_url`"               " varchar(100), "
            "primary key( `scheduler_member_id` )" );
}

//----------------------------------------------------------------Scheduler_member::start_operation

void Scheduler_member::start_operation()
{
    if( db()->opened() )
    {
        if( _is_active )
        {
            assert( !_inactive_scheduler_watchdog );
            assert( !_active_scheduler_heart_beat );
            assert( !_current_operation );

            _active_scheduler_heart_beat = Z_NEW( Active_scheduler_heart_beat( this ) );
            _active_scheduler_heart_beat->set_alarm();
            _current_operation = +_active_scheduler_heart_beat;
        }
        else
        {
            assert( !_active_scheduler_heart_beat );

            if( !_inactive_scheduler_watchdog )
                _inactive_scheduler_watchdog = Z_NEW( Inactive_scheduler_watchdog( this ) );

            _inactive_scheduler_watchdog->set_alarm();
            //show_active_scheduler();
            _current_operation = +_inactive_scheduler_watchdog;
        }

        _current_operation->set_async_manager( _spooler->_connection_manager );
    }
}

//----------------------------------------------------------------Scheduler_member::close_operation

void Scheduler_member::close_operation()
{
    if( _current_operation ) 
    {
        _current_operation->set_async_manager( NULL );
        _current_operation = NULL;
    }

    _active_scheduler_heart_beat = NULL;
    _inactive_scheduler_watchdog = NULL;
}

//------------------------------------------------------------------Scheduler_member::do_heart_beat

bool Scheduler_member::do_heart_beat( Transaction* outer_transaction, bool db_record_marked_active )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    bool   ok = false;
    time_t old_next_heart_beat = _next_heart_beat;
    bool   was_late = false;

    if( db()->opened() )
    for( Retry_transaction ta ( db(), outer_transaction ); ta.enter_loop(); ta++ ) try
    {
        ok = check_database_consistency( &ta );

        if( ok )  ok = try_to_heartbeat_member_record( &ta, db_record_marked_active );

        if( !ok )
        {
            _is_active = false;
            _activity_stolen = true;
            _log->error( message_string( "SCHEDULER-356" ) );   // "Some other Scheduler member has become active"
        }

        ta.commit();

        if( !was_late )
        {
            time_t now = ::time(NULL);
            if( now - old_next_heart_beat > max_heart_beat_processing_time )   // Herzschlag noch in der Frist?
            {
                was_late = true;
                _log->warn( message_string( "SCHEDULER-996", string_gmt_from_time_t( old_next_heart_beat ) + " UTC", now - old_next_heart_beat ) );
                continue;
            }
        }
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", x ) ); }

    return ok;
}
    
//-------------------------------------------------Scheduler_member::try_to_heartbeat_member_record

bool Scheduler_member::try_to_heartbeat_member_record( Transaction* ta, bool db_record_marked_active )
{
    time_t new_last_heart_beat = ::time(NULL);
    time_t new_next_heart_beat = new_last_heart_beat + active_heart_beat_period;

    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "  new_last_heart_beat=" << new_last_heart_beat << " (" << string_gmt_from_time_t( new_last_heart_beat ) << " UTC), "
                                                        "new_next_heart_beat=" << new_next_heart_beat << " (" << string_gmt_from_time_t( new_next_heart_beat ) << " UTC)\n" );

    assert( ta );


    sql::Update_stmt update ( &db()->_db_descr, _spooler->_members_tablename );
    
    update[ "last_heart_beat" ] = new_last_heart_beat;
    update[ "next_heart_beat" ] = new_next_heart_beat;

    update.and_where_condition( "scheduler_member_id", member_id()      );
    update.and_where_condition( "last_heart_beat"    , _last_heart_beat );
    update.and_where_condition( "next_heart_beat"    , _next_heart_beat );
    update.and_where_condition( "is_active"          , db_record_marked_active? 1 : 0 );
    update.and_where_condition( "command"            , _heart_beat_command? sql::Value( string_from_command( _heart_beat_command ) ) 
                                                                          : sql::null_value );

    bool record_is_updated = ta->try_execute_single( update );

    if( !record_is_updated )
    {
        update.remove_where_condition( "command" );
        record_is_updated = ta->try_execute_single( update );

        if( record_is_updated )     // Neues Kommando
        {
            Any_file result_set = ta->open_result_set( S() << "select `command`  from " << _spooler->_members_tablename << "  where `scheduler_member_id`=" << member_id() );
            if( !result_set.eof() )
            {
                _heart_beat_command_string = result_set.get_record().as_string( 0 );
                _heart_beat_command = command_from_string( _heart_beat_command_string) ;

                switch( _heart_beat_command )
                {
                    case cmd_terminate            : _spooler->cmd_terminate();              break;
                    case cmd_terminate_and_restart: _spooler->cmd_terminate_and_restart();  break;
                    default: ;
                }
            }
        }
    }

    if( record_is_updated )
    {
        _last_heart_beat = new_last_heart_beat;
        _next_heart_beat = new_next_heart_beat;
    }

    return record_is_updated;
}

//-----------------------------------------------------Scheduler_member::check_database_consistency

bool Scheduler_member::check_database_consistency( Transaction* ta )
{
    assert( ta );

    int active_count = ta->open_result_set( S() << 
                        "select count(*)"
                        "  from " << _spooler->_members_tablename <<
                        "  where `scheduler_id`=" << sql::quoted( _spooler->id_for_db() ) <<
                           " and `is_active`<>0" ).get_record().as_int( 0 );
    
    bool ok = active_count <= 1;
    if( !ok )  _log->error( S() << "MORE THAN ONE ACTIVE SCHEDULER: " << active_count );

    return ok;
}

//-----------------------------------------------------------Scheduler_member::insert_member_record

void Scheduler_member::insert_member_record( Transaction* ta )
{
    assert( ta );
    assert( !_is_active );

    time_t now = ::time(NULL);
    time_t new_last_heart_beat = now;
    time_t new_next_heart_beat = new_last_heart_beat + active_heart_beat_period;

    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "  new_last_heart_beat=" << new_last_heart_beat << " (" << string_gmt_from_time_t( new_last_heart_beat) << " UTC), "
                                                        "new_next_heart_beat=" << new_next_heart_beat << " (" << string_gmt_from_time_t( new_next_heart_beat) << " UTC)\n" );


    // Darf nicht vorhanden sein:  db()->execute( S() << "DELETE from " << _spooler->_members_tablename << "  where `scheduler_member_id`=" << sql::quoted( member_id() ) );

    sql::Insert_stmt insert ( &db()->_db_descr, _spooler->_members_tablename );
    
    insert[ "scheduler_member_id" ] = member_id();
    insert[ "last_heart_beat"     ] = new_last_heart_beat;
    insert[ "next_heart_beat"     ] = new_next_heart_beat;
    insert[ "is_active"           ] = 0;
    insert[ "scheduler_id"        ] = _spooler->id_for_db();
    insert[ "running_since"       ].set_datetime( string_local_from_time_t( new_last_heart_beat ) );
    insert[ "host"                ] = _spooler->_hostname;
    insert[ "pid"                 ] = getpid();
    insert[ "http_url"            ] = _spooler->http_url();

    ta->execute( insert );

    _last_heart_beat = new_last_heart_beat;
    _next_heart_beat = new_next_heart_beat;
}

//--------------------------------------------------------------Scheduler_member::async_state_text_

string Scheduler_member::async_state_text_() const
{
    S result;

    result << obj_name();

    if( _is_active )  result << " (active)";
    else  
    if( _inactive_scheduler_watchdog )
    {
        result << _inactive_scheduler_watchdog->async_state_text();
    }

    return result;
}

//---------------------------------Scheduler_member::set_command_for_all_inactive_schedulers_but_me

void Scheduler_member::set_command_for_all_inactive_schedulers_but_me( Transaction* ta, Command command )
{
    set_command_for_all_schedulers_but_me( ta, "`is_active`=0", command );
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
        update.add_where( "`scheduler_member_id`<>" + sql::quoted( member_id() ) );
        update.add_where( where );

        ta.execute( update );
        ta.commit();
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", x ) ); }
}

//----------------------------------------------------------------Scheduler_member::async_continue_

bool Scheduler_member::async_continue_( Continue_flags )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    if( _current_operation )
    {
        _current_operation->async_check_exception( "Error in Scheduler member operation" );
        close_operation();
    }

    if( !_activity_stolen )
    {
        if( db()->opened() )
        {
            start_operation();
        }
        else
        if( _is_backup )  z::throw_xc( "SCHEDULER-357" );
    }

    return true;
}

//----------------------------------------------------------Scheduler_member::show_active_scheduler

void Scheduler_member::show_active_scheduler( Transaction* outer_transaction )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    if( db()  &&  db()->opened() )
    for( Retry_transaction ta ( db(), outer_transaction ); ta.enter_loop(); ta++ )
    try
    {
        bool found = false;

        Any_file select = ta.open_result_set( S() << 
                     "select `scheduler_member_id`, `last_heart_beat`, `http_url`, `host`, `pid`"
                     "  from " << _spooler->_members_tablename <<
                     "  where `scheduler_id`=" + sql::quoted( _spooler->id_for_db() ) <<
                        " and `is_active`<>0" );

        while( !select.eof() )
        {
            Record record = select.get_record();
            found = true;

            string other_member_id  = record.as_string( 0 );
            time_t last_heart_beat  = record.as_int64 ( 1 );
            string other_http_url   = record.as_string( 2 );
            string other_hostname   = record.as_string( 3 );
            string other_pid        = record.as_string( 4 );

            _log->info( message_string( "SCHEDULER-995", other_member_id, ::time(NULL) - last_heart_beat, other_http_url, other_pid ) );
        }

        if( !found )  _log->info( message_string( "SCHEDULER-805" ) );

        ta.commit();
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", x ) ); }
}

//------------------------------------------Scheduler_member::scheduler_up_variable_name

string Scheduler_member::scheduler_up_variable_name()
{
    return "scheduler/" + _spooler->id_for_db() + "/up";
}

//------------------------------------------------------------------Scheduler_member::set_member_id
    
void Scheduler_member::set_member_id( const string& member_id )
{
    assert( !_current_operation );

    _scheduler_member_id = member_id;
    check_member_id();

    _log->set_prefix( obj_name() );
}

//---------------------------------------------------------------Scheduler_member::active_member_id

string Scheduler_member::active_member_id()
{
    if( _inactive_scheduler_watchdog )  return _inactive_scheduler_watchdog->_other_scheduler._member_id;
    if( _is_active )  member_id();
    return "";
}

//--------------------------------------------------------Scheduler_member::is_scheduler_up

bool Scheduler_member::is_scheduler_up()
{ 
    return _inactive_scheduler_watchdog  &&  _inactive_scheduler_watchdog->_is_scheduler_up; 
}

//-----------------------------------------------------------------------------Scheduler_member::db

Spooler_db* Scheduler_member::db()
{ 
    return _spooler->_db; 
}

//----------------------------------------------------------------Scheduler_member::check_member_id

void Scheduler_member::check_member_id()
{
    string prefix = _spooler->id_for_db() + ".";
    if( !string_begins_with( member_id(), prefix )  ||  member_id() == prefix )  z::throw_xc( "SCHEDULER-358", member_id(), prefix );
}

//-------------------------------------------------------Scheduler_member::make_scheduler_member_id

void Scheduler_member::make_scheduler_member_id()
{
    set_member_id( S() << _spooler->id_for_db() 
                       << "." << _spooler->_hostname << "." << _spooler->tcp_port() 
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
