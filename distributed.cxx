// $Id$

#include "spooler.h"
#include "../kram/msec.h"
#include "../zschimmer/not_in_recursion.h"

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
const int                       backup_startup_delay                    = 60;                           // Nach eigenem Start des Backup-Schedulers auf Start des non-backup-Schedulers warten
const int                       first_dead_orders_check_period          = 60;

//const int                       Distributed_scheduler::min_precedence   = 0;
//const int                       Distributed_scheduler::max_precedence   = 9999;

//----------------------------------------------------------------------------------Scheduler_member

struct Scheduler_member : Object, Scheduler_object
{
                                Scheduler_member            ( Distributed_scheduler*, const string& member_id );

    bool                        is_empty_member             () const                                { return _member_id == _distributed_scheduler->empty_member_id(); }
    bool                        check_heart_beat            ( time_t now_before_select, const Record& );
    bool                        free_occupied_orders        ( Transaction* = NULL );
    void                        deactivate_and_release_orders_after_death();
    bool                        mark_as_inactive            ( bool delete_inactive_record = false, bool delete_new_active_record = false );
    void                        on_resurrection             ()                                      { _is_dead = false;  _dead_orders_check_period = first_dead_orders_check_period; }
    string                      obj_name                    () const;


    Fill_zero                  _zero_;
    string                     _member_id;
    bool                       _is_dead;
    time_t                     _dead_orders_check_period;
    time_t                     _next_dead_orders_check_time;
    bool                       _is_exclusive;
    bool                       _is_active;
    bool                       _is_checked;
    string                     _deactivating_member_id;
    time_t                     _last_heart_beat_db;
    time_t                     _next_heart_beat_db;
    time_t                     _last_heart_beat_detected;
    string                     _http_url;
    bool                       _scheduler_993_logged;
    Distributed_scheduler*     _distributed_scheduler;
    //time_t                     _clock_difference;
    //bool                       _clock_difference_checked;
    //bool                       _checking_clock_difference;
};

//---------------------------------------------------------------------------------------Heart_beat

struct Heart_beat : Async_operation, Scheduler_object
{
                                Heart_beat                  ( Distributed_scheduler* );


    // Async_operation
    bool                        async_finished_             () const                                { return false; }
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );
    string                      obj_name                    () const                                { return Scheduler_object::obj_name(); }

    void                        set_alarm                   ();

  private:
    Fill_zero                  _zero_;
    Distributed_scheduler*     _distributed_scheduler;
};

//----------------------------------------------------------------------Exclusive_scheduler_watchdog

struct Exclusive_scheduler_watchdog : Async_operation, Scheduler_object
{
                                Exclusive_scheduler_watchdog( Distributed_scheduler* );


    // Async_operation
    bool                        async_finished_             () const                                { return false; }
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );


    void                        calculate_next_check_time   ();
    void                        set_alarm                   ();
    void                        try_to_become_exclusive     ();
  //void                        check_clock_difference      ( time_t last_heart_beat, time_t now );
    bool                        check_has_backup_precedence ();
    string                      read_distributed_scheduler_session_id();
    bool                        restart_when_active_scheduler_has_started();

  private:
    Fill_zero                  _zero_;
    time_t                     _next_check_time;
    bool                       _announced_to_become_exclusive; // Wenn eine Meldung ausgegeben oder ein Datensatz zu ändern versucht wurde 
    time_t                     _next_precedence_check;
    time_t                     _wait_for_exclusive_scheduler_start_until;
    string                     _distributed_scheduler_session_id;
    bool                       _is_starting;
    Distributed_scheduler*     _distributed_scheduler;
  //time_t                     _set_exclusive_until;           // Nur für Access (kennt keine Sperre): Aktivierung bis dann (now+database_commit_visible_time) verzögern, dann nochmal prüfen
};

//------------------------------------------------------------------------Active_schedulers_watchdog

struct Active_schedulers_watchdog : Async_operation, Scheduler_object
{
                                Active_schedulers_watchdog   ( Distributed_scheduler* );


    // Async_operation
    bool                        async_finished_             () const                                { return false; }
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );

    void                        set_alarm                   ();
    void                        calculate_next_check_time   ();

  private:
    Fill_zero                  _zero_;
    time_t                     _next_check_time;
    Distributed_scheduler*     _distributed_scheduler;
};

//----------------------------------------------------------------------------my_string_from_time_t

static string my_string_from_time_t( time_t time )
{
    return string_gmt_from_time_t( time ) + " UTC";
}

//------------------------------------------------------------------ther_scheduler::Scheduler_member
    
Scheduler_member::Scheduler_member( Distributed_scheduler* d, const string& id ) 
: 
    Scheduler_object( d->_spooler, this, Scheduler_object::type_other_scheduler ),
    _zero_(this+1), 
    _distributed_scheduler(d), 
    _member_id(id),
    _dead_orders_check_period( first_dead_orders_check_period )
{
    _log->set_prefix( obj_name() );
}

//----------------------------------------------------------------Scheduler_member::check_heart_beat

bool Scheduler_member::check_heart_beat( time_t now_before_select, const Record& record )
{
    bool result = true;

    _is_exclusive           = !record.null    ( "exclusive" );
    _is_active              = !record.null    ( "active"    );
    _deactivating_member_id = record.as_string( "deactivating_member_id" );
    _http_url               = record.as_string( "http_url"  );

    if( !is_empty_member() )
    {
        if( _last_heart_beat_db == 0  &&  _distributed_scheduler->_my_scheduler != this )     // Neu entdeckt?
        {
            log()->info( message_string( _distributed_scheduler->_demand_exclusiveness && _is_exclusive? "SCHEDULER-833" : "SCHEDULER-820" ) );
        }

        time_t last_heart_beat = record.as_int64( "last_heart_beat" );
        time_t next_heart_beat = record.null( "next_heart_beat" )? 0 : record.as_int64( "next_heart_beat" );


        if( last_heart_beat != _last_heart_beat_db )   // Neuer Herzschlag oder neu entdeckter Scheduler?
        {
            time_t now = ::time(NULL);
            if( _is_dead )
            {
                log()->error( message_string( "SCHEDULER-823", _member_id, now - _last_heart_beat_detected ) );     // Wiederauferstanden?
                on_resurrection();
            }

            _last_heart_beat_detected = now;
        }
        else
        // Behauptung des Schedulers in next_heart_beat prüfen (ohne Konsequenzen, weil die Uhren verschieden gehen können)
        //if( !_is_dead  &&  !_scheduler_993_logged  &&  next_heart_beat + max_processing_time < now_before_select )
        //{
        //    _scheduler_993_logged = true;
        //    log()->warn( message_string( "SCHEDULER-993", _member_id, 
        //                                                  my_string_from_time_t( next_heart_beat ),
        //                                                  now_before_select - next_heart_beat ) );
        //}
        if( _last_heart_beat_detected  &&  _last_heart_beat_detected + heart_beat_period + max_processing_time/2 < now_before_select )
        {
            bool is_in_database_reconnect_tolerance = db()->reopen_time() + Database::seconds_before_reopen + max_processing_time >= now_before_select;
            bool was_alive = !_is_dead;
            bool is_dead   = _last_heart_beat_detected + heart_beat_period + max_processing_time < now_before_select;

            if( was_alive )
                log()->warn( message_string( !is_dead                          ? "SCHEDULER-994" :
                                             is_in_database_reconnect_tolerance? "SCHEDULER-996" 
                                                                               : "SCHEDULER-995", 
                                             my_string_from_time_t( _last_heart_beat_detected ), 
                                             now_before_select - _last_heart_beat_detected ) );

            if( !is_in_database_reconnect_tolerance ) 
            {
                _is_dead = true; 
                result = false;
            }
        }

        _last_heart_beat_db = last_heart_beat;
        _next_heart_beat_db = next_heart_beat;
    }

    return result;
}

