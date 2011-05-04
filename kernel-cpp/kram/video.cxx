#include "precomp.h"
//#define MODULE_NAME "video"
// video.cpp
//                                                      (c) Joacim Zschimmer

#include <stdlib.h>
#include <string.h>

#include "sos.h"
//#include <sosstore.h>
#include "video.h"

using namespace std;
namespace sos {

//------------------------------------------------------Video_pos::object_store
/*
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
*/
//------------------------------------------------------------------operator <<

ostream& operator << ( ostream& s, Video_pos pos )
{
    return s << '(' << pos.column0() << ',' << pos.line0() << ')';
}

//---------------------------------------------------------------------Video_window::Buffer::Buffer

Video::Buffer::Buffer()
{
    memset( _char_buffer, ' ', sizeof _char_buffer );
}

//------------------------------------------------------------Video::set_cursor
/*
void Video::set_cursor( Video_pos pos ) {
    #ifdef __BORLANDC__
        gotoxy( 1 + pos.column0(), 1 + pos.line0() );
     #else
        REGS r;
        r.h.ah = 0x02;
        r.h.bh = 0;
        r.h.dh = pos.line0();
        r.h.dl = pos.column0();
        int86( 10, &r, &r );
    #endif
}
*/
//-----------------------------------------------------------------Video::write

void Video::write( Video_pos pos, char* text, int len, Video_attr attr )
{
    set_cursor( pos );
    while (len--)  write( pos++, *text++, attr );
}

//------------------------------------------------------------------Video::fill

void Video::fill( Video_pos pos, char c, int len, Video_attr attr )
{
    char filler [ video_line_length ];

    memset( filler, c, sizeof filler );

    while( len > 0 ) {
        int l = min( len, video_line_length - pos.column0() );
        write( pos, filler, l, attr );
        len -= l;
        pos = pos.offset() + l;
    }
}

//--------------------------------------------------------------------------------------Video::move

void Video::move( int ziel, int quelle, int zeilenanz )
{
    if( ziel < quelle ) {
        for( int i = 0; i < zeilenanz; i++ ) {
            for( int j = 0; i < video_line_length; j++ ) {
                write( Video_pos( j, ziel + i ), 
                       _buffer._char_buffer[ quelle + i * video_line_length + j ], 
                       _buffer._attr_buffer[ quelle + i * video_line_length + j ] );
            }          
        }
    }
    else
    {
        for( int i = zeilenanz - 1; i >= 0; i-- ) {
            for( int j = 0; i < video_line_length; j++ ) {
                write( Video_pos( j, ziel + i ), 
                       _buffer._char_buffer[ quelle + i * video_line_length + j ], 
                       _buffer._attr_buffer[ quelle + i * video_line_length + j ] );
            }          
        }
    }
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

//----------------------------------------------------------------------------------Video::gray

void Video::gray( Bool )
{
}

} //namespace sos
