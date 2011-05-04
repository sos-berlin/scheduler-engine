// area.h                                                  (c) SOS GmbH Berlin
//  2. 1.92                                                    Joacim Zschimmer
//

#ifndef __AREA_H
#define __AREA_H

#if !defined __STRING_H  &&  !defined _STRING_H
#   include <string.h>
#endif

#if defined SYSTEM_UNIX
#   include <stddef.h>
#endif


#if defined SOS_INLINE  &&  !defined __BORLANDC__
#   define AREA_APPEND_CHAR_INLINE              /* Genriert bei Borland viel Code */
#endif

namespace sos
{

struct Dynamic_area;

//-------------------------------------------------------------------Const_area

struct Const_area
{
    inline                      Const_area      ();
    inline                      Const_area      ( const void*, unsigned int length );
    inline                      Const_area      ( const char* string );

#   if defined __SOSSTRNG_H
    Const_area      ( const Sos_string& string )                    { _ptr = (Byte*)c_str( string );  _length = ::sos::length( string ); }
#   endif

    inline unsigned int         length          () const;

    inline const void*          ptr             () const;
    inline const char*          char_ptr        () const;
    inline const Byte*          byte_ptr        () const;

    const void*                 ptr             ( int i ) const                                 { return (Byte*)ptr() + i; }
    const char*                 char_ptr        ( int i ) const                                 { return char_ptr() + i; }
    const Byte*                 byte_ptr        ( int i ) const                                 { return byte_ptr() + i; }

    int                         compare         ( const Const_area& ) const;

  protected:
    Byte*                      _ptr;
    unsigned int               _length;
};

typedef Const_area Area_const;   // FÅr js.

       ::std::ostream&          operator<<      ( ::std::ostream&, const Const_area& );
inline unsigned int             length          ( const Const_area& area )                      { return area.length(); }
inline unsigned int             position        ( const Const_area&, char, uint pos = 0 );

// Byteweiser Vergleich von links nach rechts.
// Ist ein Area gleich dem Anfang des anderen, ist der k¸rzere kleiner.
inline int  compare     ( const Const_area& a, const Const_area& b )  { return a.compare(b); }
inline Bool operator <  ( const Const_area& a, const Const_area& b )  { return compare(a,b) <  0; }
inline Bool operator <= ( const Const_area& a, const Const_area& b )  { return compare(a,b) <= 0; }
inline Bool operator == ( const Const_area& a, const Const_area& b )  { return compare(a,b) == 0; }
inline Bool operator != ( const Const_area& a, const Const_area& b )  { return compare(a,b) != 0; }
inline Bool operator >= ( const Const_area& a, const Const_area& b )  { return compare(a,b) >= 0; }
inline Bool operator >  ( const Const_area& a, const Const_area& b )  { return compare(a,b) >  0; }


#define CONST_AREA(object)  Const_area( &object, sizeof object )
#define OPERATOR_CONST_AREA operator Const_area () const { return CONST_AREA( *this ); }

#if defined __SOSSTRNG_H
    inline Sos_string           as_string       ( const Const_area& area )                      { return as_string( area.char_ptr(), area.length() ); }
  //Sos_string&                 operator =      ( String& string, const Const_area& area )      { string = as_string( area ); return string; }
#endif

//-------------------------------------------------------------------------Area

struct Area : Const_area
{
    inline                      Area            ();
    inline                      Area            ( void*, unsigned int size );

    inline void                 length          ( unsigned int );
    void                        set_length      ( unsigned int i) {length(i);}      // Zur Kompatibilit‰t

    inline unsigned int         size            () const;
    inline unsigned int         length          () const;
    inline void*                ptr             () const;
    inline char*                char_ptr        () const;
    inline Byte*                byte_ptr        () const;

    void*                       ptr             ( int i ) const                                 { return (Byte*)ptr() + i; }
    char*                       char_ptr        ( int i ) const                                 { return char_ptr() + i; }
    Byte*                       byte_ptr        ( int i ) const                                 { return byte_ptr() + i; }
    Area                        rest            ( int mehr = 0 )                                { int l = max( 0, (int)_length + mehr ); return Area( _ptr + _length, _size - l ); }

    void                        assign          ( const Const_area& area )          { assign( area.byte_ptr(), area.length() ); }
    inline void                 assign          ( const void*, unsigned int length );
    void                        assign          ( const char* string )              { assign( string, strlen( string ) ); }
    void                        append          ( const Const_area& area )          { append( area.byte_ptr(), area.length() ); }
    void                        append          ( const char* string )              { append( string, strlen( string ) ); }
    void                        append          ( char c )                          { *this += c; }
    void                        append          ( const void*, unsigned int length );

#   if defined __SOSSTRNG_H
        void                    assign          ( const Sos_string& str )           { assign( c_str( str ), ::sos::length( str ) ); }
        void                    append          ( const Sos_string& str )           { append( c_str( str ), ::sos::length( str ) ); }
        Area&                   operator +=     ( const Sos_string& str )           { append( c_str( str ), ::sos::length( str ) ); return *this; }
      // Zu viele automatische casts bei  Area = as_string( area )
      //Area&                   operator =      ( const String& string )            { assign( c_str( string ), length( string ) ); return *this; }
#   endif
    Area&                       operator +=     ( char );
    Area&                       operator +=     ( const char* string )              { append( string ); return *this; }
    void                        xlat            ( const Byte* table )               { ::sos::xlat( byte_ptr(), length(), table ); }
    void                        xlat            ( const char* table )               { xlat( (Byte*)table ); }
    void                        lower_case      ();
    void                        upper_case      ();
    void                        fill            ( char c )                          { memset( ptr(), c, length() ); }
  //void                        fill            ( char c, int len );
    void                        resize_min      ( unsigned int size )  /*Beh‰lt die Daten*/ { if( size > _size )  _resize_min( size ); }
    inline void                 allocate_min    ( unsigned int size );  // Exception, wenn size > _size, Lˆscht Daten!
    void                        allocate_length ( uint len )                        { allocate_min( len ); _length = len; }
    virtual void                check           ();

    // Ohne inline-Code:

    void                       _assign          ( const Const_area& area )          { _assign( area.byte_ptr(), area.length() ); }
    void                       _assign          ( const char* string )              { _assign( string, strlen( string ) ); }
#   if defined __SOSSTRNG_H
        void                   _assign          ( const Sos_string& str )           { _assign( c_str( str ), ::sos::length( str ) ); }
#   endif

    void                       _assign          ( const void*, unsigned int length );
    virtual void               _allocate_min    ( unsigned int size );  // Exception, wenn size > _size
    virtual void               _resize_min      ( unsigned int size );
    virtual Bool                resizable       () const                            { return false; }

  protected:
    void                        append_inline   ( char c )                          { resize_min( _length + 1 ); char_ptr()[ _length++ ] = c;  }
    void                        length_error    ( unsigned int size );

    unsigned int               _size;
};

::std::istream&                 operator >>     ( ::std::istream&, Area& );
void                            incr            ( Area* );            // Als vorzeichenlose Bin‰rzahl um eins erhˆhen
void                            decr            ( Area* );            // Als vorzeichenlose Bin‰rzahl um eins erniedrigen
inline void                     lower_case      ( Area* area )           { area->lower_case(); }
inline void                     upper_case      ( Area* area )           { area->upper_case(); }
       void                     rtrim           ( Area* );


#define AREA(object)  Area( &object, sizeof object )

//-----------------------------------------------------------------------Collectable_const_area

struct Collectable_const_area
{
  private:
    friend struct               Dynamic_area;
    friend struct               Const_area_handle;

                                Collectable_const_area  ( unsigned int size );
                               ~Collectable_const_area  ();

    static Collectable_const_area* create               ( unsigned int size );

    void*                       operator new            ( size_t size, void* ptr );
#   ifdef SYSTEM_DELETE_WITH_PARAMS
        void                    operator delete         ( void* ptr, void* );			// MSVC++6 warning C4291
#   endif
    void                        operator delete         ( void* ptr );

    static unsigned int         base_size               ()      { return sizeof (Collectable_const_area); }
    Byte*                       buffer                  ()      { return (Byte*)this + sizeof (Collectable_const_area); }

    int                        _size;                           // von buffer()
    int                        _ref_count;
  //Byte                       _buffer [ 1/*_size*/ ];
};

//----------------------------------------------------------------------------Const_area_handle

struct Const_area_handle : Const_area
{
                                Const_area_handle   ()                              {}
                                Const_area_handle   ( const Const_area& );
                                Const_area_handle   ( /*const*/ Dynamic_area& );
                                Const_area_handle   ( const Const_area_handle& );
    virtual /*Borland 4.5*/    ~Const_area_handle   ();

