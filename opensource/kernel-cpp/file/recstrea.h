// recstrea.h
//                                                      Joacim Zschimmer

#ifndef __RECSTREA_H
#define __RECSTREA_H

#ifndef __AREA_H
#   include <area.h>
#endif

#ifndef __STREAM_H
#   include <stream.h>
#endif


#define DECLARE_RECORD_STREAM_OPERATIONS(Type)                           \
    DECLARE_INPUT_STREAM_OPERATIONS( Type )                              \
    DECLARE_OUTPUT_STREAM_OPERATIONS( Type )


struct __huge Record_stream;

#if !defined _CLASSTYPE      // _CLASSTYPE wird von Borland C++ 4.0 verwendet
#   define _CLASSTYPE
#endif

class _CLASSTYPE Record_streambuf : public streambuf
{
  public:
    Record_streambuf( Record_stream _FAR *, uint max_record_size = 2048  );
    ~Record_streambuf();

    virtual int _Cdecl sync();
    virtual int _Cdecl underflow();
    virtual int _Cdecl overflow( int = EOF );

  private:
    const int                _buffer_size;
    Dyn_area< char >         _buffer;
    Record_stream* const     _f;
    Bool                     _get_next_record;
};


struct __huge Record_stream : Streambuf_stream
{
    Record_stream();
    ~Record_stream() {}

    unsigned int  skip_record        ();                 // ???

  protected:
    virtual void put_record( const Const_area& ) = 0;
    virtual void get_record( Area&             ) = 0;
    friend class Record_streambuf;

  private:
    Record_streambuf       _record_streambuf;
};


#include "recstrea.inl"
#endif

