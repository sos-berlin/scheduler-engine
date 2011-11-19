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
#include <log.h>

#if defined SYSTEM_WIN
extern HINSTANCE _hinstance;
#endif



int sos_main( int argc, char* argv[] )
{
#if defined SYSTEM_WIN
    _hinstance = Sysdepen::GethInst();
    log_start( "" );
#endif

    Sos_string  database;
    Sos_string  input_file;
    Bool        commit_at_end = false;
    Bool        autocommit    = false;
    Sos_string  date_format;
    Sos_string  dialect;
    Sos_string  tables[100];
    int         count = 0;

    for ( Sos_option_iterator opt( argc, argv ); !opt.end(); opt.next() )
    {
        if ( opt.with_value( "db" ) )           database = opt.value();
        else
        if ( opt.with_value( 'f', "file" ) )    input_file = opt.value();
        else
        if ( opt.with_value( "dialect" ) )      dialect = opt.value();
        else
        if ( opt.with_value( "date-format" ) )  date_format = opt.value();
        else
        if ( opt.flag( "autocommit" ) )         autocommit = opt.set();
        else
        if ( opt.flag( "commit-at-end" ) )      commit_at_end = opt.set();
        else
        if ( opt.param() )                      tables[count++] = opt.value();
        else
        if ( opt.flag( "?" ) || opt.flag( "h" ) )
        {
            SHOW_MSG( "Usage: odbc2sql -db <DSN> [-autocommit] [-commit-at-end]\n"
                      "       [-date-format <format>] [-dialect <dialect>]\n"
                      "       [-f <tablelist>] [Tables ...]" );
            return 0;
        }
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

    if ( count==0 ) return 0; // Alle Tabellen aus DSN ??? => odbcdrv -tables <DSN>

    Sos_string  dbdriver_name = as_string("odbcdrv:") + database;
    Sos_string  in_prefix  = as_string("odbc -db ") + database + as_string( " SELECT * FROM " );
    Sos_string  out_prefix = as_string("sql ")
                             + as_string( commit_at_end ? "-commit-at-end " : "" )
                             + as_string( autocommit    ? "-autocommit "    : "" );

    if ( length(dialect) != 0 )
    {
        out_prefix += as_string( "-dialect " ) + dialect;
    }
    out_prefix += as_string( " -table ");



    Any_file    dbdriver;
    dbdriver.open( c_str(dbdriver_name), File_base::in  );

    for ( int i=0; i<count; i++ )
    {
      try {
        Sos_string source = in_prefix + tables[i];
        Sos_string dest   = out_prefix + tables[i] + as_string( " file:" ) + tables[i] + as_string( ".sql" );
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

