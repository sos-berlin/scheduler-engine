#define MODULE_NAME "sqlcopy"
#define AUTHOR      "Andreas Püschel"
#define COPYRIGHT   "1995 (C) Sos GmbH"

#include <sysdep.h>

#include <svwin.h>
#include <sv.hxx>
#include <sysdep.hxx>

#include <sosstrng.h>
#include <sos.h>
#include <sosfield.h>
#include <anyfile.h>
#include <sosopt.h>

#if defined SYSTEM_WIN
extern HINSTANCE _hinstance;
#endif


/*

             Eingabe                            Ausgabe
     sqlcopy file:c:/tmp/test.sql               oracle:
     sqlcopy -l c:/tmp/test.lst -prefix=<file:> oracle:

     sqlcopy file:c:/tmp/test.sql               odbc: -db <DNS>
     sqlcopy -f c:/tmp/test.lst -prefix=<file:> odbc: -db <DNS>

     sqlcopy odbc: -db SAB -table <tabelle>     sql -commit-at-end -table <tabelle> oracle:
     sqlcopy odbc: -db SAB -table <tabelle>     sql -commit-at-end -table <tabelle> oracle:


*/


int sos_main( int argc, char* argv[] )
{
#if defined SYSTEM_WIN
    _hinstance = Sysdepen::GethInst();
#endif

    Sos_string  input_list;
    Sos_string  input_prefix;
    Sos_string  output_file;
    Sos_string  files[100];
    int         count = 0;

    for ( Sos_option_iterator opt( argc, argv ); !opt.end(); opt.next() )
    {
        if ( opt.with_value( 'f', "file" ) )    files[count++] = opt.value();
        else
        if ( opt.with_value( 'l', "list" ) )    input_list = opt.value();
        else
        if ( opt.with_value( 'p', "prefix" ) )  input_prefix = opt.value();
        else
        if ( opt.param() )                      output_file = opt.value();
        else
        if ( opt.flag( "?" ) || opt.flag( "h" ) )
        {
            SHOW_MSG( "Usage: sqlcopy  [ [-l <filelist>] | [-f File] ] [-p=<dateityp>]\n" 
                      "       <ausgabedatei>\n" );
            return 0;
        }
        else throw Sos_option_error( opt );
    }

    if ( input_list != "" )
    {
      try {
        Dynamic_area area( 100 );
        Any_file f;
        f.open( c_str(input_list), File_base::Open_mode( File_base::in|File_base::seq ) );
        while (1)
        {
            try { f.get( &area ); } catch( const Eof_error& ) { break; }
            if ( count >= 100 ) { SHOW_MSG( "Mehr als 100 Dateien angegeben" ) break; }
            files[count++] = as_string( area ) + ".sql" ;
        }
        f.close();
      }
      catch( const Xc& x )
      {
        SHOW_ERR( "Fehler beim Lesen der Dateiliste: " << x );
      }
    }

    if ( count==0 ) return 0; // Alle Dateien gelesen

    for ( int i=0; i<count; i++ )
    {
      try {
        Sos_string source = input_prefix + files[i];
        Sos_string dest   = output_file;
        copy_file( c_str(source), c_str(dest) );
      }
      catch ( const Xc& x ) {
        x.insert( c_str(files[i]) ); throw x; // oder weiter ???
      }
    }

    SHOW_MSG( "SQL-Dateien erfolgreich übertragen." );

    return 0;
}

