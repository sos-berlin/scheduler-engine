// $Id$

#ifndef __INCLUDE_SOS_ENV
#define __INCLUDE_SOS_ENV

namespace sos {

void        set_environment_from_sos_ini_once   ();
void        clear_environment_from_sos_ini      ();
void        set_environment_from_sos_ini        ();
string      substitute_environment_variables    ( const string& value );

}

#endif
