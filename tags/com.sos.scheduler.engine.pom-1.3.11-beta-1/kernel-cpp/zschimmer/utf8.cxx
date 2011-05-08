// $Id$

#include "zschimmer.h"
#include "utf8.h"
#if 0   // Siehe xml_libxml2.cxx, Utf8_string!


namespace zschimmer {


static Message_code_text error_codes[] =
{
    { "Z-UTF8-101", "Puffer für UTF8-Zeichenkette ist zu klein" },
    { "Z-UTF8-102", "Puffer für Unicode-Zeichenkette ist zu klein" },
    { "Z-UTF8-104", "Zeichen an Byte-Position $1 ist nicht in UTF-8 codiert" },
    { "Z-UTF8-105", "UTF-8-Zeichen an Byte-Position $1 ist nicht nach Unicode konvertierbar" },
    { "Z-UTF8-106", "UTF-8-Zeichen an Byte-Position $1 ist nicht nach ISO-8859-1 konvertierbar" },
    { NULL }
};


Z_INIT( z_utf8 )
{
    add_message_code_texts( error_codes ); 
}

//------------------------------------------------------------------utf8_byte_count_from_iso_8859_1

int utf8_byte_count_from_iso_8859_1( const char* iso_8859_1, int length )
{
    int extra = 0;

    const Byte* p     = (const Byte*)iso_8859_1;
    const Byte* p_end = p + length;

    while( p < p_end )  
    {
        extra += *p >> 7;
        if( *p == 0 )  extra++;                 // Besonderheit: 00 wird als C0 80 codiert, um ein Null-Byte zu vermeiden (wie Java JNI)
        p++;
    }

    return length + extra;
}

//------------------------------------------------------------------utf8_byte_count_from_iso_8859_1

int utf8_byte_count_from_iso_8859_1( const char* iso_8859_1 )
{
    int extra = 0;

    const Byte* p = (const Byte*)iso_8859_1;
    if( !p )  return 0;

    while( *p )  extra += *p >> 7;

    return p - (const Byte*)iso_8859_1 + extra;
}

//-------------------------------------------------------------------------write_iso_8859_1_as_utf8

int write_iso_8859_1_as_utf8( const char* iso_8859_1, int length, char* utf8_buffer, int buffer_size )
{
    const Byte* p     = (const Byte*)iso_8859_1;
    const Byte* p_end = p + length;
    Byte*       u     = (Byte*)utf8_buffer;
    Byte*       u_end = (Byte*)utf8_buffer + buffer_size;


/*
    if( buffer_size >= length * 2 )
    {
        while( p < p_end )  
        {
            if( *p & 0x80  ||  *p == '\0' )
            {
                *u++ = 0xC0 | ( *p >> 6 );
                *u++ = 0x80 | *p & 0x3F;
            }
            else
            {
                *u++ = *p;
            }

            p++;
        }
    }
    else
*/
    {
        while( p < p_end )  
        {
            if( *p & 0x80  ||  *p == '\0' )
            {
                if( u + 1 >= u_end )  throw_xc( "Z-UTF8-101", u_end - (const Byte*)utf8_buffer );

                *u++ = 0xC0 | ( *p >> 6 );
                *u++ = 0x80 | *p & 0x3F;
            }
            else
            {
                if( u >= u_end )  throw_xc( "Z-UTF8-101", u_end - (const Byte*)utf8_buffer );
                *u++ = *p;
            }

            p++;
        }
    }

    return u - (const Byte*)utf8_buffer;
}

//--------------------------------------------------------------------------write_unicode_1_as_utf8

int write_unicode_as_utf8( const OLECHAR* unicode, int length, char* utf8_buffer, int buffer_size )
{
    const OLECHAR* p     = unicode;
    const OLECHAR* p_end = p + length;
    Byte*          u     = (Byte*)utf8_buffer;
    Byte*          u_end = (Byte*)utf8_buffer + buffer_size;

    while( p < p_end )  
    {
        if( *p >= 0x80  ||  *p == 0 )
        {
            if( u + 1 >= u_end )  throw_xc( "Z-UTF8-101", u_end - (const Byte*)utf8_buffer );

            OLECHAR o = *p;

            if( o < 0x0080 )  *u++ = (Byte)o;
            else
            if( o < 0x0800 )  *u++ = 0xC0 | ( o >>  6 ) & 0x3F,  *u++ = 0x80 | o & 0x3F;
            else
                              *u++ = 0xE0 | ( o >> 12 ) & 0x3F,  *u++ = 0x80 | ( o >> 6 ) & 0x3F,  *u++ = 0x80 | o & 0x3F;
        }
        else
        {
            if( u >= u_end )  throw_xc( "Z-UTF8-101", u_end - (const Byte*)utf8_buffer );
            *u++ = (Byte)*p;
        }

        p++;
    }

    return u - (const Byte*)utf8_buffer;
}

//-----------------------------------------------------------------------------utf8_from_iso_8859_1

string utf8_from_iso_8859_1( const char* iso_8859_1, int length )
{
    string result;
    result.reserve( length * 2 );

    const Byte* p     = (const Byte*)iso_8859_1;
    const Byte* p_end = p + length;


    while( p < p_end )  
    {
        if( *p & 0x80  ||  *p == '\0' )
        {
            result += char( 0xC0 | ( *p >> 6 ) );
            result += char( 0x80 | *p & 0x3F );
        }
        else
        {
            result += *p;
        }

        p++;
    }

    return result;
}

//--------------------------------------------------------------------------------------utf8_length

int utf8_length( const char* utf8, int byte_count )
{
    const Byte* u     = (const Byte*)utf8;
    const Byte* u_end = u + byte_count;
    int         len   = 0;

    while( u < u_end )  
    {
        if( ( *u & 0xC0 ) != 0xC0 )  len++;
        u++;
    }

    return len;
}

//--------------------------------------------------------------------------------------utf8_length

int utf8_length( const char* utf8 )
{
    const Byte* u     = (const Byte*)utf8;
    int         len   = 0;

    if( !u )  return 0;

    while( *u )  
    {
        if( ( *u & 0xC0 ) != 0xC0 )  len++;
    }

    return len;
}

//----------------------------------------------------------------------------write_utf8_as_unicode

int write_utf8_as_unicode( const char* utf8, int utf8_length, OLECHAR* unicode, int unicode_size )
{
    const Byte* u     = (const Byte*)utf8;
    const Byte* u_end = u + utf8_length;
    OLECHAR*    U     = unicode;
    OLECHAR*    U_end = U + unicode_size;


    while( u < u_end )  
    {
        if( U >= U_end )  throw_xc( "Z-UTF8-102", U_end - unicode );

        if( *u & 0x80 )
        {
            OLECHAR o = 0;

            if( !( *u & 0x40 ) )  throw_xc( "Z-UTF8-104", u - (const Byte*)utf8 );
            
            do
            {
                if( u >= u_end )  throw_xc( "Z-UTF8-104", u - (const Byte*)utf8 );
                if( !( *u & 0x80 ) )  throw_xc( "Z-UTF8-104", u - (const Byte*)utf8 );
                if( o & 0xFC00 )  throw_xc( "Z-UTF8-105", u - (const Byte*)utf8 );       // Überlauf
                // Hier sollte geprüft werden, ob es ein sparsam codiertes Zeichen ist (nur sparsame Codierungen sind erlaubt, z.B. 01, aber nicht C0 81)
                o <<= 6;
                o |= *u & 0x3F;
            }
            while( *u & 0x40 );

            *U++ = o;
        }
        else
        {
            *U++ = *u++;
        }
    }

    return U - unicode;
}

//-----------------------------------------------------------------------------iso_8859_1_from_utf8

string iso_8859_1_from_utf8( const char* utf8, int utf8_length )
{
    string      result;
    const Byte* u     = (const Byte*)utf8;
    const Byte* u_end = u + utf8_length;

    result.reserve( utf8_length );


    while( u < u_end )  
    {
        if( *u & 0x80 )
        {
            Byte o = 0;

            if( !( *u & 0x40 ) )  throw_xc( "Z-UTF8-104", u - (const Byte*)utf8 );
            
            while(1)
            {
                if( u >= u_end )  throw_xc( "Z-UTF8-104", u - (const Byte*)utf8 );
                if( !( *u & 0x80 ) )  throw_xc( "Z-UTF8-104", u - (const Byte*)utf8 );
                if( o & 0xFC )  throw_xc( "Z-UTF8-106", u - (const Byte*)utf8 );       // Überlauf
                // Hier sollte geprüft werden, ob es ein sparsam codiertes Zeichen sein (nur sparsame Codierungen sind erlaubt, z.B. 01, aber nicht C0 81)
                o <<= 6;
                o |= *u & 0x3F;
                u++;
                if( !( u[-1] & 0x40 ) )  break;
            }

            result += (char)o;
        }
        else
        {
            const Byte* u0 = u;
            while( u < u_end  &&  !( *u & 0x80 ) )  u++;
            result.append( (const char*)u0, u - u0 );
        }
    }

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer


// So macht's libxml2/encoding.c:
///**
// * UTF8Toisolat1:
// * @out:  a pointer to an array of bytes to store the result
// * @outlen:  the length of @out
// * @in:  a pointer to an array of UTF-8 chars
// * @inlen:  the length of @in
// *
// * Take a block of UTF-8 chars in and try to convert it to an ISO Latin 1
// * block of chars out.
// *
// * Returns 0 if success, -2 if the transcoding fails, or -1 otherwise
// * The value of @inlen after return is the number of octets consumed
// *     as the return value is positive, else unpredictable.
// * The value of @outlen after return is the number of ocetes consumed.
// */
//int
//UTF8Toisolat1(unsigned char* out, int *outlen,
//              const unsigned char* in, int *inlen) {
//    const unsigned char* processed = in;
//    const unsigned char* outend;
//    const unsigned char* outstart = out;
//    const unsigned char* instart = in;
//    const unsigned char* inend;
//    unsigned int c, d;
//    int trailing;
//
//    if (in == NULL) {
//        /*
//         * initialization nothing to do
//         */
//        *outlen = 0;
//        *inlen = 0;
//        return(0);
//    }
//    inend = in + (*inlen);
//    outend = out + (*outlen);
//    while (in < inend) {
//        d = *in++;
//        if      (d < 0x80)  { c= d; trailing= 0; }
//        else if (d < 0xC0) {
//            /* trailing byte in leading position */
//            *outlen = out - outstart;
//            *inlen = processed - instart;
//            return(-2);
//        } else if (d < 0xE0)  { c= d & 0x1F; trailing= 1; }
//        else if (d < 0xF0)  { c= d & 0x0F; trailing= 2; }
//        else if (d < 0xF8)  { c= d & 0x07; trailing= 3; }
//        else {
//            /* no chance for this in IsoLat1 */
//            *outlen = out - outstart;
//            *inlen = processed - instart;
//            return(-2);
//        }
//
//        if (inend - in < trailing) {
//            break;
//        } 
//
//        for ( ; trailing; trailing--) {
//            if (in >= inend)
//                break;
//            if (((d= *in++) & 0xC0) != 0x80) {
//                *outlen = out - outstart;
//                *inlen = processed - instart;
//                return(-2);
//            }
//            c <<= 6;
//            c |= d & 0x3F;
//        }
//
//        /* assertion: c is a single UTF-4 value */
//        if (c <= 0xFF) {
//            if (out >= outend)
//                break;
//            *out++ = c;
//        } else {
//            /* no chance for this in IsoLat1 */
//            *outlen = out - outstart;
//            *inlen = processed - instart;
//            return(-2);
//        }
//        processed = in;
//    }
//    *outlen = out - outstart;
//    *inlen = processed - instart;
//    return(0);
//}
//
//
///**
// * isolat1ToUTF8:
// * @out:  a pointer to an array of bytes to store the result
// * @outlen:  the length of @out
// * @in:  a pointer to an array of ISO Latin 1 chars
// * @inlen:  the length of @in
// *
// * Take a block of ISO Latin 1 chars in and try to convert it to an UTF-8
// * block of chars out.
// * Returns 0 if success, or -1 otherwise
// * The value of @inlen after return is the number of octets consumed
// *     as the return value is positive, else unpredictable.
// * The value of @outlen after return is the number of ocetes consumed.
// */
//int
//isolat1ToUTF8(unsigned char* out, int *outlen,
//              const unsigned char* in, int *inlen) {
//    unsigned char* outstart = out;
//    const unsigned char* base = in;
//    unsigned char* outend = out + *outlen;
//    const unsigned char* inend;
//    const unsigned char* instop;
//    xmlChar c = *in;
//
//    inend = in + (*inlen);
//    instop = inend;
//    
//    while (in < inend && out < outend - 1) {
//            if (c >= 0x80) {
//            *out++= ((c >>  6) & 0x1F) | 0xC0;
//            *out++= (c & 0x3F) | 0x80;
//            ++in;
//            c = *in;
//        }
//        if (instop - in > outend - out) instop = in + (outend - out); 
//        while (c < 0x80 && in < instop) {
//            *out++ =  c;
//            ++in;
//            c = *in;
//        }
//    }        
//    if (in < inend && out < outend && c < 0x80) {
//        *out++ =  c;
//        ++in;
//    }
//    *outlen = out - outstart;
//    *inlen = in - base;
//    return(0);
//}
//

// So macht's libxml2/HTMLparser.c:

///**
// * htmlCurrentChar:
// * @ctxt:  the HTML parser context
// * @len:  pointer to the length of the char read
// *
// * The current char value, if using UTF-8 this may actually span multiple
// * bytes in the input buffer. Implement the end of line normalization:
// * 2.11 End-of-Line Handling
// * If the encoding is unspecified, in the case we find an ISO-Latin-1
// * char, then the encoding converter is plugged in automatically.
// *
// * Returns the current char value and its length
// */
//
//static int
//htmlCurrentChar(xmlParserCtxtPtr ctxt, int *len) {
//    if (ctxt->instate == XML_PARSER_EOF)
//        return(0);
//
//    if (ctxt->token != 0) {
//        *len = 0;
//        return(ctxt->token);
//    }        
//    if (ctxt->charset == XML_CHAR_ENCODING_UTF8) {
//        /*
//         * We are supposed to handle UTF8, check it's valid
//         * From rfc2044: encoding of the Unicode values on UTF-8:
//         *
//         * UCS-4 range (hex.)           UTF-8 octet sequence (binary)
//         * 0000 0000-0000 007F   0xxxxxxx
//         * 0000 0080-0000 07FF   110xxxxx 10xxxxxx
//         * 0000 0800-0000 FFFF   1110xxxx 10xxxxxx 10xxxxxx 
//         *
//         * Check for the 0x110000 limit too
//         */
//        const unsigned char *cur = ctxt->input->cur;
//        unsigned char c;
//        unsigned int val;
//
//        c = *cur;
//        if (c & 0x80) {
//            if (cur[1] == 0)
//                xmlParserInputGrow(ctxt->input, INPUT_CHUNK);
//            if ((cur[1] & 0xc0) != 0x80)
//                goto encoding_error;
//            if ((c & 0xe0) == 0xe0) {
//
//                if (cur[2] == 0)
//                    xmlParserInputGrow(ctxt->input, INPUT_CHUNK);
//                if ((cur[2] & 0xc0) != 0x80)
//                    goto encoding_error;
//                if ((c & 0xf0) == 0xf0) {
//                    if (cur[3] == 0)
//                        xmlParserInputGrow(ctxt->input, INPUT_CHUNK);
//                    if (((c & 0xf8) != 0xf0) ||
//                        ((cur[3] & 0xc0) != 0x80))
//                        goto encoding_error;
//                    /* 4-byte code */
//                    *len = 4;
//                    val = (cur[0] & 0x7) << 18;
//                    val |= (cur[1] & 0x3f) << 12;
//                    val |= (cur[2] & 0x3f) << 6;
//                    val |= cur[3] & 0x3f;
//                } else {
//                  /* 3-byte code */
//                    *len = 3;
//                    val = (cur[0] & 0xf) << 12;
//                    val |= (cur[1] & 0x3f) << 6;
//                    val |= cur[2] & 0x3f;
//                }
//            } else {
//              /* 2-byte code */
//                *len = 2;
//                val = (cur[0] & 0x1f) << 6;
//                val |= cur[1] & 0x3f;
//            }
//            if (!IS_CHAR(val)) {
//                ctxt->errNo = XML_ERR_INVALID_ENCODING;
//                if ((ctxt->sax != NULL) &&
//                    (ctxt->sax->error != NULL))
//                    ctxt->sax->error(ctxt->userData, 
//                                     "libxml/Char 0x%X out of allowed range\n", val);
//                ctxt->wellFormed = 0;
//                ctxt->disableSAX = 1;
//            }    
//            return(val);
//        } else {
//            /* 1-byte code */
//            *len = 1;
//            return((int) *ctxt->input->cur);
//        }
//    }
//    /*
//     * Assume it's a fixed length encoding (1) with
//     * a compatible encoding for the ASCII set, since
//     * XML constructs only use < 128 chars
//     */
//    *len = 1;
//    if ((int) *ctxt->input->cur < 0x80)
//        return((int) *ctxt->input->cur);
//
//    /*
//     * Humm this is bad, do an automatic flow conversion
//     */
//    xmlSwitchEncoding(ctxt, XML_CHAR_ENCODING_8859_1);
//    ctxt->charset = XML_CHAR_ENCODING_UTF8;
//    return(xmlCurrentChar(ctxt, len));
//
//encoding_error:
//    /*
//     * If we detect an UTF8 error that probably mean that the
//     * input encoding didn't get properly advertized in the
//     * declaration header. Report the error and switch the encoding
//     * to ISO-Latin-1 (if you don't like this policy, just declare the
//     * encoding !)
//     */
//    ctxt->errNo = XML_ERR_INVALID_ENCODING;
//    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL)) {
//        ctxt->sax->error(ctxt->userData, 
//                         "libxml/Input is not proper UTF-8, indicate encoding !\n");
//        ctxt->sax->error(ctxt->userData, "libxml/Bytes: 0x%02X 0x%02X 0x%02X 0x%02X\n",
//                        ctxt->input->cur[0], ctxt->input->cur[1],
//                        ctxt->input->cur[2], ctxt->input->cur[3]);
//    }
//
//    ctxt->charset = XML_CHAR_ENCODING_8859_1; 
//    *len = 1;
//    return((int) *ctxt->input->cur);
//}
//


#endif
