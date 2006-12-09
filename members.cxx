// $Id$

#include "spooler.h"

#ifdef Z_WINDOWS
#   include <process.h>
#endif

using namespace zschimmer;

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

//const time_t                    accepted_clock_difference       = Z_NDEBUG_DEBUG(  5,  2 );     // Die Uhren sollten noch besser übereinstimmen! ntp verwenden!
//const time_t                    warned_clock_difference         = Z_NDEBUG_DEBUG(  1,  1 ); 
const time_t                    heart_beat_period               = Z_NDEBUG_DEBUG( 60, 20 );
const time_t                    max_heart_beat_processing_time = Z_NDEBUG_DEBUG( 10,  3 );     // Zeit, die gebraucht wird, um den Herzschlag auszuführen
//const time_t                    heart_beat_delay                = max_heart_beat_processing_time;// + accepted_clock_difference;
const time_t                    heart_beat_minimum_check_period = heart_beat_period / 2;
const time_t                    trauerfrist                     = 2*3600;   // Trauerzeit, nach der Mitgliedssätze gelöscht werden

const bool                      log_sql                         = true;

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
    bool                        try_to_become_active2       ( Transaction* );
  //void                        check_clock_difference      ( time_t last_heart_beat, time_t now );
    bool                        insert_member_record_and_prepare_to_become_active( Transaction* );
    void                        show_active_members         ( Transaction* = NULL );
    bool                        set_active                  ();
    Spooler_db*                 db                          ()                                      { return _scheduler_member->db(); }


    Fill_zero                  _zero_;
    Scheduler_member*          _scheduler_member;
    Other_scheduler            _other_scheduler;
    time_t                     _next_check_time;
    bool                       _is_scheduler_terminated;    // Scheduler ist ordentlich beendet worden
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

        
        bool ok = _scheduler_member->do_heart_beat();
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
    set_async_next_gmtime( _scheduler_member->last_heart_beat() + heart_beat_period );
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
    return "Inactive_scheduler_watchdog";
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

        _next_check_time = now + heart_beat_period + delay;
        
        time_t wait_until = _other_scheduler._next_heart_beat_db + delay;
        if( wait_until - now >= heart_beat_minimum_check_period  &&  wait_until < _next_check_time )
        {
            time_t new_next_check = wait_until;
            extra_log << ", watchdog synchronized with heart beat: " << ( new_next_check - _next_check_time  ) << "s";
            _next_check_time = new_next_check;
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

    do
    {
        try
        {
            bool become_active = false;

            {
                Transaction ta ( db() );
                ta.set_log_sql( log_sql );

                become_active = try_to_become_active2( &ta );

                if( become_active )  ta.commit();
                               else  ta.rollback();  // Doppelt, try_to_become_active2() hat auch schon einen rollback gemacht
            }

            if( become_active )
            {
                bool ok = set_active();
                if( !ok )  show_active_members();
            }
        }
        catch( exception& x )  
        { 
            db()->try_reopen_after_error( zschimmer::Xc( "SCHEDULER-360", x ) );
            continue;
        }
    } while(0);
}

//-----------------------------------------------Inactive_scheduler_watchdog::try_to_become_active2

bool Inactive_scheduler_watchdog::try_to_become_active2( Transaction* ta )
{
    // Rollback, wenn Funktion false liefert!

    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( ta );


    bool   become_active = false;
    string other_member_id;


    Any_file select ( S() << "-in " << db()->db_name() <<
                     "SELECT m.`scheduler_member_id`, m.`last_heart_beat`, m.`next_heart_beat` "
                     "from " << _spooler->_members_tablename   << " m, " <<
                         " " << _spooler->_variables_tablename << " v " <<
                         "where v.`name`=" << sql::quoted( _scheduler_member->active_member_variable_name() ) <<
                          " and v.`textwert`=m.`scheduler_member_id`" );

    if( select.eof() )
    {
        sql::Update_stmt update ( &db()->_db_descr, _spooler->_variables_tablename );
        
        update[ "textwert" ] = _scheduler_member->member_id();
        update.and_where_condition( "name"    , _scheduler_member->active_member_variable_name() );
        //update.and_where_condition( "textwert", "" );

        bool record_is_updated = db()->try_execute_single( update );

        if( record_is_updated )
        {
            become_active = insert_member_record_and_prepare_to_become_active( ta );
        }
        else
        {
            // Der Scheduler-Id-Satz ist nicht vorhanden, also ist der Scheduler ordentlich beendet worden
            _is_scheduler_terminated = true;
        }
    }
    else
    {
        time_t now = ::time(NULL);

        Record record = select.get_record();
        assert( select.eof() );


               other_member_id        = record.as_string( 0 );
        bool   other_member_timed_out = false;
        time_t last_heart_beat        = record.as_int64( 1 );                         //(time_t)( record.as_double( 1 ) + 0.5 );
        time_t next_heart_beat        = record.null( 2 )? 0 : record.as_int64( 2 );   //(time_t)( record.as_double( 2 ) + 0.5 );

        
        if( other_member_id != _other_scheduler._member_id )
        {
            _other_scheduler = Other_scheduler();
            _other_scheduler._member_id                = other_member_id;
            _other_scheduler._last_heart_beat_detected = now;
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
                if( _other_scheduler._last_heart_beat_detected + heart_beat_period + max_heart_beat_processing_time/2 < now )
                {
                    _log->warn( message_string( "SCHEDULER-994", 
                                                    other_member_id, 
                                                    string_gmt_from_time_t( _other_scheduler._last_heart_beat_detected ) + " UTC", 
                                                    _other_scheduler._last_heart_beat_detected - now ) );
                    warned = true;

                    if( _other_scheduler._last_heart_beat_detected + heart_beat_period + max_heart_beat_processing_time < now )  
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
                                                string_gmt_from_time_t( _other_scheduler._last_heart_beat_detected ) + " UTC" ) );

                //if( next_heart_beat + heart_beat_delay < now )  other_member_timed_out = true;
            }

            //if( !other_member_timed_out  &&  last_heart_beat + heart_beat_period + heart_beat_delay/2 < now )
            //{
            //    _log->warn( message_string( "SCHEDULER-994", 
            //                                    other_member_id, 
            //                                    string_gmt_from_time_t( next_heart_beat ) + " UTC", 
            //                                    now - ( last_heart_beat + heart_beat_period ) ) );

            //    //if( last_heart_beat + heart_beat_period + heart_beat_delay < now )  other_member_timed_out = true;
            //}
            
            //if( !other_member_timed_out  &&  _other_scheduler._checking_clock_difference  &&  last_heart_beat == _other_scheduler._next_heart_beat_db )
            //{
            //    check_clock_difference( last_heart_beat, now );
            //}

            if( !other_member_timed_out )
            {
                _is_scheduler_terminated = false;
            }
            else
            {
                // Der Herzschlag des bisher aktiven Schedulers hat ausgesetzt.
                // Wir versuchen, den Betrieb zu übernehmen.

                // Scheduler-Id soll auf unsere Mitglieds-Id verweisen

                sql::Update_stmt update ( &db()->_db_descr, _spooler->_variables_tablename );
                
                update[ "textwert" ] = _scheduler_member->member_id();
                update.and_where_condition( "name"    , _scheduler_member->active_member_variable_name() );
                update.and_where_condition( "textwert", other_member_id );

                update.add_where( S() << " and ( select `last_heart_beat` from " << _spooler->_members_tablename <<
                                                " where `scheduler_member_id`=" << sql::quoted( other_member_id ) << ") "
                                               "= " << last_heart_beat );

                bool record_is_updated = db()->try_execute_single( update );

                if( record_is_updated )
                {
                    become_active = insert_member_record_and_prepare_to_become_active( ta );
                }
            }
        }

        _other_scheduler._last_heart_beat_db = last_heart_beat;
        _other_scheduler._next_heart_beat_db = next_heart_beat;
    }

    if( !become_active )
    {
        if( other_member_id != _other_scheduler._member_id )
        {
            _log->info( message_string( "SCHEDULER-998", other_member_id == ""? "(none)" : other_member_id ) );
            _other_scheduler._member_id = other_member_id;
            //_other_scheduler._clock_difference_checked = false;
        }

        ta->rollback();
    }

    return become_active;
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

//-------------------Inactive_scheduler_watchdog::insert_member_record_and_prepare_to_become_active

bool Inactive_scheduler_watchdog::insert_member_record_and_prepare_to_become_active( Transaction* ta )
{
    _log->info( message_string( "SCHEDULER-803" ) );

    bool ok = _scheduler_member->insert_member_record( ta );
    
    if( ok )  ok = _scheduler_member->try_to_heartbeat_member_record( ta );
    if( ok )  return ok;
    
    _log->error( message_string( "SCHEDULER-356" ) );   // "Some other Scheduler member has become active"
    show_active_members( ta );
    return false;
}

//----------------------------------------------------------Inactive_scheduler_watchdog::set_active

bool Inactive_scheduler_watchdog::set_active()
{
    assert( !_scheduler_member->_is_active );

    _scheduler_member->_is_active = true;

    _log->info( message_string( "SCHEDULER-997" ) );

    bool is_active = _scheduler_member->do_heart_beat();    // Nochmal die Datenbank prüfen, kann _is_active=false setzen
    if( is_active )  _other_scheduler._member_id = _scheduler_member->member_id();

    assert( is_active == _scheduler_member->_is_active );
    return is_active;
}

//-------------------------------------------------Inactive_scheduler_watchdog::show_active_members

void Inactive_scheduler_watchdog::show_active_members( Transaction*  outer_transaction )
{
    if( db()  &&  db()->opened() )
    do
    {
        try
        {
            Transaction ta ( db(), outer_transaction );
            ta.set_log_sql( log_sql );

            Any_file select ( S() << "-in " << db()->db_name() <<
                             "SELECT m.`scheduler_member_id`, m.`http_url` "
                             "from " << _spooler->_members_tablename   << " m, " <<
                                 " " << _spooler->_variables_tablename << " v " <<
                                 "where v.`name`=" + sql::quoted( _scheduler_member->active_member_variable_name() ) <<
                                   "and v.`textwert`=m.`scheduler_member_id`" );

            while( !select.eof() )
            {
                Record record = select.get_record();

                string other_member_id = record.as_string( 0 );
                string other_http_url  = record.as_string( 1 );

                _log->info( message_string( "SCHEDULER-995", other_member_id, other_http_url ) );
            }

            ta.commit();
        }
        catch( exception& x )  
        { 
            db()->try_reopen_after_error( zschimmer::Xc( "SCHEDULER-360", x ) );
            continue;
        }
    } while(0);
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
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

    close_operation();
    set_async_manager( NULL );
}

//-----------------------------------------------------------------------Scheduler_member::shutdown

void Scheduler_member::shutdown()
{
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

    if( db()  &&  db()->opened() )
    do
    {
        try
        {
            Transaction ta ( db() );
            ta.set_log_sql( log_sql );

            delete_member_record( &ta );
            
            if( _is_active )
            {
                delete_old_member_records( &ta );
                delete_scheduler_id_record( &ta );
            }

            ta.commit();
        }
        catch( exception& x )  
        { 
            db()->try_reopen_after_error( zschimmer::Xc( "SCHEDULER-360", x ) );
            continue;
        }
    } while(0);

    close();
}

//-----------------------------------------------------------Scheduler_member::delete_member_record

void Scheduler_member::delete_member_record( Transaction* ta )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( ta );


    db_execute( S() << "DELETE from " << _spooler->_members_tablename << 
                       " where `scheduler_member_id`=" << sql::quoted( member_id() ) );

    // Wenn Where-Klause nicht zutrifft, sind wir zwischendurch inaktiv geworden
}

