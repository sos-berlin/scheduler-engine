// $Id$

#include "zschimmer.h"
#include "z_com.h"
#include "charset.h"
#include "log.h"
#include <ios>

using namespace zschimmer::com;

namespace zschimmer {

//-------------------------------------------------------------------------------------------------
/*
static Message_code_text error_codes[] =
{
    { "Z-CHARSET-001", "Unbekannte Zeichen-Codierung: $1" },
    { "Z-CHARSET-002", "Zeichen-Codierung ist nicht angegeben" },
    { NULL }
};
*/
extern Message_code_text charset_messages[];

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( charset )
{
    add_message_code_texts( charset_messages ); 
}

//--------------------------------------------------------------Charset::Encoded_to_bstr
/*    
HRESULT Charset::Encoded_to_bstr( const string& encoded, BSTR* result ) const
{
    HRESULT hr = S_OK;

    int wchar_count = wchar_count_of_encoded( encoded );
    if( wchar_count == 0 )
    {
        *result = NULL;
    }
    else
    {
        Bstr bstr ( (OLECHAR*)NULL, wchar_count );

        hr = Encoded_to_wchar( encoded, (wchar_t)*bstr, wchar_count );
        bstr[ wchar_count ] = 0;

        if( !FAILED(hr) )  *result = bstr.take();
    }
}
*/
//-----------------------------------------------------------------------Charset::encoded_from_bstr

string Charset::encoded_from_bstr( const BSTR bstr ) const
{
    string result;

    HRESULT hr = Bstr_to_encoded( bstr, &result );
    if( FAILED( hr ) )  throw_com( hr, "Bstr_to_encoded", name() );

    return result;
}

//----------------------------------------------------------------------------------Windows_charset

struct Windows_charset : Charset
{
    Windows_charset( const string& name, int codepage )
    :
        _name(name),
        _codepage(codepage)
    {   
    }

    //------------------------------------------------------------------------Windows_charset::name
    
    string name() const
    { 
        return _name; 
    }


    //------------------------------------------------------Windows_charset::wchar_count_of_encoded
    /*
    int wchar_count_of_encoded( const string& encoded )
    {
        if( encoded.length() == 0 )  return 0;

        int wchar_count = MultiByteToWideChar( _codepage, 0, encoded.data(), encoded.length(), NULL, 0 );
        if( wchar_count == 0 )  return E_INVALIDARG;

        return wchar_count;
    }
    */
    //-------------------------------------------------------------Windows_charset::Encoded_to_bstr
        
    HRESULT Encoded_to_bstr( const string& encoded, BSTR* result ) const
    {
        *result = NULL;

        if( encoded.length() > 0 )
        {
            DWORD flags = 0; // Muss 0 sein für UTF-8 u.a   WC_NO_BEST_FIT_CHARS
            size_t wchar_count = MultiByteToWideChar( _codepage, flags, encoded.data(), encoded.length(), NULL, 0 );
            if( wchar_count == 0 )
            {
                int error = GetLastError();
                Z_LOG( "ERROR in " << Z_FUNCTION << ": MultiByteToWideChar() ==> " << hex_from_int( error ) << " " << get_mswin_msg_text( error ) << "\n" );
                return HRESULT_FROM_WIN32( error );
            }

            *result = SysAllocStringLen( (OLECHAR*)NULL, wchar_count );
            if( !*result )  return E_OUTOFMEMORY;

            int c = MultiByteToWideChar( _codepage, 0, encoded.data(), encoded.length(), *result, wchar_count );
            if( !c )  
            {
                //if( GetLastError() == ERROR_INSUFFICIENT_BUFFER )  return E_OUTOFMEMORY;
                return HRESULT_FROM_WIN32( GetLastError() );  //return E_INVALIDARG;
            }

            (*result)[ wchar_count ] = L'\0';
        }

        return S_OK;
    }

    //------------------------------------------------------------Windows_charset::Encoded_to_wchar
    
    HRESULT Encoded_to_olechar( const string& encoded, OLECHAR* result, size_t size ) const
    {
        HRESULT hr = S_OK;

        if( encoded.length() > 0 )
        {
            DWORD flags = 0; // Muss 0 sein für UTF-8 u.a   WC_NO_BEST_FIT_CHARS
            size_t wchar_count = MultiByteToWideChar( _codepage, flags, encoded.data(), encoded.length(), NULL, 0 );
            if( wchar_count == 0 )
            {
                int error = GetLastError();
                Z_LOG( "ERROR in " << Z_FUNCTION << ": MultiByteToWideChar() ==> " << hex_from_int( error ) << " " << get_mswin_msg_text( error ) << "\n" );
                return HRESULT_FROM_WIN32( error );
            }

            if( wchar_count > size )  return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );

            int c = MultiByteToWideChar( CP_ACP, 0, encoded.data(), encoded.length(), result, wchar_count );
            if( !c )  
            {
                //if( GetLastError() == ERROR_INSUFFICIENT_BUFFER )  return E_OUTOFMEMORY;
                return HRESULT_FROM_WIN32( GetLastError() );
            }
        }

        return hr;

