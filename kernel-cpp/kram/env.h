// $Id: env.h 11394 2005-04-03 08:30:29Z jz $

#ifndef __INCLUDE_SOS_ENV
#define __INCLUDE_SOS_ENV

namespace sos {

void        set_environment_from_sos_ini_once   ();
void        clear_environment_from_sos_ini      ();
void        set_environment_from_sos_ini        ();
string      substitute_environment_variables    ( const string& value );

}

#endif
