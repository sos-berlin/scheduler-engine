// soslimtx.h                                     (c) SOS GmbH Berlin


#ifndef __SOSLIMTX_H
#define __SOSLIMTX_H


#if !defined __SOSSTRG0_H
#   include "sosstrg0.h"
#endif

#if !defined __AREA_H
#   include "area.h"
#endif

#if !defined __STDFIELD_H
#   include "stdfield.h"        // Area_type
#endif

namespace sos
{

//---------------------------------------------------------------------------------String0_area

struct String0_area : Area
{
    String0_area&               operator =              ( const Const_area& o )          { assign( o ); return *this; }
    String0_area&               operator =              ( const String0_area& );
    String0_area&               operator =              ( const char* );
    String0_area&               operator =              ( const Sos_string& str );

    String0_area&               operator +=             ( char );
    Area&                       operator +=             ( const char* string )              { append( string ); return *this; }

#   if defined __SOSSTRNG_H
        Area&                   operator +=             ( const Sos_string& str )           { append( c_str( str ), ::sos::length( str ) ); return *this; }
#   endif

    char&                       operator []             ( unsigned int );

    Bool                        operator ==             ( const char* s         ) const  { return cmp( s ) == 0; }
    Bool                        operator !=             ( const char* s         ) const  { return cmp( s ) != 0; }
    Bool                        operator <              ( const char* s         ) const  { return cmp( s ) <  0; }
    Bool                        operator <=             ( const char* s         ) const  { return cmp( s ) <= 0; }
    Bool                        operator >=             ( const char* s         ) const  { return cmp( s ) >= 0; }
    Bool                        operator >              ( const char* s         ) const  { return cmp( s ) >  0; }

    const char*                _c_str                   () const                                { *((char*)char_ptr() +  length()) = '\0'; return char_ptr(); }

  protected:
    friend struct               Sos_limited_text_type;

                                String0_area            ();
                                String0_area            ( char*, uint size );

    char&                       chr                     ( unsigned int );
#if !defined SYSTEM_MICROSOFT
    int                         cmp                     ( const String0_area& s ) const         { return cmp( s._c_str() ); }
#endif
    int                         cmp                     ( const char* s ) const                 { return strcmp( _c_str(), s ); }
};


// Allgemeine String-Funktionen wie in sosstrng.h:

inline const char*              c_str                   ( const String0_area& string )          { return string._c_str(); }
inline Bool                     empty                   ( const String0_area& string )          { return empty( string._c_str() ); }

#   if defined __SOSSTRNG_H
inline Sos_string operator + ( const Sos_string& rStr, const String0_area& str0 )
{
    Sos_string aTmpStr = rStr;
    aTmpStr += c_str(str0);
    return aTmpStr;
}

inline Sos_string operator + ( const String0_area& str0, const Sos_string& rStr )
{
    Sos_string aTmpStr;
    aTmpStr =  c_str(str0);
    aTmpStr += rStr;
    return aTmpStr;
}
#   endif

//-----------------------------------------------------------------------Sos_limited_text<SIZE>

template< int SIZE >
struct Sos_limited_text : String0_area
{
                                Sos_limited_text        ();
                                Sos_limited_text        ( const char* );
                                Sos_limited_text        ( const Sos_limited_text<SIZE>& o )     : String0_area( _text, SIZE ) { _length = o._length; memcpy( _text, o._text, o.length() ); }

                                Sos_limited_text        ( const Sos_string& str );
    Sos_limited_text<SIZE>&     operator =              ( const Sos_string& str );

  //virtual void                allocate_min            ( uint size )                           { if ( size > SIZE ) throw Too_long_error(); }

    Sos_limited_text<SIZE>&     operator =              ( const Sos_limited_text<SIZE>& o )     { _length = o._length; memcpy( _text, o._text, o.length() ); return *this; }
    Sos_limited_text<SIZE>&     operator =              ( const Const_area& str )               { assign( str ); return *this; }
    Sos_limited_text<SIZE>&     operator =              ( const char* str )                     { assign( str ); return *this; }

    static int                  size                    ()                                      { return SIZE; }   // Für RECORD_TYPE_ADD_LIMTEXT
    static int                  static_size             ()                                      { return SIZE; }   // Für RECORD_TYPE_ADD_LIMTEXT

//private:
    friend struct               Sos_limited_text_type;

