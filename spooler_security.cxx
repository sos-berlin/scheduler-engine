// $Id: spooler_security.cxx,v 1.4 2004/12/03 18:39:00 jz Exp $

#include "spooler.h"

namespace sos {
namespace spooler {

//----------------------------------------------------------------------------------Security::level

Security::Level Security::level( const in_addr& host )
{
    Host_map::iterator it;
    
    it = _host_map.find( host );
    if( it != _host_map.end() )  return it->second;
    
    Host net = Host(host).net();
    if( net != host )
    {
        it = _host_map.find( net );
        if( it != _host_map.end() )  return it->second;

        it = _host_map.find( Host( 0 ) );                 // host="0.0.0.0" gilt für alle übrigen Hosts
        if( it != _host_map.end() )  return it->second;
    }
    
    return seclev_none;
}

//-------------------------------------------------------------------------------Security::as_level

Security::Level Security::as_level( const string& name )
{
    if( name == "none"   )  return seclev_none;
    if( name == "signal" )  return seclev_signal;
    if( name == "info"   )  return seclev_info;
    if( name == "all"    )  return seclev_all;

    throw_xc( "SCHEDULER-119", name ); return seclev_none;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos


