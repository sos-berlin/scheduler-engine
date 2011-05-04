#define MODULE_NAME "odbc2sql"
#define AUTHOR      "Jörg Schwiemann"
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


int sos_main( int argc, char* argv[] )
{

    Any_file f;
    f.open( "fs: -server tcp:aplsun/4001 file:/usr/sos/e/tmp/smb.cfg", File_base::in, File_spec() );
    Dynamic_area area(1024);
    SHOW_MSG( "Datei geöffnet" );
    f.get( &area );
    f.close();
    return 0;


#if defined SYSTEM_WIN
    _hinstance = Sysdepen::GethInst();
#endif

    Bool        field_names     = false;
    Bool        decimal_comma   = false;
    Sos_string  database;
    Sos_string  input_file;
    Sos_string  tables[100];
    int         count = 0;

    for ( Sos_option_iterator opt( argc, argv ); !opt.end(); opt.next() )
    {
        if ( opt.with_value( "db" ) )           database = opt.value();
        else
        if ( opt.with_value( 'f', "file" ) )    input_file = opt.value();
        else
        if ( opt.flag( "field-names" ) )        field_names = opt.set();
        else
        if ( opt.flag( "decimal-comma" ) )        decimal_comma = opt.set();
        else
        if ( opt.param() )                      tables[count++] = opt.value();
        else
        if ( opt.flag( "?" ) || opt.flag( "h" ) ) { SHOW_MSG( "Usage: odbc2tab -db <DSN> [-decimal-comma] [-field-names] [-f <tablelist>] [Tables ...] " ); return 0; }
        else throw Sos_option_error( opt );
    }

    if ( input_file != "" )
    {
      try {
        Dynamic_area area( 100 );
        Any_file f;
        f.open( c_str(input_file), File_base::Open_mode( File_base::in|File_base::seq ) );
        while (1)
        {
            try { f.get( &area ); } catch( const Eof_error& ) { break; }
            if ( count >= 100 ) { SHOW_MSG( "Mehr als 100 Tabellen angegeben" ) break; }
            tables[count++] = as_string( area );
        }
        f.close();
      }
      catch( const Xc& x )
      {
        SHOW_ERR( "Fehler beim Lesen der Tabellenliste: " << x );
      }
    }

    Sos_string  dbdriver_name = as_string("odbcdrv:") + database;
/*
    Sos_string  in_prefix  = as_string( "tabbed/record " ) +
                             as_string( field_names ? "-field-names " : "" ) +
                             as_string("odbc -db ") + database + as_string( " SELECT * FROM " );
    Sos_string  out_prefix = as_string("file:");
*/
    Sos_string  in_prefix  = as_string("odbc -empty-is-null -db ") + database + as_string( " SELECT * FROM " );
    Sos_string  out_prefix = as_string("tabbed " ) +
                             as_string( field_names   ? "-field-names "   : "" ) +
                             as_string( decimal_comma ? "-decimal-comma " : "" ) +
                             as_string( "file:");


    if ( count==0 ) return 0; // Alle Tabellen aus DSN ??? => odbcdrv -tables <DSN>

    Any_file    dbdriver;
    dbdriver.open( c_str(dbdriver_name), File_base::in  );

    for ( int i=0; i<count; i++ )
    {
      try {
        Sos_string source = in_prefix + tables[i];
        Sos_string dest   = out_prefix + tables[i] + as_string( ".tab" );
        copy_file( c_str(source), c_str(dest) );
      }
      catch ( const Xc& x ) {
        x.insert( c_str(tables[i]) ); throw x; // oder weiter ???
      }
    }

    dbdriver.close();
    SHOW_MSG( "Tabellen aus DSN " << database << " erfolgreich übertragen." );

    return 0;
}

