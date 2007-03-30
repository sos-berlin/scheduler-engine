// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SCHEDULER_SUBSYSTEM_H
#define __SCHEDULER_SUBSYSTEM_H

namespace sos {
namespace scheduler {

//----------------------------------------------------------------------------------Subsystem_state

enum Subsystem_state
{
    subsys_not_initialized,
    subsys_initialized,
    subsys_loaded,
    subsys_active,
    subsys_stopped 
};

string                          string_from_subsystem_state ( Subsystem_state );

//----------------------------------------------------------------------------------------Subsystem

struct Subsystem : Object, Non_cloneable, Scheduler_object
{
                                Subsystem                   ( Spooler* spooler, Type_code t )       : Scheduler_object( spooler, this, t ), _zero_(this+1) {}

              Scheduler_object::obj_name;

    virtual void                close                       ()                                      = 0;
    virtual bool                switch_subsystem_state      ( Subsystem_state );
    
    Subsystem_state             subsystem_state             () const                                { return _subsystem_state; }

  protected:
    virtual bool                subsystem_initialize        ();
    virtual bool                subsystem_load              ();
    virtual bool                subsystem_activate          ();

    Z_NORETURN void             throw_subsystem_state_error ( Subsystem_state, const string& message_text );
    void                        assert_subsystem_state      ( Subsystem_state, const string& message_text );
    void                    set_subsystem_state             ( Subsystem_state state )               { _subsystem_state = state; }

    Fill_zero                  _zero_;
    Subsystem_state            _subsystem_state;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
