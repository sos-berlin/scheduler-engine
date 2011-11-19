// $Id: spooler_security.cxx 12905 2007-07-12 17:51:58Z jz $

#include "spooler.h"

namespace sos {
namespace scheduler {

//----------------------------------------------------------------------------------Security::level

Security::Level Security::level( const in_addr& host )
{
    Host_map::iterator it;
    
    it = _host_map.find( host );
    if( it != _host_map.end() )  return it->second;
    
    Ip_address net = Ip_address(host).net();
    if( net != host )
    {
        it = _host_map.find( net );
        if( it != _host_map.end() )  return it->second;

        it = _host_map.find( Ip_address( 0 ) );                 // host="0.0.0.0" gilt für alle übrigen Hosts
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
    if( name == "no_add" )  return seclev_no_add;
    if( name == "all"    )  return seclev_all;

    z::throw_xc( "SCHEDULER-119", name ); return seclev_none;
}

//--------------------------------------------------------------------------------Security::set_dom

void Security::set_dom( const xml::Element_ptr& security_element ) 
{ 
    if( !security_element )  return;

    bool ignore_unknown_hosts = as_bool( security_element.getAttribute( "ignore_unknown_hosts" ), true );

    DOM_FOR_EACH_ELEMENT( security_element, e )
    {
        if( e.nodeName_is( "allowed_host" ) )
        {
            string          hostname = e.getAttribute( "host" );
            set<Ip_address> host_set;

            try 
            {
                host_set = Ip_address::get_host_set_by_name( hostname );
            }
            catch( exception& x )
            {
                _spooler->log()->warn( "<allowed_host host=\"" + hostname + "\">  " + x.what() );
                if( !ignore_unknown_hosts )  throw;
            }
            
            FOR_EACH( set<Ip_address>, host_set, h )
            {
                _host_map[ *h ] = as_level( e.getAttribute( "level" ) );
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos


