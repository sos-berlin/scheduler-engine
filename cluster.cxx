// $Id$

#include "spooler.h"
#include "../kram/msec.h"
//#include "../zschimmer/not_in_recursion.h"
#include "../zschimmer/threads.h"

#ifdef Z_WINDOWS
#   include <process.h>
#endif

using namespace zschimmer;

namespace sos {
namespace scheduler {
namespace cluster {

//-------------------------------------------------------------------------------------------------

//const time_t                    accepted_clock_difference       = Z_NDEBUG_DEBUG(  5,  2 );     // Die Uhren sollten noch besser übereinstimmen! ntp verwenden!
//const time_t                    warned_clock_difference         = Z_NDEBUG_DEBUG(  1,  1 ); 
const int                       heart_beat_period                       = Z_NDEBUG_DEBUG( 60, 20 );
const int                       max_processing_time                     = Z_NDEBUG_DEBUG(  5,  3 );     // Zeit, die gebraucht wird, um den Herzschlag auszuführen
const int                       max_processing_time_2                   = Z_NDEBUG_DEBUG( 30, 10 );     // Nachfrist, inkl. außergewöhnlicher Verzögerungen (gestrichen: zzgl. Database::lock_timeout)
const int                       max_processing_time_2_short             = max_processing_time_2 - 10;   // Zur eigenen Prüfung
const int                       active_heart_beat_minimum_check_period  = heart_beat_period / 2;
const int                       active_heart_beat_check_period          = heart_beat_period + max_processing_time + 2;
//const int                       trauerfrist                             = 12*3600;                      // Trauerzeit, nach der Mitgliedssätze gelöscht werden
const int                       database_commit_visible_time            = 10;                           // Zeit, die die Datenbank braucht, um nach Commit Daten für anderen Client sichtbar zu machen.
const int                       precedence_check_period                 = active_heart_beat_check_period + 5;
const int                       backup_startup_delay                    = 60;                           // Nach eigenem Start des Backup-Schedulers auf Start des non-backup-Schedulers warten
const int                       first_dead_orders_check_period          = 60;

const int                       Cluster::default_non_backup_precedence  = 0;
const int                       Cluster::default_backup_precedence      = 1;
const string                    Cluster::continue_exclusive_non_backup  = "";               // Das ist der Default
const string                    Cluster::continue_exclusive_this        = "this";
const string                    Cluster::continue_exclusive_any         = "any";            // In der Datenbank, Feld http_url, wird das zu ""

//-------------------------------------------------------------------------------------------------

struct Heart_beat_watchdog_thread : Thread, Scheduler_object
{
                                Heart_beat_watchdog_thread  ( Cluster* );

    int                         thread_main                 ();
    void                        sleep                       ( int seconds );
    void                        kill                        ();

    Fill_zero                  _zero_;
    bool                       _kill;
    Cluster*                   _cluster;
};

//-----------------------------------------------------------------------------------Cluster_member

struct Cluster_member : Object, Scheduler_object
{
    enum Mark_as_inactive_option
    {
      //mai_delete_empty_record,
        mai_delete_my_inactive_record,
      //mai_delete_inactive_and_empty_records,
        mai_mark_inactive_record_as_dead
    };


                                Cluster_member              ( Cluster*, const string& member_id );

    bool                        is_empty_member             () const                                { return _member_id == _cluster->empty_member_id(); }
    bool                        its_me                      () const                                { return _member_id == _cluster->my_member_id(); }
    bool                        check_heart_beat            ( time_t now_before_select, const Record& );
    bool                        free_occupied_orders        ( Transaction* = NULL );
    void                        deactivate_and_release_orders_after_death();
    bool                        mark_as_inactive            ( Mark_as_inactive_option );
    void                        on_resurrection             ()                                      { _is_dead = false;  _dead_orders_check_period = first_dead_orders_check_period; }
    bool                        delete_dead_record          ();
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    string                      obj_name                    () const;


    Fill_zero                  _zero_;
    string                     _member_id;
    bool                       _is_dead;
    bool                       _is_db_dead;
    time_t                     _dead_orders_check_period;
    time_t                     _next_dead_orders_check_time;
    bool                       _is_exclusive;
    bool                       _is_active;
    bool                       _is_checked;
    string                     _deactivating_member_id;
    time_t                     _db_last_heart_beat;
    time_t                     _db_next_heart_beat;
    time_t                     _last_heart_beat_detected;
    Time                       _last_heart_beat_detected_local_time;
    bool                       _is_heart_beat_late;
    int                        _heart_beat_count;
    string                     _http_url;
    Cluster*                   _cluster;
    //time_t                     _clock_difference;
    //bool                       _clock_difference_checked;
    //bool                       _checking_clock_difference;
};

//----------------------------------------------------------------------------------------Has_alarm

struct Has_alarm
{
                                Has_alarm                   ()                                      : _recommended_next_check_time(time_max) {}

    virtual void                set_alarm                   ()                                      = 0;
    virtual void                recommend_next_check_time   ( time_t );
    void                        on_alarm                    ();


    time_t                     _recommended_next_check_time;
};

//---------------------------------------------------------------------------------------Heart_beat

struct Heart_beat : Async_operation, Scheduler_object, Has_alarm
{
                                Heart_beat                  ( Cluster* );


    // Async_operation
    bool                        async_finished_             () const                                { return false; }
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );
    string                      obj_name                    () const                                { return Scheduler_object::obj_name(); }

    void                        set_alarm                   ();

  private:
    Fill_zero                  _zero_;
    Cluster*                   _cluster;
};

//----------------------------------------------------------------------Exclusive_scheduler_watchdog

struct Exclusive_scheduler_watchdog : Async_operation, Scheduler_object, Has_alarm
{
                                Exclusive_scheduler_watchdog( Cluster* );


    // Async_operation
    bool                        async_finished_             () const                                { return false; }
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );


    void                        calculate_next_check_time   ();
    void                        set_alarm                   ();

    void                        try_to_become_exclusive     ();
  //void                        check_clock_difference      ( time_t last_heart_beat, time_t now );
    bool                        check_has_backup_precedence ();
  //string                      read_cluster_session_id     ();
  //bool                        restart_when_active_scheduler_has_started();

  private:
    Fill_zero                  _zero_;
    time_t                     _next_check_time;
    bool                       _announced_to_become_exclusive; // Wenn eine Meldung ausgegeben oder ein Datensatz zu ändern versucht wurde 
    time_t                     _next_precedence_check;                      // Um herauszufinden, welcher inaktive Scheduler exklusiv wird
    time_t                     _wait_for_backup_scheduler_start_until;      // Der non-backup-Scheduler hat Vorrang, auch wenn etwas später kommt
  //string                     _cluster_session_id;
    bool                       _is_starting;
    Cluster*                   _cluster;
  //time_t                     _set_exclusive_until;           // Nur für Access (kennt keine Sperre): Aktivierung bis dann (now+database_commit_visible_time) verzögern, dann nochmal prüfen
};

//------------------------------------------------------------------------Active_schedulers_watchdog

struct Active_schedulers_watchdog : Async_operation, Scheduler_object, Has_alarm
{
                                Active_schedulers_watchdog   ( Cluster* );


    // Async_operation
    bool                        async_finished_             () const                                { return false; }
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );

    void                        set_alarm                   ();
    void                        calculate_next_check_time   ();

  private:
    Fill_zero                  _zero_;
    time_t                     _next_check_time;
    Cluster*                   _cluster;
};

//----------------------------------------------------------------------------my_string_from_time_t

static string my_string_from_time_t( time_t time )
{
    return string_gmt_from_time_t( time ) + " UTC";
}

//-------------------------------------------Heart_beat_watchdog_thread::Heart_beat_watchdog_thread

Heart_beat_watchdog_thread::Heart_beat_watchdog_thread( Cluster* d )
:
    Scheduler_object( d->_spooler, this, type_heart_beat_watchdog_thread ),
    _zero_(this+1),
    _cluster(d)
{
}

//----------------------------------------------------------Heart_beat_watchdog_thread::thread_main
    
void Heart_beat_watchdog_thread::sleep( int seconds )
{
    time_t now   = ::time(NULL);
    time_t until = now + seconds;

    while( !_kill  &&  now < until )
    {
        int sleep_time = (int)( until - now );

#       ifdef Z_WINDOWS

            int ms = sleep_time * 1000;
            Z_LOG2( "scheduler.cluster", __FUNCTION__ << "  Sleep(" << ms << ")\n" );
            Sleep( ms );

#        else

            //Z_LOG2( "scheduler.cluster", __FUNCTION__ << "  sleep(" << seconds << ")\n" );
            while( !_kill  &&  sleep_time-- > 0 )
            {
                ::sleep( 1 );     // Nur eine Sekunde, um _kill abfragen zu können
            }

#       endif

        now = ::time(NULL);
    }
}

//-----------------------------------------------------------------Heart_beat_watchdog_thread::kill

void Heart_beat_watchdog_thread::kill()
{
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );


    _kill = true;

#   ifdef Z_WINDOWS
        TerminateThread( thread_handle(), 1 );
#    else
        //Funktioniert nicht: pthread_kill( thread_handle(), SIGTERM );
#   endif
}

//----------------------------------------------------------Heart_beat_watchdog_thread::thread_main
    
