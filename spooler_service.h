// $Id: spooler_service.h,v 1.7 2002/11/13 21:20:48 jz Exp $

#ifndef __SPOOLER_SERVICE_H
#define __SPOOLER_SERVICE_H
#ifdef Z_WINDOWS

namespace sos {
namespace spooler {

int                             spooler_service             ( const string& service_name, int argc, char** argv );
void                            install_service             ( const string& service_name, const string& service_description, const string& service_display, 
                                                              const string& dependencies, const string& command_line );
void                            remove_service              ( const string& service_name );
DWORD                           service_state               ( const string& service_name );
void                            service_start               ( const string& service_name );
inline bool                     service_is_started          ( const string& service_name )    { return service_state(service_name) == SERVICE_START_PENDING; }

string                          make_service_name           ( const string& id );
string                          make_service_display        ( const string& id );

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
#endif