//-----------------------------------------------------Scheduler_member::delete_scheduler_id_record

void Scheduler_member::delete_scheduler_id_record( Transaction* ta )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( ta );


    db_execute( S() << "DELETE from " << _spooler->_variables_tablename << 
                         " where `name`="     << sql::quoted( active_member_variable_name() ) <<
                            "and `textwert`=" << sql::quoted( member_id() ) );

    // Wenn Where-Klause nicht zutrifft, sind wir zwischendurch inaktiv geworden
}

//------------------------------------------------------Scheduler_member::delete_old_member_records

void Scheduler_member::delete_old_member_records( Transaction* ta )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( ta );

    db_execute( S() << "DELETE from " << _spooler->_members_tablename << 
                        " where `scheduler_id`= " << sql::quoted( _spooler->id_for_db() ) <<
                          " and `next_heart_beat`<" << ( ::time(NULL) - trauerfrist ) );

    if( int record_count = db()->record_count() )
    {
        Z_LOG2( "scheduler.distributed", record_count << " alte Sätze aus " << _spooler->_members_tablename << " gelöscht\n" );
    }
}

//--------------------------------------------------------------------------Scheduler_member::start

void Scheduler_member::start()
{
    Z_LOGI2( "scheduler", __FUNCTION__ << "\n" );

    assert( !_current_operation );
    assert( !_active_scheduler_heart_beat );
    assert( !_inactive_scheduler_watchdog );

    if( !db()->opened() )  z::throw_xc( "SCHEDULER-357" ); 

    create_table_when_needed();

    if( _scheduler_member_id == "" )  make_scheduler_member_id();
    check_member_id();


    do
    {
        try
        {
            {
                Transaction ta ( db() );
                ta.set_log_sql( log_sql );

                if( !_is_backup )  insert_scheduler_id_record( &ta );   // Falls die Scheduler-Id noch nicht registriert ist
                delete_old_member_records( &ta );

                ta.commit();
            }

            _inactive_scheduler_watchdog = Z_NEW( Inactive_scheduler_watchdog( this ) );
            _inactive_scheduler_watchdog->try_to_become_active();   // Stellt sofort _is_scheduler_terminated fest
            if( _is_active )  _inactive_scheduler_watchdog = NULL;
        }
        catch( exception& x )  
        { 
            db()->try_reopen_after_error( zschimmer::Xc( "SCHEDULER-360", x ) );
            continue;
        }
    } while(0);


    set_async_manager( _spooler->_connection_manager );
    start_operation();
}

//-------------------------------------------------------Scheduler_member::create_table_when_needed

void Scheduler_member::create_table_when_needed()
{
    db()->create_table_when_needed( _spooler->_members_tablename,
            "`scheduler_id`"           " varchar(100) not null, "
            "`scheduler_member_id`"    " varchar(100) not null, "
            "`running_since`"          " datetime, "
            "`last_heart_beat`"        " integer, "     //numeric(14,3) not null, "
            "`next_heart_beat`"        " integer, "     //numeric(14,3), "
          //"`ip_address`"             " varchar(40), "
          //"`udp_port`"               " integer, "
          //"`tcp_port`"               " integer, "
            "`http_url`"               " varchar(100), "
            "primary key( `scheduler_member_id` )" );
}

//-----------------------------------------------------Scheduler_member::insert_scheduler_id_record

bool Scheduler_member::insert_scheduler_id_record( Transaction* ta )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( ta );
    assert( !_is_active );


    bool   result           = false;
    bool   record_exists    = false;
    string active_member_id = db()->get_variable( ta, active_member_variable_name(), &record_exists );
    
    if( record_exists )
    {
        if( active_member_id != "" )  Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "  Aktives Mitglied ist anscheinend " << active_member_id << "\n" );
    }
    else
    {
        try
        {
            db()->insert_variable( ta, active_member_variable_name(), "" );
            result = true;
        }
        catch( exception& )
        {
            active_member_id = db()->get_variable( ta, active_member_variable_name(), &record_exists );
            if( !record_exists )  throw;
        }
    }

    return result;
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
            _inactive_scheduler_watchdog->show_active_members();
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

