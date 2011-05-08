#if 0

#define MODULE_NAME "delphtyp"
#define COPYRIGHT   "©1995 SOS GmbH Berlin"
#define AUTHOR      "Joacim Zschimmer"

#include <sos.h>
#include <sosfield.h>
#include <delphtyp.h>

//---------------------------------------------------------------------------------------static

Delphi_string_type delphi_string_type;

//--------------------------------------------------------------------Delphi_string_type::print

void Delphi_string_type::print( const Byte* p, ostream* s, const Text_format& f ) const
{
    s->write( p + 1, *p );
}

//--------------------------------------------------------------------Delphi_string_type::input

void Delphi_string_type::input( Byte* p, istream* s, const Text_format& f ) const
{
    s->read( p + 1, _size );
    *p = s->gcount();

    if( s->peek() != EOF ) {
        char c [30];
        s->read( c, sizeof c );
        Syntax_error x ( "SOS-1196" );
        x.insert( c, s->gcount() );
        throw x;
    }
}




