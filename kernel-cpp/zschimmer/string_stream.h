// $Id: string_stream.h 13579 2008-06-09 08:09:33Z jz $

#ifndef __ZSCHIMMER_STRING_STREAM_
#define __ZSCHIMMER_STRING_STREAM_

namespace zschimmer {

//------------------------------------------------------------------------------------String_stream
// Beispiel: _log.info( S() << "i=" << 42 << " bool" << true );
namespace com {}

struct String_stream : std::ostringstream
{
                                String_stream           ();
                               ~String_stream           ();


  //template< typename TYPE >
  //String_stream( const TYPE& first )
  //{ 
  //    //error C4717: Rekursiv für alle Steuerelementpfade. Die Funktion verursacht einen Stapelüberlauf zur Laufzeit.
  //    *static_cast<std::ostringstream*>( this ) << first; 
  //}


    template< typename TYPE >
    String_stream& operator = ( const TYPE& first )
    { 
        clear();
        append( first );
        return *this;
    }


    template< typename TYPE >
    void append( const TYPE& value )
    { 
        using namespace com;    // Für VARIANT und BSTR
        
        *static_cast<std::ostringstream*>( this ) << value;
    }


    void append( const char* s, size_t length )
    {
        assert( (std::streamsize)length >= 0 );
        std::ostringstream::write( s, (std::streamsize)length );
    }


    template< typename TYPE >
    String_stream& operator<< ( const TYPE& value )
    { 
        using namespace com;    // Für VARIANT und BSTR
        
        append( value );
        return *this; 
    }


                                operator string         () const                                    { return to_string(); }
    string                      to_string               () const                                    { return str(); }
    bool                        empty                   ()                                          { return length() == 0; }
    size_t                      length                  ();
    void                        truncate                ( size_t );
    void                        clear                   ()                                          { truncate( 0 ); }
    friend ostream&             operator <<             ( ostream& s, const String_stream& ss )     { return s << ss.str(); }
};

typedef String_stream           S;

inline string                   operator +              ( const string &a, const String_stream& b )  { return a + b.to_string(); }
inline string                   operator +              ( const char* a  , const String_stream& b )  { return a + b.to_string(); }
inline string                   operator +              ( const String_stream& a, const string& b )  { return a.to_string() + b; }
inline string                   operator +              ( const String_stream& a, const char*   b )  { return a.to_string() + b; }

/*
inline S operator << ( const string& a, const string& b ) 
{
    return S() << a << b;
}
*/

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
