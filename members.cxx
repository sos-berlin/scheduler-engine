// $Id$

#include "spooler.h"

#ifdef Z_WINDOWS
#   include <process.h>
#endif

using namespace zschimmer;

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

const time_t                    accepted_clock_difference       = 5;    // Die Uhren sollten noch besser übereinstimmen! ntp verwenden!
const time_t                    warned_clock_difference         = 1; 
const time_t                    heart_beat_period               = Z_NDEBUG_DEBUG( 60, 30 );
const time_t                    heart_beat_maximum_process_time = 10;   // Zeit, die gebraucht wird, um den Herzschlag auszuführen
const time_t                    heart_beat_delay                = heart_beat_maximum_process_time + accepted_clock_difference;
const time_t                    heart_beat_minimum_check_period = heart_beat_period / 2;
const time_t                    trauerfrist                     = 2*3600;   // Trauerzeit, nach der Mitgliedssätze gelöscht werden

//#ifdef Z_DEBUG
    const bool  log_sql = true;
//#else
//    const boo   log_sql = false;
//#endif

//----------------------------------------------------------------------Active_scheduler_heart_beat

struct Active_scheduler_heart_beat : Async_operation
{
    Active_scheduler_heart_beat( Scheduler_member* m ) 
    :
        _scheduler_member(m)
    {
    }


    bool async_finished_() const
    { 
        return !_scheduler_member->is_active();
    }


    string async_state_text_() const
    {
        return "Active_scheduler_heart_beat";
    }


    bool async_continue_( Continue_flags )
    {
        Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

        try
        {
            if( !_scheduler_member->db()->opened() )
            {
                _scheduler_member->async_wake();
                return true;
            }

            
            _scheduler_member->do_heart_beat();
        
            if( _scheduler_member->is_active() )
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


    void set_alarm()
    {
        set_async_next_gmtime( _scheduler_member->last_heart_beat() + heart_beat_period );
    }


  private:
    Scheduler_member*               _scheduler_member;
};

//----------------------------------------------------------------------Inactive_scheduler_watchdog

struct Inactive_scheduler_watchdog : Async_operation
{
    Inactive_scheduler_watchdog( Scheduler_member* m ) 
    :
        _scheduler_member(m)
    {
    }


    bool async_finished_() const
    { 
        return _scheduler_member->is_active();
    }


    string async_state_text_() const
    {
        return "Inactive_scheduler_watchdog";
    }


    bool async_continue_( Continue_flags )
    {
        Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    
        try
        {
            if( !_scheduler_member->db()->opened() )
            {
                _scheduler_member->async_wake();
                return true;
            }


            _scheduler_member->try_to_become_active();

            if( _scheduler_member->is_active() )
            {
                // Wir sind aktives Mitglied geworden, Operation beenden!
                _scheduler_member->async_wake();
            }
            else
            {
                // Wir sind weiterhin inaktives Mitglied
                set_alarm();

                //if( _last_active_member_id != ""  &&  _last_active_member_id  != _last_clockchecked_member_id )
                //{
                //    check_clock_difference();
                //    _last_clockchecked_member_id;
                //}
            }
        }
        catch( exception& )
        {
            _scheduler_member->async_wake();
            throw;
        }

        return true;
    }


    void set_alarm()
    {
        time_t now         = ::time(NULL);
        time_t next_check;

        if( !_scheduler_member->_clock_difference_checked )
        {
            next_check = now + warned_clock_difference;
        }
        else
        {
            next_check = now + heart_beat_period;
            
            time_t expected_heart_beat = _scheduler_member->_expected_next_heart_beat_of_active_scheduler;
            if( expected_heart_beat < next_check  &&  expected_heart_beat >= heart_beat_minimum_check_period )
            {
                next_check = expected_heart_beat + accepted_clock_difference;
                Z_LOG2( "scheduler.distributed", __FUNCTION__ << " synchonized with heart beat\n" );
            }
        }

        set_async_next_gmtime( next_check );
    }

  private:
    Scheduler_member*          _scheduler_member;
};

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

    if( !db()->opened() )  z::throw_xc( "SCHEDULER-357" ); 

    create_table_when_needed();

    if( _scheduler_member_id == "" )  make_scheduler_member_id();
    check_member_id();


    do
    {
        try
        {
            Transaction ta ( db() );
            ta.set_log_sql( log_sql );

            if( !_is_backup )  insert_scheduler_id_record( &ta );   // Falls die Scheduler-Id noch nicht registriert ist
            delete_old_member_records( &ta );

            ta.commit();

            try_to_become_active();                                 // Stellt auch _is_scheduler_terminated fest
            if( _become_active )  become_active();
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
            "`last_heart_beat`"        " double not null, "
          //"`last_heart_beat`"        " timestamp not null, "
            "`next_heart_beat`"        " double, "
          //"`next_heart_beat`"        " timestamp, "
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
        if( active_member_id != "" )  Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "  Aktives Mitglied ist anscheinend " << active_member_id << "\n" );//show_active_members( ta );//_log->info( message_string( "SCHEDULER-995", active_member_id, "" ) );
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
    assert( !_operation );

    if( db()->opened() )
    {
        if( _is_active )
        {
            ptr<Active_scheduler_heart_beat> operation = Z_NEW( Active_scheduler_heart_beat( this ) );
            operation->set_alarm();
            _operation = +operation;
        }
        else
        {
            ptr<Inactive_scheduler_watchdog> operation = Z_NEW( Inactive_scheduler_watchdog( this ) );
            operation->set_alarm();
            _operation = +operation;
        }

        _operation->set_async_manager( _spooler->_connection_manager );
    }
}

//----------------------------------------------------------------Scheduler_member::close_operation

void Scheduler_member::close_operation()
{
    if( _operation ) 
    {
        _operation->set_async_manager( NULL );
        _operation = NULL;
    }
}

//-----------------------------------------------------------Scheduler_member::try_to_become_active

void Scheduler_member::try_to_become_active()
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( !_is_active );
    if( !db()->opened() )  z::throw_xc( "SCHEDULER-357" );

    do
    {
        try
        {
            Transaction ta ( db() );
            ta.set_log_sql( log_sql );

            try_to_become_active2( &ta );

            if( _become_active )
            {
                ta.commit();
                become_active();
            }
            else
                ta.rollback();  // Doppelt, try_to_become_active2() hat auch schon einen rollback gemacht
        }
        catch( exception& x )  
        { 
            db()->try_reopen_after_error( zschimmer::Xc( "SCHEDULER-360", x ) );
            continue;
        }
    } while(0);
}

//----------------------------------------------------------Scheduler_member::try_to_become_active2

void Scheduler_member::try_to_become_active2( Transaction* ta )
{
    // Rollback, wenn am Ende nicht _become_active gilt!

    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );
    assert( ta );
    assert( !_become_active );