bool Scheduler_member::do_heart_beat()
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    bool   is_active;
    time_t old_next_heart_beat = _next_heart_beat;

    if( db()->opened() )
    do
    {
        try
        {
            Transaction ta ( db() );
            ta.set_log_sql( log_sql );

            is_active = do_heart_beat( &ta );

            ta.commit();
        }
        catch( exception& x )  
        { 
            db()->try_reopen_after_error( zschimmer::Xc( "SCHEDULER-360", x ) );
            continue;
        }
    } while(0);


    time_t available_time = old_next_heart_beat + max_heart_beat_processing_time - ::time(NULL);    // Herzschlag noch in der Frist?
    if( available_time < 0 )
    {
        _log->warn( message_string( "SCHEDULER-996", string_gmt_from_time_t( _next_heart_beat ), -available_time ) );
    }

    assert( is_active == _is_active );
    return is_active;
}
    
//------------------------------------------------------------------Scheduler_member::do_heart_beat

bool Scheduler_member::do_heart_beat( Transaction* ta )
{
    assert( ta );
    assert( _is_active );
    

    bool ok = try_to_heartbeat_member_record( ta );

    if( !ok )
    {
        _is_active = false;

        _log->error( message_string( "SCHEDULER-356" ) );   // "Some other Scheduler member has become active"
        //show_active_members( ta );
    }

    assert( ok == _is_active );
    return ok;
}

//-------------------------------------------------Scheduler_member::try_to_heartbeat_member_record

bool Scheduler_member::try_to_heartbeat_member_record( Transaction* ta )
{
    time_t new_last_heart_beat = ::time(NULL);
    time_t new_next_heart_beat = new_last_heart_beat + heart_beat_period;

    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "  new_last_heart_beat=" << new_last_heart_beat << " (" << string_gmt_from_time_t( new_last_heart_beat ) << " UTC), "
                                                        "new_next_heart_beat=" << new_next_heart_beat << " (" << string_gmt_from_time_t( new_next_heart_beat ) << " UTC)\n" );

//#   ifdef Z_DEBUG
//        _log->info( S() << "new_last_heart_beat=" << new_last_heart_beat << " (" << string_local_from_time_t( new_last_heart_beat) << "), "
//                           "new_next_heart_beat=" << new_next_heart_beat << " (" << string_local_from_time_t( new_next_heart_beat) << ")" );
//#   endif

    assert( ta );


    sql::Update_stmt update ( &db()->_db_descr, _spooler->_members_tablename );
    
    update[ "last_heart_beat" ] = new_last_heart_beat;
    update[ "next_heart_beat" ] = new_next_heart_beat;

    update.and_where_condition( "scheduler_member_id", member_id()      );
    update.and_where_condition( "last_heart_beat"    , _last_heart_beat );
    update.and_where_condition( "next_heart_beat"    , _next_heart_beat );

    update.add_where( S() << " and ( select `textwert` from " << _spooler->_variables_tablename <<
                                    " where `name`="     << sql::quoted( active_member_variable_name() ) << ")"
                                 " = " << sql::quoted( member_id() ) );


    bool record_is_updated = db()->try_execute_single( update );

    if( record_is_updated )
    {
        _last_heart_beat = new_last_heart_beat;
        _next_heart_beat = new_next_heart_beat;
    }

    return record_is_updated;
}

//-----------------------------------------------------------Scheduler_member::insert_member_record

