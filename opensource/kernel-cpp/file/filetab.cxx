//#define MODULE_NAME "filetab"
//#define COPYRIGHT   "(c) SOS GmbH Berlin"
//#define AUTHOR      "Jörg Schwiemann, Joacim Zschimmer"

#include "precomp.h"
#include "../kram/sysdep.h"
#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/log.h"
#include "../file/absfile.h"

namespace sos {

//-------------------------------------------------------------------------------------------static

static long                     initialized                 = false;

//-------------------------------------------------------------------------------------------static

struct Sos_object_descr;

extern void call_for_linker( const void* );   // Damit *o eingebunden wird

#define REFERENCE_FOR_LINKER( TYPE, SYMBOL ) \
    extern TYPE SYMBOL;                      \
    call_for_linker( &SYMBOL );

// Initialisierung der Filetypen (s.anyfile.cpp und filetab.cpp)
void init_file_types()
{
    if( initialized )  return;
    initialized = true;

    REFERENCE_FOR_LINKER( const Abs_file_type&, alias_file_type         );  // alias.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, jdbc_file_type          );  // jdbc.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, jmail_file_type         );  // jmail_file.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, licence_key_file_type   );  // licenceg.cxx
#ifdef SYSTEM_ODBC
    REFERENCE_FOR_LINKER( const Abs_file_type&, odbc_file_type          );  // odbc.cxx
#endif
    REFERENCE_FOR_LINKER( const Abs_file_type&, sossql_file_type        );  // sossqlfl.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, sql_file_type           );  // sqlfile.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, std_file_type           );  // stdfile.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, zlib_file_type          );  // zlibfile.cxx
}

} //namespace sos