//-----------------------------------------------------------Scheduler_member::free_occupied_orders

bool Scheduler_member::free_occupied_orders( Transaction* outer_transaction )
{
    //Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    bool result = false;


    try
    {
        Transaction ta ( db(), outer_transaction );
        int         record_count;
        string      fake_member_id = _member_id + "?";


        // Vielleicht beschränken auf Jobketten und Zustände des eigenen Schedulers. Erstmal nicht.

        {
            sql::Update_stmt update ( db()->database_descriptor(), _spooler->_orders_tablename );
            update[ "occupying_scheduler_member_id" ] = fake_member_id;
            update.and_where_condition( "occupying_scheduler_member_id", _member_id );
         //?update.add_where( " and `distributed_next_time` is not null" );               // Die Spalte ist vielleicht für schnellen Zugriff indiziert
            ta.execute( update, __FUNCTION__ );
            record_count = ta.record_count();
        }

        if( record_count )
        {
            Any_file result_set = ta.open_result_set
                ( 
                    S() << "select `job_chain`, `id`, `state` " <<
                    "  from " << _spooler->_orders_tablename << 
                    "  where `occupying_scheduler_member_id`=" << sql::quoted( fake_member_id ) <<
                    //?" and `distributed_next_time` is not null" <<               // Die Spalte ist vielleicht für schnellen Zugriff indiziert
                    "  order by `job_chain`, `state`, `distributed_next_time`, `ordering`",
                    __FUNCTION__
                );

            while( !result_set.eof() )
            {
                Record record         = result_set.get_record();

                string job_chain_name = record.as_string( 0 );
                string order_id       = record.as_string( 1 );
                string state          = record.as_string( 2 );

                _log->warn( message_string( "SCHEDULER-829", job_chain_name, order_id ) );

                if( Job_chain* job_chain = order_subsystem()->job_chain_or_null( job_chain_name ) )
                    job_chain->tip_for_new_order( state, Time(0) );
            }

            sql::Update_stmt update ( db()->database_descriptor(), _spooler->_orders_tablename );
            update[ "occupying_scheduler_member_id" ] = sql::null_value;
            update.and_where_condition( "occupying_scheduler_member_id", fake_member_id );
         //?update.add_where( " and `distributed_next_time` is not null" );               // Die Spalte ist vielleicht für schnellen Zugriff indiziert
            ta.execute( update, __FUNCTION__ );
            record_count = ta.record_count();
        }

        ta.commit( __FUNCTION__ );
        result = record_count > 0;
    }
    catch( exception& x ) { _log->error( S() << x.what() << ", in " << __FUNCTION__ ); }

    return result;
}

//--------------------------------------Scheduler_member::deactivate_and_release_orders_after_death

void Scheduler_member::deactivate_and_release_orders_after_death()
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "  _next_dead_orders_check_time=" << my_string_from_time_t( _next_dead_orders_check_time ) << "\n" );


    time_t now = ::time(NULL);

    if( _next_dead_orders_check_time <= now )
    {
        if( _is_active )  mark_as_inactive();             // ruft free_occupied_orders()
                    else  free_occupied_orders();

        _dead_orders_check_period *= 2;
        _next_dead_orders_check_time = now + _dead_orders_check_period;
    }
}

//----------------------------------------------------------Distributed_scheduler::mark_as_inactive

bool Scheduler_member::mark_as_inactive( bool delete_inactive_record, bool delete_empty_member_record )
{
    assert( !delete_empty_member_record || delete_inactive_record );
    assert( _is_active || delete_inactive_record );

    bool ok = false;

    if( _is_dead )  _log->warn( message_string( "SCHEDULER-836" ) );  // "Deactivating dead Scheduler"


    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        _distributed_scheduler->lock_member_records( &ta, _member_id, _distributed_scheduler->empty_member_id() );


        free_occupied_orders( &ta );


        sql::Update_stmt update ( ta.database_descriptor(), _spooler->_members_tablename );

        update[ "active"    ] = sql::null_value;
        update[ "exclusive" ] = sql::null_value;
        if( _member_id != _distributed_scheduler->my_member_id() )  update[ "deactivating_member_id" ] = _distributed_scheduler->my_member_id();
        update.and_where_condition( "scheduler_member_id", _member_id );
      //update.and_where_condition( "active"             , _is_active   ? sql::Value( 1 ) : sql::null_value );
        update.and_where_condition( "exclusive"          , _is_exclusive? sql::Value( 1 ) : sql::null_value );
      //update.and_where_condition( "last_heart_beat"    , _db_last_heart_beat );
      //update.and_where_condition( "next_heart_beat"    , _db_next_heart_beat );

        ok = ta.try_execute_single( delete_inactive_record? update.make_delete_stmt() 
                                                          : update.make_update_stmt(), 
                                    __FUNCTION__ );


        if( ok && _is_exclusive )
        {
            sql::Update_stmt update ( ta.database_descriptor(), _spooler->_members_tablename );
        
            update[ "exclusive" ] = 1;
            update.and_where_condition( "scheduler_member_id", _distributed_scheduler->empty_member_id() );
          //update.and_where_condition( "active"             , sql::null_value );
            update.and_where_condition( "exclusive"          , sql::null_value );

            ok = ta.try_execute_single( delete_empty_member_record? update.make_delete_stmt() 
                                                                  : update.make_update_stmt(), __FUNCTION__ );
            if( !ok )
            {
                _log->error( message_string( "SCHEDULER-371", "Update without effect: " + update.make_stmt(), "Drop table " + _spooler->_members_tablename ) );
                _distributed_scheduler->_is_in_error = true;
                _distributed_scheduler->show_exclusive_scheduler( &ta );
            }
        }

        if( ok )  ta.commit( __FUNCTION__ );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ), __FUNCTION__ ); }


    if( this == _distributed_scheduler->_my_scheduler )
    {
        _is_active    = false;
        _is_exclusive = false;
    }

    if( _member_id == _distributed_scheduler->my_member_id() )
    {
        _distributed_scheduler->_is_active         = false;
        _distributed_scheduler->_has_exclusiveness = false;
    }

    return ok;
}

//---------------------------------------------------------Distributed::shift_exclusiveness_from_to

