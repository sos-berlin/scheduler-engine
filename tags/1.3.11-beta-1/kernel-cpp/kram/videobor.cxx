#define MODULE_NAME "video"
// video.cpp
//                                                      (c) Joacim Zschimmer
#if 0

#pragma hdrstop

#include <stdlib.h>
#include <string.h>

#if defined __BORLANDC__
    //#include <memory.h>
#   include <conio.h>
#   include <bios.h>    // biosequip
#endif

#include <sos.h>
#include <sosstore.h>
#include "video.h"

//------------------------------------------------------Video_pos::object_store

void Video_pos::object_store( Sos_output_store& s )
{
    s.write_int1( _offset / video_line_length );
    s.write_int1( _offset % video_line_length );
}

//-------------------------------------------------------Video_pos::object_load

Video_pos& Video_pos::object_load( Sos_input_store& s )
{
    _offset = s.read_int1();
    _offset = _offset * video_line_length + s.read_int1();
    return *this;
}

//----------------------------------------------------------------------operator <<

ostream& operator << ( ostream& s, Video_pos pos )
{
    return s << '(' << pos.column0() << ',' << pos.line0() << ')';
}

//------------------------------------------------------------Video::set_cursor

void Video::set_cursor( Video_pos pos ) {
#   ifdef __BORLANDC__
        gotoxy( 1 + pos.column0(), 1 + pos.line0() );
#    else
        REGS r;
        r.h.ah = 0x02;
        r.h.bh = 0;
        r.h.dh = pos.line0();
        r.h.dl = pos.column0();
        int86( 10, &r, &r );
#   endif
}

//-----------------------------------------------------------------Video::write

void Video::write(
    Video_pos   pos, 
    char*       text, 
    int         len, 
    Video_attr  attr 
)
{
    set_cursor( pos );
    while (len--)  write( pos++, *text++, attr );
}

//------------------------------------------------------------------Video::fill

void Video::fill(
    Video_pos   pos, 
    char        c,
    int         len,
    Video_attr  attr 
)
{
    char filler [ video_line_length ];

    memset( filler, c, sizeof filler );

    while( len > 0 ) {
        int l = min( len, video_line_length - pos.column0() );
        write( pos, filler, l, attr );
        len -= l;
        pos = pos.offset() + l;
    }

    //? set_cursor( pos );
    //while (len--)  write( pos++, c, attr );
}

//---------------------------------------------------------------------------------Video::scroll_up

void Video::scroll_up( int oben, int unten, int anzahl )
{
    move( oben, oben + anzahl, unten - oben + 1 - anzahl );
}

//-------------------------------------------------------------------------------Video::scroll_down

void Video::scroll_down( int oben, int unten, int anzahl )
{
    move( oben + anzahl, oben, unten - oben + 1 - anzahl );
}

//--------------------------------------------------------------------------------------Video::sync

void Video::sync()
{
}

#endif
