// subrange.h
#if 0







#ifndef __SUBRANGE_H
#define __SUBRANGE_H

//#pragma implementation

#if !defined __ASSERT_H
#   include <assert.h>
#endif

namespace sos
{

//struct Subrange_error : Xc { Subrange_error( long value, long mini, long maxi ); };
typedef Xc Subrange_error;

void subrange_fail(
    const char* source,
    long        value,
    const char* typ,
    long        mini,
    long        maxi
);


template< class T, int mini, int maxi >
struct Const_subrange
{
    Const_subrange( T t ) : _value( t )  { check( t ); }
    Const_subrange( const Const_subrange& s ) : _value( s._value )  {}

    operator T () const { return _value; }

    static Const_subrange min()  { return mini; }
    static Const_subrange max()  { return maxi; }

    static void check( T t )
    {
#		ifdef __BORLANDC__
#			pragma option -w-ccc
#		endif

        if (t < mini  ||  t > maxi) {
            subrange_fail( SOURCE, t, "", mini, maxi );
        }

#		ifdef __BORLANDC__
#			pragma warn -wccc.
#		endif

        // assert( t>=mini && t<=maxi ); }
     }

  protected:
    Const_subrange()  {}

    T _value;
};


template< class T, int mini, int maxi >
struct Subrange : Const_subrange<T,mini,maxi>
{
    Subrange() {}
    Subrange( T t )  { set( t ); }
    Subrange( const Subrange& s ) : Const_subrange<T,mini,maxi> ( s )  {}
    Subrange( const Const_subrange<T,mini,maxi>& s ) : Const_subrange<T,mini,maxi> ( s )  {}

    Subrange& operator +=  ( T t )   { *this = _value +=  t; return *this; }
    Subrange& operator -=  ( T t )   { *this = _value -=  t; return *this; }
    Subrange& operator *=  ( T t )   { *this = _value *=  t; return *this; }
    Subrange& operator /=  ( T t )   { *this = _value /=  t; return *this; }
    Subrange& operator <<= ( int i ) { *this = _value <<= i; return *this; }
    Subrange& operator >>= ( int i ) { *this = _value >>= i; return *this; }
    Subrange& operator ++  ()        { *this = _value + 1;   return *this; }
    Subrange& operator --  ()        { *this = _value - 1;   return *this; }
    Subrange  operator ++  ( int   ) { Subrange<T,mini,maxi> s = *this; ++*this; return s; }
    Subrange  operator --  ( int   ) { Subrange<T,mini,maxi> s = *this; --*this; return s; }

    // Fr Schleifen: (#define loop_in_subrange)
    Subrange& step         ()        { _value++; return *this; }
    Bool      overflowed   ()        { return _value > maxi; }
    operator T () const { return _value; }

  private:
    void set( T t )  { check( t ); _value = t; }
};

#define loop_in_subrange(i)  for (i = i.min(); !i.overflowed(); i.step() )


} //namespace sos

#endif


#endif