//bool Distributed::shift_exclusiveness_from_to( Scheduler_member* from, Scheduler_member* to, bool delete_from_record, bool delete_empty_member_record )
//{
//    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
//    assert( from );
//    assert( from->_is_exclusive );
//    assert( to );
//    assert( !to->_is_exclusive );
//    assert( !delete_empty_member_record || delete_inactive_record );
//    assert( from->_is_active || delete_from_record );
//
//    bool   ok;
//    time_t now = ::time(NULL);
//
//    calculate_next_heart_beat( now );
//
//    time_t new_db_last_heart_beat = now;
//    time_t new_db_next_heart_beat = _next_heart_beat;
//
//
//    if( to == _my_scheduler )  _log->info( message_string( "SCHEDULER-835" ) );  // "This Scheduler becomes exclusive"
//
//    try
//    {
//        Transaction ta ( db() );
//
//        lock_member_records( &ta, from->_member_id, to->_member_id );
//
//
//        // Dem bisher exklusiven Scheduler die Exklusivität nehmen
//
//        {
//            sql::Update_stmt update ( ta.database_descriptor(), _spooler->_members_tablename );
//        
//            update[ "exclusive" ] = sql::null_value;
//            if( from != _my_scheduler   )  update[ "deactivating_member_id" ] = my_member_id();
//            if( from->is_empty_member() )  update[ "active" ] = 1;
//            update.and_where_condition( "scheduler_member_id", from->_member_id );
//            update.add_where( " and `exclusive` is not null" );
//            
//            if( !from->is_empty_member() )
//            {
//                update.and_where_condition( "last_heart_beat", from->_last_heart_beat_db );
//                update.and_where_condition( "next_heart_beat", from->_next_heart_beat_db );
//            }
//
//            ok = ta.try_execute_single( update, __FUNCTION__ );
//        }
//
//
//        if( ok )
//        {
//            sql::Update_stmt update ( ta.database_descriptor(), _spooler->_members_tablename );
//        
//            update[ "active"          ] = 1;
//            update[ "exclusive"       ] = 1;
//            update[ "last_heart_beat" ] = new_db_last_heart_beat;
//            update[ "next_heart_beat" ] = new_db_next_heart_beat;
//            update.and_where_condition( "scheduler_member_id", my_member_id() );
//            update.and_where_condition( "exclusive"          , sql::null_value );
//            update.add_where( S() << " and `active`" << ( to->_is_active? " is not null" : " is null" ) );
//
//            ok = ta.try_execute_single( update, __FUNCTION__ );
//        }
//
//        if( ok )  ta.commit( __FUNCTION__ );
//    }
//    catch( exception& x )   // Bei optimistischer Sperrung kann es eine Exception geben
//    { 
//        ok = false;
//        _log->debug3( S() << x.what() << ", in " << __FUNCTION__ );
//    }
//
//    if( ok )
//    {
//        _db_last_heart_beat = new_db_last_heart_beat;
//        _db_next_heart_beat = new_db_next_heart_beat;
//
//        _is_active         = true;
//        _has_exclusiveness = true;
//
//        //_active_schedulers_map.erase( _exclusive_scheduler->my_member_id() );
//        _exclusive_scheduler = NULL;
//        //check_schedulers_heart_beat();
//        int ACTIVE_SCHEDULER_MAP_AKTUALISIEREN;
//        //_set_exclusive_until = ::time(NULL);
//        //if( db_mode == use_commit_visible_time )  _set_exclusive_until += database_commit_visible_time + 1;     // Nachfolgende Sekunde
//    }
//
//    if( ok )  assert_database_integrity( __FUNCTION__ );
//    if( ok )  ok = do_a_heart_beat();
//    if( ok )  _log->info( message_string( "SCHEDULER-806" ) );
//
//    return ok;
//}

//------------------------------------------------------------------------Scheduler_member::obj_name

string Scheduler_member::obj_name() const
{ 
    S result;
    result << Scheduler_object::obj_name();
    if( is_empty_member() )  result << " (empty record)";
                       else  result << " " << _member_id;

    return result;
}

//---------------------------------------------------------------------------Heart_beat::Heart_beat

Heart_beat::Heart_beat( Distributed_scheduler* m ) 
:
    Scheduler_object( m->_spooler, this, Scheduler_object::type_heart_beat ),
    _zero_(this+1),
    _distributed_scheduler(m)
{
    set_alarm();
}

//--------------------------------------------------------------------Heart_beat::async_state_text_
    
string Heart_beat::async_state_text_() const
{
    return obj_name();
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
        bool ok = _distributed_scheduler->do_a_heart_beat();
        if( ok )
        {
            set_alarm();                            // Wir sind weiterhin aktiv
        }
        else
        {
            _distributed_scheduler->async_wake();   // Wir sind nicht mehr aktiv, Operation beenden!
        }
    }
    catch( exception& )
    {
        _distributed_scheduler->async_wake();       // Distributed_scheduler wird die Exception übernehmen
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
    Scheduler_object( m->_spooler, this, Scheduler_object::type_exclusive_scheduler_watchdog ),
    _zero_(this+1),
    _distributed_scheduler(m)
{
    _log = m->_log;
    _distributed_scheduler_session_id = read_distributed_scheduler_session_id();
    _is_starting = true;
    set_alarm();
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
        _distributed_scheduler->async_wake();       // Datenbank ist geschlossen worden
        return true;
    }

    try
    {
        bool ok = restart_when_active_scheduler_has_started(); 
        if( !ok )  try_to_become_exclusive();
    }
    catch( exception& )
    {
        _distributed_scheduler->async_wake();       // Distributed_scheduler wird die Exception übernehmen
        throw;
    }

    if( _distributed_scheduler->has_exclusiveness() )
    {
        _distributed_scheduler->async_wake();       // Wir sind aktives Mitglied geworden, Exclusive_scheduler_watchdog beenden
    }
    else
    {
        calculate_next_check_time();
        set_alarm();                                // Wir sind weiterhin inaktives Mitglied
    }

    return true;
}

//----------------------------------------------------------Exclusive_scheduler_watchdog::set_alarm

void Exclusive_scheduler_watchdog::set_alarm()
{
    //if( _distributed_scheduler->_db_next_heart_beat < _next_check_time )
    //{
    //    Z_LOG2( "scheduler.distributed", __FUNCTION__ << "  Next heart beat at " << my_string_from_time_t( _distributed_scheduler->_db_next_heart_beat ) << "\n" );
    //    set_async_next_gmtime( _distributed_scheduler->_db_next_heart_beat );
    //}
    //else
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


    _distributed_scheduler->check_schedulers_heart_beat();  // Stellt fest: is_scheduler_up(), _exclusive_scheduler


    Scheduler_member* exclusive_scheduler = _distributed_scheduler->exclusive_scheduler();

    if( exclusive_scheduler )  _wait_for_exclusive_scheduler_start_until = 0;
    else
    if( _is_starting  &&  _distributed_scheduler->_is_backup  &&  _distributed_scheduler->is_scheduler_up() )
    {
        _wait_for_exclusive_scheduler_start_until = ::time(NULL) + backup_startup_delay;
        _log->info( message_string( "SCHEDULER-831" ) );
    }
    _is_starting = false;


    if( !_wait_for_exclusive_scheduler_start_until ) 
    {
        if( exclusive_scheduler  &&  !exclusive_scheduler->_is_dead ) 
        {
            _next_precedence_check = 0;
        }
        else
        if( _distributed_scheduler->is_scheduler_up()  &&  check_has_backup_precedence() )
        {
            _announced_to_become_exclusive = true;
            _distributed_scheduler->mark_as_exclusive();
        }
    }


    if( _announced_to_become_exclusive  &&  !_distributed_scheduler->_has_exclusiveness )  //&&  !_set_exclusive_until )
    {
        _announced_to_become_exclusive = false;
        _distributed_scheduler->show_exclusive_scheduler( (Transaction*)NULL );
    }
}

//------------------------------------------Exclusive_scheduler_watchdog::calculate_next_check_time

void Exclusive_scheduler_watchdog::calculate_next_check_time()
{
    time_t now = ::time(NULL);

    _next_check_time = now + active_heart_beat_maximum_check_period;

    //if( _set_exclusive_until  &&  _next_check_time > _set_exclusive_until )
    //{
    //    _next_check_time = _set_exclusive_until;
    //    extra_log << ", warten, bis Datenbank Änderungen für alle sichtbar gemacht hat";
    //}
    //else
    
    if( Scheduler_member* watched_scheduler = _distributed_scheduler->exclusive_scheduler() )
    {
        time_t delay          = max_processing_time + 1;   // Erst in der folgenden Sekunde prüfen
        time_t new_next_check = watched_scheduler->_next_heart_beat_db + delay + 1;

        if( new_next_check - now >= active_heart_beat_minimum_check_period  &&  new_next_check < _next_check_time )
        {
            time_t diff = new_next_check - _next_check_time;
            if( abs(diff) > 2 )  Z_LOG2( "scheduler.distributed", __FUNCTION__ << "  Synchronized with _next_heart_beat_db=" << my_string_from_time_t( watched_scheduler->_next_heart_beat_db ) << ": " << diff << "s\n" );
            _next_check_time = new_next_check;
        }
    }
}

