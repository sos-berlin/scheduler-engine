#include "precomp.h"
//#define MODULE_NAME "SOSSYS"
//#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"


#if defined SYSTEM_UNIX && __JS_TEST
//-------------------------------------------------------------------------------------exe_directory

#include <libgen.h>

using namespace std;
namespace sos {

void exe_directory( char* dirname, const char* exe_name )
{
    char *p = NULL;
    if ( p = strchr( exe_name, '/' ) != NULL ) 
    {
      // dirname( ... )
      dirpath( exe_name, dirname )  
    } else {
      const char* env_path = getenv( "PATH" );
      char buf[5000];
      strcpy( buf, env_path );
      char* p = buf;
  
      while ( p = strchr( p, ':' ) != NULL ) // ':' => ';'
      {
        *p = ';'; p++;
      }  

      if ( char* p = pathfind( buf, exe_name, "rx" ) != NULL )
      {
        dirpath( p, dirname );
      } else {
        dirname[0] = 0;
      }
    }
}


} //namespace sos

#endif

