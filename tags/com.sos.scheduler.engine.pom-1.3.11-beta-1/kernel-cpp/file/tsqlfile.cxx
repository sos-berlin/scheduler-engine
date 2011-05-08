#define MODULE_NAME "tsqlfile"
#define COPYRIGHT   "© SOS GmbH Berlin"
#define AUTHOR      "Jörg Schwiemann"

#define DONT_DEFINE_TEMPLATE_COPY_FIELD 1


#include <sosstrng.h>
#include <sos.h>

#include <stdlib.h>    // wg. exit()
#include <sysxcept.h>  // wg. xmsg
#include <soslimtx.h>
#include <sosprof.h>   // Sosprofile
#include <sosopt.h>    // Sos_option_iterator
#include <sosfield.h>
#include <ebcdifld.h>
#include <sosdate.h>
#include <anyfile.h>
#include <log.h>

#if !defined SYSTEM_BORLAND
    int     _argc = 0;
    char**  _argv = 0;
#endif

struct Sql_file_test_descr
{

                                        Sql_file_test_descr() :
                                            _key(0)
                                        {
                                        }

    typedef int4  Key;   // Sql_file_test_descr::Key

    Key                                 _key;
    Sos_limited_text<30>                _name;
    Sos_date                            _date;  // Datum
};

extern const char ddmmyy [] = "ddmmyy";

struct Sql_file_test_type : Record_type_as< 3 >
{

    typedef Named_field_descr_as< /*Odbc_int4*/ Ebcdic_number_as<8> >       Key;

                                                    Sql_file_test_type ();

    Key                                                 _key;
                        /*Iso_string0_as<30+1>*/
    Named_field_descr_as< Ebcdic_string_as<30> >        _name;
    Named_field_descr_as< Ebcdic_date_as< ddmmyy > >    _date;
};

inline void copy_field( const Sql_file_test_type& type,        Byte* ptr,
                              Sql_file_test_descr* object_ptr, Field_copy_direction copy_direction )
{
    COPY_EQUIV_FIELD( key              );
    COPY_EQUIV_FIELD( name             );
    COPY_EQUIV_FIELD( date             );
}

inline Sql_file_test_type::Sql_file_test_type()
:
    _key(  this, "beta"  ),
    _name( this, "alpha" ),
    _date( this, "gamma" )
{
}

Sql_file_test_type  type;

typedef Typed_indexed_file< Sql_file_test_descr, Sql_file_test_descr::Key,
                            Sql_file_test_type,  Sql_file_test_type::Key >  Sql_file_test_file;


inline ostream& operator << ( ostream& o, Sql_file_test_descr& s )
{
    return o << "{ key=" << s._key << ", name=\'" << s._name << "\', date=\'" << s._date << "\' }";
}

int sos_main( int argc, char** argv )
{
#   if !defined SYSTEM_BORLAND
        _argc = argc;
        _argv = argv;
#   endif

#if defined SYSTEM_WIN
    log_start( "" );
#else
    log_start( "/tmp/tsqlfile.log" );
#endif

  try{
  //  try {
        Sql_file_test_descr record;

      if ( read_profile_bool( "", "js-debug", "ingres-online", true ) )
      {
        Sql_file_test_file  output( type, type._key );

        if ( !read_profile_bool( "", "js-debug", "sqlfile-local", false ) )
        {   // alte Test-Daten löschen
            Any_file sql;

            sql.open( "fs:(tcp aplsun/4101)ingres:", File_base::out );
            try {
                sql.put( Const_area( "DROP TABLE test" ) );
                sql.put( Const_area( "CREATE TABLE test ( alpha VARCHAR(30), beta NUMERIC NOT NULL UNIQUE PRIMARY KEY, gamma DATE )" ) );
                //sql.put( Const_area( "DELETE FROM test WHERE beta > 4710" ) );
            } catch(...) { sql.put( Const_area( "ROLLBACK" ) ); throw; }
            sql.put( Const_area( "COMMIT" ) );
            sql.close();
        }

        record._date.assign( "2.8.1995" );
        record._key = 4711;
        record._name.assign( "Insert-Text" );

        Sos_string filename;
        if ( read_profile_bool( "", "js-debug", "sqlfile-local", false ) )
        {
            //output.open( "sql:fs:(tcp aplsun/4101)ingres: -table huba -noblock", File_base::out );
            output.open( "sql -table huba file:z:\\tmp\\sqlfile.dat ", File_base::Open_mode(File_base::out|File_base::trunc) );
        } else {
            output.open( "sql -table test fs:(tcp aplsun/4101)ingres: -noblock", File_base::out );
        }

        // Insert
        for ( int i=0; i < 10; i++ )
        {
            record._key = record._key + i;
            output.insert( record );
        }

        // Update
        record._key = 4711;
        record._name.assign( "Update-Text" );
        for ( i=0; i < 10; i++ )
        {
            record._key = record._key + i;
            output.update( record );
        }

        // Delete
        //Sql_file_test_descr::Key key = 4711;
        //output.del( key );

        output.close();
        return 0;
      } else
      {
        // Tabbed Test
        Sql_file_test_file  input(  type, type._key );
        Sql_file_test_file  output( type, type._key );
        Bool tabbed_test = read_profile_bool( "", "js-debug", "tabbed-test", false );
        if ( tabbed_test )
        {
            input.open(  "tabbed -with-field-order u:\\js\\tabbed2.dat", File_base::Open_mode(File_base::in|File_base::seq) );
            output.open( "alias:tabbed-test", File_base::Open_mode(File_base::out|File_base::trunc) );
            //output.open( "tabbed -with-field-order fs:(tcp aplsun/4101)/usr/sos/e/tmp/tabbed.dat", File_base::Open_mode(File_base::out|File_base::trunc) );
            //output.open( "tabbed -with-field-order file:c:\\tmp\\tabbed.dat", File_base::Open_mode(File_base::out|File_base::trunc) );
        } else
        {
            input.open( "tabbed:fs:(tcp aplsun/4101)ingres:select * from test", File_base::Open_mode(File_base::in|File_base::seq) );
        }

        while (1)
        {
            try { input.get( &record ); } catch( const Eof_error& ) { break; }
            LOG( "record= " << record << endl );
            if ( tabbed_test ) output.put( record );
        }

        input.close();
        if ( tabbed_test ) output.close();
        return 0;
      }
    }
//    CATCH_AND_THROW_XC
//    }
  catch( const Xc& x )
  {
    SHOW_ERR( "Exception aufgetreten: " << x << ", code=" << x.code() );
    return -1;
  }
}

// DELCR klaut letzte Zeile (2)

#include <sosfield.inl>