    Const_area_handle&          operator =          ( const Const_area_handle& );
    Const_area_handle&          operator =          ( const Const_area& );
    int                         ref_count           () const               { return _ptr? collectable_area_ptr()->_ref_count : 1; }

  private:
    friend struct               Dynamic_area;

//                              Const_area_handle   ( Collectable_const_area* );

    void                        del                 ();
    Collectable_const_area*     collectable_area_ptr() const               { return (Collectable_const_area*)( (Byte*)_ptr - Collectable_const_area::base_size() ); }
};

//---------------------------------------------------------------------------------Dynamic_area

struct Dynamic_area : Area
{
                                Dynamic_area        ();
                                Dynamic_area        ( unsigned int size );
                                Dynamic_area        ( const Dynamic_area& );
    virtual                    ~Dynamic_area        ();

    Dynamic_area&               operator =          ( const Dynamic_area& area )                { assign( area ); return *this; }
    Dynamic_area&               operator =          ( const Const_area& area )                  { assign( area ); return *this; }

    void                        allocate_min        ( unsigned int size );  // _ref_count == 1 !!!
    void                        allocate            ( unsigned int size );  // Genau size Bytes
    void                        free                ();
    int                         ref_count           () const                                    { return _ptr? collectable_area_ptr()->_ref_count : 1; }
    virtual Bool                resizable           () const                                    { return true; }
    void                        take                ( Dynamic_area* geber );   // ‹bernimmt den Puffer und leert geber.

    friend void                 exchange_dynamic_area( Dynamic_area*, Dynamic_area* );

  protected:
    void                       _resize_min          ( unsigned int size );
    void                       _allocate_min        ( unsigned int size );  // Mindestens size Bytes

  private:
    friend struct               Const_area_handle;

    Collectable_const_area*     collectable_area_ptr() const                { return (Collectable_const_area*)( (Byte*)_ptr - Collectable_const_area::base_size() ); }
};


// Borland 4.53 mag nicht friend exchange( Dynamic_area* a, Dynamic_area* b ):
// Error K:\E\PROD\KRAM\AREA.CXX 429: Body has already been defined for function 'exchange(Dynamic_area *,Dynamic_area *)'

inline void exchange( Dynamic_area* a, Dynamic_area* b )
{
    exchange_dynamic_area( a, b );
}

//-------------------------------------------------------------------------------Area_streambuf

struct /*_CLASSTYPE Borland*/ Area_streambuf : ::std::streambuf
{
                                Area_streambuf          ( Area* = NULL );
                               ~Area_streambuf          ();

    virtual int _Cdecl          sync                    ();
    virtual int _Cdecl          underflow               ();
    virtual int _Cdecl          overflow                ( int = EOF );

  protected:
    Area*                      _area;
    Dynamic_area               _buffer;                 // Falls kein Area von auﬂen ¸bergeben wird
};

//----------------------------------------------------------------------------------Area_stream

struct Area_stream : private Area_streambuf, public ::std::iostream
{
                                Area_stream             ( Area* = NULL );
                               ~Area_stream             ();

