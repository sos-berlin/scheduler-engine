// Sos_limited_string

#ifndef __SOSLIMST_H
#define __SOSLIMST_H

#if !defined SYSTEM_LINUX            // Bis die Syntax-Fehler raus sind, jz

#if !defined __STORABLE_H
#   include "storable.h"
#endif


struct Abs_limited_string : Storable_as<Sos_string>
{
    virtual unsigned int        length      () const { return 0; }
    virtual const char*         c_str       () const { return ""; }
    virtual const char*         char_ptr    () const { return ""; } // Area-Style
    virtual const Sos_string    sos_string  () const { return Sos_string(c_str()); };

    virtual unsigned int        size        () const  = 0;
    virtual void                object_load ( const Sos_string& str ) { _assign(::c_str(str)); };
    virtual void                object_store( Sos_string* str_ptr ) const { *str_ptr = c_str(); };

 protected:
    virtual void                _assign     ( const char* ) {};
    friend  void                object_check( const Sos_string& str, Abs_limited_string* str_ptr );
};

inline const char* c_str( const Abs_limited_string& str )
{
    return str.c_str();
}

inline void object_check( const Sos_string& str, Abs_limited_string* str_ptr )
{
    str_ptr->_assign( ::c_str(str) );
}

inline Sos_string as_string( const Abs_limited_string& str )
{
    return str.sos_string();
}

template <unsigned int sz>
struct Sos_limited_string : Abs_limited_string
{
    Sos_limited_string              () { _init(); }
    Sos_limited_string              ( const char* str ) { _init(); _assign(str); }
    Sos_limited_string              ( const Sos_string& str );
    ~Sos_limited_string             () {}

    Sos_limited_string& operator=   ( const char* str ) { _assign( str ); return *this; }
    Sos_limited_string& operator=   ( const Sos_string& str );
    operator Abs_limited_string*    () { return (Abs_limited_string*) this; }

    unsigned int        length      () const { return strlen(_buffer); }
    const char*         c_str       () const { return _buffer; }
    const char*         char_ptr    () const { return _buffer; } // Area-Style
    const Sos_string    sos_string  () const { return Sos_string(_buffer); };
    unsigned int        size        () const { return sz; };

    char&               operator[]  ( unsigned int );

 private:

    void                _init       () { _buffer[0] = 0; _buffer[sz] = 0; };
 protected:
    void                _assign     ( const char* );

    char                _buffer[sz+1];
};


// sosstrng.inl

// -------------------------------------------------------------- Sos_limited_string::Sos_limited_string

template <unsigned int size>
inline Sos_limited_string<size>::Sos_limited_string( const Sos_string& str )
{
    _init();
    _assign( ::c_str(str) );
}


// ----------------------------------------------------------------------- Sos_limited_string::operator=

template <unsigned int size>
inline Sos_limited_string<size>& Sos_limited_string<size>::operator=( const Sos_string& str )
{
    _assign( ::c_str( str ) );
    return *this;
}

// ------------------------------------------------------------------------- Sos_limited_string::_assign
#if defined SYSTEM_EXCEPTIONS

template <unsigned int sz>
inline void Sos_limited_string<sz>::_assign( const char* str )
{
    unsigned int len = strlen( str );
    if ( len > sz ) throw Xc( "D320" );
    strcpy( _buffer, str );
}

#endif

// ----------------------------------------------------------------------- Sos_limited_string::operator=
#if defined SYSTEM_EXCEPTIONS

template <unsigned int size>
inline char& Sos_limited_string<size>::operator[]  ( unsigned int pos )
{
    if ( pos >= size() ) throw Xc( "D???" );
    return _buffer[pos];
}

#endif


struct ostream;
struct istream;

ostream& operator << ( ostream&, const Abs_limited_string& );

istream& operator >> ( istream&, Abs_limited_string& );

#endif   // SYSTEM_LINUX
#endif
