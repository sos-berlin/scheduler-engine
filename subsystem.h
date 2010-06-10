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

struct Subsystem : Scheduler_object, Non_cloneable
{
                                Subsystem                   ( Spooler*, IUnknown*, Type_code );
                                ~Subsystem                  ();

              Scheduler_object::obj_name;

    virtual void                close                       ()                                      = 0;
    virtual string              name                        () const                                = 0;
    virtual bool                switch_subsystem_state      ( Subsystem_state );
    
    Subsystem_state             subsystem_state             () const                                { return _subsystem_state; }
    virtual xml::Element_ptr    dom_element                 ( const xml::Document_ptr&, const Show_what& ) const;

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

//----------------------------------------------------------------------------------------Subsystem_register

struct Subsystem_register : Scheduler_object, Non_cloneable, Object
{
    typedef stdext::hash_set<Subsystem*> Set;

                                Subsystem_register          ( Scheduler* scheduler ) : Scheduler_object( scheduler, this, type_subsystem_register ), _zero_(this+1) {}

    void                        add(Subsystem* s)           { _set.insert(s); }
    void                        remove(Subsystem* s)        { _set.erase(s); }
    const Set*                  set()                       { return &_set; }
    bool                        contains(const string&) const;
    Subsystem*                  get(const string&) const;

private:
    Fill_zero                  _zero_;
    Set                        _set;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
