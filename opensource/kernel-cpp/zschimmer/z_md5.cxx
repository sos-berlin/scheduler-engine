// $Id: z_md5.cxx 13242 2008-01-01 20:36:00Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "zschimmer.h"
#include "z_md5.h"
#include "log.h"

namespace zschimmer {

using namespace std;
using namespace zschimmer::third_party::md5;

//-------------------------------------------------------------------------------------------------

static Message_code_text error_codes[] =
{
    { "Z-MD5-001", "MD5 sum does not consist of 32 hexadecimal digits: '$1'" },
    { "Z-MD5-002", "Invalid MD5 sum: $1" },
    { NULL }
};

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( z_md5 )
{
    add_message_code_texts( error_codes ); 
}

//-------------------------------------------------------------------------------------Md5::set_hex

void Md5::set_hex( const string& hex_md5_sum )
{
    if( hex_md5_sum.length() != 2*md5_sum_byte_count )  z::throw_xc( "Z-MD5-001", hex_md5_sum );

    try
    {
        for( size_t i = 0; i < sizeof _bytes; i++ )
        {
            _bytes[ i ] = (Byte)( hex_to_digit( hex_md5_sum[ 2*i ] ) * 16 + hex_to_digit( hex_md5_sum[ 2*i+1 ] ) );
        }
    }
    catch( exception& x )  { throw_xc( "Z-MD5-002", x ); }
}

//----------------------------------------------------------------------------------------------md5

Md5 md5( const io::Byte_sequence& seq )
{
    Md5_calculator md5_sum_calculator;
    md5_sum_calculator.add( seq );
    return md5_sum_calculator.md5();
}

//-------------------------------------------------------------------Md5_calculator::Md5_calculator

Md5_calculator::Md5_calculator()
:
    _finished(false)
{
    MD5Init( &_md5_context );
}

//------------------------------------------------------------------------------Md5_calculator::add
    
void Md5_calculator::add( const io::Byte_sequence& seq )
{
    if( _finished )  throw_xc( Z_FUNCTION, "finished" );

    MD5Update( &_md5_context, seq.ptr(), seq.length() );
}

//------------------------------------------------------------------------------Md5_calculator::md5

Md5 Md5_calculator::md5()
{
    Md5 result;

    MD5Final( result._bytes, &_md5_context );
    _finished = true;

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
