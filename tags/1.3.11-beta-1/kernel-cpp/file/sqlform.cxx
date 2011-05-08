#define MODULE_NAME "sqlform"
#define COPYRIGHT   "©1996 SOS GmbH Berlin"
#define AUTHOR      "Jörg Schwiemann"

#include <stdio.h>

#include <stdlib.h>
#include <errno.h>

#include <sosstrng.h>
#include <sos.h>
#include <anyfile.h>

#if defined SYSTEM_WIN
#include <sosprof.h>
#include <sosopt.h>
#include <log.h>
#endif

#include <sqlform.h>

#define EX_NOOUTPUTFILE 1
#define EX_USAGE        9

const Sos_string db_prefix = "alias:sqlform-sql-command ";

#if defined SYSTEM_WIN
int sos_main( int argc, char** argv )
{
    Sos_string output_filename;
    Sos_string formular;
    Sos_string vgnr;
    Sos_string vgnr_zaehlnr = "000";
    Sos_string sprache = "D";
    Sos_string skript_dir = read_profile_string( "", "sab formulare", "skript-dir" );

    Sos_option_iterator opt ( argc, argv );
    opt.max_params( 1 );

    for(; !opt.end(); opt.next() ) {
        if( opt.with_value( "formular"  ))  formular = opt.value();
        else
        if( opt.with_value( "vgnr"  ))      vgnr = opt.value();
        else
        if( opt.with_value( "zaehlnr"  ))   vgnr_zaehlnr = opt.value();
        else
        if( opt.with_value( "sprache"  ))   sprache = opt.value();
        else
        if( opt.with_value( "skript-dir" )) skript_dir = opt.value();
        else
        if( opt.with_value( "log"   ))      log_start( opt.value() );
        else
        if( opt.with_value( "o"     ))      output_filename = opt.value();
        else
        if( opt.param     ( 1       ))      output_filename = opt.value();
        else {
            SHOW_ERR( "usage: sqlform <params> destfile" );
            return EX_USAGE;
        }
    }

    if( empty( output_filename ) ) return EX_NOOUTPUTFILE;
    if( output_filename == "-" ) output_filename = "/dev/stdout"; /*SOS-Name*/

    Any_file db;
    db.open( db_prefix + " select * from tbvgg where 1=0", File_base::in );
    sqlform_do_select( output_filename, formular, vgnr, vgnr_zaehlnr, sprache, skript_dir );
    db.close();
    return 0;
}
#endif

void sqlform_do_select( const Sos_string& out,     const Sos_string& formular,
                        const Sos_string& vgnr,    const Sos_string& vgnr_zaehlnr,
                        const Sos_string& sprache, const Sos_string& skript_dir )
{
    Any_file output;
    Any_file selects;

    Sos_string out_filename = "program: -rs=012 ";
    out_filename += skript_dir;

    Sos_string form_filename = "concat | ";
    form_filename += skript_dir;
#if defined SYSTEM_UNIX
    out_filename += "/";
    form_filename += "/";
#else
    out_filename += "\\";
    form_filename += "\\";
#endif
    out_filename += out;
    form_filename += formular;
    form_filename += ".sql";

    output.open( out_filename, File_base::Open_mode(File_base::out|File_base::trunc) );
    selects.open( form_filename, File_base::in );

    Dynamic_area select_stmt( 8192 );
    Dynamic_area a1( 8192 );
    Dynamic_area a2( 8192 );
    Dynamic_area area;

    // Info-Satz schreiben
    sprintf( a2.char_ptr(), "%s\t%s\t%s\t%s",
             c_str(formular), c_str(vgnr), c_str(vgnr_zaehlnr), c_str(sprache) );
    output.put( a2.char_ptr() );

    while (1) {
        Sos_string input_filename = db_prefix;
        Any_file input;

        try { selects.get( &select_stmt ); } catch ( const Eof_error& ) { break; }
        // Sonderfall Ansprechpartner (Zweimal substituieren)
        // besser replace( String*, str1, str2 ) o.ä.
        Sos_string s = as_string( select_stmt );
        sprintf( a1.char_ptr(), c_str(s), c_str(vgnr), c_str(vgnr_zaehlnr), "%s", "%s" );
        a1.length( strlen(a1.char_ptr()) );
        sprintf( a2.char_ptr(), a1.char_ptr(), c_str(vgnr), c_str(vgnr_zaehlnr) );
        a2.length( strlen(a2.char_ptr()) );

        input_filename += a2.char_ptr();
        input.open( input_filename, File_base::Open_mode(File_base::in|File_base::seq) );
        while (1) {
            try { input.get( &area ); } catch ( const Eof_error& ) { break; }
            output.put(  area );
        }
        input.close();
    }
    selects.close();
    output.close();
}


