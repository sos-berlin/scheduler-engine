//#define MODULE_NAME "recfile"
// recfile.cpp
// 15. 3.92                                             (c) Joacim Zschimmer

#include "precomp.h"

#include <stdio.h>      //sprintf
#include "../kram/sysdep.h"


#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../kram/sos.h"
#include "../kram/log.h"

#include "../file/recfile.h"

namespace sos {
using namespace std;


//-----------------------------------------------------------Record_file_type

struct Record_file_type : Abs_file_type
{
    Record_file_type() : Abs_file_type() {};

    virtual const char* name() const { return "record"; }

    virtual Sos_ptr<Abs_file> create_base_file() const
    {
        Sos_ptr<Record_file> f = SOS_NEW_PTR( Record_file );
        return +f;
    }
};

const Record_file_type  _record_file_type;
const Abs_file_type&     record_file_type = _record_file_type;

//-----------------------------------------------------------Record_file::open

void Record_file::open( const char* filename, Open_mode open_mode, const File_spec& )
{
    //LOG( "Record_file::open(\"" << filename << "\")\n");
    f.clear();  //jz 960331
    f.open( filename, (open_mode & standard_mask) | binary );

    if (f.fail())  { f.clear(); throw_errno( errno, "iostream::read" ); }

    f.seekg( 0, ios::beg );
}

//----------------------------------------------------------Record_file::close

void Record_file::close ( Close_mode )
{
    f.seekp( 0, ios::end );
    f.close();
    if (f.fail())  { f.clear(); throw_errno( errno, "iostream::close" ); }
} // Record_file::close

// -------------------------------------------------------------------------Record_file::rewind

void Record_file::rewind( Key::Number )
{
    f.seekg( 0, ios::beg );
    if (f.fail())  { f.clear(); throw_errno( errno, "iostream::seekg" ); }
}

//------------------------------------------------------Record_file::get_record

void Record_file::get_record( Area& area )
{
    //LOG( "Record_file::get_record()\n");
    Record_length   len;
    int             c;

    // Lä„ngenfeld lesen. DasääL„ngenfeld ist variabel lang.

    _pos = f.tellg();

    len = 0;
    int s = 0;
    do {
        c = f.get();
        if (c == EOF)  break;
        len += (c & 0x7F) << s;
        s += 7;
    } while (c & 0x80);

    if (f.eof())  { f.clear(); throw_eof_error(); }

    if (len == 0) {
        area.length( 0 );
    } else {
        try {
            area.allocate_min( len );
        }
        catch(...) {
            f.seekg( len, ios::cur );
            throw;
        }
        f.read( area.char_ptr(), min ((uint4)len, (uint4)area.size()));
        area.length( f.gcount() );

        if (!f.good())  { f.clear(); throw_errno( errno, "iostream::get" ); }
    }

    _current_length = area.length();        // für update()
/*
    if (len > area.size() ) {
        f.seekg( len - area.size(), ios::cur );
        throw Too_long_error( "D320" );
    }
*/
}

//------------------------------------------------------Record_file::put_record

void Record_file::put_record( const Const_area& area )
{
    f.seekp( 0, ios::end );
    write( area );
}

//------------------------------------------------------Record_file::put_record

void Record_file::write( const Const_area& area )
{
    Record_length l;

    if ( area.length() < 0)  throw_too_long_error( "D420");

    l = area.length();
    do {
        int c = l & 0x7F;
        f.put( (char) ( c | ((l >>= 7)? 0x80 : 0) ));
    } while (l);

    f.write (area.char_ptr(), area.length() );
    if (f.fail())  { f.clear(); throw_errno( errno, "iostream::write" ); }
}

//------------------------------------------------------Record_file::insert

void Record_file::insert( const Const_area& record )
{
    put_record( record );
}

//------------------------------------------------------Record_file::update

void Record_file::update( const Const_area& record )
{
    if( record.length() != _current_length )  throw_xc( "recfile", "length error" );

    f.seekp( _pos, ios::beg );
    write( record );
}

} //namespace sos