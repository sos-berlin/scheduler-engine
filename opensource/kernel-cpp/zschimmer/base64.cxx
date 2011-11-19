// $Id: base64.cxx 13256 2008-01-02 23:07:53Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com
// Grundlage ist der Code von Michael Collard, iinet.net.au
// Siehe auch http://www.w3.org/Library/src/HTUU.c

#include "zschimmer.h"
#include "base64.h"
#include "z_io.h"
#include "log.h"

namespace zschimmer {

using namespace ::std;

//-------------------------------------------------------------------------------------------------

static Message_code_text error_codes[] =
{
    { "Z-BASE64-001", "Invalid base64 encoded string, near position $1" },
    { NULL }
};

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( z_base64 )
{
    add_message_code_texts( error_codes ); 
}

//-------------------------------------------------------------------------------------------------

static const Byte pr2six[ 256 ] =
{
    /* ASCII table */                                                   //    0123456789abcdef
   222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,     // 00
   222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,     // 10
   222,222,222,222,222,222,222,222,222,222,222, 62,222,222,222, 63,     // 20            +   /
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61,222,222,222,199,222,222,     // 30  0123456789  =  
   222,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,     // 40  ABCDEFGHIJKLMNO
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,222,222,222,222,222,     // 50 OQRSTUVWXYZ
   222, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,     // 60  abcdefghijklmno
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,222,222,222,222,222,     // 70 pqrstuvwxyz
   222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,     
   222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,
   222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,     // 222 & 0x80 ==> true
   222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,
   222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,
   222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,
   222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,
   222,222,222,222,222,222,222,222,222,222,222,222,222,222,222,222
};

static const char six2pr[ 64 ] = 
{
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};

//-----------------------------------------------------------------------------------base64_decoded

string base64_decoded( const io::Char_sequence& base64_encoded )
{
    string result;

    if( base64_encoded.length() > 0 )
    {
        if( base64_encoded.length() % 4 != 0 )  throw_xc( "Z-BASE64-001", base64_encoded.length() );

        const  Byte* p         = (const Byte*) base64_encoded.ptr();
        const  Byte* p_end     = p + base64_encoded.length();
        size_t r               = 0;
        size_t expected_length = base64_encoded.length() * 6 / 8;
        
        result.resize( expected_length );


        while( p <= p_end - 2*4 )
        {
            Byte a = pr2six[ p[0] ];
            Byte b = pr2six[ p[1] ];
            Byte c = pr2six[ p[2] ];
            Byte d = pr2six[ p[3] ];
            
            if( ( a | b | c | d ) & 0x80 )  throw_xc( "Z-BASE64-001", p - (const Byte*)base64_encoded.ptr() );

            result[ r++ ] = (char)( a << 2  |  b >> 4 );
            result[ r++ ] = (char)( b << 4  |  c >> 2 );
            result[ r++ ] = (char)( c << 6  |  d      );

            p += 4;
        }

        {
            assert( p == p_end - 4 );

            Byte a = pr2six[ p[0] ];
            Byte b = pr2six[ p[1] ];
            Byte c = pr2six[ p[2] ];
            Byte d = pr2six[ p[3] ];
            
            if( a & 0x80                ||
                b & 0x80  &&  b != 199  ||   // '=' ist Füllzeichen und erlaubt
                c & 0x80  &&  c != 199  || 
                d & 0x80  &&  d != 199     )  throw_xc( "Z-BASE64-001", p - (const Byte*)base64_encoded.ptr() );

            if( b < 64 )  result[ r++ ] = (char)( a << 2  |  b >> 4 );
            if( c < 64 )  result[ r++ ] = (char)( b << 4  |  c >> 2 );
            if( d < 64 )  result[ r++ ] = (char)( c << 6  |  d      );
        }

        //p += 4;
        //if( p_end - p >= 2 )  result += (char)( pr2six[ p[0] ] << 2 | pr2six[ p[1] ] >> 4 );
        //if( p_end - p >= 3 )  result += (char)( pr2six[ p[1] ] << 4 | pr2six[ p[2] ] >> 2 );
        //if( p_end - p >= 4 )  result += (char)( pr2six[ p[2] ] << 6 | pr2six[ p[3] ]      );

        assert( r >= expected_length - 3  &&  r <= expected_length );
        result.resize( r );
    }

    return result;
}

//-----------------------------------------------------------------------------------base64_encoded

string base64_encoded( const io::Byte_sequence& seq )
{
    string      result;
    int         r               = 0;
    size_t      expected_length = ( seq.length() + 2 ) / 3 * 4;
    const Byte* p               = seq.ptr();
    const Byte* p_end           = p + seq.length();

    result.resize( expected_length );

    while( p < p_end - 2 )
    {                                                                                       // p[0]     p[1]     p[2]
        result[ r++ ] = six2pr[     p[0]          >> 2                                ];    // xxxxxx.. ........ ........
        result[ r++ ] = six2pr[ ( ( p[0] & 0x03 ) << 4 )  |  ( ( p[1] & 0xF0 ) >> 4 ) ];    // ......xx xxxx.... ........
        result[ r++ ] = six2pr[ ( ( p[1] & 0x0F ) << 2 )  |  ( ( p[2] & 0xC0 ) >> 6 ) ];    // ........ ....xxxx xx......
        result[ r++ ] = six2pr[     p[2] & 0x3f                                       ];    // ........ ........ ..xxxxxx
        p += 3;
    }

    if( p < p_end )
    {
        assert( p_end - p < 3 );
        Byte remainder [ 3 ];
        memset( remainder, '\0', sizeof remainder );
        memcpy( remainder, p, p_end - p );

        result.replace( r, 4, base64_encoded( io::Byte_sequence( remainder, sizeof remainder ) ) );

        r += 4;
        p += 3;     assert( p > p_end );    // *p ist ungültig
        result[ r-1 ] = '=';
        if( p > p_end + 1 )  result[ r-2 ] = '=';
    }

    assert( r == expected_length );
    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
