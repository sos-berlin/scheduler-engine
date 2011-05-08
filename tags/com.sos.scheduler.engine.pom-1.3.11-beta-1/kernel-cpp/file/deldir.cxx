//#define MODULE_NAME "deldir"
// deldir.cpp

#include <precomp.h>
/*

#include <sysdep.h>
#include <dos.h>
#include <dir.h>
//#include <io.h>
//#include <process.h>
#include <sos.h>
#include <dosdir.h>
#include <xception.h>
#include "strlist.h"

#include "deldir.h"

static char* is_dir( char* dirname ) {
  int slen = strlen( dirname );
  if ( dirname[slen-1] != '\\' ) {
    return 0;
  } else {
    return &dirname[slen-1];
  };
};

static void delete_file( const char* filename ) {
  if ( unlink( filename ) == -1 ) {
     raise_errno( errno );
  };

exceptions
};

static void delete_directory( const char* filename ) {
  char buf[_MAX_PATH], *ptr;
  strcpy( buf, filename );
  if ( ptr = is_dir( buf ) ) {
    *ptr = '\0';
  };
  if ( rmdir( buf ) == -1 ) {
     raise_errno( errno );
  };

exceptions
};

static int cmp_backslashes( String& s1, String& s2 ) {
   return ( s1.freq( "\\" ) < s2.freq( "\\" ) );
};


void deldir( const char* dirname ) {

   static char buf[_MAX_PATH+13], file[_MAX_PATH];
   char *ptr;
   strcpy( buf, dirname );
   strcat( buf, "\\*.* -r -f -d"  );

   Area a( file, _MAX_PATH );
   StringList liste;
   Dosdir d;

   d.open( buf, File_base::in, File_spec() );  xc;

   while(!d.eof()) {
     d.get( a ); xc;
     if ( ptr = is_dir( (char *) a.ptr()) ) {
       *ptr = '\0'; // Hack, am besten noch die letzte Position abfragen.
       liste.push( String( (char *) a.ptr() ));
     } else {
       delete_file( (char *) a.ptr() ); xc;
     };
   };

   liste.sort( cmp_backslashes ); // Sortieren nach der "Tiefe" der Dirs

   while (liste) {
     delete_directory( (const char*) liste.pop() ); xc;
   };

   delete_directory( (const char*) String( d.pwd() ) ); xc;
   d.close();

exceptions
   d.close();
};

*/