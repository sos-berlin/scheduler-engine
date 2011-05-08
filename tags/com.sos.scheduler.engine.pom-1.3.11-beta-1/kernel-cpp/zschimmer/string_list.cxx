// $Id$

#include "zschimmer.h"
#include "string_list.h"

using namespace std;
using namespace zschimmer::io;


namespace zschimmer {

//------------------------------------------------------------------------------String_list::length

size_t String_list::length() const
{
    size_t result = 0;
    Z_FOR_EACH_CONST( list<string>, _list, it )  result += it->length();
    return result;
}

//---------------------------------------------------------------------------String_list::to_string

string String_list::to_string() const
{
    string result;

    result.reserve( length() );
    Z_FOR_EACH_CONST( list<string>, _list, it )  result += *it;

    return result;
}

//------------------------------------------------------------------String_list::next_char_sequence

Char_sequence String_list::next_char_sequence() const
{
    if( _list.empty() )
    {
        return Char_sequence( NULL, 0 );
    }
    else
    {
        const string* s = &*_list.begin();
        return Char_sequence( s->data() + _read_offset, s->length() - _read_offset );
    }
}

//------------------------------------------------------------------String_list::next_byte_sequence

Byte_sequence String_list::next_byte_sequence() const
{
    if( _list.empty() )
    {
        return Byte_sequence( NULL, 0 );
    }
    else
    {
        const string* s = &*_list.begin();
        return Byte_sequence( (const Byte*)s->data() + _read_offset, s->length() - _read_offset );
    }
}

//---------------------------------------------------------------------------------String_list::eat

void String_list::eat( size_t length )
{
    _read_offset += length;
    assert( _read_offset <= _list.begin()->length() );
    
    if( _read_offset == _list.begin()->length() ) 
    {
        _list.erase( _list.begin() );
        _read_offset = 0;
    }
}

//--------------------------------------------------------------------------String_list::string_eat

string String_list::string_eat( size_t len )
{
    string result;
    result.reserve( min( len, length() ) );

    while( length() )
    {
        Char_sequence seq = next_char_sequence();
        result.append( seq.ptr(), seq.length() );
        eat( seq.length() );
    }

    return result;
}

//--------------------------------------------------------------------String_list::immutable_string

//Immutable_string String_list::to_immutable_string() const
//{
//    Immutable_string_builder result ( length() );
//
//    Z_FOR_EACH_CONST( list<string>, _list, it )  result += *it;
//
//    return result.to_immutable_string();
//}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

