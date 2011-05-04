// video.h

/*
   videobor.* sind derzeit Kopien von video.* und müssen noch für Borland angepaßt werden.
   Das meiste wird gelöscht, Video heißt Video_borland etc.
*/

#ifndef __VIDEO_H
#define __VIDEO_H

#include <conio.h>

#include <jzincl.h>


struct Sos_output_store;
struct Sos_input_store;


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

    void                        object_store            ( Sos_output_store& );
    Video_pos&                  object_load             ( Sos_input_store& );

  private:
    int                        _offset;
};

struct ostream;
ostream& operator << ( ostream&, Video_pos );

struct Video;
// Funktionspointer-Typ fuer e9750_init
typedef Video* (*Video_create_func)();


struct Video                            // Abstrakt
{
    virtual void                set_cursor              ( Video_pos );
    virtual void                write                   ( Video_pos, char, Video_attr )        = 0;
    virtual void                write                   ( Video_pos, char*, int len, Video_attr );
    virtual void                fill                    ( Video_pos, char, int len, Video_attr );
    virtual void                move                    ( int ziel, int quelle, int zeilenanz ) = 0;
    virtual void                scroll_up               ( int oben, int unten, int anzahl );
    virtual void                scroll_down             ( int oben, int unten, int anzahl );
    virtual void                sync                    ();

    static Video_create_func    create_func;
};


#include <video.inl>
#endif