//--------------------------------------------Exclusive_scheduler_watchdog::check_has_backup_precedence

bool Exclusive_scheduler_watchdog::check_has_backup_precedence()
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

//----------------------------------------------Exclusive_scheduler_watchdog::check_clock_difference

//void Exclusive_scheduler_watchdog::check_clock_difference( time_t last_heart_beat, time_t now )
//{
//    _exclusive_scheduler->_clock_difference         = last_heart_beat - now;
//    _exclusive_scheduler->_clock_difference_checked = true;
//    
//    time_t own_delay = ::time(NULL) - _next_check_time;
//
//    if( abs( _exclusive_scheduler->_clock_difference ) < own_delay + warned_clock_difference + 1 )
//    {
//        _log->info( message_string( "SCHEDULER-804", _exclusive_scheduler->_member_id ) );
//    }
//    else
//    {
//        _log->warn( message_string( "SCHEDULER-364", _exclusive_scheduler->_clock_difference, _exclusive_scheduler->_member_id ) );
//    }
//
//    _exclusive_scheduler->_checking_clock_difference = false;
//}

//--------------------------Exclusive_scheduler_watchdog::restart_when_active_scheduler_has_started

bool Exclusive_scheduler_watchdog::restart_when_active_scheduler_has_started()
{
    bool result = false;

    if( _distributed_scheduler->is_scheduler_up() )
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

    if( _distributed_scheduler->is_scheduler_up() )
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
        catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ), __FUNCTION__ ); }
    }

    return result;
}

//---------------------------------------Active_schedulers_watchdog::Active_schedulers_watchdog

Active_schedulers_watchdog::Active_schedulers_watchdog( Distributed_scheduler* m ) 
:
    Scheduler_object( m->_spooler, this, Scheduler_object::type_active_schedulers_watchdog ),
    _zero_(this+1),
    _distributed_scheduler(m)
{
    set_alarm();
}

//--------------------------------------------------Active_schedulers_watchdog::async_state_text_

string Active_schedulers_watchdog::async_state_text_() const
{
    S result;

    result << "Active_schedulers_watchdog";

    return result;
}

//----------------------------------------------------Active_schedulers_watchdog::async_continue_

bool Active_schedulers_watchdog::async_continue_( Continue_flags )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );


    if( !db()->opened() )
    {
        _distributed_scheduler->async_wake();       // Datenbank ist geschlossen worden
        return true;
    }

    try
    {
        bool ok = _distributed_scheduler->check_schedulers_heart_beat();

        if( !ok )
        {
            Z_FOR_EACH( Distributed_scheduler::Scheduler_map, _distributed_scheduler->_scheduler_map, it )
            {
                Scheduler_member* other_scheduler = it->second;

                if( other_scheduler->_is_dead  &&  
                    other_scheduler->_member_id != _distributed_scheduler->my_member_id() )
                {
                    other_scheduler->deactivate_and_release_orders_after_death();
                }
            }
        }
    }
    catch( exception& )
    {
        _distributed_scheduler->async_wake();       // Distributed_scheduler wird die Exception übernehmen
        throw;
    }

    calculate_next_check_time();
    set_alarm();

    return true;
}

//--------------------------------------------Active_schedulers_watchdog::calculate_next_check_time

void Active_schedulers_watchdog::calculate_next_check_time()
{
    time_t now = ::time(NULL);

    _next_check_time = now + active_heart_beat_maximum_check_period;
}

//------------------------------------------------------------Active_schedulers_watchdog::set_alarm

void Active_schedulers_watchdog::set_alarm()
{
    Z_LOG2( "scheduler.distributed", __FUNCTION__ << "  Next check at " << my_string_from_time_t( _next_check_time ) << "\n" );
    set_async_next_gmtime( _next_check_time );
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

        if( my_member_id() != "" )
        try
        {
            if( _my_scheduler )  _my_scheduler->mark_as_inactive( true );    // Member-Satz löschen
            
            if( _was_start_ok  &&  !_is_in_error )
            {
                delete_old_member_records( (Transaction*)NULL );
            }
        }
        catch( exception& x )
        {
            if( _log )  _log->warn( S() << x.what() << ", in " << __FUNCTION__ );
        }

        _closed = true;
    }
}

//------------------------------------------------------------Distributed_scheduler::begin_shutdown

//void Distributed_scheduler::mark_begin_of_shutdown()
//
//// Kündigt das Herunterfahren an durch active=null im leeren Memberdatensatz 
//// Für den Fall, dass das nicht vollständig heruntergefahren werden kann und der Datensatz nicht gelöscht wird.
//
//{
//    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );
//
//    if( db()  &&  db()->opened()  )
//    {
//        for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
//        {
//            sql::Update_stmt update ( ta.database_descriptor(), _spooler->_members_tablename );
//
//            update[ "deactivating_member_id" ] = my_member_id();
//            update[ "active"                 ] = sql::null_value;
//            update.and_where_condition( "active"             , sql::Value( 1 ) );
//            update.and_where_condition( "scheduler_member_id", empty_member_id() );
//
//            bool ok = ta.try_execute_single( update, __FUNCTION__ );
//
//            if( ok )  ta.commit( __FUNCTION__ );
//        }
//        catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ), __FUNCTION__ ); }
//    }
//}

//------------------------------------------------------------------Distributed_scheduler::shutdown

void Distributed_scheduler::shutdown()
{
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

    if( db()  &&  db()->opened()  )
    {
        if( _my_scheduler )  _my_scheduler->mark_as_inactive( true, true );  // Member-Satz und leeren Member-Satz löschen
    }

    close();
}

//-------------------------------------------------Distributed_scheduler::calculate_next_heart_beat

void Distributed_scheduler::calculate_next_heart_beat( time_t now )
{
    _next_heart_beat = now + heart_beat_period;
}

//---------------------------------------------------------------------Distributed_scheduler::start

bool Distributed_scheduler::start()
{
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

    assert( !_heart_beat );
    assert( !_exclusive_scheduler_watchdog );
    assert( !_active_schedulers_watchdog );
    assert( !_is_backup || _demand_exclusiveness );

    if( !db()->opened() )  z::throw_xc( "SCHEDULER-357" ); 
    if( db()->lock_syntax() == db_lock_none )  z::throw_xc( "SCHEDULER-359", db()->dbms_name() );

    if( _scheduler_member_id == "" )  make_scheduler_member_id();
    check_member_id();
    create_table_when_needed();


    // Datenbanksätze einrichten

    _is_active = !_demand_exclusiveness;

    check_empty_member_record();
    assert_database_integrity( __FUNCTION__ );

    insert_member_record();


    // Aufräumen

    delete_old_member_records( (Transaction*)NULL );


    // Informationen über die anderen Scheduler einholen

    check_schedulers_heart_beat();

    if( !_my_scheduler )  
    {
        _is_in_error = true;
        z::throw_xc( "SCHEDULER-371", S() << "Missing own record '" << my_member_id() << "' in table " << _spooler->_members_tablename );
    }


    // Start

    set_async_manager( _spooler->_connection_manager );
    start_operations();



    if( !is_scheduler_up() )  _log->info( message_string( "SCHEDULER-832" ) );
    //else
    //if( _demand_exclusiveness  &&  !_has_exclusiveness )  _log->info( message_string(  ) );



    _was_start_ok = true;

    return true;
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
            "`deactivating_member_id`" " varchar(100), "
            "primary key( `scheduler_member_id` )" );

    ta.commit( __FUNCTION__ );
}

//----------------------------------------------------------Distributed_scheduler::start_operations

