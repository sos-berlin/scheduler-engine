// $Id$

#ifndef __SPOOLER_SECURITY_H
#define __SPOOLER_SECURITY_H

namespace sos {
namespace scheduler {

//-----------------------------------------------------------------------------------------Security

struct Security
{
    enum Level
    {
        seclev_none,
        seclev_signal,
        seclev_info,
        seclev_no_add,
        seclev_all
    };

    typedef map<Ip_address,Level>  Host_map;


                                Security                    ( Spooler* spooler ) : _spooler(spooler) {}

    void                        set_dom                     ( const xml::Element_ptr& );
    void                        clear                       ()                                  { _host_map.clear(); }

    Level                       level                       ( const in_addr& host );

    static Level                as_level                    ( const string& );

  protected:
    Spooler*                   _spooler;
    Host_map                   _host_map;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
