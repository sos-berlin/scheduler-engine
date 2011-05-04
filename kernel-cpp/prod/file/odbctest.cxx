#define MODULE_NAME "odbctest"
#define COPYRIGHT   "© SOS GmbH Berlin"
#define AUTHOR      "Joacim Zschimmer"

#include <sosstrng.h>
#include <sos.h>
#include <sosfield.h>
#include <anyfile.h>
#include <msec.h>
#include <log.h>


struct Object
{
    typedef int4             Key;

                             Object( int, int );
    Key                     _number;
    int4                    _number2;
    Sos_limited_text<30>    _name;
};


inline Object::Object( int a, int b )
:
    _number ( a ),
    _number2( b )
{
}

//--------------------------------------------------------------------------------------Test_record

struct Test_record : Record_type_as< 3/*Felder*/ >
{
    typedef Named_field_descr_as< Int4_field >       Key;

    struct Key_record : Record_type_as< 1/*Felder*/ >
    {
                                                     Key_record     ();

        Key                                         _key;
    };

                                                     Test_record    ();

    Key                                             _number;
    Named_field_descr_as< Int4_field >              _number2;
    Named_field_descr_as< Sos_string_as< 30 > >     _name;
}
test_record;

//-------------------------------------------------------------------------Test_record::Test_record

Test_record::Test_record()
:
    _number  ( this, "number" , 0 ),
    _number2 ( this, "number2", 4 ),
    _name    ( this, "name"   , 8 )
{
}

//--------------------------------------------------------------Test_record::Key_record::Key_record

Test_record::Key_record::Key_record()
:
    _key( this, "number", 0 )
{
}

//-------------------------------------------------------------------------copy_field(Test_record,)

inline void copy_field( const Test_record& type, Byte* ptr, Object* object_ptr, Field_copy_direction copy_direction )
{
    COPY_EQUIV_FIELD( number               );
    COPY_EQUIV_FIELD( number2              );
    COPY_EQUIV_FIELD( name                 );
}


typedef Typed_indexed_file< Object     , Test_record,
                            Object::Key, Test_record::Key >  Test_file;

//-----------------------------------------------------------------------------------------sos_main

int sos_main( int, char** )
{
try {
    Test_file odbc_file( test_record, test_record._number );
    Object    o         ( 1, 4711 );
    Object    o2        ( 2, 42 );

    odbc_file.open( "odbc:Odbctest@DYNCTRL", File_base::out );  xc;

    odbc_file.store( o );  xc;
    odbc_file.store( o2 );  xc;

    odbc_file.close();  xc;
    
    return 0;

  exception_handler:
    SHOW_EXCEPTION( MODULE_NAME );
    return 1;
}
catch(...)
{
    throw;
}
}
