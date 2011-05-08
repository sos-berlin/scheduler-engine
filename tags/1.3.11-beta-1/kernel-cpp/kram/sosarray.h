/* sosarray.h                                           (c) SOS GmbH Berlin
                                                        Joacim Zschimmer
*/

#if !defined __SOSARRAY_H
#define __SOSARRAY_H

#ifndef __SOSALLOC_H
#   include "../kram/sosalloc.h"
#endif

#if 1
//Fehler jz 4.12.96 /*defined _MSC_VER  ||*/  defined SOS_INLINE  ||  defined SOS_OPTIMIZE_SPEED
#   define SOSARRAY_INLINE
#endif

namespace sos
{

//----------------------------------------------------------------------Sos_array_iterator_as<>
/*
template< class TYPE >
struct Sos_array_iterator_as
{
                                Sos_array_iterator_as   ( Sos_array<TYPE>* a ) : _array_ptr(a)  { _index = _array_ptr->first_index(); }

                                operator TYPE*          () const    { return valid()? &(*_array_ptr)[ _index ] : 0; }
    int                         valid                   () const    { return _index <= _array_ptr->last_index(); }
    int                         operator !              () const    { return !valid(); }
  //TYPE*                       operator ->             () const    { return _array_ptr->elem_ptr( _index ); }
    Sos_array_iterator_as<TYPE>& operator ++            ( int )     { _index++; return *this; }

  private:
    Sos_array<TYPE>*           _array_ptr;
    int                        _index;
};
*/
//-------------------------------------------------------------------------------Sos_array_base

struct Sos_array_base
{
                                Sos_array_base          ()          : _first_index(0),_last_index(-1),_obj_const_name(NULL) {}
    virtual                    ~Sos_array_base          ();

    void                        check_index             ( int4 ) const;
    void                       _check_index             ( int4 ) const;

    virtual void                size                    ( int4 )            = 0;

            void                first_index             ( int );                    // Korrigiert last_index
    inline  void                last_index              ( int4 );
            void                clear                   ()                          { last_index( first_index()-1 ); }

            int                 first_index             () const                    { return _first_index; }
            int                 last_index              () const                    { return _last_index; }
            int                 count                   () const                    { return last_index() - first_index() + 1; }

            int                 add_empty               ();
    void                        pop                     ()                          { last_index( last_index() - 1 ); }

    void                        obj_const_name          ( const char* name_ptr )    { _obj_const_name = name_ptr; }

  protected:
    int                        _first_index;
    int                        _last_index;
    const char*                _obj_const_name;
};

//----------------------------------------------------------------------------------Sos_array<>

template< class T >
struct Sos_array : Sos_array_base
{
    //typedef Sos_array_iterator_as<T> Iterator;

            void                add                     ( const T& );
    T&                          operator[]              ( int4 ) const;
    T*                          elem_ptr                ( int4 i ) const            { return &(*this)[ i ]; }
    T*                          checked_elem_ptr        ( int4 i ) const;           // Index wird geprüft, nicht ptr
  //T                           pop                     ()                          { T o = *last_elem(); last_index( last_index() - 1 ); return o; }  // Benötigt operator=
  //Iterator                    iterator                ()                          { return Iterator( this ); }

  protected:
    virtual T*                 _elem_ptr                ( int4 ) const       = 0;
};


//---------------------------------------------------------------------------Sos_simple_array<>

template< class T >
struct Sos_simple_array : Sos_array< T >
{
                                Sos_simple_array        ();
                               ~Sos_simple_array        ();

    DECLARE_PUBLIC_MEMBER( int , increment                     )

#   if !defined OPTIMIZE_SIZE
    T&                          operator[]              ( int4 i ) const        { this->check_index( i ); return _array[ i - this->_first_index ]; }
#   endif

    void                        size                    ( int4 );
    int                         size                    () const                { return _size; }
    T*                          add_empty               ()                      { return &_array[ Sos_array_base::add_empty() ]; }
    T*                          last_elem               ()                      { return &_array[ this->last_index() ]; }
    int                         index                   ( const T* t )          { return t - _array + this->first_index(); }

  protected:
    T*                         _elem_ptr                ( int4 ) const;

