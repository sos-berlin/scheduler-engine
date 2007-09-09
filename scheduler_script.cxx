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
    void                        close                       ();
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();


    // Scheduler_script_interface:
    void                        set_dom_script              ( const xml::Element_ptr& script_element );
    Module*                     module                      ()                                      { return &_module; }
    Module_instance*            module_instance             ()                                      { return _module_instance; }


  private:
    Fill_zero                  _zero_;
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

//---------------------------------------------------------------Scheduler_script::Scheduler_script

Scheduler_script::Scheduler_script( Scheduler* scheduler )
: 
    Scheduler_script_interface( scheduler, type_scheduler_script ),
    _zero_(this+1),
    _module( scheduler, scheduler->include_path(), _log )
{
    _module._dont_remote = true;

    _com_log = new Com_log( _log );
}

//--------------------------------------------------------------Scheduler_script::~Scheduler_script
    
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

void Scheduler_script::set_dom_script( const xml::Element_ptr& script_element )
{
    _module.set_dom( script_element );
}

//-----------------------------------------------------------Scheduler_script::subsystem_initialize

bool Scheduler_script::subsystem_initialize()
{
    if( _module.set() )
    {
        _module.init();
    }

    _subsystem_state = subsys_initialized;
    return true;
}

//-----------------------------------------------------------------Scheduler_script::subsystem_load

bool Scheduler_script::subsystem_load()
{
    bool result = false;

    if( _module.set() )
    {
        Z_LOGI2( "scheduler", "Scheduler-Skript wird geladen\n" );

        _module_instance = _module.create_instance();
      //_module_instance->_title = "Scheduler-Script";
        _module_instance->init();

        _module_instance->add_obj( (IDispatch*)_spooler->_com_spooler, "spooler"     );
        _module_instance->add_obj( (IDispatch*)_com_log              , "spooler_log" );

        _module_instance->load();

        Z_LOG2( "scheduler", "Scheduler-Skript ist geladen\n" );
        _subsystem_state = subsys_loaded;
        result = true;
    }

    return result;
}

//-------------------------------------------------------------Scheduler_script::subsystem_activate

bool Scheduler_script::subsystem_activate()
{
    bool result = false;

    if( _module.set() )
    {
        Z_LOGI2( "scheduler", "Scheduler-Skript wird gestartet\n" );
        _subsystem_state = subsys_active;  // Jetzt schon aktiv für die auszuführenden Skript-Funktionen

        _module_instance->start();

        bool ok = check_result( _module_instance->call_if_exists( spooler_init_name ) );

        _spooler->detect_warning_and_send_mail();

        if( !ok )  z::throw_xc( "SCHEDULER-183" );

        Z_LOG2( "scheduler", "Scheduler-Skript ist gestartet worden\n" );
        result = true;
    }

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
