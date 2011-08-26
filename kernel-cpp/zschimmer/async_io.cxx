// $Id: async_io.cxx 13322 2008-01-27 15:27:19Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "zschimmer.h"
#include "async_io.h"
#include "log.h"

namespace zschimmer {
namespace io {

using namespace ::std;



Async_output_stream::~Async_output_stream()
{
}

Filter_async_output_stream::~Filter_async_output_stream()
{
}

int Filter_async_output_stream::try_write_bytes( const Byte_sequence& )
{
    return 0;   // ??
}

bool Filter_async_output_stream::try_flush()
{
    return false;   // ??
}

//--------------------------------------------------------Buffered_async_output_stream::write_bytes

void Buffered_async_output_stream::write_bytes( const Byte_sequence& s )
{
    try_flush();

    if( _string_list.is_empty() )
    {
        size_t written = _async_output_stream->try_write_bytes( s );

        if( written < s.length() )
        {
            _string_list.append( string( (const char*)s.ptr() + written, s.length() - written ) );
        }
    }
}

//----------------------------------------------------------Buffered_async_output_stream::try_flush

bool Buffered_async_output_stream::try_flush()
{
    while( !_string_list.is_empty() )
    {
        Byte_sequence byte_sequence = _string_list.next_byte_sequence();
        size_t written = _async_output_stream->try_write_bytes( byte_sequence );
        size_t length  = byte_sequence.length();
        _string_list.eat( written );

        if( written < length )  break;
    }

    return _string_list.is_empty();
}

//-------------------------------------------------------------------------------------------------

} //namespace io
} //namespace zschimmer