int Heart_beat_watchdog_thread::thread_main()
{
#   ifdef Z_WINDOWS
        SetThreadPriority( thread_handle(), THREAD_PRIORITY_HIGHEST );
        Z_LOG2( "scheduler", __FUNCTION__ << "  IsDebuggerPresent() ==> " << IsDebuggerPresent() << "\n" );
#   else
        // Wie erhöht man die Priorität unter Unix?
#   endif
    

    while( !_kill )
    {
        time_t heart_beat_time = _cluster->_db_last_heart_beat;       // Gleichzeitiger Zugriff von einem anderen Thread!
        
        int delay = (int)( heart_beat_time + 1 + heart_beat_period + max_processing_time + max_processing_time_2_short - ::time(NULL) );
        sleep( delay + 1 );
        
        if( _cluster->_is_active  &&  _cluster->_db_last_heart_beat == heart_beat_time )  
        {
            //Z_WINDOWS_ONLY( Z_DEBUG_ONLY( DebugBreak() ) );
            Z_WINDOWS_ONLY( if( !IsDebuggerPresent() ) )
            {
                _cluster->_spooler->kill_all_processes();
                _log->error( message_string( "SCHEDULER-386", my_string_from_time_t( heart_beat_time ), ::time(NULL) - heart_beat_time ) );
                _cluster->_spooler->abort_immediately( true );
            }
        }

        //Z_DEBUG_ONLY( _log->info( __FUNCTION__ ) );
    }

    return 0;
}

//--------------------------------------------------------------------Cluster_member::Cluster_member

Cluster_member::Cluster_member( Cluster* d, const string& id ) 
: 
    Scheduler_object( d->_spooler, this, Scheduler_object::type_cluster_member ),
    _zero_(this+1), 
    _cluster(d), 
    _member_id(id),
    _dead_orders_check_period( first_dead_orders_check_period )
{
    _log->set_prefix( obj_name() );
}

//-----------------------------------------------------------------Cluster_member::check_heart_beat

bool Cluster_member::check_heart_beat( time_t now_before_select, const Record& record )
{
    bool result = true;

    _is_exclusive           = !record.null    ( "exclusive" );
    _is_active              = !record.null    ( "active"    );
    _deactivating_member_id = record.as_string( "deactivating_member_id" );
    _http_url               = record.as_string( "http_url"  );
    _is_db_dead             = !record.null    ( "dead"      );
    
    time_t last_heart_beat = record.null( "last_heart_beat" )? 0 : record.as_int64( "last_heart_beat" );
    time_t next_heart_beat = record.null( "next_heart_beat" )? 0 : record.as_int64( "next_heart_beat" );


    if( !is_empty_member() )
    {
        if( _db_last_heart_beat == 0  &&  _cluster->_my_scheduler != this )     // Neu entdeckt?
        {
            log()->info( message_string( _cluster->_demand_exclusiveness && _is_exclusive? "SCHEDULER-833" : "SCHEDULER-820" ) );
        }

        if( last_heart_beat != _db_last_heart_beat )   // Neuer Herzschlag oder neu entdeckter Scheduler?
        {
            time_t now = ::time(NULL);

            if( _is_dead  &&  !_is_db_dead )
            {
                log()->warn( message_string( "SCHEDULER-823", _member_id, now - _last_heart_beat_detected ) );     // Wiederauferstanden?
                on_resurrection();
            }

            if( _last_heart_beat_detected )  _heart_beat_count++;   // Den ersten Durchlauf zählen wir nicht, denn dann haben wir noch keinen Herzschlag
            _last_heart_beat_detected = now + 1;                    // Nächste volle Sekunde
            _last_heart_beat_detected_local_time = Time::now();  //.set_datetime( string_local_from_time_t( _last_heart_beat_detected ) );

            _is_heart_beat_late = false;

            if( _heart_beat_count == 1  &&  this != _cluster->_my_scheduler )  _log->info( message_string( "SCHEDULER-838", _heart_beat_count ) );
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
        if( _last_heart_beat_detected  &&  _last_heart_beat_detected + heart_beat_period + max_processing_time < now_before_select )
        {
            time_t standard_deadline                  = _last_heart_beat_detected + heart_beat_period + max_processing_time + max_processing_time_2; //+ Database::lock_timeout;
            time_t database_reconnect_deadline        = db()->reopen_time() + Database::seconds_before_reopen + max_processing_time;
            bool   is_in_database_reconnect_tolerance = database_reconnect_deadline >= now_before_select;
            bool   was_alive                          = !_is_dead;

            time_t deadline = is_in_database_reconnect_tolerance? database_reconnect_deadline
                                                                : standard_deadline;
            bool   is_dead  = !is_in_database_reconnect_tolerance  &&  deadline < now_before_select;

            if( was_alive )
            {
                string message_code;

                if( is_in_database_reconnect_tolerance )  message_code = "SCHEDULER-995";
                else
                if( !is_dead                           )  message_code = "SCHEDULER-994", _is_heart_beat_late = true; 
                else 
                                                          message_code = "SCHEDULER-996";

                if( !string_begins_with( _log->last_line(), message_code ) )
                {
                    Message_string m ( message_code, my_string_from_time_t( _last_heart_beat_detected ), now_before_select - _last_heart_beat_detected );
                    if( !is_dead )  m.insert( 3, deadline - ::time(NULL) );
                    log()->warn( m );
                }

                time_t now = ::time(NULL);
                if( deadline > now + 2 )  _cluster->recommend_next_deadline_check_time( deadline + 1 );
            }

            if( is_dead ) 
            {
                if( _is_active )  _is_dead = true;      // Inaktive Scheduler deaktivieren wir nicht und erklären sie auch nicht für tot
                result = false;
            }
        }
    }

    _is_dead |= _is_db_dead;

    _db_last_heart_beat = last_heart_beat;
    _db_next_heart_beat = next_heart_beat;

    return result;
}

//------------------------------------------------------------Cluster_member::free_occupied_orders

bool Cluster_member::free_occupied_orders( Transaction* outer_transaction )
{
    //Z_LOGI2( "scheduler.cluster", __FUNCTION__ << "\n" );
    bool result = false;


    try
    {
        Transaction ta ( db(), outer_transaction );
        int         record_count;
        string      fake_member_id = _member_id + "?";


        // Vielleicht beschränken auf Jobketten und Zustände des eigenen Schedulers. Erstmal nicht.

        {
            sql::Update_stmt update ( db()->database_descriptor(), _spooler->_orders_tablename );
            update[ "occupying_cluster_member_id" ] = fake_member_id;
            update.and_where_condition( "occupying_cluster_member_id", _member_id );
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
                    "  where `occupying_cluster_member_id`=" << sql::quoted( fake_member_id ) <<
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
                    job_chain->tip_for_new_distributed_order( state, Time(0) );
            }

            sql::Update_stmt update ( db()->database_descriptor(), _spooler->_orders_tablename );
            update[ "occupying_cluster_member_id" ] = sql::null_value;
            update.and_where_condition( "occupying_cluster_member_id", fake_member_id );
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

//---------------------------------------Cluster_member::deactivate_and_release_orders_after_death

void Cluster_member::deactivate_and_release_orders_after_death()
{
    Z_LOGI2( "scheduler.cluster", __FUNCTION__ << "  _next_dead_orders_check_time=" << my_string_from_time_t( _next_dead_orders_check_time ) << "\n" );

    assert( !its_me() );


    time_t now = ::time(NULL);

    if( _next_dead_orders_check_time <= now )
    {
        if( !_is_db_dead  &&  !_cluster->_is_backup )   // Nur der primäre Scheduler darf dead=1 setzen und diese Sätze später (oder sofort) löschen
        {
            _log->warn( message_string( "SCHEDULER-836" ) );        // "Deactivating dead Scheduler"
            mark_as_inactive( mai_mark_inactive_record_as_dead );   // ruft free_occupied_orders()
        }
        else    
            free_occupied_orders();

        _dead_orders_check_period *= 2;
        _next_dead_orders_check_time = now + _dead_orders_check_period;
    }
}

//------------------------------------------------------------------------Cluster::mark_as_inactive

bool Cluster_member::mark_as_inactive( Mark_as_inactive_option option )
{
    bool delete_empty_member_record = its_me()  &&  _cluster->_continue_exclusive_operation == Cluster::continue_exclusive_non_backup;

    assert( !delete_empty_member_record  ||  option == mai_delete_my_inactive_record );
    assert( ( option == mai_delete_my_inactive_record    ) == its_me() );
    assert( ( option == mai_mark_inactive_record_as_dead ) != its_me() );

    bool ok = false;


    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        _cluster->lock_member_records( &ta, _member_id, _cluster->empty_member_id() );

        free_occupied_orders( &ta );


        sql::Update_stmt update ( ta.database_descriptor(), _spooler->_clusters_tablename );

        update[ "active"    ] = sql::null_value;
        update[ "exclusive" ] = sql::null_value;

        if( _member_id != _cluster->my_member_id() )  update[ "deactivating_member_id" ] = _cluster->my_member_id();
        update.and_where_condition( "member_id", _member_id );
      //update.and_where_condition( "active"   , _is_active   ? sql::Value( 1 ) : sql::null_value );
        update.and_where_condition( "exclusive", _is_exclusive? sql::Value( 1 ) : sql::null_value );

        if( option == mai_delete_my_inactive_record )
        {
            ok = ta.try_execute_single( update.make_delete_stmt(), __FUNCTION__ );
        }
        else
        if( option == mai_mark_inactive_record_as_dead )  
        {
            update[ "dead" ] = 1;
            ok = ta.try_execute_single( update.make_update_stmt(), __FUNCTION__ );
        }
        else
            assert(false);


        if( ok  &&  _is_exclusive )
        {
            Z_DEBUG_ONLY( assert( _is_active ) );

            sql::Update_stmt update ( ta.database_descriptor(), _spooler->_clusters_tablename );
        
            update.and_where_condition( "member_id", _cluster->empty_member_id() );

            if( delete_empty_member_record )
            {
                ta.try_execute_single( update.make_delete_stmt(), __FUNCTION__ );   
                ok = true;  // ok, wenn der Satz schon gelöscht ist
            }
            else
            {
                assert( _is_exclusive );
                update[ "exclusive" ] = 1;

                if( its_me() )
                {
                    string http_url = _cluster->_continue_exclusive_operation;
                    if( http_url == Cluster::continue_exclusive_any )  http_url = "";
                    update[ "http_url" ] = http_url;
                }

                string update_sql = update.make_update_stmt();
                ok = ta.try_execute_single( update_sql, __FUNCTION__ );
                if( !ok )
                {
                    _log->error( message_string( "SCHEDULER-371", "Statement without effect: " + update_sql ) );
                    _cluster->_is_in_error = true;
                    _cluster->show_exclusive_scheduler( &ta );
                }
            }
        }

        if( ok )  ta.commit( __FUNCTION__ );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_clusters_tablename, x ), __FUNCTION__ ); }


    if( this == _cluster->_my_scheduler )
    {
        _is_active    = false;
        _is_exclusive = false;
    }

    if( _member_id == _cluster->my_member_id() )
    {
        _cluster->_is_active         = false;
        _cluster->_has_exclusiveness = false;
    }

    return ok;
}

