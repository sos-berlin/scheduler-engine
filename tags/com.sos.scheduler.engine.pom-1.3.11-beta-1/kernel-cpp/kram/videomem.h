// videomem.h                                           (c) SOS GmbH Berlin

#ifndef __VIDEOMEM_H
#define __VIDEOMEM_H

#include <string.h>     // memmove (MS)
#include <memory.h>     // memmove (Borland)

#include <jzincl.h>
#include <video.h>


struct Video_memory : public Video {
    Video_memory(int dummy = 0);
    void write( Video_pos pos, char c, Video_attr attr )
    {
        _video_buffer[ pos.offset() ] = Video_char( c, attr );
    }
    void write( Video_pos pos, char *text, int len, Video_attr attr );
    void fill ( Video_pos pos, char c    , int len, Video_attr attr );
    void move (
        Subrange< int, 0, video_line_count >  ziel_zeilennr,
        Subrange< int, 0, video_line_count >  quell_zeilennr,
        Subrange< int, 0, video_line_count >  zeilenanz )  
    {
        memmove( _video_buffer + ziel_zeilennr  * video_line_length,
                 _video_buffer + quell_zeilennr * video_line_length,
                 zeilenanz * video_line_length * sizeof (Video_char) );
    }

    
  private:
    static Video_char *_video_buffer;

};

#endif