void Distributed_scheduler::start_operations()
{
    if( !_heart_beat )
    {
        _heart_beat = Z_NEW( Heart_beat( this ) );
        _heart_beat->set_async_manager( _spooler->_connection_manager );
    }

    if( _demand_exclusiveness  &&  !has_exclusiveness() )
    {
        assert( !_active_schedulers_watchdog );

        if( !_exclusive_scheduler_watchdog )
        {
            _exclusive_scheduler_watchdog = Z_NEW( Exclusive_scheduler_watchdog( this ) );
            _exclusive_scheduler_watchdog->set_async_manager( _spooler->_connection_manager );
        }
    }
    else
    if( _is_active  &&  _watch_distributed_order_execution  &&  !_active_schedulers_watchdog )
    {
        assert( !_exclusive_scheduler_watchdog );

        //-------------------------------------------------------------------------------------------------
        
        _active_schedulers_watchdog = Z_NEW( Active_schedulers_watchdog( this ) );
        _active_schedulers_watchdog ->set_async_manager( _spooler->_connection_manager );
    }
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
        _exclusive_scheduler_watchdog->set_async_manager( NULL );
        _exclusive_scheduler_watchdog = NULL;
    }

    if( _active_schedulers_watchdog )
    {
        _active_schedulers_watchdog->set_async_manager( NULL );
        _active_schedulers_watchdog = NULL;
    }
}

//------------------------------------------------Distributed_scheduler::wait_until_is_scheduler_up

//bool Distributed_scheduler::wait_until_is_scheduler_up()
//{
//    _log->info( message_string( "SCHEDULER-832" ) );
//
//    while( !_spooler->is_termination_state_cmd()  &&  !is_scheduler_up() )
//    {
//        _spooler->simple_wait_step();
//        async_check_exception();
//    }
//
//    bool ok = is_scheduler_up();
//
//    if( ok )  assert_database_integrity( __FUNCTION__ );
//
//    return ok;
//}
//
////------------------------------------------------------Distributed_scheduler::wait_until_is_active
//
//bool Distributed_scheduler::wait_until_is_active()
//{
//    bool was_scheduler_up = is_scheduler_up();
//
//    //if( !was_scheduler_up )  _log->info( message_string( "SCHEDULER-832" ) );
//
//    while( !_spooler->is_termination_state_cmd()  &&  !_is_active )  
//    {
//        if( was_scheduler_up  &&  !is_scheduler_up() ) 
//        {
//            _log->info( message_string( "SCHEDULER-834" ) );
//            was_scheduler_up = false; 
//        }
//
//        _spooler->simple_wait_step();
//        async_check_exception();
//    }
//
//    bool ok = _is_active;
//
//    if( ok )  assert_database_integrity( __FUNCTION__ );
//
//    return ok;
//}
//
////----------------------------------------------Distributed_scheduler::wait_until_has_exclusiveness
//
//bool Distributed_scheduler::wait_until_has_exclusiveness()
//{
//    wait_until_is_active();
//
//    if( exclusive_member_id() != "" )  _log->info( message_string( "SCHEDULER-833", exclusive_member_id() ) );
//
//    while( !_spooler->is_termination_state_cmd()  &&  !has_exclusiveness() )  _spooler->simple_wait_step();
//
//    bool ok = has_exclusiveness();
//    if( ok )  assert_database_integrity( __FUNCTION__ );
//
//    return ok;
//}
//
//-----------------------------------------------------------Distributed_scheduler::async_continue_

bool Distributed_scheduler::async_continue_( Continue_flags )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    if( _heart_beat                   )  _heart_beat                  ->async_check_exception( "Heart_beat" );
    if( _exclusive_scheduler_watchdog )  _exclusive_scheduler_watchdog->async_check_exception( "Exclusive_scheduler_watchdog" );
    if( _active_schedulers_watchdog   )  _active_schedulers_watchdog  ->async_check_exception( "Active_schedulers_watchdog" );


    if( _has_exclusiveness  &&  _exclusive_scheduler_watchdog )
    {
        _exclusive_scheduler_watchdog->set_async_manager( NULL );
        _exclusive_scheduler_watchdog = NULL;
    }

    if( _was_start_ok  &&  !_is_in_error )
    {
        start_operations();

    }

    return true;
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
            _log->error( message_string( "SCHEDULER-827", my_string_from_time_t( expected_next_heart_beat ), now - expected_next_heart_beat ) );
        }
    }

    return result;
}

//------------------------------------------------------------Distributed_scheduler::do_a_heart_beat

bool Distributed_scheduler::do_a_heart_beat()
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    bool   had_exclusiveness   = has_exclusiveness();
    bool   ok                  = true;


    if( !db()->opened() )  return false;

    if( _spooler->_db_check_integrity )  assert_database_integrity( __FUNCTION__ );

    if( ok )
    {
        if( Not_in_recursion not_in_recursion = &_is_in_do_a_heart_beat )   
        // commit() und reopen_database_after_error() rufen _spooler->check(), der nach Fristablauf wieder in diese Routine steigen kann
        {
            time_t old_next_heart_beat = _db_next_heart_beat;

            ok = heartbeat_member_record();
            if( ok )  check_heart_beat_is_in_time( old_next_heart_beat );  // Verspätung wird nur gemeldet
        }
        else
        {
            Z_LOG2( "scheduler", __FUNCTION__ << "  Rekursiver Aufruf\n" );
            ok = true;
        }
    }
    
    if( !ok )
    {
        if( had_exclusiveness )  _is_exclusiveness_lost = true;
        
        bool force_error = true;
        check_my_member_record( (Transaction*)NULL, force_error );
    }

    return ok;
}
    
//--------------------------------------------Distributed_scheduler::heartbeat_member_record

bool Distributed_scheduler::heartbeat_member_record()
{
    bool   ok          = false;
    bool   has_command = false;


    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        time_t now = ::time(NULL);
        calculate_next_heart_beat( now );

        time_t new_db_last_heart_beat = now;
        time_t new_db_next_heart_beat = _next_heart_beat;

        //Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "  new_db_last_heart_beat=" << new_db_last_heart_beat << " (" << my_string_from_time_t( new_db_last_heart_beat ) << "), "
        //                                                    "new_db_next_heart_beat=" << new_db_next_heart_beat << " (" << my_string_from_time_t( new_db_next_heart_beat ) << ")\n" );

        sql::Update_stmt update ( ta.database_descriptor(), _spooler->_members_tablename );
        
        update[ "last_heart_beat" ] = new_db_last_heart_beat;
        update[ "next_heart_beat" ] = new_db_next_heart_beat;

        update.and_where_condition( "scheduler_member_id", my_member_id()      );
        update.and_where_condition( "last_heart_beat"    , _db_last_heart_beat );
        update.and_where_condition( "next_heart_beat"    , _db_next_heart_beat );
        update.add_where( S() << " and `active`"    << ( _is_active        ? " is not null" : " is null" ) );
        update.add_where( S() << " and `exclusive`" << ( _has_exclusiveness? " is not null" : " is null" ) );
        update.and_where_condition( "command", _heart_beat_command_string != ""? sql::Value( _heart_beat_command_string ) 
                                                                               : sql::null_value );
        bool record_is_updated = ta.try_execute_single( update, __FUNCTION__ );

        if( !record_is_updated )
        {
            update.remove_where_condition( "command" );
            record_is_updated = ta.try_execute_single( update, __FUNCTION__ );
            has_command = record_is_updated;     // Neues Kommando
        }

        ta.commit( __FUNCTION__ );      // do_a_heart_beat() verhindert den rekursiven Aufruf über _spooler->check_is_active()
        ok = record_is_updated;

        if( ok )
        {
            _db_last_heart_beat = new_db_last_heart_beat;
            _db_next_heart_beat = new_db_next_heart_beat;
        }
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ), __FUNCTION__ ); }

    //if( !ok )
    //{
    //    check_schedulers_heart_beat();   // Aktualisiert _exclusive_scheduler->... und _my_scheduler->...
    //}

    if( has_command )  read_and_execute_command();

    return ok;
}