//---------------------------------------------------------Distributed::shift_exclusiveness_from_to

//bool Distributed::shift_exclusiveness_from_to( Cluster_member* from, Cluster_member* to, bool delete_from_record, bool delete_empty_member_record )
//{
//    Z_LOGI2( "scheduler.cluster", __FUNCTION__ << "\n" );
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
//            sql::Update_stmt update ( ta.database_descriptor(), _spooler->_clusters_tablename );
//        
//            update[ "exclusive" ] = sql::null_value;
//            if( from != _my_scheduler   )  update[ "deactivating_member_id" ] = my_member_id();
//            if( from->is_empty_member() )  update[ "active" ] = 1;
//            update.and_where_condition( "member_id", from->_member_id );
//            update.add_where( " and `exclusive` is not null" );
//            
//            if( !from->is_empty_member() )
//            {
//                update.and_where_condition( "last_heart_beat", from->_db_last_heart_beat );
//                update.and_where_condition( "next_heart_beat", from->_db_next_heart_beat );
//            }
//
//            ok = ta.try_execute_single( update, __FUNCTION__ );
//        }
//
//
//        if( ok )
//        {
//            sql::Update_stmt update ( ta.database_descriptor(), _spooler->_clusters_tablename );
//        
//            update[ "active"          ] = 1;
//            update[ "exclusive"       ] = 1;
//            update[ "last_heart_beat" ] = new_db_last_heart_beat;
//            update[ "next_heart_beat" ] = new_db_next_heart_beat;
//            update.and_where_condition( "member_id", my_member_id() );
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

//--------------------------------------------------------------Cluster_member::delete_dead_record

bool Cluster_member::delete_dead_record()
{
    bool result = false;

    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ )  try
    {
        sql::Delete_stmt delete_stmt ( ta.database_descriptor(), _spooler->_clusters_tablename );
     
        delete_stmt.and_where_condition( "scheduler_id", _spooler->id_for_db() );
        delete_stmt.and_where_condition( "member_id"   , _member_id );
        delete_stmt.add_where( " and `dead` is not null" );

        ta.execute( delete_stmt, __FUNCTION__ );
        int count = ta.record_count();

        ta.commit( __FUNCTION__ );

        result = count > 0;
        if( result )  _log->info( message_string( "SCHEDULER-809" ) );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_clusters_tablename, x ), __FUNCTION__ ); }

    return result;
}

//----------------------------------------------------------------------Cluster_member::dom_element

xml::Element_ptr Cluster_member::dom_element( const xml::Document_ptr& dom_document, const Show_what& )
{
    Read_transaction  transaction  ( db() );
    string            where        = S() << "where `member_id`=" << sql::quoted( _member_id );
    string            xml          = transaction.read_clob_or_empty( _spooler->_clusters_tablename, "xml", where );
    xml::Element_ptr  result;

    if( xml != "" )
    {
        xml::Document_ptr dom_document = xml;
        if( dom_document.documentElement() )  result = dom_document.documentElement().cloneNode( true );
    }

    if( !result )  result = dom_document.createElement( "cluster_member" );


    result.setAttribute( "cluster_member_id", _member_id );
    result.setAttribute( "http_url"         , _http_url  );
    result.setAttribute( "heart_beat_count" , _heart_beat_count );
    if( _is_active    )  result.setAttribute( "active"   , "yes" );
    if( _is_exclusive )  result.setAttribute( "exclusive", "yes" );
    if( _is_dead      )  result.setAttribute( "dead"     , "yes" );

    if( _heart_beat_count )  
    {
        result.setAttribute( "last_detected_heart_beat"    , _last_heart_beat_detected_local_time.as_string( Time::without_ms ) );
        result.setAttribute( "last_detected_heart_beat_age", max( (time_t)0, ::time(NULL) - _last_heart_beat_detected ) );
        result.setAttribute( "heart_beat_quality"          , /*_is_dead? "bad" :*/ _is_heart_beat_late? "late" : "good" );
    }
    else
    {
        result.setAttribute( "database_last_heart_beat", my_string_from_time_t( _db_last_heart_beat ) );
    }

    result.setAttribute_optional( "deactivating_member_id", _deactivating_member_id );

    return result;
}

//-------------------------------------------------------------------------Cluster_member::obj_name

string Cluster_member::obj_name() const
{ 
    S result;
    result << Scheduler_object::obj_name();
    if( is_empty_member() )  result << " (empty record)";
                       else  result << " " << _member_id;

    return result;
}

//-------------------------------------------------------------Has_alarm::recommend_next_check_time

void Has_alarm::recommend_next_check_time( time_t t )
{ 
    if( _recommended_next_check_time > t ) 
    {
        _recommended_next_check_time = t; 
        set_alarm();
    }
}

//------------------------------------------------------------------------------Has_alarm::on_alarm

void Has_alarm::on_alarm()
{
    _recommended_next_check_time = time_max;
}

//---------------------------------------------------------------------------Heart_beat::Heart_beat

Heart_beat::Heart_beat( Cluster* m ) 
:
    Scheduler_object( m->_spooler, this, Scheduler_object::type_heart_beat ),
    _zero_(this+1),
    _cluster(m)
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
    Z_LOGI2( "scheduler.cluster", __FUNCTION__ << "\n" );


    Has_alarm::on_alarm();

    if( !db()->opened() )
    {
        _cluster->async_wake();    // Datenbank ist geschlossen worden
        return true;
    }


    try
    {
        bool ok = _cluster->do_a_heart_beat();
        if( ok )
        {
            set_alarm();                            // Wir sind weiterhin aktiv
        }
        else
        {
            _cluster->async_wake();   // Wir sind nicht mehr aktiv, Operation beenden!
        }
    }
    catch( exception& )
    {
        _cluster->async_wake();       // Cluster wird die Exception übernehmen
        throw;
    }
        
    return true;
}

//----------------------------------------------------------------------------Heart_beat::set_alarm

void Heart_beat::set_alarm()
{
    time_t t = _cluster->_next_heart_beat;

    if( t > _recommended_next_check_time )  t = _recommended_next_check_time;

    set_async_next_gmtime( t );
}

//---------------------------------------Exclusive_scheduler_watchdog::Exclusive_scheduler_watchdog

Exclusive_scheduler_watchdog::Exclusive_scheduler_watchdog( Cluster* m ) 
:
    Scheduler_object( m->_spooler, this, Scheduler_object::type_exclusive_scheduler_watchdog ),
    _zero_(this+1),
    _cluster(m)
{
    _log = m->_log;
  //_cluster_session_id = read_cluster_session_id();
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
    Z_LOGI2( "scheduler.cluster", __FUNCTION__ << "\n" );

    Has_alarm::on_alarm();

    if( !db()->opened() )
    {
        _cluster->async_wake();       // Datenbank ist geschlossen worden
        return true;
    }

    try
    {
        //bool ok = restart_when_active_scheduler_has_started(); 
        //if( !ok )  
        try_to_become_exclusive();
    }
    catch( exception& )
    {
        _cluster->async_wake();       // Cluster wird die Exception übernehmen
        throw;
    }

    if( _cluster->has_exclusiveness() )
    {
        _cluster->async_wake();       // Wir sind aktives Mitglied geworden, Exclusive_scheduler_watchdog beenden
    }
    else
    {
        calculate_next_check_time();
        set_alarm();                                // Wir sind weiterhin inaktives Mitglied
    }

    return true;
}

//------------------------------------------Exclusive_scheduler_watchdog::calculate_next_check_time

void Exclusive_scheduler_watchdog::calculate_next_check_time()
{
    time_t now = ::time(NULL);

    _next_check_time = now + active_heart_beat_check_period;

    //if( _set_exclusive_until  &&  _next_check_time > _set_exclusive_until )
    //{
    //    _next_check_time = _set_exclusive_until;
    //    extra_log << ", warten, bis Datenbank Änderungen für alle sichtbar gemacht hat";
    //}
    //else
    
    if( Cluster_member* watched_scheduler = _cluster->exclusive_scheduler() )
    {
        time_t delay          = max_processing_time + 1;   // Erst in der folgenden Sekunde prüfen
        time_t new_next_check = watched_scheduler->_db_next_heart_beat + delay + 1;

        if( new_next_check - now >= active_heart_beat_minimum_check_period  &&  new_next_check < _next_check_time )
        {
            time_t diff = new_next_check - _next_check_time;
            if( abs(diff) > 2 )  Z_LOG2( "scheduler.cluster", __FUNCTION__ << "  Synchronized with _db_next_heart_beat=" << my_string_from_time_t( watched_scheduler->_db_next_heart_beat ) << ": " << diff << "s\n" );
            _next_check_time = new_next_check;
        }
    }
}

