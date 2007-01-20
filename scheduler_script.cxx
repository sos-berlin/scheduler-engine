// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"

namespace sos {
namespace scheduler {

//---------------------------------------------------------------------------------Scheduler_script

struct Scheduler_script : Scheduler_script_interface
{
                                Scheduler_script            ( Scheduler* );
                               ~Scheduler_script            ();


    // Subsystem:
    bool                    set_subsystem_state             ( Subsystem_state );
    void                        close                       ();


    // Scheduler_script_interface:
    void                        set_dom_script              ( const xml::Element_ptr& script_element, const Time& xml_mod_time, const string& include_path );
    Module_instance*            module_instance             ()                                      { return _module_instance; }


  private:
    void                        start                       ();
    void                        load                        ();

    Module                     _module;                     // <script>
    ptr<Module_instance>       _module_instance;
    ptr<Com_log>               _com_log;                    // COM-Objekt spooler.log
};

//-----------------------------------------------------------------------------new_scheduler_script

ptr<Scheduler_script_interface> new_scheduler_script( Scheduler* scheduler )
{
    ptr<Scheduler_script> scheduler_script = Z_NEW( Scheduler_script( scheduler ) );
    return +scheduler_script;
}

//----------------------------------------------------------------cheduler_script::Scheduler_script

Scheduler_script::Scheduler_script( Scheduler* scheduler )
: 
    Scheduler_script_interface( scheduler, type_scheduler_script ),
    _module(_spooler,_log)
{
    _module._dont_remote = true;
    _com_log = new Com_log( _log );
}

//---------------------------------------------------------------cheduler_script::~Scheduler_script
    
Scheduler_script::~Scheduler_script()
{
    if( _com_log )  _com_log->set_log( NULL );
}

//--------------------------------------------------------------------------Scheduler_script::close
    
void Scheduler_script::close()
{
    if( _module_instance )      
    {
        Z_LOG2( "scheduler", "Scheduler-Skript wird beendet ...\n" );

        try
        {
            _module_instance->call_if_exists( spooler_exit_name );
        }
        catch( exception& x )  { _log->warn( message_string( "SCHEDULER-260", x.what() ) ); }  // "Scheduler-Skript spooler_exit(): $1"

        _module_instance->close();
        _module_instance = NULL;

        Z_LOG2( "scheduler", "Scheduler-Skript ist beendet.\n" );
    }

    _subsystem_state = subsys_stopped;
}

//-----------------------------------------------------------------Scheduler_script::set_dom_script

void Scheduler_script::set_dom_script( const xml::Element_ptr& script_element, const Time& xml_mod_time, const string& include_path )
{
    _module.set_dom( script_element, xml_mod_time, include_path );
}

//------------------------------------------------------------Scheduler_script::set_subsystem_state
    
bool Scheduler_script::set_subsystem_state( Subsystem_state new_state )
{
    switch( new_state )
    {
        case subsys_loaded:
        {
            if( _subsystem_state != subsys_not_initialized )  throw_subsystem_state_error( new_state, __FUNCTION__ );
            load();
            _subsystem_state = subsys_loaded;
            Z_LOG2( "scheduler", "Startskript ist geladen\n" );
            return true;
        }

        case subsys_started:
        {
            if( _subsystem_state != subsys_loaded )  throw_subsystem_state_error( new_state, __FUNCTION__ );

            Z_LOGI2( "scheduler", "Startskript wird gestartet\n" );
            start();
            _subsystem_state = subsys_started;
            Z_LOG2( "scheduler", "Startskript ist gestartet worden\n" );

            return true;
        }

        default:
            throw_subsystem_state_error( new_state, __FUNCTION__ );
    }
}

//---------------------------------------------------------------------------Scheduler_script::load
    
void Scheduler_script::load()
{
    try
    {
        if( _module.set() )
        {
            _module.init();

            Z_LOGI2( "scheduler", "Startskript wird geladen\n" );
        
            _module_instance = _module.create_instance();
          //_module_instance->_title = "Scheduler-Script";
            _module_instance->init();

            _module_instance->add_obj( (IDispatch*)_spooler->_com_spooler, "spooler"     );
            _module_instance->add_obj( (IDispatch*)_com_log              , "spooler_log" );

            _module_instance->load();
        }
    }
    catch( exception& )
    {
        _log->error( message_string( "SCHEDULER-332" ) );
        throw;
    }
}

//--------------------------------------------------------------------------Scheduler_script::start

void Scheduler_script::start()
{
    try
    {
        if( _module.set() )
        {
            _module_instance->start();

            bool ok = check_result( _module_instance->call_if_exists( spooler_init_name ) );

            _spooler->detect_warning_and_send_mail();

            if( !ok )  z::throw_xc( "SCHEDULER-183" );
        }
    }
    catch( exception& )
    {
        _log->error( message_string( "SCHEDULER-332" ) );
        throw;
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

