// chkarray.h
#if 0





#ifndef __CHKARRAY_H
#define __CHKARRAY_H

//#pragma interface

#include "subrange.h"
#include "area.h"

namespace sos
{

template< class T, int mini, int maxi >
struct Checked_array
{
    /*
       Objekte dieser Klasse sollen mit obj = {...} initialisierbar sein.
       Virtuelle Basisklassen(?), Konstruktoren und private Elemente sind
       daher nicht m”glich.
    */

    typedef Subrange<int,mini,maxi>        Index;
    typedef Const_subrange<int,mini,maxi>  Const_index;

    T&                          operator []             ( Const_index i ) const  { return (T&) _array [ _index(i) ]; }  // non-const cast!
    const T*                    operator&               () const           { return _array; }
                                operator Const_area     () const { return Const_area( _array, sizeof _array ); }
  //                            operator Area       () const { return Area( _array, sizeof _array ); }

    // Borland: Cannot convert 'const int*' to 'int*' ????
    //T*                        operator +              ( int i ) const  { return _array + _index( i ); }
    //T*                        operator -              ( int i ) const  { return _array - _index( i ); }

    static int                  min                     ()  { return mini; }
    static int                  max                     ()  { return maxi; }
    static int                  size                    () { return (maxi-mini+1) * sizeof(T); }

  //private:
    static int                 _index( Const_index i )  { return i - mini; }

    T                          _array [ maxi - mini + 1 ];
};

//----------------------------------------------------------------------------

template< class T, int mini, int maxi >
struct Checked_array_ptr
{
    typedef Subrange<int,mini,maxi>         Index;
    typedef Const_subrange<int,mini,maxi>   Const_index;


                                Checked_array_ptr       ( T* ptr ) : _ptr ( ptr )  {}

    T                           operator []             ( Const_index i ) const { return _ptr[ index( i ) ]; }

    T*                          operator +              ( int i ) const { return _ptr + index( i ); }
    T*                          operator -              ( int i ) const { return _ptr - index( i ); }

  private:
    static int                  index                   ( Const_index i )  { return i - mini; }

    T* const                   _ptr;
};

} //namespace sos

#endif
#endif