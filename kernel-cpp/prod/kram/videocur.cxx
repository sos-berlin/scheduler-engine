#ifndef SYSTEM_GNU
#define MODULE_NAME "videocur"
//                                                      (c) SOS GmbH Berlin
//                                                          Joacim Zschimmer

#include <sysdep.h>
#if defined SYSTEM_LINUX

#include <stdlib.h>
#include <string.h>

#include <sos.h>
#include <sosstore.h>

#include "videocur.h"
#include <curses.h>


//------------------------------------------------------------------curses_attr

uint4 curses_attr( Video_attr attr )
{
    return 0;
}

//---------------------------------------------------Video_curses::Video_curses

Video_curses::Video_curses()
{
    _curses_window_ptr = initscr();
    cbreak();
    noecho();

    //??:
    nonl();
    //ncurses: intrflush( stdscr, FALSE );
    //ncurses: keypad( stdscr, TRUE );

    scrollok( ((WINDOW*)_curses_window_ptr), true );
}

//--------------------------------------------------Video_curses::~Video_curses

Video_curses::~Video_curses()
{
    endwin();
}

//-----------------------------------------------------Video_curses::set_cursor

void Video_curses::set_cursor( Video_pos pos )
{
    /*::*/move( pos.line0(), pos.column0() );
}

//-----------------------------------------------------------------Video::write

void Video::write( Video_pos pos, char c, Video_attr attr )
{
    set_cursor( pos );
    addch( curses_attr( attr ) | c );
}

//-----------------------------------------------------------------Video::write
/*
void Video::write( Video_pos pos, char* text, int len, Video_attr attr )
{
    set_cursor( pos );
    addchnstr( ... );
}
*/
//------------------------------------------------------------------Video::fill
/*
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
*/
//--------------------------------------------------------------------------------------Video::move

#undef move         // curses.h
#if 0

void Video::move( int wohin, int woher, int anzahl )
{
    WINDOW* w = newwin( max( wohin, woher ) + anzahl, video_line_length, min( wohin, woher ), 0 );
    wscrl( w, anzahl );
    delwin( w );
}

#endif
//--------------------------------------------------------------------------------------Video::sync

void Video::sync()
{
    refresh();
}


#endif
#endif
