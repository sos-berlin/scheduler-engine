// $Id: spooler_service.h,v 1.2 2001/02/14 22:06:56 jz Exp $

#ifndef __SPOOLER_SERVICE_H
#define __SPOOLER_SERVICE_H

namespace sos {
namespace spooler {

int                             spooler_service             ( const string& id, int argc, char** argv );
void                            install_service             ( const string& id, int argc, char** argv );
void                            remove_service              ( const string& id );
bool                            service_is_started          ( const string& id );

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
