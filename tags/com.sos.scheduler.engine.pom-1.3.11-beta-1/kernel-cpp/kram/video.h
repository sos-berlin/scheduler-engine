// video.h

#ifndef __VIDEO_H
#define __VIDEO_H

//#include <sos.h>

namespace sos
{

//struct Sos_output_store;
//struct Sos_input_store;


const int video_line_length = 80;
const int video_line_count  = 25;

struct Video_attr
{
                                Video_attr              ();
                                Video_attr              ( unsigned char attr );
                                operator unsigned char  () const;

    void                        dark                    ( Bool b );
    Bool                        dark                    () const;
    void                        half_light              ( Bool b );
    Bool                        half_light              () const;
    void                        blinking                ( Bool b );
    Bool                        blinking                () const;
    void                        underlined              ( Bool b );
    Bool                        underlined              () const;
    int                         operator==              ( const Video_attr& a ) const;
    int                         operator!=              ( const Video_attr& a ) const;

    unsigned char _attr;
};

struct Video_char
{
                                Video_char              ();
                                Video_char              ( char, Video_attr );

    char                        chr                     () const;
    Video_attr                  attr                    () const;

    char                       _char;
    Video_attr                 _attr;
};


struct Video_pos
{
                                Video_pos               ();
                                Video_pos               ( int offset );
                                Video_pos               ( int column0, int line0 );
                                Video_pos               ( const Video_pos& vp );

    Video_pos                   operator++              ( int );
    int                         offset                  () const;
    int                         column0                 () const;
    int                         line0                   () const;

  //void                        object_store            ( Sos_output_store& );
  //Video_pos&                  object_load             ( Sos_input_store& );

  private:
    int                        _offset;
};

//class ostream;
ostream& operator << ( ostream&, Video_pos );

struct SOS_CLASS Video;
// Funktionspointer-Typ fuer e9750_init
typedef Video* (*Video_create_func)();


struct SOS_CLASS Video                            // Abstrakt
{
    virtual void                set_cursor              ( Video_pos )                          = 0;
    virtual void                write                   ( Video_pos, char, Video_attr )        = 0;
    virtual void                write                   ( Video_pos, char*, int len, Video_attr );
    virtual void                fill                    ( Video_pos, char, int len, Video_attr );
    virtual void                move                    ( int ziel, int quelle, int zeilenanz );
    virtual void                scroll_up               ( int oben, int unten, int anzahl );
    virtual void                scroll_down             ( int oben, int unten, int anzahl );
    virtual void                sync                    ();
    virtual void                gray                    ( Bool );       // Bild abdunkeln

    static Video_create_func    create_func;

  protected:
    struct Buffer
    {
                                Buffer          ();
        Area                    char_line       ( int line_no );
        Area                    attr_line       ( int line_no );
        char&                   chr             ( Video_pos );
        Video_attr&             attr            ( Video_pos );

    //private:
        char                   _char_buffer [video_line_length * video_line_count + 1];  // Eins mehr für 0-Bytes.
        Video_attr             _attr_buffer [video_line_length * video_line_count + 1];  // Eins mehr für Endemarke
    };

    Buffer                     _buffer;
};

} //namespace sos

#include "video.inl"
#endif