//----------------------------------------------------------Exclusive_scheduler_watchdog::set_alarm

void Exclusive_scheduler_watchdog::set_alarm()
{
    //if( _cluster->_db_next_heart_beat < _next_check_time )
    //{
    //    Z_LOG2( "scheduler.cluster", __FUNCTION__ << "  Next heart beat at " << my_string_from_time_t( _cluster->_db_next_heart_beat ) << "\n" );
    //    set_async_next_gmtime( _cluster->_db_next_heart_beat );
    //}
    //else
    {
        //Z_LOG2( "scheduler.cluster", __FUNCTION__ << "  Next check at " << my_string_from_time_t( _next_check_time ) << "\n" );

        time_t t = _next_check_time;

        if( t > _recommended_next_check_time )  t = _recommended_next_check_time;

        set_async_next_gmtime( t );
    }
}

//--------------------------------------------Exclusive_scheduler_watchdog::try_to_become_exclusive

void Exclusive_scheduler_watchdog::try_to_become_exclusive()
{
    Z_LOGI2( "scheduler.cluster", __FUNCTION__ << "\n" );
    assert( !_cluster->_has_exclusiveness );


    _cluster->check_schedulers_heart_beat();  // Stellt fest: is_member_allowed_to_start(), _exclusive_scheduler


    Cluster_member* exclusive_scheduler = _cluster->exclusive_scheduler();

    if( exclusive_scheduler )  _wait_for_backup_scheduler_start_until = 0;
    else
    if( _is_starting  &&  _cluster->_is_backup  &&  _cluster->is_member_allowed_to_start() )
    {
        _wait_for_backup_scheduler_start_until = ::time(NULL) + backup_startup_delay;
        _log->info( message_string( "SCHEDULER-831", backup_startup_delay ) );
    }
    _is_starting = false;


    if( !_wait_for_backup_scheduler_start_until  ||  _wait_for_backup_scheduler_start_until <= ::time(NULL) ) 
    {
        _wait_for_backup_scheduler_start_until = 0;

        if( exclusive_scheduler  &&  !exclusive_scheduler->_is_dead )
        {
            _next_precedence_check = 0;
        }
        else
        if( _cluster->is_member_allowed_to_start()  &&  check_has_backup_precedence() )
        {
            _announced_to_become_exclusive = true;
            _cluster->mark_as_exclusive();
        }
    }


    if( _announced_to_become_exclusive  &&  !_cluster->_has_exclusiveness )  //&&  !_set_exclusive_until )
    {
        _announced_to_become_exclusive = false;
        _cluster->show_exclusive_scheduler( (Transaction*)NULL );
    }
}

//--------------------------------------------Exclusive_scheduler_watchdog::check_has_backup_precedence

bool Exclusive_scheduler_watchdog::check_has_backup_precedence()
{
    bool   result = false;
    time_t now    = ::time(NULL);


    if( !_next_precedence_check  ||  _next_precedence_check <= now )
    {
        try
        {
            Transaction ta ( db() );

            bool     another_has_precedence = false;
            Any_file result_set             = ta.open_result_set
                ( 
                    S() << "select `precedence`, `member_id`, `http_url`  from " << _spooler->_clusters_tablename << 
                       "  where `precedence` < " << _cluster->backup_precedence() <<
                         "  and `active` is null " 
                         "  and `exclusive` is null "
                         "  and `dead` is null " 
                         "  and `next_heart_beat` >= " << ( now - ( heart_beat_period + max_processing_time ) ) <<    // Vorrang funktioniert nur, 
                         "  and `next_heart_beat` <= " << ( now + ( heart_beat_period + max_processing_time ) ),      // wenn die Uhren übereinstimmen
                    __FUNCTION__
                );
            
            while( !result_set.eof() )
            {
                Record record = result_set.get_record();
                _log->info( message_string( "SCHEDULER-814", record.as_string(0), record.as_string(1), record.as_string(2) ) );
                another_has_precedence = true;
            }

            if( another_has_precedence  &&  !_next_precedence_check )
            {
                _next_precedence_check = now + precedence_check_period;
            }
            else
            {
                result = true;
                _next_precedence_check = 0;
            }
        }
        catch( exception& x )  { _log->error( S() << x.what() << ", while checking backup precendence" ); }
    }
    else
    {
        Z_LOG2( "scheduler.cluster", __FUNCTION__ << "  continuing waiting...\n" );
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

//bool Exclusive_scheduler_watchdog::restart_when_active_scheduler_has_started()
//{
//    bool result = false;
//
//    if( _cluster->is_backup_member_allowed_to_start() )
//    {
//        string current_id = read_cluster_session_id();
//
//        if( current_id != ""  &&  current_id != _cluster_session_id )
//        {
//            _cluster_session_id = current_id;
//
//            _log->info( message_string( "SCHEDULER-818" ) );
//            bool restart = true;
//            bool shutdown = false;
//            _spooler->cmd_terminate( restart, INT_MAX, shutdown );
//            result = true;
//        }
//    }
//
//    return result;
//}

//------------------------------Exclusive_scheduler_watchdog::read_cluster_session_id
//
//string Exclusive_scheduler_watchdog::read_cluster_session_id()
//{
//    S result;
//
//    if( _cluster->is_backup_member_allowed_to_start() )
//    {
//        if( db()  &&  db()->opened() )
//        for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ )
//        try
//        {
//            string where  = S() << "where `member_id`=" << sql::quoted( _cluster->empty_member_id() );
//            string xml    = ta.read_clob_or_empty( _spooler->_clusters_tablename, "xml", where );
//            if( xml != "" )
//            //Any_file select = ta.open_result_set( 
//            //                    S() << "select `active` "
//            //                           "  from " << _spooler->_clusters_tablename << "  " <<
//            //                          where ),
//            //                    __FUNCTION__ );
//
//            //if( !select.eof() )
//            {
//                //Record record = select.get_record();
//
//                xml::Document_ptr dom_document ( xml );
//                xml::Element_ptr  element = dom_document.documentElement();
//
//                result << element.getAttribute( "host" ) << "." << 
//                          element.getAttribute( "pid" ) << "." << 
//                          element.getAttribute( "running_since" );
//            }
//        }
//        catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_clusters_tablename, x ), __FUNCTION__ ); }
//    }
//
//    return result;
//}
//
//---------------------------------------Active_schedulers_watchdog::Active_schedulers_watchdog

Active_schedulers_watchdog::Active_schedulers_watchdog( Cluster* m ) 
:
    Scheduler_object( m->_spooler, this, Scheduler_object::type_active_schedulers_watchdog ),
    _zero_(this+1),
    _cluster(m)
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
    Z_LOGI2( "scheduler.cluster", __FUNCTION__ << "\n" );

    Has_alarm::on_alarm();


    if( db()->opened() )
    {
        try
        {
            bool ok = _cluster->check_schedulers_heart_beat();

            if( !ok )
            {
                Z_FOR_EACH( Cluster::Scheduler_map, _cluster->_scheduler_map, it )
                {
                    Cluster_member* other_scheduler = it->second;

                    if( !other_scheduler->its_me()  &&  !other_scheduler->is_empty_member()  &&  other_scheduler->_is_active )
                    {
                        other_scheduler->deactivate_and_release_orders_after_death();
                    }
                }
            }
        }
        catch( exception& )
        {
            _cluster->async_wake();       // Cluster wird die Exception übernehmen
            throw;
        }
    }

    calculate_next_check_time();
    set_alarm();

    return true;
}

//--------------------------------------------Active_schedulers_watchdog::calculate_next_check_time

void Active_schedulers_watchdog::calculate_next_check_time()
{
    time_t now = ::time(NULL);

    _next_check_time = now + active_heart_beat_check_period;
}

//------------------------------------------------------------Active_schedulers_watchdog::set_alarm

void Active_schedulers_watchdog::set_alarm()
{
    Z_LOG2( "scheduler.cluster", __FUNCTION__ << "  Next check at " << my_string_from_time_t( _next_check_time ) << "\n" );

    time_t t = _next_check_time;

    if( t > _recommended_next_check_time )  t = _recommended_next_check_time;

    set_async_next_gmtime( t );
}

//-------------------------------------------------------------------Cluster::Cluster

Cluster::Cluster( Spooler* spooler )
:
    Scheduler_object( spooler, this, type_cluster_member ),
    _zero_(this+1),
    _continue_exclusive_operation( continue_exclusive_any )
{
}

//------------------------------------------------------------------Cluster::~Cluster
    
Cluster::~Cluster()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler.cluster", "ERROR in " << __FUNCTION__ << ": " << x.what() << "\n" ); }
}

//-----------------------------------------------------------------------------------Cluster::close

void Cluster::close()
{
    if( !_closed )
    {
        Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

        if( _heart_beat_watchdog_thread )  _heart_beat_watchdog_thread->kill();

        close_operations();
        set_async_manager( NULL );

        if( my_member_id() != "" )
        try
        {
            if( _my_scheduler )  
            {
                bool ok = _my_scheduler->mark_as_inactive( Cluster_member::mai_delete_my_inactive_record );
                if( ok )  _my_scheduler = NULL;
            }
            
            //if( _was_start_ok  &&  !_is_in_error )
            //{
            //    delete_old_member_records( (Transaction*)NULL );
            //}
        }
        catch( exception& x )
        {
            if( _log )  _log->warn( S() << x.what() << ", in " << __FUNCTION__ );
        }

        if( _heart_beat_watchdog_thread )
        {
            Z_LOG2( "scheduler", __FUNCTION__ << "  thread_wait_for_termination()\n" );
            _heart_beat_watchdog_thread->thread_wait_for_termination();
            _heart_beat_watchdog_thread = NULL;
        }

        _closed = true;
    }
}

//---------------------------------------------------------------Cluster::calculate_next_heart_beat

void Cluster::calculate_next_heart_beat( time_t now )
{
    _next_heart_beat = now + heart_beat_period;
}

//-----------------------------------------------------------------------------------Cluster::start

bool Cluster::start()
{
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

    assert( !_heart_beat );
    assert( !_exclusive_scheduler_watchdog );
    assert( !_active_schedulers_watchdog );
    assert( !_is_backup || _demand_exclusiveness );


    if( !_is_backup_precedence_set )  _backup_precedence = _is_backup? default_backup_precedence : default_non_backup_precedence;

    if( !db()->opened() )  z::throw_xc( "SCHEDULER-357" ); 
    if( db()->lock_syntax() == db_lock_none )  z::throw_xc( "SCHEDULER-359", db()->dbms_name() );

    make_cluster_member_id();
    //check_member_id();
    create_table_when_needed();


    // Datenbanksätze einrichten

    _is_active = !_demand_exclusiveness;

    if( _demand_exclusiveness )  check_empty_member_record();
    assert_database_integrity( __FUNCTION__ );

    insert_member_record();


    // Aufräumen
    //delete_old_member_records( (Transaction*)NULL );


    // Informationen über die anderen Scheduler einholen

    check_schedulers_heart_beat();

    if( !_my_scheduler )  
    {
        _is_in_error = true;
        z::throw_xc( "SCHEDULER-371", S() << "Missing own record '" << my_member_id() << "' in table " << _spooler->_clusters_tablename );
    }


    // Start

    set_async_manager( _spooler->_connection_manager );
    start_operations();

    assert( _heart_beat );

    if( !_spooler->_suppress_watchdog_thread )
    {
        _heart_beat_watchdog_thread = Z_NEW( Heart_beat_watchdog_thread( this ) );
        _heart_beat_watchdog_thread->thread_start();
    }


    if( !_is_active )
    {
        if( _is_backup  &&  !is_member_allowed_to_start() )  _log->info( message_string( "SCHEDULER-832" ) );
        //else
        //if( _demand_exclusiveness  &&  !_has_exclusiveness )  _log->info( message_string(  ) );
    }



    _was_start_ok = true;

    return true;
}

//----------------------------------------------------------------Cluster::create_table_when_needed

void Cluster::create_table_when_needed()
{
    Transaction ta ( db() );

    db()->create_table_when_needed( &ta, _spooler->_clusters_tablename,
            "`member_id`"                      " varchar(100) not null, "
            "`scheduler_id`"                   " varchar(100) not null, "
            "`precedence`"                     " integer, "
            "`last_heart_beat`"                " integer, "                     // time_t
            "`next_heart_beat`"                " integer, "                     // time_t
            "`active`"                         " boolean, "                     // null oder 1 (not null)
            "`exclusive`"                      " boolean, "                     // null oder 1 (not null)
            "`dead`"                           " boolean, "                     // null oder 1 (not null)
            "`command`"                        " varchar(250), "
            "`http_url`"                       " varchar(100), "
            "`deactivating_member_id`"         " varchar(100), "
            "`xml`"                            " clob, "
            "primary key( `member_id` )" );

    ta.commit( __FUNCTION__ );
}

//------------------------------------------------------------------------Cluster::start_operations

void Cluster::start_operations()
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
    
    if( _is_active  &&  !_active_schedulers_watchdog )
    {
        assert( !_exclusive_scheduler_watchdog );

        //-------------------------------------------------------------------------------------------------
        
        _active_schedulers_watchdog = Z_NEW( Active_schedulers_watchdog( this ) );
        _active_schedulers_watchdog ->set_async_manager( _spooler->_connection_manager );
    }
}

//------------------------------------------------------------------------Cluster::close_operations

void Cluster::close_operations()
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

//--------------------------------------------------------------Cluster::wait_until_is_scheduler_up

//bool Cluster::wait_until_is_scheduler_up()
//{
//    _log->info( message_string( "SCHEDULER-832" ) );
//
//    while( !_spooler->is_termination_state_cmd()  &&  !is_backup_member_allowed_to_start() )
//    {
//        _spooler->simple_wait_step();
//        async_check_exception();
//    }
//
//    bool ok = is_backup_member_allowed_to_start();
//
//    if( ok )  assert_database_integrity( __FUNCTION__ );
//
//    return ok;
//}
//
////--------------------------------------------------------------------Cluster::wait_until_is_active
//
//bool Cluster::wait_until_is_active()
//{
//    bool was_scheduler_up = is_backup_member_allowed_to_start();
//
//    //if( !was_scheduler_up )  _log->info( message_string( "SCHEDULER-832" ) );
//
//    while( !_spooler->is_termination_state_cmd()  &&  !_is_active )  
//    {
//        if( was_scheduler_up  &&  !is_backup_member_allowed_to_start() ) 
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
////------------------------------------------------------------Cluster::wait_until_has_exclusiveness
//
//bool Cluster::wait_until_has_exclusiveness()
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
//-------------------------------------------------------------------------Cluster::async_continue_

bool Cluster::async_continue_( Continue_flags )
{
    Z_LOGI2( "scheduler.cluster", __FUNCTION__ << "\n" );

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

//-------------------------------------------------------------Cluster::check_heart_beat_is_in_time

bool Cluster::check_heart_beat_is_in_time( time_t expected_next_heart_beat )
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

//--------------------------------------------------------------------------Cluster::do_a_heart_beat

bool Cluster::do_a_heart_beat()
{
    Z_LOGI2( "scheduler.cluster", __FUNCTION__ << "\n" );

    bool   had_exclusiveness   = has_exclusiveness();
    bool   ok                  = true;


    if( !db()->opened() )  return false;

    if( _spooler->_db_check_integrity )  assert_database_integrity( __FUNCTION__ );

    if( ok )
    {
        //if( Not_in_recursion not_in_recursion = &_is_in_do_a_heart_beat )   
        //// commit() und reopen_database_after_error() rufen _spooler->check(), der nach Fristablauf wieder in diese Routine steigen kann
        //{
            time_t old_next_heart_beat = _db_next_heart_beat;

            ok = heartbeat_member_record();
            if( ok )  check_heart_beat_is_in_time( old_next_heart_beat );  // Verspätung wird nur gemeldet
        //}
        //else
        //{
        //    Z_LOG2( "joacim", __FUNCTION__ << "  Rekursiver Aufruf\n" );
        //    ok = true;
        //}
    }
    
    if( !ok )
    {
        if( had_exclusiveness )  _is_exclusiveness_lost = true;
        
        bool force_error = true;
        check_my_member_record( (Transaction*)NULL, force_error );
    }

    return ok;
}
    
//----------------------------------------------------------Cluster::heartbeat_member_record

bool Cluster::heartbeat_member_record()
{
    bool   ok          = false;
    bool   has_command = false;


    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        ta.suppress_heart_beat_timeout_check();

        time_t now = ::time(NULL);
        calculate_next_heart_beat( now );

        time_t new_db_last_heart_beat = now;
        time_t new_db_next_heart_beat = _next_heart_beat;

        //Z_LOGI2( "scheduler.cluster", __FUNCTION__ << "  new_db_last_heart_beat=" << new_db_last_heart_beat << " (" << my_string_from_time_t( new_db_last_heart_beat ) << "), "
        //                                                    "new_db_next_heart_beat=" << new_db_next_heart_beat << " (" << my_string_from_time_t( new_db_next_heart_beat ) << ")\n" );

        sql::Update_stmt update ( ta.database_descriptor(), _spooler->_clusters_tablename );
        
        update[ "last_heart_beat" ] = new_db_last_heart_beat;
        update[ "next_heart_beat" ] = new_db_next_heart_beat;

        update.and_where_condition( "member_id"      , my_member_id()      );
        update.and_where_condition( "last_heart_beat", _db_last_heart_beat );
        update.and_where_condition( "next_heart_beat", _db_next_heart_beat );
        update.and_where_condition( "command"          , sql::null_value     );
        update.add_where( S() << " and `active`"    << ( _is_active        ? " is not null" : " is null" ) );
        update.add_where( S() << " and `exclusive`" << ( _has_exclusiveness? " is not null" : " is null" ) );
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
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_clusters_tablename, x ), __FUNCTION__ ); }

    //if( !ok )
    //{
    //    check_schedulers_heart_beat();   // Aktualisiert _exclusive_scheduler->... und _my_scheduler->...
    //}

    if( has_command )  read_and_execute_command();

    return ok;
}

//------------------------------------------------------------------Cluster::check_my_member_record

