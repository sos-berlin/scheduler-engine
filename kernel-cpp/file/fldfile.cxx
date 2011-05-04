// fldfile.cxx
#include "precomp.h"
#if 0

#include <sos.h>
#include <sosfield.h>


#include <fldfile.h>


struct Field_file : Abs_file
{
    static void                     print_field_names( ostream* s, const Field_type& record_type,
                                                       const char* name = "", const char* separator = ", "
                                                     )
 protected:
    // Methoden zum Zugriff ???
    const Field_type*               _record_type_ptr;
    const Field_descr*              _key_descr_ptr;
};


//------------------------------------------------------------------------- print_name

static void print_name( ostream* s, const char* n )
{
    // Ersetzen von Minuszeichen in Unterstriche (Konvention ?)
    char c;
    while( c = *n++ )  s->put( (char)( c == '-' ? '_' : c ) );
}


// --------------------------------------------------------------- Field_file::print_field_names

void Field_file::print_field_names( ostream* s, const Field_type& record_type, const char* name, const char* separator  )
{
    if ( record_type.field_count() == 0 ) {
        if ( name[0] == 0 ) throw Xc( "print_field_names" );
        print_name( s, name );
    } else
    {
        for( int i = 0; i < record_type.field_count(); i++ )
        {
            const Field_descr& field_descr = record_type.named_field_descr( i );

            if( i > 0 )  *s << separator; // ", "
            print_field_names( s, field_descr.type(), field_descr.name() );
        }
    }
}


#endif