    char                       _text [ SIZE + 1 ];      // 0-terminiert
};

//------------------------------------------------------------------------Sos_limited_text_type
#if defined __SOSFIELD_H

struct Sos_limited_text_type : Area_type
{
                                Sos_limited_text_type   ( int size );

    void                        construct               ( Byte* ) const;
    int                         alignment               () const                                { return sizeof (int); }
    void                        field_copy              ( Byte* p, const Byte* s ) const;
    void                        check_type              ( const String0_area* ) {}

    static Type_info           _type_info;

  protected:
    void                       _get_param               ( Type_param* ) const;
    int                        _size;
};

extern Sos_limited_text_type sos_limited_text_type;

// in soslimt2.cxx:
void add_sos_limited_text_field(
    Record_type* t, const String0_area* offset, const char* name, int size,
    const Bool* null_offset = (Bool*)-1, uint flags = 0 );

#define RECORD_TYPE_ADD_LIMTEXT( NAME, FLAGS )   \
    add_sos_limited_text_field( t, &o->_##NAME, #NAME, o->_##NAME.static_size(), (Bool*)-1, FLAGS )

#define RECORD_TYPE_ADD_LIMTEXT_NULL( NAME, FLAGS )   \
    add_sos_limited_text_field( t, &o->_##NAME, #NAME, o->_##NAME.static_size(), &o->_##NAME##_null, FLAGS )

#endif
//-----------------------------------------------------------------------String0_area::String0_area

inline String0_area::String0_area()
{
}

//-----------------------------------------------------------------------String0_area::String0_area

inline String0_area::String0_area( char* buffer_ptr, uint size )
:
    Area( buffer_ptr, size )
{
}

// -------------------------------------------------------------------- String0_area::operator=
#if defined __SOSSTRNG_H

inline String0_area& String0_area::operator= ( const Sos_string& str )
{
    assign( ::sos::c_str( str ) );
    return *this;
}

#endif
// ------------------------------------------------------------------------ String0_area::operator=

inline String0_area& String0_area::operator= ( const String0_area& str )
{
    assign( str );
    return *this;
}

// ------------------------------------------------------------------------ String0_area::operator=

inline String0_area& String0_area::operator= ( const char* str )
{
    assign( str );
    return *this;
}

// ------------------------------------------------------------------------String0_area::operator+=

inline String0_area& String0_area::operator+= ( char c )
{
    //if( length() >= size() )  throw Xc( "SOS-1117" );   // wird von Area::length() erledigt (?)

    char_ptr()[ length() ] = c;
    length( length() + 1 );
    //char_ptr()[ length() ] = '\0';

    return *this;
}

// ---------------------------------------------------------------------- String0_area::operator []

inline char& String0_area::operator[]  ( unsigned int pos )
{
    if ( pos >= size() ) return chr( pos );
    return char_ptr()[ pos ];
}

//---------------------------------------------------------Sos_limited_text<SIZE>::Sos_limited_text
#if !defined SYSTEM_SOLARIS

template< int SIZE >
inline Sos_limited_text<SIZE>::Sos_limited_text()
:
    String0_area( _text, SIZE )
{
    _length = 0;
}
#else
    // in soslimtx.tpl definiert wegen Solaris-Macke, C++ 3.0.1 1.8.96
#endif
//---------------------------------------------------------Sos_limited_text<SIZE>::Sos_limited_text

template<int SIZE >
inline Sos_limited_text<SIZE>::Sos_limited_text( const char* str )
:
    String0_area( _text, SIZE )
{
    assign( str );
}

//-----------------------------------------------------Sos_limited_text<SIZE>::Sos_limited_text
#if defined __SOSSTRNG_H

template< int SIZE >
inline Sos_limited_text<SIZE>::Sos_limited_text( const Sos_string& str )
:
    String0_area( _text, SIZE )
{
    assign( c_str( str ), ::sos::length( str ) );
}

//------------------------------------------------------------Sos_limited_text<SIZE>::operator=

template< int SIZE >
inline Sos_limited_text<SIZE>& Sos_limited_text<SIZE>::operator= ( const Sos_string& str )
{
    assign( c_str( str ), ::sos::length( str ) );
    return *this;
}

#endif

} //namespace sos

#if defined SYSTEM_INCLUDE_TEMPLATES
#   include "../kram/soslimtx.tpl"
#endif


#endif
