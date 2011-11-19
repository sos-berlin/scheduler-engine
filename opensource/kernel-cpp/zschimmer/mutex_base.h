// $Id: mutex_base.h 13487 2008-04-14 11:43:52Z jz $

#ifndef ZSCHIMMER_MUTEX_BASE_H
#define ZSCHIMMER_MUTEX_BASE_H

namespace zschimmer {

//---------------------------------------------------------------------------------------Mutex_base
// AUFBAU NICHT ÄNDERN, wird mit zur Laufzeit nachgeladenem Modul ausgetauscht!

struct Mutex_base
{
    enum Kind
    {
        kind_recursive          = 0x00,
        kind_nonrecursive       = 0x01,
        kind_recursive_dont_log = 0x02
    };


                                Mutex_base                  ( const string& name, Kind kind )       : _name(name), _dont_log( kind == kind_recursive_dont_log ) {}

    void                    set_name                        ( const string& name )                  { _name = name; }
    string                      name                        ()                                      { return _name; }


    string                     _name;
    bool                       _dont_log;

  private:
                                Mutex_base                  ( const Mutex_base& );                   // Nicht implementiert. CRITICAL_SECTION darf nicht kopiert oder verschoben werden
    void                        operator =                  ( const Mutex_base& );                   // Nicht implementiert.
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
