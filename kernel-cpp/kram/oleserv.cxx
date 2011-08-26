// $Id: oleserv.cxx 11394 2005-04-03 08:30:29Z jz $

// oleserv.cxx                                (C)1997 SOS GmbH Berlin
//                                            Joacim Zschimmer

#include "precomp.h"
#include "sos.h"
#include "sosfield.h"
#include "sysxcept.h"
#include "sosprof.h"
#include "log.h"
//#include "olestd.h"

#include "thread_semaphore.h"
#include "com.h"
#include "com_simple_standards.h"
#include "olereg.h"
#include "oleserv.h"


using namespace std;

namespace sos {


extern const bool   _dll;

Sos_string  module_filename     ();
Sos_string  directory_of_path   ( const Sos_string& );
Sos_string  basename_of_path    ( const Sos_string& );


//DEFINE_GUID( IID_NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );

//-------------------------------------------------------------------------------------------------


} //namespace sos
