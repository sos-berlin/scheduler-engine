// $Id: spooler_service.h,v 1.3 2001/02/16 18:23:12 jz Exp $

#ifndef __SPOOLER_SERVICE_H
#define __SPOOLER_SERVICE_H

namespace sos {
namespace spooler {

int                             spooler_service             ( const string& id, int argc, char** argv );
void                            install_service             ( const string& id, int argc, char** argv );
void                            remove_service              ( const string& id );
DWORD                           service_state               ( const string& id );
void                            service_start               ( const string& id );
inline bool                     service_is_started          ( const string& id )    { return service_state(id) == SERVICE_START_PENDING; }

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