//----------------------------------------------------Distributed_scheduler::check_my_member_record

bool Distributed_scheduler::check_my_member_record( Transaction* outer_transaction, bool force_error )
{
    bool   is_active              = false;
    bool   is_exclusive           = false;
    string deactivating_member_id;

    for( Retry_transaction ta ( db(), outer_transaction ); ta.enter_loop(); ta++ ) try
    {
        Any_file result_set = ta.open_result_set( S() << "select `active`, `exclusive`, `deactivating_member_id` "
                                                         "  from " << _spooler->_members_tablename << 
                                                         "  where `scheduler_member_id`=" << sql::quoted( my_member_id() ),
                                                  __FUNCTION__ );
        if( !result_set.eof() )
        {
            Record record = result_set.get_record();

            is_active              = !record.null( 0 );
            is_exclusive           = !record.null( 1 );
            deactivating_member_id = record.as_string( 2 );
        }
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ), __FUNCTION__ ); }


    bool ok = true;

    if( force_error  ||  _is_active  &&  !is_active )
    {
        _is_active = false;
        ok = false;
    }

    if( force_error  ||  _has_exclusiveness  &&  !is_exclusive )
    {
        _is_exclusiveness_lost |= _has_exclusiveness;
        _has_exclusiveness = false;
        ok = false;
    }

    if( !ok )
    {
        _is_in_error = true;

        check_heart_beat_is_in_time( _next_heart_beat );      // Vielleicht haben wir uns verspätet? (Setzt _late_heart_beat)

        if( deactivating_member_id == "" )  deactivating_member_id = "(unknown)";
        
        if( _is_exclusiveness_lost )  _log->error( message_string( _late_heart_beat? "SCHEDULER-377" : "SCHEDULER-372", deactivating_member_id ) );     // "SOME OTHER SCHEDULER HAS STOLEN EXCLUSIVENESS"
        _log->error( message_string( _late_heart_beat? "SCHEDULER-378" : "SCHEDULER-373", deactivating_member_id ) );   
    }

    return ok;
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
    bool transaction_written = true;
    ta->open_result_set( sql, __FUNCTION__, transaction_written );
}

//-------------------------------------------------Distributed_scheduler::delete_old_member_records

void Distributed_scheduler::delete_old_member_records( Transaction* outer_transaction )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    try
    {
        Transaction ta ( db(), outer_transaction );

        ta.execute
        ( 
            S() << "DELETE from " << _spooler->_members_tablename << 
                    "  where `scheduler_id`= " << sql::quoted( _spooler->id_for_db() ) <<
                       " and `active` is null"
                       " and `next_heart_beat`<" << ( ::time(NULL) - trauerfrist ), 
            S() << "Deleting inactive records with a heart beat more then " << trauerfrist << "s ago"
        );

        int record_count = ta.record_count();

        ta.commit( __FUNCTION__ );

        if( record_count )  _log->warn( message_string( "SCHEDULER-828", record_count, _spooler->_members_tablename ) );
    }
    catch( exception& x )
    {
        _log->warn( S() << x.what() << ", in " << __FUNCTION__ );
    }
}

//--------------------------------------------------Distributed_scheduler::read_and_execute_command

void Distributed_scheduler::read_and_execute_command()
{
    Command command = cmd_none;

    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        Any_file result_set = ta.open_result_set
        ( 
            S() << "select `command`"
                   "  from " << _spooler->_members_tablename << 
                   "  where `scheduler_member_id`=" << sql::quoted( my_member_id() ),
            __FUNCTION__
        );

        //if( !result_set.eof() )  Darf nicht passieren!
        {
            _heart_beat_command_string = result_set.get_record().as_string( 0 );
            command = command_from_string( _heart_beat_command_string) ;
        }
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ), __FUNCTION__ ); }

    if( command )  execute_command( command );
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

//-----------------------------------------------Distributed_scheduler::check_schedulers_heart_beat

bool Distributed_scheduler::check_schedulers_heart_beat()
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    
    bool   result            = true;
    time_t now_before_select = ::time(NULL);      // Vor dem Datenbankzugriff

    ptr<Scheduler_member> previous_exclusive_scheduler = _exclusive_scheduler;
    _exclusive_scheduler = NULL;
    Z_FOR_EACH( Scheduler_map, _scheduler_map, it )  it->second->_is_checked = false;


    // Alle eingetragenen Scheduler lesen

    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        Any_file result_set = ta.open_result_set( S() << 
                     "select `scheduler_member_id`, `last_heart_beat`, `next_heart_beat`, `exclusive`, `active`, `deactivating_member_id`, `http_url` "
                      " from " << _spooler->_members_tablename << 
                     "  where `scheduler_id`=" << sql::quoted( _spooler->id_for_db() ),
                        //" and `active` is not null",
                     __FUNCTION__ );

        while( !result_set.eof() )
        {
            Record record                    = result_set.get_record();
            string other_scheduler_member_id = record.as_string( 0 );

            ptr<Scheduler_member> other_scheduler = _scheduler_map[ other_scheduler_member_id ];
            if( !other_scheduler )
            {
                other_scheduler = Z_NEW( Scheduler_member( this, other_scheduler_member_id ) );
                _scheduler_map[ other_scheduler_member_id ] = other_scheduler;
                if( !_my_scheduler  &&  other_scheduler->_member_id == my_member_id() )  _my_scheduler = other_scheduler;
            }

            other_scheduler->_is_checked = true;

            result &= other_scheduler->check_heart_beat( now_before_select, record );

            if( other_scheduler->_is_exclusive )
            {
                if( _exclusive_scheduler )  
                {
                    _is_in_error = true;
                    z::throw_xc( "SCHEDULER-371", __FUNCTION__, "double exclusive" );       // "DATABASE INTEGRITY IS BROKEN"
                }

                _exclusive_scheduler = other_scheduler;
            }
        }
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ), __FUNCTION__ ); }


    for( Scheduler_map::iterator it = _scheduler_map.begin(); it != _scheduler_map.end(); )
    {
        Scheduler_map::iterator next_it = it;  next_it++;

        Scheduler_member* o = it->second;
        if( !o->_is_checked )
        {
            if( o == _my_scheduler )  _my_scheduler = NULL, _log->warn( S() << __FUNCTION__ << "  Own Scheduler member record has been deleted" );
            if( !o->_is_dead )  o->log()->info( message_string( "SCHEDULER-826" ) );

            Z_WINDOWS_ONLY( next_it= )  _scheduler_map.erase( it );        // next_it= vorsichtshalber (oder bleibt next_it stabil?). gcc-erase liefert void
        }

        it = next_it;
    }

    if( previous_exclusive_scheduler != exclusive_scheduler() )
    {
        if( !exclusive_scheduler() )  _log->debug( message_string( "SCHEDULER-825" ) );
        else
        if( exclusive_scheduler() != _my_scheduler )   _log->info( message_string( "SCHEDULER-824", exclusive_scheduler()->_member_id ) );
    }

    return result;
}

//-------------------------------------------------------Distributed_scheduler::exclusive_scheduler

Scheduler_member* Distributed_scheduler::exclusive_scheduler()
{
    return _exclusive_scheduler && !_exclusive_scheduler->is_empty_member()?  _exclusive_scheduler
                                                                           : NULL;
}

//---------------------------------------------------------Distributed_scheduler::mark_as_exclusive

