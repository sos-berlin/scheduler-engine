// $Id$

#include "zschimmer.h"
#include "string_stream.h"

using namespace std;


namespace zschimmer {

//---------------------------------------------------------------------String_stream::String_stream

String_stream::String_stream()
{
}

//--------------------------------------------------------------------String_stream::~String_stream

String_stream::~String_stream()
{
}

//----------------------------------------------------------------------------String_stream::length

size_t String_stream::length()
{ 
    size_t result = tellp(); 
    return (int)result == -1? 0 : result;     // Am Ánfang liefert tellp() BADOFF
} 

//--------------------------------------------------------------------------String_stream::truncate

void String_stream::truncate( size_t length )
{ 
    seekp( length ); 
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
