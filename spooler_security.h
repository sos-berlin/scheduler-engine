// $Id: spooler_security.h,v 1.3 2002/11/11 23:10:34 jz Exp $

#ifndef __SPOOLER_SECURITY_H
#define __SPOOLER_SECURITY_H

namespace sos {
namespace spooler {

//-----------------------------------------------------------------------------------------Security

struct Security
{
    enum Level
    {
        seclev_none,
        seclev_signal,
        seclev_info,
        seclev_all
    };
/*
    struct Host_entry
    {
        Host                   _host;
        Level                  _level;
    };
*/
    typedef map<Host,Level>     Host_map;


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

} //namespace spooler
} //namespace sos

#endif
