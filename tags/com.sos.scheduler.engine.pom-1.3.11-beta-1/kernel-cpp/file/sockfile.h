// sockfile.h
//                                                    (c) SOS GmbH Berlin

#ifndef __SOCKFILE_H
#define __SOCKFILE_H

#include <sosobj.h>

struct Socket_file : Abs_file
{
                                Socket_file             ();
                               ~Socket_file             ();

    static const Abs_file_type& static_file_type        ();

    void                        open                    ( const char* filename, Open_mode );
    void                        open                    ( const char* filename, Open_mode, const File_spec& );
    void                        close                   ( Close_mode = close_normal );
  //void                        flush                   ();  // flush() in Streambuf_stream bedeutet Satzende!!  Nicht überladen!!

  protected:
    virtual void                get_record              ( Area& );
    virtual void                put_record              ( const Const_area& );

  private:
    Sos_object_ptr             _socket_ptr;
    Dynamic_area               _send_buffer;
};

#endif