bool Distributed_scheduler::mark_as_exclusive()
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( !_has_exclusiveness );
    assert( _exclusive_scheduler );

    bool   ok;
    time_t now = ::time(NULL);


    calculate_next_heart_beat( now );

    time_t new_db_last_heart_beat = now;
    time_t new_db_next_heart_beat = _next_heart_beat;


    _log->info( message_string( "SCHEDULER-835" ) );  // "This Scheduler becomes exclusive"

    try
    {
        Transaction ta ( db() );

        lock_member_records( &ta, _exclusive_scheduler->_member_id, my_member_id() );


        // Dem bisher exklusiven Scheduler die Exklusivität nehmen

        sql::Update_stmt update ( ta.database_descriptor(), _spooler->_members_tablename );
    
        update[ "exclusive"              ] = sql::null_value;
        update[ "deactivating_member_id" ] = my_member_id();
        if( _exclusive_scheduler->is_empty_member() )  update[ "active" ] = 1;
        update.and_where_condition( "scheduler_member_id", _exclusive_scheduler->_member_id );
        update.add_where( " and `exclusive` is not null" );
        
        if( !_exclusive_scheduler->is_empty_member() )
        {
            update.and_where_condition( "last_heart_beat"    , _exclusive_scheduler->_last_heart_beat_db );
            update.and_where_condition( "next_heart_beat"    , _exclusive_scheduler->_next_heart_beat_db );
        }

        ok = ta.try_execute_single( update, __FUNCTION__ );

        if( ok )
        {
            sql::Update_stmt update ( ta.database_descriptor(), _spooler->_members_tablename );
        
            update[ "active"          ] = 1;
            update[ "exclusive"       ] = 1;
            update[ "last_heart_beat" ] = new_db_last_heart_beat;
            update[ "next_heart_beat" ] = new_db_next_heart_beat;
            update.and_where_condition( "scheduler_member_id", my_member_id() );
            update.and_where_condition( "exclusive"          , sql::null_value );
            update.add_where( S() << " and `active`" << ( _is_active? " is not null" : " is null" ) );

            ok = ta.try_execute_single( update, __FUNCTION__ );
        }

        if( ok )  ta.commit( __FUNCTION__ );
    }
    catch( exception& x )   // Bei optimistischer Sperrung kann es eine Exception geben
    { 
        ok = false;
        _log->debug3( S() << x.what() << ", in " << __FUNCTION__ );
    }

    if( ok )
    {
        _db_last_heart_beat = new_db_last_heart_beat;
        _db_next_heart_beat = new_db_next_heart_beat;

        _has_exclusiveness = true;
        _is_active         = true;

        if( _my_scheduler        ) _my_scheduler       ->_is_exclusive = true ,  _my_scheduler->_is_active = true;
        if( _exclusive_scheduler ) _exclusive_scheduler->_is_exclusive = false;

        _exclusive_scheduler = _my_scheduler;

        //_set_exclusive_until = ::time(NULL);
        //if( db_mode == use_commit_visible_time )  _set_exclusive_until += database_commit_visible_time + 1;     // Nachfolgende Sekunde
    }

    if( ok )  assert_database_integrity( __FUNCTION__ );
    if( ok )  ok = do_a_heart_beat();
    if( ok )  _log->info( message_string( "SCHEDULER-806" ) );

    return ok;
}

//-------------------------------------------------Distributed_scheduler::assert_database_integrity

void Distributed_scheduler::assert_database_integrity( const string& message_text )
{
    bool ok              = false;
    int  exclusive_count = 0;
    int  empty_count     = 0;


    S sql_stmt;
    sql_stmt << "select 1, count( `exclusive` )"
                "  from " << _spooler->_members_tablename <<
                "  where `scheduler_id`=" << sql::quoted( _spooler->id_for_db() ) <<
             "   union  "
              "  select 2, count(*)"
                "  from " << _spooler->_members_tablename <<
                "  where `scheduler_member_id`=" << sql::quoted( empty_member_id() );

    
    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        Any_file result = ta.open_result_set( sql_stmt, __FUNCTION__ );    // In _einem_ Snapshot abfragen

        while( !result.eof() )
        {
            Record record = result.get_record();
            switch( record.as_int( 0 ) )
            {
                case 1: exclusive_count = record.as_int( 1 );  break;
                case 2: empty_count     = record.as_int( 1 );  break;
                default: assert(__FUNCTION__==NULL);
            }
        }

        ok = exclusive_count == 1 && empty_count == 1  ||
             exclusive_count == 0 && empty_count == 0;
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ), __FUNCTION__ ); }

    if( !ok )
    {
        _is_in_error = true;
        z::throw_xc( "SCHEDULER-371", S() << "exclusive_count=" << exclusive_count << " empty_count=" << empty_count, 
                                      "TABLE " + _spooler->_members_tablename, 
                                      message_text );
    }

    //return ok;
}

//------------------------------------------------Distributed_scheduler::check_empty_member_record

void Distributed_scheduler::check_empty_member_record()
{
    time_t now        = ::time(NULL);
    Record record;
    bool   second_try = false;


    do
    {
        try
        {
            Transaction ta ( db() );

            Any_file result_set = ta.open_result_set
            (
                S() << "select `active`"
                       "  from " << _spooler->_members_tablename << 
                       "  where `scheduler_member_id`=" << sql::quoted( empty_member_id() ),
                __FUNCTION__
            );

            if( !result_set.eof() )
            {
                if( result_set.get_record().null( "active" ) )
                {
                    sql::Update_stmt update ( db()->database_descriptor(), _spooler->_members_tablename );
                    update[ "active" ] = 1;
                    update.and_where_condition( "scheduler_member_id", empty_member_id() );
                    ta.execute_single( update, __FUNCTION__ );
                }
            }
            else
            if( !_is_backup )
            {
                sql::Insert_stmt stmt ( ta.database_descriptor(), _spooler->_members_tablename );

                stmt[ "scheduler_member_id" ] = empty_member_id();
              //stmt[ "precedence"          ] = 0;
              //stmt[ "last_heart_beat"     ] = Beide Felder NULL lassen, damit sie nicht als veraltete Einträge angesehen und gelöscht werden
              //stmt[ "next_heart_beat"     ] = 
                stmt[ "active"              ] = _is_active? sql::Value( 1 ) : sql::null_value;
                stmt[ "exclusive"           ] = 1;
                stmt[ "scheduler_id"        ] = _spooler->id_for_db();
                stmt[ "version"             ] = _spooler->_version;
                stmt[ "running_since"       ].set_datetime( string_local_from_time_t( now ) );
                stmt[ "host"                ] = _spooler->_complete_hostname;
                stmt[ "pid"                 ] = getpid();
                stmt[ "http_url"            ] = _spooler->http_url();

                ta.execute( stmt, "The empty member record" );
            }

            ta.commit( __FUNCTION__ );
        }
        catch( exception& )
        {
            if( second_try )  throw;
            second_try = true;
        }
    }
    while( second_try );
}

//------------------------------------------------------Distributed_scheduler::insert_member_record

void Distributed_scheduler::insert_member_record()
{
    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        time_t now = ::time(NULL);

        calculate_next_heart_beat( now );

        time_t new_db_last_heart_beat = now;
        time_t new_db_next_heart_beat = _next_heart_beat;


        sql::Insert_stmt insert ( ta.database_descriptor(), _spooler->_members_tablename );
        
        insert[ "scheduler_member_id" ] = my_member_id();
        insert[ "precedence"          ] = _backup_precedence;
        insert[ "last_heart_beat"     ] = new_db_last_heart_beat;
        insert[ "next_heart_beat"     ] = new_db_next_heart_beat;
        insert[ "active"              ] = _is_active? sql::Value( 1 ) : sql::null_value;
        insert[ "exclusive"           ] = sql::null_value;
        insert[ "scheduler_id"        ] = _spooler->id_for_db();
        insert[ "version"             ] = _spooler->_version;
        insert[ "running_since"       ].set_datetime( string_local_from_time_t( new_db_last_heart_beat ) );
        insert[ "host"                ] = _spooler->_complete_hostname;
        
        if( _spooler->tcp_port() )
        insert[ "tcp_port"            ] = _spooler->tcp_port();

        if( _spooler->udp_port() )
        insert[ "udp_port"            ] = _spooler->udp_port();

        insert[ "pid"                 ] = getpid();
        insert[ "http_url"            ] = _spooler->http_url();

        ta.execute( insert, __FUNCTION__ );

        ta.commit( __FUNCTION__ );


        _db_last_heart_beat = new_db_last_heart_beat;
        _db_next_heart_beat = new_db_next_heart_beat;
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ), __FUNCTION__ ); }
}

