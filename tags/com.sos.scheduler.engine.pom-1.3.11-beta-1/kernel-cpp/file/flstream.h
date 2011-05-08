// flstream.h                   ©1996 SOS GmbH Berlin

#ifndef __FLSTREAM_H
#define __FLSTREAM_H

#ifndef __ANYFILE_H
#   include "anyfile.h"
#endif

namespace sos {


//---------------------------------------------------------------------------Any_file_streambuf

struct /*_CLASSTYPE Borland*/ Any_file_streambuf : std::streambuf
{
                                Any_file_streambuf      ();
                                Any_file_streambuf      ( const Any_file& );
                               ~Any_file_streambuf      ();

    void                        init                    ( const Any_file& );
    void                        open                    ( const Sos_string& filename, Any_file::Open_mode mode );
    void                        close                   ( Close_mode mode = close_normal );

    virtual int _Cdecl          sync                    ();
    virtual int _Cdecl          underflow               ();
    virtual int _Cdecl          overflow                ( int = EOF );

  protected:
    Sos_ptr<Any_file>          _any_file;
    Dynamic_area               _read_buffer;
    Dynamic_area               _write_buffer;
    Bool                       _close_it;
};

//------------------------------------------------------------------------------Any_file_stream

struct Any_file_stream : private Any_file_streambuf, public std::iostream
{
                                Any_file_stream         ();
                                Any_file_stream         ( const Any_file& );
                               ~Any_file_stream         ();

    void                        init                    ( const Any_file& file )                { Any_file_streambuf::init( file ); }
    void                        open                    ( const Sos_string& filename, Any_file::Open_mode mode )  { Any_file_streambuf::open( filename, mode ); }
    void                        close                   ( Close_mode mode = close_normal )      { Any_file_streambuf::close( mode ); }
    bool                        opened                  () const                                { return _any_file && _any_file->opened(); }
};


} //namespace sos

#endif

