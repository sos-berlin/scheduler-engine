// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#if 0//ndef __ZSCHIMMER_IMMUTABLE_STRING_
#define __ZSCHIMMER_IMMUTABLE_STRING_

namespace zschimmer {

//---------------------------------------------------------------------------------------Comparable
    
struct Comparable
{
    virtual                    ~Comparable                  ()                                      = 0;
    virtual int                 compare                     ( const Comparable& ) const             = 0;

    bool                        operator ==                 ( const Comparable& c ) const           { return compare( c ) == 0; }
    bool                        operator <                  ( const Comparable& c ) const           { return compare( c ) <  0; }
    bool                        operator <=                 ( const Comparable& c ) const           { return compare( c ) <= 0; }
    bool                        operator >=                 ( const Comparable& c ) const           { return compare( c ) >= 0; }
    bool                        operator >                  ( const Comparable& c ) const           { return compare( c ) >  0; }
};

//-------------------------------------------------------------------------Immutable_string_content

#ifdef Z_WINDOWS
#   pragma warning( push )
#   pragma warning( disable:4200 )   // warning C4200: nonstandard extension used : zero-sized array in struct/union
#endif

struct Immutable_string_content : Object
{
    static Immutable_string_content* new_string             ( const char* s, size_t length );

    void*                       operator new                ( size_t size, size_t string_length );
    void                        operator delete             ( void* p, size_t );

                                Immutable_string_content    ( const char* s, size_t length );

    size_t                      length                      () const                                { return _length; }
    const char*                 c_str                       () const                                { return _string; }
    int                         compare                     ( const Immutable_string_content& s )   { strcmp( _string, s._string ); }
    string                      to_string                   () const                                { return string( _string, _length ); }

  private:
                                Immutable_string_content    ( const Immutable_string_content& );    // Nicht implementiert
    Immutable_string_content&   operator=                   ( const Immutable_string_content& );    // Nicht implementiert

    size_t const               _length;
    char                       _string [ 0 ];
};

#ifdef Z_WINDOWS
#   pragma warning( pop )
#endif

//---------------------------------------------------------------------------------Immutable_string

struct Immutable_string : ptr<Immutable_string_content>, Comparable
{
    static Immutable_string_content null_content;


                                Immutable_string            ()                                      : ptr<Immutable_string_content>( &null_content ) {}
                                Immutable_string            ( const char* s, const size_t length )  : ptr<Immutable_string_content>( Immutable_string_content::new_string( s, length ) ) {}
                                Immutable_string            ( const string& s )                     : ptr<Immutable_string_content>( Immutable_string_content::new_string( s.data(), s.length() ) ) {}
    private:                    Immutable_string            ( Immutable_string_content* s )         : ptr<Immutable_string_content>( s ) {}
public:

                                operator string             () const                                { return _ptr->to_string(); }
    int                         compare                     ( const Immutable_string& s )           { return _ptr->compare( s->_ptr ); }

    size_t                      length                      ()                                      { return _ptr->length(); }
    const char*                 c_str                       ()                                      { return _ptr->c_str(); }
    int                         compare                     ( const Immutable_string& s ) const     { return _ptr->compare( *s._ptr ); }
};

//-------------------------------------------------------------------------Immutable_string_builder

struct Immutable_string_builder
{
                                Immutable_string_builder    ( size_t size )                         : _string( new( length ) Immutable_string_content( length ) ), _offset(0) {}
    Immutable_string_builder&   operator +=                 ( const string& s )                     { write( _offset, s.data(), s.length() );  _offset += s.length(); }
    void                        write                       ( size_t offset, const char* s, size_t length )  { assert( offset + length <= _string->length() );
                                                                                                           memcpy( _string->_string + offset, s, length );   }
    Immutable_string            to_immutable_string         ()                                      { Immutable_string result = _ptr; _ptr = NULL; return result; }

  private:
    Immutable_string           _string;
    size_t                     _offset;
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