  private:
                                Sos_simple_array        ( const Sos_simple_array<T>& );  // NICHT IMPLEMENTIERT
    Sos_simple_array<T>&        operator =              ( const Sos_simple_array<T>& );  // NICHT IMPLEMENTIERT

    T*                         _array;
    int                        _size;
};

//-----------------------------------------------------------------------Sos_simple_ptr_array<>

template< class T >
struct Sos_simple_ptr_array : Sos_simple_array< Sos_ptr< T > >
{
    // Diese Klasse könnte size(int) und ~Sos_simple_ptr_array für alle Sos_ptr<T>
    // einmal implementieren, so dass nicht für jedes T der gleiche Code generiert wird.
};

//------------------------------------------------------------------Sos_simple_auto_ptr_array<>

template< class T >
struct Sos_simple_auto_ptr_array : Sos_simple_array< T* >     // mit delete auf Elemente!
{
                               ~Sos_simple_auto_ptr_array();
};

//-------------------------------------------------------------------------------sos_array_free

inline void sos_array_free( void* array )
{
    sos_free( array );
}

//----------------------------------------------------------------------DEFINE_SOS_DELETE_ARRAY
#if 1 //defined JZ_TEST
    #define DEFINE_SOS_DELETE_ARRAY( TYPE ) \
        inline void destruct( TYPE* )  {}
#else
    #define DEFINE_SOS_DELETE_ARRAY( TYPE ) \
        inline void sos_delete_array( TYPE* p, int  )  { sos_array_free( p ); }
#endif

DEFINE_SOS_DELETE_ARRAY( int )
DEFINE_SOS_DELETE_ARRAY( uint )
DEFINE_SOS_DELETE_ARRAY( int4 )
DEFINE_SOS_DELETE_ARRAY( uint4 )
DEFINE_SOS_DELETE_ARRAY( void* )


//--------------------------------------------------------------------------------------inlines

//------------------------------------------------------------------Sos_array_base::first_index

inline void Sos_array_base::first_index( int i )
{
    _last_index += i - _first_index;
    _first_index = i;
}

//-------------------------------------------------------------------Sos_array_base::last_index

inline void Sos_array_base::last_index( int4 index )
{
    size( index - first_index() + 1 );
    _last_index = index;
}

//------------------------------------------------------------------Sos_array_base::check_index

inline void Sos_array_base::check_index( int4 i ) const
{
#   if !defined SOS_DEBUGGED
#       if defined SOSARRAY_INLINE
            if( i >= _first_index  &&  i <= _last_index )  return;
#       endif

        _check_index( i );
#   endif
}

//--------------------------------------------------------------Sos_simple_array<T>::operator[]

template< class T >
inline T& Sos_array<T>::operator[]( int4 index ) const
{
#   if defined SOSARRAY_INLINE
        check_index( index );
        return *_elem_ptr( index );
#   else
        return *checked_elem_ptr( index );
#   endif
}

//------------------------------------------------------------Sos_simple_array<T>::Sos_simple_array

template< class T >
inline Sos_simple_array<T>::Sos_simple_array()
:
    _increment ( max( 1u, 100 / sizeof( T ))),
    _array     ( 0 ),
    _size      ( 0 )
{
}

//------------------------------------------------------------------Sos_simple_array<T>::operator[]

template< class T >
inline T* Sos_simple_array<T>::_elem_ptr( int4 index ) const
{
    return &_array[ index - this->first_index() ];
}

//----------------------------------------------------------------------------------------index

template< class TYPE >
int index( const Sos_array<TYPE>&, const TYPE&  );

#if defined SYSTEM_SOLARISxxxx
template< class TYPE >
inline
int index( const Sos_simple_array<TYPE>& a, const TYPE& e ) {
    return ::index( (const Sos_array<TYPE>&) a, e );
}
#endif

//-------------------------------------------------------------------------------------destruct
// Gehört nach sos.h

template< class TYPE >
inline void destruct( TYPE* p )
{
    p->~TYPE(); // Bei einem Syntaxfehler: DEFINE_SOS_DELETE_ARRAY( <TYPE> ) in .h oder .cxx
                // Oder: inline void destruct( TYPE* ) {}
    memset( p, 0, sizeof *p );
}


} //namespace sos

#if defined SYSTEM_INCLUDE_TEMPLATES
#   include "../kram/sosarray.tpl"
#endif

#endif