    Area*                       area                    ()                                      { return _area; }
                                operator Area&          ()                                      { return *_area; }
};

//---------------------------------------------------------------------------------------------
// Ein/Ausgabe f¸r Strings

void        write_char( char, Area* output, char quote, char quote_quote = '\\' );
void        read_char ( char* p, const Const_area& input, char quote, char quote_quote = '\\' );

void        write_string( const char*, int len, Area*, char quote, char quote_quote = '\\' );
inline void write_string( const Const_area& text, Area* buffer, char quote, char quote_quote = '\\' )  { write_string( text.char_ptr(), text.length(), buffer, quote, quote_quote ); }
void        read_string( Area* buffer, const Const_area&, char quote, char quote_quote = '\\' );
void        read_string( Area* buffer, const char*, char quote, char quote_quote = '\\' );

void        print_string( const char*, int len, ::std::ostream*, char quote, char quote_quote = '\\' );
inline void print_string( const Const_area& text, ::std::ostream* s, char quote, char quote_quote = '\\' )  { print_string( text.char_ptr(), text.length(), s, quote, quote_quote ); }
void        input_string( Area* buffer, ::std::istream*, char quote, char quote_quote = '\\' );

//======================================================================================inlines

//--------------------------------------------------------------------Const_area

inline Const_area::Const_area()
{
    _ptr    = 0;
    _length = 0;
}

inline Const_area::Const_area( const void* p, unsigned int length )
{
    _ptr    = (Byte*) p;
    _length = length;
}

inline Const_area::Const_area( const char* string )
{
    _ptr    = (Byte*) string;
    _length = strlen( string );
}

//inline Const_area::operator const void*() const { return (const void *) _ptr; }
inline const void* Const_area::ptr     () const { return (const void*) /*checked*/( _ptr ); }
inline const char* Const_area::char_ptr() const { return (const char*) /*checked*/( _ptr ); }
inline const Byte* Const_area::byte_ptr() const { return (const Byte*) /*checked*/( _ptr ); }
inline unsigned int Const_area::length () const { return _length; }

/*
inline Bool Const_area::operator == ( const Const_area& area2 )
{
    return memcmp( ptr(), area2.ptr(), length() ) == 0;
}

inline Bool Const_area::operator != ( const Const_area& area2 )
{
    return ! (*this == area2);
}
*/

//-------------------------------------------------------------------------------------position

inline unsigned int position( const Const_area& a, char to_find, unsigned int pos )
{
    const char* p = (const char*) memchr( a.char_ptr() + pos, to_find, a.length() );
    return p? p - a.char_ptr() : a.length();
}

//-----------------------------------------------------------------------------------Area::Area

inline Area::Area()
  : Const_area( 0, 0 ),
    _size      ( 0 )
{
}

//-----------------------------------------------------------------------------------Area::Area

inline Area::Area( void* p, unsigned int size )
  : Const_area( p, size )
{
    _size   = size;
}

//---------------------------------------------------------------------------------Area::length

inline void Area::length( unsigned int length_ )
{
    if( length_ > _size )  length_error( length_ );   // nicht wegoptimieren! wird f¸r String0_area u.a gebraucht
    _length = length_;
}

//-----------------------------------------------------------------------------------Area::xxxx

inline unsigned int Area::size           () const  { return _size;                }
inline unsigned int Area::length         () const  { return Const_area::length(); }
inline void*        Area::ptr            () const  { return _ptr;                 }
inline char*        Area::char_ptr       () const  { return (char*) _ptr;         }
inline Byte*        Area::byte_ptr       () const  { return (Byte*) _ptr;         }
inline void         Area::check          ()        { assert( _length <= _size );  }

//---------------------------------------------------------------------------Area::allocate_min

inline void Area::allocate_min( unsigned int size )
{
#   if defined SOS_INLINE
    if( size > _size )
#   endif
    {
        _allocate_min( size );
    }
}

//---------------------------------------------------------------------------------Area::assign

inline void Area::assign( const void* p, unsigned int length )
{
#   if defined SOS_INLINE
    if( length <= _size )
    {
        memcpy( _ptr, p, length );
        _length = length;
    }
    else
#   endif
    {
        _assign( p, length );
    }
}

//----------------------------------------------------------------------------Area::operator +=
#if !defined SYSTEM_WIN16        /* Generiert sehr viel Code */
#define AREA_APPEND_CHAR_INLINE

inline Area& Area::operator +=( char c )
{
    append_inline( c );
    return *this;
}

#endif
//---------------------------------------------------Dynamic_area::Dynamic_area

inline Dynamic_area::Dynamic_area()  {}

//---------------------------------------------------Dynamic_area::allocate_min

inline void Dynamic_area::allocate_min( unsigned int size )
{
#   if defined SOS_INLINE
    if( size > _size || ( _ptr && collectable_area_ptr()->_ref_count > 1/*=gesperrt!*/ ) )
#   endif
    {
        _allocate_min( size );
    }
}

//----------------------------------------------------------Const_area_handle::Const_area_handle
/*
inline Const_area_handle::Const_area_handle( Collectable_const_area* area_ptr )
:
    Const_area            ( *area_ptr )
{
    collectable_area_ptr()->_ref_count++;
}
*/
//----------------------------------------------------------Const_area_handle::Const_area_handle

inline Const_area_handle::Const_area_handle( const Const_area_handle& area_handle )
:
    Const_area            ( area_handle )
{
    if( _ptr )  collectable_area_ptr()->_ref_count++;
}


void format( Area* buffer, double a, const char* form, char decimal_symbol );
void format( Area* buffer, const Const_area& zahl, const char* form, char decimal_symbol );

} //namespace sos
                                                                             
#endif
