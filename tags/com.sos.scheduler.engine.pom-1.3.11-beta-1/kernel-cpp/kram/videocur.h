// videocur.h

#ifndef __VIDEOCUR_H
#define __VIDEOCUR_H

#if !defined __VIDEO_H
#   include <video.h>
#endif


struct Video_curses                     // Abstrakt
{
                                Video_curses            ();
    virtual                    ~Video_curses            (); 

    virtual void                set_cursor              ( Video_pos );
    virtual void                write                   ( Video_pos, char, Video_attr );
  //virtual void                write                   ( Video_pos, char*, int len, Video_attr );
  //virtual void                fill                    ( Video_pos, char, int len, Video_attr );
  //virtual void                move                    ( int ziel, int quelle, int zeilenanz );
  //virtual void                scroll_up               ( int oben, int unten, int anzahl );
  //virtual void                scroll_down             ( int oben, int unten, int anzahl );
    virtual void                sync                    ();

  private:
    void*                      _curses_window_ptr;  
};

#endif
