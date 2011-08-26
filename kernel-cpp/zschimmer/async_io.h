// $Id: async_io.h 13670 2008-09-26 15:09:47Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __ZSCHIMMER_ASYNC_IO_H
#define __ZSCHIMMER_ASYNC_IO_H

#include "z_io.h"
#include "string_list.h"

namespace zschimmer {
namespace io {

//-------------------------------------------------------------------------------------------------
    
//struct Notification_source
//{
//    virtual                    ~Notification_source         ()                                      = 0;
//};

//---------------------------------------------------------------------------------------Notifyable

struct Notifyable
{
    virtual                    ~Notifyable                  ()                                      = 0;
    virtual void                notify                      ( IUnknown* )                           = 0;
};

//------------------------------------------------------------------------------------Output_stream

struct Async_output_stream : IUnknown
{
    virtual                    ~Async_output_stream         ();                                     // Für AIX,  gcc 4.2.2;

    int                         try_write_bytes             ( const string& bytes )                 { return try_write_bytes( Byte_sequence( &bytes ) ); }
    virtual int                 try_write_bytes             ( const Byte_sequence& )                = 0;
    virtual void                request_notification        ( Notifyable* )                         = 0;
    virtual bool                try_flush                   ()                                      = 0;
};

//---------------------------------------------------------------------Filter_async_output_stream

struct Filter_async_output_stream : simple_iunknown_implementation< Async_output_stream >
{
                                Filter_async_output_stream( Async_output_stream* os )             : _async_output_stream(os) {}
    virtual                    ~Filter_async_output_stream();

    virtual void                close                       ()                                    {}
    virtual int                 try_write_bytes             ( const Byte_sequence& );
    virtual bool                try_flush                   ();


  protected:
    ptr<Async_output_stream>   _async_output_stream;
};

//---------------------------------------------------------------------Buffered_async_output_stream

struct Buffered_async_output_stream : Filter_async_output_stream, Output_stream
{
                                Buffered_async_output_stream( Async_output_stream* o )              : Filter_async_output_stream( o ) {}
    virtual                    ~Buffered_async_output_stream()                                      {}

    // Async_output_stream
    int                         try_write_bytes             ( const Byte_sequence& s )              { write_bytes( s );  return s.length(); }
    bool                        try_flush                   ();

    void                        write_bytes                 ( const Byte_sequence& );

  private:
    String_list                _string_list;
};

//-------------------------------------------------------------------------------Async_input_stream

struct Async_input_stream : IUnknown
{
    virtual                    ~Async_input_stream          ()                                      = 0;

    virtual string              read_bytes                  ( size_t maximum )                      = 0;
    virtual bool                eof                         ()                                      = 0;
};

//-------------------------------------------------------------------------------------------------

} //namespace io
} //namespace zschimmer

#endif