bool Cluster::check_my_member_record( Transaction* outer_transaction, bool force_error )
{
    bool   is_active              = false;
    bool   is_exclusive           = false;
    string deactivating_member_id = "(unknown)";
    bool   my_record_lost         = false;


    for( Retry_nested_transaction ta ( db(), outer_transaction ); ta.enter_loop(); ta++ ) try
    {
        Any_file result_set = ta.open_result_set( S() << "select `active`, `exclusive`, `deactivating_member_id` "
                                                         "  from " << _spooler->_clusters_tablename << 
                                                         "  where `member_id`=" << sql::quoted( my_member_id() ),
                                                  __FUNCTION__ );

        if( !result_set.eof() )  
        {
            Record record = result_set.get_record();

            is_active              = !record.null( 0 );
            is_exclusive           = !record.null( 1 );
            deactivating_member_id = record.as_string( 2 );
        }
        else
            my_record_lost = true;
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_clusters_tablename, x ), __FUNCTION__ ); }

    if( my_record_lost )
    {
        check_heart_beat_is_in_time( _next_heart_beat );
        z::throw_xc( "SCHEDULER-MEMBER-RECORD-LOST", __FUNCTION__, my_member_id() );
    }


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
        
        if( _is_exclusiveness_lost )  _log->error( message_string( _late_heart_beat? "SCHEDULER-377" : "SCHEDULER-372", deactivating_member_id ) );     // "SOME OTHER SCHEDULER HAS STOLEN EXCLUSIVENESS"
        _log->error( message_string( _late_heart_beat? "SCHEDULER-378" : "SCHEDULER-373", deactivating_member_id ) );   
    }

    return ok;
}

//---------------------------------------------------------------------Cluster::lock_member_records

void Cluster::lock_member_records( Transaction* ta, const string& member1_id, const string& member2_id )
{
    assert( ta );

    S sql;
    sql << "select `member_id`  from " << _spooler->_clusters_tablename << " %update_lock"
           "  where `member_id`"
           " in (" << sql::quoted( member1_id ) << "," 
                   << sql::quoted( member2_id ) << ")";

    ta->open_commitable_result_set( sql, __FUNCTION__ );
}

//---------------------------------------------------------------Cluster::delete_old_member_records

//void Cluster::delete_old_member_records( Transaction* outer_transaction )
//{
//    Z_LOGI2( "scheduler.cluster", __FUNCTION__ << "\n" );
//
//    try
//    {
//        Transaction ta ( db(), outer_transaction );
//
//        S t;
//        t << ( trauerfrist / 3600 ) << " hours";
//
//        ta.execute
//        ( 
//            S() << "DELETE from " << _spooler->_clusters_tablename << 
//                    "  where `scheduler_id`= " << sql::quoted( _spooler->id_for_db() ) <<
//                       " and `active` is null"
//                       " and `next_heart_beat`<" << ( ::time(NULL) - trauerfrist ), 
//            S() << "Deleting inactive records with no heart beat since more then " << t
//        );
//
//        int record_count = ta.record_count();
//
//        ta.commit( __FUNCTION__ );
//
//        if( record_count )  _log->warn( message_string( "SCHEDULER-828", record_count, _spooler->_clusters_tablename, t ) );
//    }
//    catch( exception& x )
//    {
//        _log->warn( S() << x.what() << ", in " << __FUNCTION__ );
//    }
//}

//----------------------------------------------------------------Cluster::read_and_execute_command

void Cluster::read_and_execute_command()
{
    string command;

    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        Any_file result_set = ta.open_result_set
        ( 
            S() << "select `command`"
                   "  from " << _spooler->_clusters_tablename << " %update_lock"
                   "  where `member_id`=" << sql::quoted( my_member_id() ),
            __FUNCTION__
        );

        if( !result_set.eof() )
        {
            Record record = result_set.get_record();
            command = record.as_string( 0 );

            sql::Update_stmt update ( ta.database_descriptor(), _spooler->_clusters_tablename );
            update[ "command" ] = sql::null_value;
            update.and_where_condition( "member_id", my_member_id() );
            ta.execute_single( update, __FUNCTION__ );

            ta.commit( __FUNCTION__ );
        }
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_clusters_tablename, x ), __FUNCTION__ ); }


    if( command != "" )  execute_command( command );
}

//-------------------------------------------------------------------------Cluster::execute_command

void Cluster::execute_command( const string& command )
{
    _log->info( message_string( "SCHEDULER-811", command ) );

    try
    {
        Command_processor cp ( _spooler, Security::seclev_all );
        bool indent = true;
        string xml = cp.execute( "<cluster_member_command>" + command + "</cluster_member_command>", Time::now(), indent );  // Siehe scheduler.xsd
        _log->info( xml );   // Fehler werden im XML gemeldet
    }
    catch( exception& x )
    {
        _log->error( S() << __FUNCTION__ << " " << command << "  ==>  " << x.what() << "\n" );
    }
}

//-------------------------------------------------------------Cluster::check_schedulers_heart_beat