//------------------------------------------------------------Distributed_scheduler::mark_as_active

//void Distributed_scheduler::mark_as_active()
//{
//    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
//    assert( !_is_active );
//
//    _log->debug( message_string( "SCHEDULER-819" ) );   // "Scheduler becomes active"
//
//    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
//    {
//        Transaction ta ( db() );
//
//        sql::Update_stmt update ( ta.database_descriptor(), _spooler->_members_tablename );
//    
//        update[ "active"          ] = 1;
//        update.and_where_condition( "scheduler_member_id", _distributed_scheduler->my_member_id() );
//        update.and_where_condition( "active"             , sql::null_value );
//        update.and_where_condition( "exclusive"          , sql::null_value );
//        update.and_where_condition( "last_heart_beat"    , _db_last_heart_beat );
//        update.and_where_condition( "next_heart_beat"    , _db_next_heart_beat );
//
//        ta.execute_single( update, __FUNCTION__ );
//        ta.commit( __FUNCTION__ );
//    }
//    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ) ); }
//
//    _is_active = true;
//}

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
        update.add_where( " and `scheduler_member_id`<>" + sql::quoted( my_member_id() ) );
        if( where != "" )  update.add_where( " and " + where );

        ta.execute( update, __FUNCTION__ );
        ta.commit( __FUNCTION__ );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ), __FUNCTION__ ); }
}

//----------------------------------------------------Distributed_scheduler::show_active_schedulers

void Distributed_scheduler::show_active_schedulers( Transaction* outer_transaction, bool exclusive_only )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    if( db()  &&  db()->opened() )
    for( Retry_transaction ta ( db(), outer_transaction ); ta.enter_loop(); ta++ )
    try
    {
        bool found      = false;
        S    select_sql;

        select_sql << "select `scheduler_member_id`, `last_heart_beat`, `http_url`, `host`, `pid`, `running_since`, `exclusive`"
                       "  from " << _spooler->_members_tablename <<
                      "  where `scheduler_id`=" + sql::quoted( _spooler->id_for_db() ) <<
                      " and " << ( exclusive_only? "`exclusive`" : "`active`" ) << " is not null";


        Any_file select = ta.open_result_set( select_sql, __FUNCTION__ );

        while( !select.eof() )
        {
            Record record = select.get_record();

            string active_scheduler_id = record.as_string( 0 );
            string running_since       = record.as_string( 5 );

            if( active_scheduler_id == empty_member_id() )
            {
                _log->info( message_string( "SCHEDULER-809", running_since ) );
            }
            else
            {
                found = true;

                time_t last_heart_beat = record.as_int64 ( 1 );
                string http_url        = record.as_string( 2 );
              //string hostname        = record.as_string( 3 );
                string pid             = record.as_string( 4 );
                bool   is_exclusive    = !record.null( 6 );

                _log->info( message_string( is_exclusive? "SCHEDULER-822" : "SCHEDULER-821", active_scheduler_id, ::time(NULL) - last_heart_beat, http_url, pid ) );
            }
        }

        if( !found )  _log->info( message_string( "SCHEDULER-805" ) );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_members_tablename, x ), __FUNCTION__ ); }
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

//----------------------------------------------------------Distributed_scheduler::set_my_member_id

void Distributed_scheduler::set_my_member_id( const string& member_id )
{
    assert( !_heart_beat  &&  !_exclusive_scheduler_watchdog );

    _scheduler_member_id = member_id;
    check_member_id();

    _log->set_prefix( obj_name() );
}

//-------------------------------------------------------Distributed_scheduler::exclusive_member_id

string Distributed_scheduler::exclusive_member_id()
{
    if( _has_exclusiveness    )  return my_member_id();
    if( exclusive_scheduler() )  return exclusive_scheduler()->_member_id;

    return "";
}

//-----------------------------------------------------------Distributed_scheduler::check_is_active

bool Distributed_scheduler::check_is_active( Transaction* outer_transaction )
{
    if( _is_active )  
    {
        if( Not_in_recursion not_in_recursion = &_is_in_do_a_heart_beat )   
        {
            bool is_in_time = check_heart_beat_is_in_time( _next_heart_beat );
            if( !is_in_time )      // Nicht in laufender Transaction
            {
                if( db()->is_in_transaction() )
                {
                    check_my_member_record( outer_transaction );
                }
                else
                {
                    _log->warn( message_string( "SCHEDULER-997" ) );
                    do_a_heart_beat();
                }
            }
        }
        else
        {
            Z_LOG2( "scheduler", __FUNCTION__ << "  Rekursiver Aufruf von do_a_heart_beat()\n" );  // Passiert in heartbeat_member_record() bei commit()
        }
    }

    return _is_active;
}

//-----------------------------------------------------------Distributed_scheduler::is_scheduler_up

bool Distributed_scheduler::is_scheduler_up()
{
    return empty_scheduler_record() != NULL; 
       //&&  empty_scheduler_record()->_is_active;        // Siehe mark_begin_of_shutdown()
}

//-----------------------------------------------------------Distributed_scheduler::check_member_id

void Distributed_scheduler::check_member_id()
{
    string prefix = _spooler->id_for_db() + "/";
    if( !string_begins_with( my_member_id(), prefix )  ||  my_member_id() == prefix )  z::throw_xc( "SCHEDULER-358", my_member_id(), prefix );
}

//--------------------------------------------------Distributed_scheduler::make_scheduler_member_id

void Distributed_scheduler::make_scheduler_member_id()
{
    set_my_member_id( S() << _spooler->id_for_db() 
                          << "/" << _spooler->_complete_hostname << ":" << _spooler->tcp_port() 
                          << "/" << getpid() 
                          << "." << ( as_string( 1000000 + (uint64)( double_from_gmtime() * 1000000 ) % 1000000 ).substr( 1 ) ) );  // Mikrosekunden, sechsstellig
}

//----------------------------------------------------Distributed_scheduler::empty_scheduler_record

Scheduler_member* Distributed_scheduler::empty_scheduler_record()
{
    return scheduler_member_or_null( empty_member_id() );
}

//-----------------------------------------------------Distributed_scheduler::http_url_of_member_id

string Distributed_scheduler::http_url_of_member_id( const string& scheduler_member_id )
{
    string result;

    if( Scheduler_member* scheduler_member = scheduler_member_or_null( scheduler_member_id ) )
    {
        result = scheduler_member->_http_url;
    }

    return result;
}

//--------------------------------------------------Distributed_scheduler::scheduler_member_or_null

Scheduler_member* Distributed_scheduler::scheduler_member_or_null( const string& scheduler_member_id )
{
    Scheduler_map::iterator it = _scheduler_map.find( scheduler_member_id );
    return it != _scheduler_map.end()? it->second : NULL;
}

//------------------------------------------------------------------Distributed_scheduler::obj_name

string Distributed_scheduler::obj_name() const
{ 
    return "Distributed_scheduler";   // + _scheduler_member_id;
} 

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
