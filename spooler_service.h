// $Id: spooler_service.h,v 1.1 2001/01/25 20:28:38 jz Exp $

#ifndef __SPOOLER_SERVICE_H
#define __SPOOLER_SERVICE_H

namespace sos {
namespace spooler {

int                             spooler_service             ( int argc, char** argv );
void                            install_service             ();
void                            remove_service              ();
bool                            service_is_started          ();

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
