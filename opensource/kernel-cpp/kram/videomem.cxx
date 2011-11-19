#include <precomp.h>
#define MODULE_NAME "videomem"
// videomem.cpp
//                                                      (c) Joacim Zschimmer

#include <sysdep.h>
#if defined SYSTEM_DOS               // Dieses Modul gilt nur für DOS-BIOS


#include <stdlib.h>
#include <memory.h>
#include <conio.h>

#include <bios.h>    // biosequip

#include <videomem.h>

//-----------------------------------------------------------------------------

Video_char* Video_memory::_video_buffer = 0;


Video_memory::Video_memory( int )
{
    if (!_video_buffer) {
        _video_buffer =
            (Video_char _far*)
            ((_bios_equiplist() & 0x0030) == 0x0030 ? 0xB0000000     // s/w
                                                    : 0xB8000000);   // Farbe
    }
}

//---------------------------------------------------------Video_memory::write

void Video_memory::write( Video_pos pos, char *text, int len, Video_attr attr )
{
    for (int i = 0; i < len; i++) {
        _video_buffer[ pos.offset() + i ] = Video_char( text[ i ], attr );
    }
}

//----------------------------------------------------------Video_memory::fill

void Video_memory::fill( Video_pos pos, char c, int len, Video_attr attr )
{
    for (int i = 0; i < len; i++) {
        _video_buffer[ pos.offset() + i ] = Video_char( c, attr );
    }
}


#endif                  // SYSTEM_DOS