bool Cluster::check_schedulers_heart_beat()
{
    Z_LOGI2( "scheduler.cluster", __FUNCTION__ << "\n" );
    
    bool   result            = true;                // Auch im Fehlerfall
    time_t now_before_select = ::time(NULL);        // Vor dem Datenbankzugriff

    ptr<Cluster_member> previous_exclusive_scheduler = _exclusive_scheduler;
    _exclusive_scheduler = NULL;
    Z_FOR_EACH( Scheduler_map, _scheduler_map, it )  it->second->_is_checked = false;


    // Alle eingetragenen Scheduler lesen

    try
    {
        Read_transaction ta ( db() );

        Any_file result_set = ta.open_result_set( S() << 
                     "select `member_id`, `last_heart_beat`, `next_heart_beat`, `exclusive`, `active`, `dead`, `deactivating_member_id`, `http_url` "
                      " from " << _spooler->_clusters_tablename << 
                     "  where `scheduler_id`=" << sql::quoted( _spooler->id_for_db() ),
                        //" and `active` is not null",
                     __FUNCTION__ );

        while( !result_set.eof() )
        {
            Record record                  = result_set.get_record();
            string other_cluster_member_id = record.as_string( 0 );

            ptr<Cluster_member> other_scheduler = _scheduler_map[ other_cluster_member_id ];
            if( !other_scheduler )
            {
                other_scheduler = Z_NEW( Cluster_member( this, other_cluster_member_id ) );
                _scheduler_map[ other_cluster_member_id ] = other_scheduler;
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


        for( Scheduler_map::iterator it = _scheduler_map.begin(); it != _scheduler_map.end(); )
        {
            Scheduler_map::iterator next_it = it;  next_it++;

            Cluster_member* o = it->second;
            if( !o->_is_checked )
            {
                if( o == _my_scheduler )  _my_scheduler = NULL, _log->error( S() << __FUNCTION__ << "  Own cluster member record has been deleted" );
                if( !o->_is_dead )  
                {
                    o->log()->info( message_string( o == empty_member_record()? "SCHEDULER-841"          // "It's requested, that the exclusive operation will not be continued"
                                                                              : "SCHEDULER-826" ) );     // "That Scheduler has terminated"
                    if( _heart_beat )  _heart_beat->recommend_next_check_time( 0 );   // Wecken, vielleicht gibt's ein command für uns.
                }

                Z_WINDOWS_ONLY( next_it= )  _scheduler_map.erase( it );        // next_it= vorsichtshalber (oder bleibt next_it stabil?). gcc-erase liefert void
            }

            it = next_it;
        }

        if( previous_exclusive_scheduler != _exclusive_scheduler )
        {
            if( !exclusive_scheduler() )  _log->info( message_string( "SCHEDULER-825" ) );
            //else
            //if( exclusive_scheduler() != _my_scheduler )   _log->info( message_string( "SCHEDULER-824", exclusive_scheduler()->_member_id ) );
        }
    }
    catch( exception& x ) { _log->error( S() << x << ", in " << __FUNCTION__ ); }

    return result;
}

//------------------------------------------------------Cluster::recommend_next_deadline_check_time

void Cluster::recommend_next_deadline_check_time( time_t recommended_check_time )
{
    if( _active_schedulers_watchdog   )  _active_schedulers_watchdog->recommend_next_check_time( recommended_check_time );
    if( _exclusive_scheduler_watchdog )  _exclusive_scheduler_watchdog->recommend_next_check_time( recommended_check_time );
}

//---------------------------------------------------------------------Cluster::exclusive_scheduler

Cluster_member* Cluster::exclusive_scheduler()
{
    return _exclusive_scheduler && !_exclusive_scheduler->is_empty_member()?  _exclusive_scheduler
                                                                           : NULL;
}

//-----------------------------------------------------------------------Cluster::mark_as_exclusive

bool Cluster::mark_as_exclusive()
{
    Z_LOGI2( "scheduler.cluster", __FUNCTION__ << "\n" );
    assert( !_has_exclusiveness );

    bool   ok;
    time_t now = ::time(NULL);


    if( !_exclusive_scheduler )
    {
        check_empty_member_record();
        check_schedulers_heart_beat();  // Setzt _exclusive_member

        if( !_exclusive_scheduler )
        {
            _log->error( S() << __FUNCTION__ << "  Missing exclusive member record" );
            return false;
        }
    }


    calculate_next_heart_beat( now );

    time_t new_db_last_heart_beat = now;
    time_t new_db_next_heart_beat = _next_heart_beat;


    if( !_exclusive_scheduler->is_empty_member() )  _exclusive_scheduler->_log->warn( message_string( "SCHEDULER-837" ) );   // "Taking exclusiveness from that Scheduler"
    _log->info( message_string( "SCHEDULER-835" ) );  // "This Scheduler becomes exclusive"

    try
    {
        Transaction ta ( db() );

        lock_member_records( &ta, _exclusive_scheduler->_member_id, my_member_id() );


        // Dem bisher exklusiven Scheduler die Exklusivität nehmen

        sql::Update_stmt update ( ta.database_descriptor(), _spooler->_clusters_tablename );
    
        update[ "exclusive"              ] = sql::null_value;
        update[ "deactivating_member_id" ] = my_member_id();
        if( _exclusive_scheduler->is_empty_member() )  update[ "active" ] = 1;
        update.and_where_condition( "member_id", _exclusive_scheduler->_member_id );
        update.add_where( " and `exclusive` is not null" );
        
        if( !_exclusive_scheduler->is_empty_member() )
        {
            update.and_where_condition( "last_heart_beat", _exclusive_scheduler->_db_last_heart_beat );
            update.and_where_condition( "next_heart_beat", _exclusive_scheduler->_db_next_heart_beat );
        }

        ok = ta.try_execute_single( update, __FUNCTION__ );

        if( ok )
        {
            sql::Update_stmt update ( ta.database_descriptor(), _spooler->_clusters_tablename );
        
            update[ "active"          ] = 1;
            update[ "exclusive"       ] = 1;
            update[ "last_heart_beat" ] = new_db_last_heart_beat;
            update[ "next_heart_beat" ] = new_db_next_heart_beat;
            update.and_where_condition( "member_id", my_member_id() );
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

//---------------------------------------------------------------Cluster::assert_database_integrity

void Cluster::assert_database_integrity( const string& message_text )
{
    bool ok              = false;
    int  exclusive_count = -1;
    int  empty_count     = -1;


    S sql_stmt;
    sql_stmt << "select 1, count(*)"
                "  from " << _spooler->_clusters_tablename <<
                "  where `member_id`=" << sql::quoted( empty_member_id() )<<
             "   union   "
              "  select 2, count( `exclusive` )"
                "  from " << _spooler->_clusters_tablename <<
                "  where `scheduler_id`=" << sql::quoted( _spooler->id_for_db() );

    
    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        Any_file result = ta.open_result_set( sql_stmt, __FUNCTION__ );    // In _einem_ Snapshot abfragen

        while( !result.eof() )
        {
            Record record = result.get_record();
            switch( record.as_int( 0 ) )
            {
                case 1: empty_count     = record.as_int( 1 );  break;
                case 2: exclusive_count = record.as_int( 1 );  break;
                default: assert(__FUNCTION__==NULL);
            }
        }

        ok = empty_count == 0  &&  exclusive_count <= 1  ||
             empty_count == 1  &&  exclusive_count == 1;
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_clusters_tablename, x ), __FUNCTION__ ); }

    if( !ok )
    {
        _is_in_error = true;
        z::throw_xc( "SCHEDULER-371", S() << "exclusive_count=" << exclusive_count << " empty_count=" << empty_count, 
                                      "TABLE " + _spooler->_clusters_tablename, 
                                      message_text );
    }
}

//--------------------------------------------------------------Cluster::check_empty_member_record

void Cluster::check_empty_member_record()
{
    Record record;
    bool   second_try = false;


    do
    {
        try
        {
            Transaction ta ( db() );

            string   where_clause = S() << "where `member_id`=" << sql::quoted( empty_member_id() );

            Any_file result_set   = ta.open_result_set
            (
                S() << "select `active`  from " << _spooler->_clusters_tablename << "  " << where_clause,
                __FUNCTION__
            );

            if( !result_set.eof() )
            {
                //if( result_set.get_record().null( "active" ) )
                //{
                //    sql::Update_stmt update ( db()->database_descriptor(), _spooler->_clusters_tablename );
                //    update[ "active" ] = 1;
                //    update.and_where_condition( "member_id", empty_member_id() );
                //    ta.execute_single( update, __FUNCTION__ );
                //}
            }
            else
            if( !_is_backup )
            {
                Any_file result_set = ta.open_commitable_result_set
                (
                    S() << "select `exclusive`  from " << _spooler->_clusters_tablename  << " %update_lock"
                           "  where `scheduler_id`=" << sql::quoted( _spooler->id_for_db() ) 
                        <<   " and `exclusive` is not null",
                    __FUNCTION__
                );

                int exclusive_count = 0;
                while( !result_set.eof() )  result_set.get_record(), exclusive_count++;     // count(*) kann nicht mit For Update verbunden werden (Postgres)
                
                if( exclusive_count > 1 )  z::throw_xc( "SCHEDULER-371", S() << exclusive_count << " exclusive Schedulers" );


                sql::Insert_stmt stmt ( ta.database_descriptor(), _spooler->_clusters_tablename );

                stmt[ "member_id"       ] = empty_member_id();
              //stmt[ "precedence"      ] = 0;
              //stmt[ "last_heart_beat" ] = Beide Felder NULL lassen, damit sie nicht als veraltete Einträge angesehen und gelöscht werden
              //stmt[ "next_heart_beat" ] = 
                stmt[ "active"          ] = _is_active          ? sql::Value( 1 ) : sql::null_value;
                stmt[ "exclusive"       ] = exclusive_count == 0? sql::Value( 1 ) : sql::null_value;
                stmt[ "scheduler_id"    ] = _spooler->id_for_db();
                stmt[ "http_url"        ] = "";

                ta.execute( stmt, "The empty member record" );


                ta.update_clob( _spooler->_clusters_tablename, "xml", my_member_dom_document().xml(), where_clause );
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

//--------------------------------------------------------------------Cluster::insert_member_record

void Cluster::insert_member_record()
{
    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        time_t now = ::time(NULL);

        calculate_next_heart_beat( now );

        time_t new_db_last_heart_beat = now;
        time_t new_db_next_heart_beat = _next_heart_beat;


        sql::Insert_stmt insert ( ta.database_descriptor(), _spooler->_clusters_tablename );
        
        insert[ "member_id"       ] = my_member_id();
        insert[ "precedence"      ] = _backup_precedence;
        insert[ "last_heart_beat" ] = new_db_last_heart_beat;
        insert[ "next_heart_beat" ] = new_db_next_heart_beat;
        insert[ "active"          ] = _is_active? sql::Value( 1 ) : sql::null_value;
        insert[ "exclusive"       ] = sql::null_value;
        insert[ "scheduler_id"    ] = _spooler->id_for_db();
        insert[ "http_url"        ] = _spooler->http_url();

        ta.execute( insert, __FUNCTION__ );

        ta.update_clob( _spooler->_clusters_tablename, "xml", my_member_dom_document().xml(), "where `member_id`=" + sql::quoted( my_member_id() ) );

        ta.commit( __FUNCTION__ );


        _db_last_heart_beat = new_db_last_heart_beat;
        _db_next_heart_beat = new_db_next_heart_beat;
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_clusters_tablename, x ), __FUNCTION__ ); }
}

//--------------------------------------------------------------------------Cluster::mark_as_active

//void Cluster::mark_as_active()
//{
//    Z_LOGI2( "scheduler.cluster", __FUNCTION__ << "\n" );
//    assert( !_is_active );
//
//    _log->debug( message_string( "SCHEDULER-819" ) );   // "Scheduler becomes active"
//
//    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
//    {
//        Transaction ta ( db() );
//
//        sql::Update_stmt update ( ta.database_descriptor(), _spooler->_clusters_tablename );
//    
//        update[ "active"          ] = 1;
//        update.and_where_condition( "member_id"      , _cluster->my_member_id() );
//        update.and_where_condition( "active"         , sql::null_value );
//        update.and_where_condition( "exclusive"       , sql::null_value );
//        update.and_where_condition( "last_heart_beat"    , _db_last_heart_beat );
//        update.and_where_condition( "next_heart_beat"    , _db_next_heart_beat );
//
//        ta.execute_single( update, __FUNCTION__ );
//        ta.commit( __FUNCTION__ );
//    }
//    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_clusters_tablename, x ) ); }
//
//    _is_active = true;
//}

//-----------------------------------------------------------------------Cluster::async_state_text_

string Cluster::async_state_text_() const
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

//--------------------------------------------Cluster::set_command_for_all_active_schedulers_but_me

//void Cluster::set_command_for_all_active_schedulers_but_me( Transaction* ta, Command command )
//{
//    set_command_for_all_schedulers_but_me( ta, "`active` is not null", command );
//}

//---------------------------------------------------Cluster::set_command_for_all_schedulers_but_me

bool Cluster::set_command_for_all_schedulers_but_me( Transaction* ta, const string& command )
{
    return set_command_for_all_schedulers_but_me( ta, command, "" );
}

//-----------------------------------------------------------Distributed::set_command_for_scheduler

bool Cluster::set_command_for_scheduler( Transaction* ta, const string& command, const string& member_id )
{
    return set_command_for_all_schedulers_but_me( ta, command, S() << "`member_id`=" << sql::quoted( member_id ) );
}

//---------------------------------------------------Cluster::set_command_for_all_schedulers_but_me

bool Cluster::set_command_for_all_schedulers_but_me( Transaction* outer_transaction, const string& command, const string& where_condition )
{
    bool result = false;

    try
    {
        Transaction ta ( db(), outer_transaction );

        sql::Update_stmt update ( ta.database_descriptor(), _spooler->_clusters_tablename );
     
        update[ "command" ] = command;
        update.and_where_condition( "scheduler_id", _spooler->id_for_db() );

        S w;
        w << " and not( `member_id` in (" << sql::quoted( my_member_id()    ) << "," 
                                                  << sql::quoted( empty_member_id() ) << ") )";
        w << " and `dead` is null";
        if( where_condition != "" )  w << " and " << where_condition;

        update.add_where( w );

        ta.execute( update, __FUNCTION__ );
        int count = ta.record_count();

        ta.commit( __FUNCTION__ );
        result = count > 0;
    }
    catch( exception& x ) 
    { 
        _log->error( S() << x.what() << ", in " << __FUNCTION__ ); 
    }

    return result;
}

//------------------------------------------------------------Cluster::delete_dead_scheduler_record

bool Cluster::delete_dead_scheduler_record( const string& member_id )
{
    bool result = false;

    if( Cluster_member* member = cluster_member_or_null( member_id ) )
    {
        result = member->delete_dead_record();
    }

    return result;
}

//------------------------------------------------------------------Cluster::show_active_schedulers

void Cluster::show_active_schedulers( Transaction* outer_transaction, bool exclusive_only )
{
    Z_LOGI2( "scheduler.cluster", __FUNCTION__ << "\n" );

    if( db()  &&  db()->opened() )
    for( Retry_nested_transaction ta ( db(), outer_transaction ); ta.enter_loop(); ta++ )  try
    {
        bool found      = false;
        S    select_sql;

        string where_clause = S() << "where `scheduler_id`=" + sql::quoted( _spooler->id_for_db() ) <<
                                     " and " << ( exclusive_only? "`exclusive`" : "`active`" ) << " is not null";
        select_sql << "select `member_id`, `last_heart_beat`, `http_url`, `exclusive`"
                       "  from " << _spooler->_clusters_tablename << "  " <<
                       where_clause;


        Any_file select = ta.open_result_set( select_sql, __FUNCTION__ );

        while( !select.eof() )
        {
            Record record = select.get_record();
            string xml    = ta.read_clob_or_empty( _spooler->_clusters_tablename, "xml", where_clause );

            if( xml != "" ) // Satz nicht zwischenzeitlich gelöscht?
            {
                xml::Document_ptr dom_document = xml;
                xml::Element_ptr  dom_element  = dom_document.documentElement();


                string active_scheduler_id = record.as_string( "member_id" );

                if( active_scheduler_id == empty_member_id() )
                {
                    //_log->info( message_string( "SCHEDULER-809", dom_element.getAttribute( "running_since" ) ) );
                }
                else
                {
                    found = true;

                    time_t last_heart_beat = record.null     ( "last_heart_beat" )? 0 : record.as_int64 ( "last_heart_beat" );
                    string http_url        = record.as_string( "http_url" );
                    bool   is_exclusive    = !record.null    ( "exclusive" );

                    _log->info( message_string( is_exclusive? "SCHEDULER-822" : "SCHEDULER-821", active_scheduler_id, ::time(NULL) - last_heart_beat, http_url, 
                                                dom_element.getAttribute( "pid" ) ) );
                }
            }
        }

        if( !found )  _log->info( message_string( "SCHEDULER-805" ) );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->_clusters_tablename, x ), __FUNCTION__ ); }
}

//---------------------------------------------------Cluster::scheduler_up_variable_name

//string Cluster::scheduler_up_variable_name()
//{
//    return "scheduler/" + _spooler->id_for_db() + "/up";
//}

//-------------------------------------------------------------------------Cluster::empty_member_id

string Cluster::empty_member_id()
{ 
    return _spooler->id_for_db(); 
}

//------------------------------------------------------------------------Cluster::set_my_member_id

void Cluster::set_my_member_id( const string& member_id )
{
    assert( !_heart_beat  &&  !_exclusive_scheduler_watchdog );

    _cluster_member_id = member_id;
    //check_member_id();

    _log->set_prefix( obj_name() );
}

//---------------------------------------------------------------------Cluster::exclusive_member_id

string Cluster::exclusive_member_id()
{
    if( _has_exclusiveness    )  return my_member_id();
    if( exclusive_scheduler() )  return exclusive_scheduler()->_member_id;

    return "";
}

//-------------------------------------------------------------------------Cluster::check_is_active

bool Cluster::check_is_active( Transaction* outer_transaction )
{
    if( _is_active )   // &&  !_is_in_do_a_heart_beat )  
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

    return _is_active;
}

//--------------------------------------------------------------Cluster::is_member_allowed_to_start

bool Cluster::is_member_allowed_to_start()
{
    bool result = true;

    if( Cluster_member* empty_record = empty_member_record() )
    {
        result = true;
        //Noch nicht realisiert:  result = empty_record->_http_url == ""  ||  empty_record->_http_url == _spooler->http_url();
    }
    else
    if( _is_backup )
    {
        result = false;
    }

    return result;
}

//-------------------------------------------------------------------------Cluster::check_member_id

//void Cluster::check_member_id()
//{
//    string prefix = _spooler->id_for_db() + "/";
//    if( !string_begins_with( my_member_id(), prefix )  ||  my_member_id() == prefix )  z::throw_xc( "SCHEDULER-358", my_member_id(), prefix );
//}

//------------------------------------------------------------------Cluster::make_cluster_member_id

void Cluster::make_cluster_member_id()
{
    set_my_member_id( S() << _spooler->id_for_db() 
                          << "/" << _spooler->_complete_hostname << ":" << _spooler->tcp_port() 
                          << "/" << getpid() 
                          << "." << ( as_string( 1000000 + (uint64)( double_from_gmtime() * 1000000 ) % 1000000 ).substr( 1 ) ) );  // Mikrosekunden, sechsstellig
}

//---------------------------------------------------------------------Cluster::empty_member_record

Cluster_member* Cluster::empty_member_record()
{
    return cluster_member_or_null( empty_member_id() );
}

//-------------------------------------------------------------------Cluster::http_url_of_member_id

string Cluster::http_url_of_member_id( const string& member_id )
{
    string result;

    if( Cluster_member* scheduler_member = cluster_member_or_null( member_id ) )
    {
        result = scheduler_member->_http_url;
    }

    return result;
}

//------------------------------------------------------------------Cluster::cluster_member_or_null

Cluster_member* Cluster::cluster_member_or_null( const string& member_id )
{
    Scheduler_map::iterator it = _scheduler_map.find( member_id );
    return it != _scheduler_map.end()? it->second : NULL;
}

//--------------------------------------------------------Cluster::set_continue_exclusive_operation

void Cluster::set_continue_exclusive_operation( const string& http_url_ )
{
    string http_url = http_url_;

    if( http_url == continue_exclusive_this )  http_url = _spooler->http_url();
    else
    if( http_url == continue_exclusive_any  ); // ok
    else
    if( http_url == continue_exclusive_non_backup || http_url == "" )   // Default
    {
        http_url = "";
    }
    else
    if( string_begins_with( http_url, "http://" ) );  // ok
    else
        z::throw_xc( __FUNCTION__, http_url );


    _continue_exclusive_operation = http_url;
}

//------------------------------------------------------------------Cluster::my_member_dom_document

xml::Document_ptr Cluster::my_member_dom_document()
{
    xml::Document_ptr result;

    result.create();
    result.appendChild( my_member_dom_element( result ) );

    return result;
}

//-------------------------------------------------------------------Cluster::my_member_dom_element

xml::Element_ptr Cluster::my_member_dom_element( const xml::Document_ptr& document )
{
    xml::Element_ptr result = document.createElement( "cluster_member" );

    result.setAttribute( "version"          , _spooler->_version );
    result.setAttribute( "host"             , _spooler->_complete_hostname );
    result.setAttribute( "pid"              , getpid() );
    result.setAttribute( "running_since"    , Time::now().as_string() );
    result.setAttribute( "backup_precedence", _backup_precedence );


    if( is_backup()           )  result.setAttribute( "backup"              , "yes" );
    if( _demand_exclusiveness )  result.setAttribute( "demand_exclusiveness", "yes" );
    if( _are_orders_distributed )  result.setAttribute( "distributed_orders", "yes" );
    if( _spooler->tcp_port()  )  result.setAttribute( "tcp_port", _spooler->tcp_port() );
    if( _spooler->udp_port()  )  result.setAttribute( "udp_port", _spooler->udp_port() );


    return result;
}

//-----------------------------------------------------------------------------Cluster::dom_element

xml::Element_ptr Cluster::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    xml::Element_ptr result = document.createElement( "cluster" );

    if( _spooler->db()->opened() )  check_schedulers_heart_beat();  // _scheduler_map auf den Stand bringen

    result.setAttribute( "cluster_member_id", my_member_id() );
    if( _is_active         )  result.setAttribute( "active"   , "yes" );
    if( _has_exclusiveness )  result.setAttribute( "exclusive", "yes" );
    if( _is_backup         )  result.setAttribute( "backup"   , "yes" );
    result.setAttribute( "is_member_allowed_to_start", is_member_allowed_to_start()? "yes" : "no" );


    if( show_what.is_set( show_cluster ) )
    {
        Z_FOR_EACH( Scheduler_map, _scheduler_map, it ) 
        {
            Cluster_member* member = it->second;

            if( !member->is_empty_member() 
             //&& ( member->_heart_beat_count || !member->_is_dead )
            )     // Nur die Scheduler, von denen wir einmal einen Herzschlag gehört haben, oder die neu und noch ohne Herzschlag sind
            {
                result.appendChild( member->dom_element( document, show_what ) );
            }
        }
    }

    return result;
}

//--------------------------------------------------------------------------------Cluster::obj_name

string Cluster::obj_name() const
{ 
    return "Cluster";   // + _cluster_member_id;
} 

//-------------------------------------------------------------------------------------------------

} //namespace cluster
} //namespace scheduler
} //namespace sos