bool Scheduler_member::insert_member_record( Transaction* ta )
{
    assert( ta );
    assert( !_is_active );

    time_t now = ::time(NULL);
    time_t new_last_heart_beat = now;
    time_t new_next_heart_beat = new_last_heart_beat + heart_beat_period;

    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "  new_last_heart_beat=" << new_last_heart_beat << " (" << string_gmt_from_time_t( new_last_heart_beat) << " UTC), "
                                                        "new_next_heart_beat=" << new_next_heart_beat << " (" << string_gmt_from_time_t( new_next_heart_beat) << " UTC)\n" );


    sql::Update_stmt update ( &db()->_db_descr, _spooler->_members_tablename );
    
    update[ "last_heart_beat" ] = new_last_heart_beat;
    update[ "next_heart_beat" ] = new_next_heart_beat;
    update[ "scheduler_id"    ] = _spooler->id_for_db();
    update[ "running_since"   ].set_datetime( string_local_from_time_t( new_last_heart_beat ) );
    update[ "http_url"        ] = _spooler->http_url();
    update.and_where_condition( "scheduler_member_id", member_id() );
  //update.add_where( S() << " and `last_heart_beat`<" << ( now - heart_beat_too_late_period ) );
  //update.add_where( S() << " and `next_heart_beat`<" << now );


    bool record_is_updated = db()->try_execute_single( update );

    if( !record_is_updated )
    {
        update[ "scheduler_member_id" ] = member_id();

        string insert_sql = update.make_insert_stmt();
        try
        {
            db_execute( insert_sql );
            record_is_updated = true;
        }
        catch( exception& x )
        {
            // Falls Update fehlschlug wegen aktuellem last_heart_beat: Ein anderer aktiver Scheduler hat dieselbe Member-Id (das darf nicht sein)
            _log->warn( insert_sql );
            _log->warn( x.what() );
        }
    }

    _last_heart_beat = new_last_heart_beat;
    _next_heart_beat = new_next_heart_beat;

    return record_is_updated;
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

    if( db()->opened() )
    {
        start_operation();
    }
    else
    if( _is_backup )  z::throw_xc( "SCHEDULER-357" );

    return true;
}

//----------------------------------------------------Scheduler_member::active_member_variable_name

string Scheduler_member::active_member_variable_name()
{
    return "scheduler/" + _spooler->id_for_db() + "/active_member";
}

//------------------------------------------------------------------Scheduler_member::set_member_id
    
void Scheduler_member::set_member_id( const string& member_id )
{
    assert( !_current_operation );

    _scheduler_member_id = member_id;
    check_member_id();

    _log->set_prefix( obj_name() );
}

//--------------------------------------------------------Scheduler_member::is_scheduler_terminated

bool Scheduler_member::is_scheduler_terminated()
{ 
    return _inactive_scheduler_watchdog  &&  _inactive_scheduler_watchdog->_is_scheduler_terminated; 
}

//-----------------------------------------------------------------------------Scheduler_member::db

Spooler_db* Scheduler_member::db()
{ 
    return _spooler->_db; 
}

//---------------------------------------------------------------------Scheduler_member::db_execute

void Scheduler_member::db_execute( const string& stmt )
{
    db()->execute( stmt );
}

//----------------------------------------------------------------Scheduler_member::check_member_id

void Scheduler_member::check_member_id()
{
    //if( _spooler->id() == "" )  z::throw_xc( "SCHEDULER-358" );

    string prefix = _spooler->id_for_db() + ".";
    if( !string_begins_with( member_id(), prefix )  ||  member_id() == prefix )  z::throw_xc( "SCHEDULER-358", member_id(), prefix );
}

//-------------------------------------------------------Scheduler_member::make_scheduler_member_id

void Scheduler_member::make_scheduler_member_id()
{
    //if( _is_backup  &&  _spooler->id() == "" )  z::throw_xc( "SCHEDULER-359" );

    set_member_id( S() << _spooler->id_for_db() << "." << _spooler->_hostname << "." << getpid() );
}

//-----------------------------------------------------------------------Scheduler_member::obj_name

string Scheduler_member::obj_name() const
{ 
    return "Distributed_scheduler";   // + _scheduler_member_id;
} 

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