        /*
        if( wchar_count_of_encoded( encoded ) > size )  return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );

        wchar_t* r = result
        for( const char* e = encoded.data(), e_end = e + encoded.length(); e < e_end; e++ )  *r++ = e;

        return S_OK;
        */
    }

    //------------------------------------------------------------Windows_charset::Wchar_to_encoded
    
    HRESULT Olechar_to_encoded( const OLECHAR* wstr, size_t length, string* encoded_result ) const
    {
        HRESULT hr = S_OK;

        if( length == 0 )
        {
            encoded_result->erase();
        }
        else
        {
            size_t encoded_length = WideCharToMultiByte( _codepage, 0, wstr, length, NULL, 0, NULL, NULL );
            if( encoded_length == 0 )
            {
                int error = GetLastError();
                Z_LOG( "ERROR in " << Z_FUNCTION << ": MultiByteToWideChar() ==> " << hex_from_int( error ) << " " << get_mswin_msg_text( error ) << "\n" );
                return HRESULT_FROM_WIN32( error );
            }

            encoded_result->resize( encoded_length );

            size_t ret = WideCharToMultiByte( _codepage, 0, wstr, length, &(*encoded_result)[0], encoded_length, NULL, NULL );
            if( ret == 0 )  hr = HRESULT_FROM_WIN32( GetLastError() );
/*
            char  local_buffer [ 200+1 ];
            char* encoded_buffer = local_buffer;
            
            size_t encoded_length = WideCharToMultiByte( _codepage, 0, wstr, length, NULL, 0, NULL, NULL );
            if( encoded_length == 0 )  return HRESULT_FROM_WIN32( GetLastError() );

            result->reserve( encoded_length );

            if( encoded_length > NO_OF( local_buffer ) - 1 )
            {
                encoded_buffer = (char*)malloc( (encoded_length+1) * sizeof (char) );
                if( !encoded_buffer )  return E_OUTOFMEMORY;
            }

            size_t ret = WideCharToMultiByte( _codepage, 0, wstr, length, encoded_buffer, encoded_length, NULL, NULL );
            
            if( ret )  *result = string( encoded_buffer, length );
                 else  hr = HRESULT_FROM_WIN32( GetLastError() );
            
            if( encoded_buffer != local_buffer )  free( encoded_buffer );
*/
        }

        return hr;
    }

    //---------------------------------------------------------------------------------------------


    /*
    size_t utf8_length_of_encoded( const Byte* encoded, size_t length ) const
    {
        int result = 0;
        for( const Byte* p = encoded, p_end = p + length; p < p_end; p++ )  result += 1 + ( *p >> 7 );
        return result;
    }


    string utf8_from_encoded( const Byte* encoded, size_t length ) const
    {
        string result;
        result.reserve( utf8_length_of_encoded( encoded, length ) );

        for( const Byte* p = encoded, p_end = p + length; p < p_end; p++ )
        {
            char c = *p;
            if( c < 0x80 )  result += c;
                      else  result += 0xC0 + ( c << 6 ),  result += 0x80 + ( c & 0x3F );
        }            

        return result;
    }


    size_t encoded_length_of_utf8( const string& utf8 )
    {

    }


    int get_utf8_char( const char* utf8, int limit )
    {
        const char* u     = u_end;
        const char* u_end = u + limit;
        
        if( u >= u_end )  throw_xc( Z_FUNCTION );
        int result = *u++;

        if( result >= 0x80 )
        {
            if( u >= u_end )  throw_xc( Z_FUNCTION );

            if( result == result_end )  throw_xc( Z_FUNCTION );
            *result++ = (char)c;
        }
        
        return result;
    }

    size_t utf8_to_encoded( const string& utf8, Byte* result, size_t size ) const 
    {
        const Byte* result_end = result + size;

        for( const char* u = utf8.c_str(); *u; )
        {
            int c = *u++;

            if( c < 0x80 )
            {
                if( result == result_end )  throw_xc( Z_FUNCTION );
                *result++ = (char)c;
            }
            else
            if( *u < 
        }

        if( u != utf8.data() + utf.length() )  throw_xc( Z_FUNCTION );
    }


    string iso8859_1_from_encoded( const Byte* p, size_t length ) const
    {
        // 0x80-0x9F sind ungültig!
        return string( p, length );
    }
    */


    string                     _name;
    int                        _codepage;
};

//--------------------------------------------------------------------------------------------const

const Windows_charset           iso8859_1_charset    ( "ISO-8859-1", windows_codepage_iso_8859_1 );
const Windows_charset           utf8_charset         ( "UTF-8"     , CP_UTF8 );

const Charset* charsets[] = 
{
    &iso8859_1_charset,

#   ifdef Z_WINDOWS
        &utf8_charset,
#   endif

    NULL
};

//---------------------------------------------------------------------Charset::for_name

const Charset* Charset::for_name( const string& name )
{
    if( name == "" )  throw_xc( "Z-CHARSET-002" );

    string uname = ucase( name );

    for( const Charset** e = charsets; *e; e++ )
    {
        if( (*e)->name() == uname )  return *e;
    }

    throw_xc( "Z-CHARSET-001", name );
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