    string other_member_id;

    Any_file select ( S() << "-in " << db()->db_name() <<
                     "SELECT m.`scheduler_member_id`, m.`last_heart_beat`, m.`next_heart_beat` "
                     "from " << _spooler->_members_tablename   << " m, " <<
                         " " << _spooler->_variables_tablename << " v " <<
                         "where v.`name`=" << sql::quoted( active_member_variable_name() ) <<
                          " and v.`textwert`=m.`scheduler_member_id`" );

    if( select.eof() )
    {
        sql::Update_stmt update ( &db()->_db_descr, _spooler->_variables_tablename );
        
        update[ "textwert" ] = member_id();
        update.and_where_condition( "name"    , active_member_variable_name() );
        update.and_where_condition( "textwert", "" );

        bool record_is_updated = db()->try_execute_single( update );

        if( record_is_updated )
        {
            insert_member_record_and_prepare_to_become_active( ta );
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


               other_member_id          = record.as_string( 0 );
        bool   other_member_timed_out   = false;
        time_t last_heart_beat          = (time_t)( record.as_double( 1 ) + 0.5 );
        time_t next_heart_beat          = record.null( 2 )? 0 : (time_t)( record.as_int64( 2 ) + 0.5 );

        
        if( last_heart_beat > now )
        {
            if( !_clock_difference_checked )
            {
                _clock_difference = last_heart_beat - now;
                _clock_difference_checked = true;
            }
            
            if( _clock_difference >= warned_clock_difference + 1 )
                _log->warn( message_string( "SCHEDULER-364", last_heart_beat - now, other_member_id ) );
        }

        if( !_clock_difference_checked )
        {
            if( next_heart_beat != _expected_next_heart_beat_of_active_scheduler )  set_clock_difference( last_heart_beat - now );
        }

        if( next_heart_beat + heart_beat_delay < now )
        {
            _log->warn( message_string( "SCHEDULER-993", 
                                            other_member_id, 
                                            string_gmt_from_time_t( last_heart_beat ) + " UTC", 
                                            string_gmt_from_time_t( next_heart_beat ) + " UTC" ) );
            other_member_timed_out = true;
        }
        else
        if( last_heart_beat + heart_beat_period + heart_beat_delay < now )
        {
            _log->warn( message_string( "SCHEDULER-994", 
                                            other_member_id, 
                                            string_gmt_from_time_t( next_heart_beat ) + " UTC", 
                                            heart_beat_period ) );
            other_member_timed_out = true;
        }


        if( !other_member_timed_out )
        {
            _is_scheduler_terminated = false;

            // Eigenes Überwachung mit Herzschlag des aktiven Schedulers synchronisieren, nur leicht verzögert
            _expected_next_heart_beat_of_active_scheduler = next_heart_beat;
        }
        else
        {
            // Der Herzschlag des bisher aktiven Schedulers hat ausgesetzt.
            // Wir versuchen, den den Betrieb zu übernehmen.

            // Scheduler-Id soll auf unsere Mitglieds-Id verweisen

            sql::Update_stmt update ( &db()->_db_descr, _spooler->_variables_tablename );
            
            update[ "textwert" ] = member_id();
            update.and_where_condition( "name"    , active_member_variable_name() );
            update.and_where_condition( "textwert", other_member_id );

            update.add_where( S() << " and ( select `last_heart_beat` from " << _spooler->_members_tablename <<
                                            " where `scheduler_member_id`=" << sql::quoted( other_member_id ) << ") "
                                           "= " << last_heart_beat );

            bool record_is_updated = db()->try_execute_single( update );

            if( record_is_updated )
            {
                insert_member_record_and_prepare_to_become_active( ta );
            }
        }
    }

    if( !_become_active  &&  other_member_id != _last_active_member_id )
    {
        _log->info( message_string( "SCHEDULER-998", other_member_id == ""? "(none)" : other_member_id ) );
        _last_active_member_id = other_member_id;
        _clock_difference_checked = false;
    }

    if( !_become_active )  ta->rollback();
}

//------------------------------------------------------------------Scheduler_member::become_active

void Scheduler_member::become_active()
{
    assert( _become_active );
    assert( !_is_active );

    _become_active = false;
    _is_active = true;
    

    _log->info( message_string( "SCHEDULER-997" ) );

    do_heart_beat();    // Nochmal die Datenbank prüfen

    if( _is_active )  _last_active_member_id = member_id();
}

//-----------------------------------------------------------Scheduler_member::set_clock_difference

void Scheduler_member::set_clock_difference( time_t difference )
{
    _clock_difference         = difference;
    _clock_difference_checked = true;

    if( abs( _clock_difference ) >= warned_clock_difference + 1 )
    {
        _log->info( message_string( "SCHEDULER-804", _last_active_member_id ) );
    }
    else
    {
        _log->warn( message_string( "SCHEDULER-364", difference, _last_active_member_id ) );
    }
}

//-------------------------------------Scheduler::insert_member_record_and_prepare_to_become_active

void Scheduler_member::insert_member_record_and_prepare_to_become_active( Transaction* ta )
{
    _log->info( message_string( "SCHEDULER-803" ) );

    bool ok = insert_member_record( ta );
    
    if( ok )  ok = try_to_heartbeat_member_record( ta );

    if( ok )
    {
        _become_active = true;
    }
    else
    {
        _log->error( message_string( "SCHEDULER-356" ) );   // "Some other Scheduler member has become active"
        show_active_members( ta );
    }
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

//------------------------------------------------------------------Scheduler_member::do_heart_beat

void Scheduler_member::do_heart_beat()
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    time_t old_next_heart_beat = _next_heart_beat;

    if( db()->opened() )
    do
    {
        try
        {
            Transaction ta ( db() );
            ta.set_log_sql( log_sql );

            do_heart_beat( &ta );

            ta.commit();
        }
        catch( exception& x )  
        { 
            db()->try_reopen_after_error( zschimmer::Xc( "SCHEDULER-360", x ) );
            continue;
        }
    } while(0);


    time_t available_time = old_next_heart_beat - ::time(NULL);    // Herzschlag noch in der Frist?
    if( available_time < 0 )
    {
        _log->warn( message_string( "SCHEDULER-996", string_gmt_from_time_t( _next_heart_beat ), -available_time ) );
    }
}
    
//------------------------------------------------------------------Scheduler_member::do_heart_beat

void Scheduler_member::do_heart_beat( Transaction* ta )
{
    assert( ta );
    
    bool ok = try_to_heartbeat_member_record( ta );

    if( !ok )
    {
        _is_active = false;

        _log->error( message_string( "SCHEDULER-356" ) );   // "Some other Scheduler member has become active"
        show_active_members( ta );
    }
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

//------------------------------------------------------------Scheduler_member::show_active_members

void Scheduler_member::show_active_members( Transaction*  ta )
{
    assert( ta );

    // Aktives Mitglied zeigen:

    Any_file select ( S() << "-in " << db()->db_name() <<
                     "SELECT m.`scheduler_member_id`, m.`http_url` "
                     "from " << _spooler->_members_tablename   << " m, " <<
                         " " << _spooler->_variables_tablename << " v " <<
                         "where v.`name`=" + sql::quoted( active_member_variable_name() ) <<
                           "and v.`textwert`=m.`scheduler_member_id`" );

    while( !select.eof() )
    {
        Record record = select.get_record();

        string other_member_id = record.as_string( 0 );
        string other_http_url  = record.as_string( 1 );

        _log->info( message_string( "SCHEDULER-995", other_member_id, other_http_url ) );
    }
}

//----------------------------------------------------------------Scheduler_member::async_continue_

bool Scheduler_member::async_continue_( Continue_flags )
{
    Z_LOGI2( "scheduler.distributed", __FUNCTION__ << "\n" );

    if( _operation )
    {
        _operation->async_check_exception( "Error in Scheduler member operation" );
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
    assert( !_operation );

    _scheduler_member_id = member_id;
    check_member_id();

    _log->set_prefix( obj_name() );
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
