// videobios.h
//                                                       (c) Joacim Zschimmer

#ifndef __VIDEOBIOS_H
#define __VIDEOBIOS_H

#include <bios.h>

#include "video.h"


struct Video_bios : public Video {
    Video_bios(int dummy = 0)
    {
#       ifdef __BORLANDC__
            _directvideo = 0;
#       endif
    }
    void write( Video_pos pos, char c, Video_attr attr )
    {
#       ifdef __BORLANDC__
            gotoxy( pos.column0() + 1, pos.line0() + 1 );
            textattr (attr);
            putch (c);
#        else
            set_cursor( pos );

            REGS r;
            r.h.ah = 0x09;
            r.h.al = c;
            r.h.bh = 0;
            r.h.bl = attr;
            r.x.cx = 1;
            int86( 10, &r, &r );
#       endif
    }
#   ifdef __BORLANDC__
        void write( Video_pos pos, char *text, int len, Video_attr attr );
        void fill ( Video_pos pos, char c    , int len, Video_attr attr );
#   endif
    void move (
        Subrange< int, 0, video_line_count >  ziel_zeilennr,
        Subrange< int, 0, video_line_count >  quell_zeilennr,
        Subrange< int, 0, video_line_count >  zeilenanz )
    {
#       ifdef __BORLANDC__
            movetext( 1                , 1 + quell_zeilennr,
                      video_line_length, 1 + quell_zeilennr + zeilenanz - 1,
                      1                , 1 + ziel_zeilennr );
#        else
            REGS r;
            if (quell_zeilennr > ziel_zeilennr) {
                r.h.ah = 0x06;
                r.h.ch = ziel_zeilennr;
                r.h.dh = quell_zeilennr;
            } else {
                r.h.ah = 0x07;
                r.h.ch = quell_zeilennr;
                r.h.dh = ziel_zeilennr;
            }
            r.h.al = zeilenanz;
            r.h.cl = 0;
            r.h.dl = 79;
            int86( 10, &r, &r );
#       endif
    }
};

#endif
