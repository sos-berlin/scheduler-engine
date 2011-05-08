// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#if 0

#include "zschimmer.h"
#include "immutable_string.h"

using namespace std;


namespace zschimmer {

//-----------------------------------------------------------Immutable_string_content::operator new
    
void* Immutable_string_content::operator new( size_t size, size_t string_length )   
{ 
    return z_malloc( size + string_length, "Immutable_string", 0 ); 
}

//--------------------------------------------------------Immutable_string_content::operator delete

void Immutable_string_content::operator delete( void* p, size_t )
{ 
    return z_free( p ); 
}

//-------------------------------------------------------------Immutable_string_content::new_string

Immutable_string_content* Immutable_string_content::new_string( const char* s, size_t length )
{
    Immutable_string_content* result = NULL;

    result = new( length ) Immutable_string_content( s, length );
    memcpy( result->_string, s, sizeof (char) * length );

    return result;
}

//-----------------------------------------------Immutable_string_content::Immutable_string_content

Immutable_string_content::Immutable_string_content( const char* s, size_t length )
: 
    _length( length )
{
    _string[ length ] = char();
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
