/* soslist.h                                            (c) SOS GmbH Berlin
                                                        Joacim Zschimmer
*/

#ifndef __SOSLIST_H
#define __SOSLIST_H

namespace sos
{

//#pragma interface

/* Sos_simple_list_node<T>
   -----------------------
   ist eine einfach Listenimplementierung, wie sie aus funktionalen Sprachen
   bekannt ist. Es beschreibt einen Listenknoten.
   Der Destruktor schließt nicht die folgenden Listenknoten.
   Eine Liste ist dann einfach ein Zeiger auf das erste Listenknoten.
   List: Sos_simple_list_node<T>*.
   Sos_simple_list_node<T>: ( T, Sos_simple_list_node<T>* ).

   Folgende Funktionen unterstützen diese Liste:
   empty( Sos_simple_list_node<T>* )
   length( Sos_simple_list_node<T>* )
   store( Sos_simple_list_node<T>*, ostream& )
   load( Sos_simple_list_node<T>**, istream& )


   Sos_simple_list<T>
   ------------------
   beschreibt die ganze Liste.
   Der Destruktor gibt alle Listenknoten frei.
   empty(), length(), store() und load() sind als Methoden implementiert.
*/

struct Sos_simple_list_node_0   // Liste ohne Objekte, Hilfsklasse für Sos_simple_list_node<T>
{
    Sos_simple_list_node_0*    _tail;

  protected:
                                Sos_simple_list_node_0  ();
                                Sos_simple_list_node_0  ( Sos_simple_list_node_0* tail_ptr );
    virtual                    ~Sos_simple_list_node_0  ();

    Sos_simple_list_node_0*     tail                    () const;
    Sos_simple_list_node_0*     last                    ();
  //Sos_simple_list_node_0*     node_ptr                ( int i ) const;

  friend void delete_list( Sos_simple_list_node_0** );
};


inline Bool empty( const Sos_simple_list_node_0* );
void delete_list ( Sos_simple_list_node_0** ptr );   // setzt *ptr = 0


template< class T >
struct Sos_simple_list_node : Sos_simple_list_node_0
{
                                Sos_simple_list_node    ( const T& );
                                Sos_simple_list_node    ( const T&, Sos_simple_list_node<T>* tail_ptr );

    const T&                    head                    () const;
    T*                          head_ptr                ();
    Sos_simple_list_node<T>*    tail                    () const;
    Sos_simple_list_node<T>*    last                    ();
  //T*                          node_ptr                ( int );
    void                        append                  ( const T& );
    Sos_simple_list_node<T>*    copy                    () const;

  private:
    T                          _object;
};


template<class T> int                      empty    ( const Sos_simple_list_node<T>* );
template<class T> int                      length   ( const Sos_simple_list_node<T>* );
template<class T> Sos_simple_list_node<T>* copy     ( const Sos_simple_list_node<T>* );               // funktional
template<class T> Sos_simple_list_node<T>* prepend  ( const T ,       Sos_simple_list_node<T>** );    // prozedural
template<class T> Sos_simple_list_node<T>* prepend  ( const T ,       Sos_simple_list_node<T>* );    // prozedural
//template<class T> Sos_simple_list_node<T>* prepend  ( const T&, const Sos_simple_list_node<T>*  );    // funktional
//template<class T> Sos_simple_list_node<T>* append   (       Sos_simple_list_node<T>**, const T& );    // prozedural
//template<class T> Sos_simple_list_node<T>* append   ( const Sos_simple_list_node<T>* , const T& );    // funktional
template<class T> void                     concat   (       Sos_simple_list_node<T>**,       Sos_simple_list_node<T>** );     // prozedural
template<class T> Sos_simple_list_node<T>* concat   ( const Sos_simple_list_node<T>* , const Sos_simple_list_node<T>*  );     // funktional
template<class T> Sos_simple_list_node<T>* operator&( const Sos_simple_list_node<T>*, const T& );
template<class T> Sos_simple_list_node<T>* operator&( const T&, const Sos_simple_list_node<T>* );
#if 0   /* defined __BORLANDC__    Gnu C++ 2.5.8 mag wohl die Pointer nicht: */
template<class T> Sos_simple_list_node<T>* operator&( const Sos_simple_list_node<T>*, const Sos_simple_list_node<T>* );
#endif
template<class T> void                     map      ( Sos_simple_list_node<T>*, void f( T* ) );   // prozedural
template<class T, class U> Sos_simple_list_node<U>* map( const Sos_simple_list_node<T>*, U f( const T& ) );  // funktional
template<class T> void                     delete_node( Sos_simple_list_node<T>** ptr );
template<class T> void                     delete_node_with_object( Sos_simple_list_node<T>** ptr );
template<class T> void                     delete_list( Sos_simple_list_node<T>** ptr );  // setzt *ptr = 0
template<class T> void                     delete_list_with_objects( Sos_simple_list_node<T>** ptr );  // setzt *ptr = 0
template<class T> Sos_simple_list_node<T>**node_ptr_ptr( Sos_simple_list_node<T>**, T );
template<class T> Sos_simple_list_node<T>* node_ptr    ( Sos_simple_list_node<T>* , const T& );
template<class T> std::ostream&            operator<<  ( std::ostream&, const Sos_simple_list_node<T>* );
template<class T> void                     object_print( const Sos_simple_list_node<T>*, std::ostream& );
template<class T> void                     object_store( const Sos_simple_list_node<T>*, std::ostream& );
template<class T> std::istream&            operator>>  ( std::ostream&, Sos_simple_list_node<T>*& );
template<class T> Sos_simple_list_node<T>* object_input( Sos_simple_list_node<T>**, std::istream& );
template<class T> Sos_simple_list_node<T>* object_load ( Sos_simple_list_node<T>**, std::istream& );



template< class T >
struct Sos_simple_list
{
    typedef Sos_simple_list_node<T> Elem;

                                Sos_simple_list         ( Elem* = 0 );
                                Sos_simple_list         ( const T& );
  //                            Sos_simple_list         ( const T&, const Sos_simple_list<T>& tail );
                                Sos_simple_list         ( const T&, Elem* tail );
                               ~Sos_simple_list         ();             // Löscht die ganze Liste

    Bool                        empty                   () const;
    int                         length                  () const;
    const T&                    head                    () const;
    T*                          head_ptr                ();
    Sos_simple_list_node<T>*    tail                    ();
    Sos_simple_list_node<T>*    last                    ();
  //T*                          node_ptr                ( int );
    void                        add                     ( const T& );
    void                        append                  ( const T& );
  //Sos_simple_list<T>          map                     ( T f( const T& ) );
    void                        map                     ( void f( T* ) );
    Sos_simple_list_node<T>*&   node_ptr                ();
    void                        delete_list             ();
    void                        object_store            ( std::ostream& );
    Sos_simple_list<T>&         object_load             ( std::istream& );
    void                        object_print            ( std::ostream& );
    Sos_simple_list<T>&         object_input            ( std::istream& );

  private:
    void                       _object_load             ( std::istream& );
    void                       _object_input            ( std::istream& );

    Sos_simple_list_node<T>*   _node_ptr;
};

//==========================================================================================


/* soslist.inl                                          (c) SOS GmbH Berlin
                                                        Joacim Zschimmer
*/

//---------------------------------------------------Sos_simple_list_node_0::Sos_simple_list_elem_0

inline Sos_simple_list_node_0::Sos_simple_list_node_0( Sos_simple_list_node_0* tail )
 :  _tail ( tail )
{
}

//--------------------------------------------------Sos_simple_list_node_0::~Sos_simple_list_node_0

inline Sos_simple_list_node_0::~Sos_simple_list_node_0()
{
}

//---------------------------------------------------------------------Sos_simple_list_node_0::tail

inline Sos_simple_list_node_0* Sos_simple_list_node_0::tail() const
{
    return _tail;
}

//-----------------------------------------------------------------empty( Sos_simple_list_node_0* )

inline Bool empty( const Sos_simple_list_node_0* liste )
{
    return liste == 0;
}

//----------------------------------------------------Sos_simple_list_node<T>::Sos_simple_list_node

template<class T>
inline Sos_simple_list_node<T>::Sos_simple_list_node( const T& object )
 :  Sos_simple_list_node_0 ( 0 ),
	_object ( object )
{
}

//----------------------------------------------------Sos_simple_list_node<T>::Sos_simple_list_node

template<class T>
inline Sos_simple_list_node<T>::Sos_simple_list_node( const T& object, Sos_simple_list_node<T>* tail )
 :  Sos_simple_list_node_0 ( tail ),
	_object ( object )
{
}

//--------------------------------------------------------------------Sos_simple_list_node<T>::head

template<class T>
inline const T& Sos_simple_list_node<T>::head() const
{
    return _object;
}

//----------------------------------------------------------------Sos_simple_list_node<T>::head_ptr

template<class T>
inline T* Sos_simple_list_node<T>::head_ptr()
{
    return (T*)&_object;
}

//--------------------------------------------------------------------Sos_simple_list_node<T>::tail

template<class T>
inline Sos_simple_list_node<T>* Sos_simple_list_node<T>::tail() const
{
    return (Sos_simple_list_node<T>*) Sos_simple_list_node_0::_tail;
}

//--------------------------------------------------------------------Sos_simple_list_node<T>::last

template<class T>
inline Sos_simple_list_node<T>* Sos_simple_list_node<T>::last()
{
    return (Sos_simple_list_node<T>*) Sos_simple_list_node_0::last();
}

//------------------------------------------------------------------Sos_simple_list_node<T>::append

template<class T>
inline void Sos_simple_list_node<T>::append( const T& object)
{
    last()->_tail = new Sos_simple_list_node<T>( object );
}

//----------------------------------------------------------------empty( Sos_simple_list_node<T>* )

template< class T >
inline int empty( const Sos_simple_list_node<T>* liste )
{
    return empty( (const Sos_simple_list_node_0*) liste );
}

//-----------------------------------------------------------------copy( Sos_simple_list_node<T>* )

template< class T >
Sos_simple_list_node<T>* copy( const Sos_simple_list_node<T>* liste )
{
    Sos_simple_list_node<T>*  new_list = 0;
    Sos_simple_list_node<T>** l        = &new_list;

    while( !empty( liste ) ) {
        append( l, liste->head() );
        l = &(*l)->tail();
        liste = liste->tail();
    }

    return new_list;
}

//---------------------------------------------------------------length( Sos_simple_list_node<T>* )

template< class T >
int length( const Sos_simple_list_node<T>* liste )
{
	 int n = 0;

	 while( !empty( liste ) ) {
		  n++;
		  liste = liste->tail();
	 }

	 return n;
}

//------------------------------------------------------------------------------------delete_node()

template< class T >
inline void delete_node( Sos_simple_list_node<T>** node_ptr )
{
    Sos_simple_list_node<T>* n = *node_ptr;
    *node_ptr = n->tail();
    delete n;
}

//------------------------------------------------------------------------delete_node_with_object()

template< class T >
inline void delete_node_with_object( Sos_simple_list_node<T>** node_ptr )
{
    Sos_simple_list_node<T>* n = *node_ptr;
    delete n->head();  *n->head_ptr() = 0;
    *node_ptr = n->tail();
    delete n;
}

//------------------------------------------------------------------------------------delete_list()

template< class T >
inline void delete_list( Sos_simple_list_node<T>** node_ptr )
{
    delete_list( (Sos_simple_list_node_0**)node_ptr );
}

//-----------------------------------------------------------------------delete_list_with_objects()

template< class T >
void delete_list_with_objects( Sos_simple_list_node<T>** node_ptr )
{
    Sos_simple_list_node<T>* n = *node_ptr;

    while( !empty( n ) )  {
        delete n->head();
        *n->head_ptr() = 0;
    }

    delete_list( node_ptr );
}

//----------------------------------------------------------------------------------------prepend()

template< class T >
inline Sos_simple_list_node<T>* prepend( const T object, Sos_simple_list_node<T>** list_ptr )
{
    *list_ptr = new Sos_simple_list_node<T>( object, *list_ptr );
    return *list_ptr;
}

//----------------------------------------------------------------------------------------prepend()

template< class T >
inline Sos_simple_list_node<T>* prepend( const T object, Sos_simple_list_node<T>* liste )
{
    liste = new Sos_simple_list_node<T>( object, liste );
    return liste;
}

//----------------------------------------------------------------------------------------prepend()
#if 0

template< class T >
inline Sos_simple_list_node<T>* prepend( const T& object, const Sos_simple_list_node<T>* liste )
{
    Sos_simple_list_node<T>* list_copy = copy( liste );
    prepend( object, &list_copy );
    return list_copy;
}

#endif
//-----------------------------------------------------------------------------------------append()

template< class T >
Sos_simple_list_node<T>* append( Sos_simple_list_node<T>** list_ptr, const T object )
{
    if( !empty( *list_ptr ) ) {
        list_ptr = &(*list_ptr)->last()->tail();
    }

    *list_ptr = new Sos_simple_list_node<T>( object );
}

//-----------------------------------------------------------------------------------------append()
#if 0
template< class T >
inline Sos_simple_list_node<T>* append( const Sos_simple_list_node<T>* liste, const T& object )
{
    return append( &copy( liste ), object );
}
#endif
//-----------------------------------------------------------------------------------------concat()

template< class T >
void concat( Sos_simple_list_node<T>** list1_ptr, Sos_simple_list_node<T>** list2_ptr )
{
    if( empty( *list1_ptr ) ) {
        *list1_ptr = *list2_ptr;
    } else {
        (*list1_ptr)->last()._tail = *list2_ptr;
    }
    return *list1_ptr;
}

//-----------------------------------------------------------------------------------------concat()

template< class T >
inline Sos_simple_list_node<T>* concat( const Sos_simple_list_node<T>* list1, const Sos_simple_list_node<T>* list2 )
{
    return concat( &copy( list1 ), &copy( list2 ) );
}

//----------------------------------------------------------------------------------------operator&

template< class T >
inline Sos_simple_list_node<T>* operator&( const Sos_simple_list_node<T>* liste, const T& object )
{
    Sos_simple_list_node<T>* list_copy = copy( liste );
    append( &list_copy, object );
    return list_copy;
}

//----------------------------------------------------------------------------------------operator&
#if 0   /*defined __BORLANDC__    Gnu C++ 2.5.8 mag wohl die Pointer nicht: */

template< class T >
inline Sos_simple_list_node<T>* operator&( const Sos_simple_list_node<T>* list1, const Sos_simple_list_node<T>* list2 )
{
    return concat( list1, list2 );
}

#endif
//----------------------------------------------------------------------------------------operator&

template< class T >
inline Sos_simple_list_node<T>* operator&( const T& object, const Sos_simple_list_node<T>* liste )
{
    return prepend( object, copy( liste ) );
}

//----------------------------------------------------------------------------------------------map

template< class T >
void map( Sos_simple_list_node<T>* liste, void f( T* ) )
{
    Sos_simple_list_node<T>* l = liste;

    while( !empty( l ) ) {
        f( l->head_ptr() );
        l = l->tail();
    }

    return liste;
}

//----------------------------------------------------------------------------------------------map

template< class T, class U >
Sos_simple_list_node<U>* map( const Sos_simple_list_node<T>* t, U f( const T& ) )
{
    Sos_simple_list_node<U>*  u_list = 0;
    Sos_simple_list_node<U>** u      = &u_list;

    while( !empty( t ) ) {
        *u = new Sos_simple_list_node<U>( f( t->head() ));
        u = &(*u)->tail();
        t = t->tail();
    }

    return u_list;
}

//-------------------------------------------------------------------------------------------reduce

template< class T, class U >
U reduce( const Sos_simple_list_node<T>* liste, U f( const T&, const U& ), U u )
{
    const Sos_simple_list_node<T>*  l = liste;

    while( !empty( l ) ) {
        u = f( l->head(), u );
        l = l->tail();
    }

    return u;
}

//-------------------------------------------------------------------------------------------reduce

template< class T, class U >
U reduce( const Sos_simple_list_node<T>* liste, void f( U*, const T& ), U u )
{
    Sos_simple_list_node<T>*  l = liste;

    while( !empty( l ) ) {
        f( &u, l->head() );
        l = l->tail();
    }

    return u;
}

//-----------------------------------------------------------------------------------node_ptr_ptr()

template<class T>
#if !defined __BORLANDC__
inline
#endif
Sos_simple_list_node<T>** node_ptr_ptr( Sos_simple_list_node<T>** n, T object )
{
    while( !empty( *n )  &&  !( (*n)->head() == object )  ) {
        //sun n = &(*n)->tail();
        n = (Sos_simple_list_node<T>**) &(*n)->_tail;
    }
    return n;
}

//-----------------------------------------------------------------------------------------node_ptr

template<class T>
inline Sos_simple_list_node<T>* node_ptr( Sos_simple_list_node<T>* n, const T& object )
{
    return *node_ptr_ptr( &n, object );
}

//-----------------------------------------------------------operator<<( Sos_simple_list_node<T>* )

template<class T>
inline std::ostream& operator<<  ( std::ostream& s, const Sos_simple_list_node<T>* liste )
{
    object_print( liste, s );
    return s;
}

//-----------------------------------------------object_print( Sos_simple_list_node<T>*, ostream& )

template< class T >
void object_print( const Sos_simple_list_node<T>* liste, std::ostream& s )
{
    s << '(';

    while( !empty( liste )) {
        liste->head_ptr()->object_print( s );
        liste = liste->tail();
        if( !empty( liste ))  s << ' ';
    }

    s << ") ";
}

//-------------------------------------------------object_store( Sos_simple_list_node<T>, ostream )

template< class T >
void object_store( const Sos_simple_list_node<T>* liste, std::ostream& s )
{
    uint4 count = length( liste );
    s << (Byte) (count >> 24)
      << (Byte) (count >> 16)
      << (Byte) (count >> 8)
      << (Byte)  count;

    while( !empty( liste )) {
        liste->head_ptr()->object_store( s );
        liste = liste->tail();
    }
}

//-------------------------------------------------object_load( Sos_simple_list_node<T>*, istream )

template< class T >
Sos_simple_list_node<T>* object_load( Sos_simple_list_node<T>** list_ptr, std::istream& s )
{
    uint4 count = ( (uint4)s.get() << 24 )
                | ( (uint4)s.get() << 16 )
                | ( (uint4)s.get() <<  8 )
                |   (uint4)s.get();

	 if( s.fail() )  count = 0;

	 if( count == 0 ) {
		  *list_ptr = 0;
	 } else {
		  *list_ptr = new Sos_simple_list_node<T>( T() );
		  Sos_simple_list_node<T>* last = *list_ptr;
		  last->head_ptr()->object_load( s );
		  count--;

		  while( count ) {
				last->_tail = new Sos_simple_list_node<T>( T() );
				last = last->tail();
				last->head_ptr()->object_load( s );
				count--;
		  }
	 }

	 return *list_ptr;
}

//--------------------------------------------------------------Sos_simple_list<T>::Sos_simple_list

template< class T >
inline Sos_simple_list<T>::Sos_simple_list( const T& object )
 :  _node_ptr( new Sos_simple_list_node<T>( object ) )
{
}

//--------------------------------------------------------------Sos_simple_list<T>::Sos_simple_list

template< class T >
inline Sos_simple_list<T>::Sos_simple_list( const T& object, Sos_simple_list_node<T>* tail_node )
 :  _node_ptr( new Sos_simple_list_node<T>( object, tail_node ) )
{
}

//-------------------------------------------------------------Sos_simple_list<T>::~Sos_simple_list

template< class T >
inline Sos_simple_list<T>::~Sos_simple_list()
{
    delete_list();
}

//-------------------------------------------------------------------------Sos_simple_list<T>::head

template< class T >
inline const T& Sos_simple_list<T>::head() const
{
    return node_ptr()->head();
}

//------------------------------------------------------------------------Sos_simple_list<T>::empty

template< class T >
inline Bool Sos_simple_list<T>::empty() const
{
    return sos::empty( this->node_ptr() );
}

//-----------------------------------------------------------------------Sos_simple_list<T>::length

template< class T >
inline int Sos_simple_list<T>::length() const
{
    return sos::length( this->node_ptr() );
}

//---------------------------------------------------------------------Sos_simple_list<T>::head_ptr

template< class T >
inline T* Sos_simple_list<T>::head_ptr()
{
    return this->node_ptr()->head_ptr();
}

//-------------------------------------------------------------------------Sos_simple_list<T>::tail

template< class T >
inline Sos_simple_list_node<T>* Sos_simple_list<T>::tail()
{
    return this->node_ptr()->tail();
}

//-------------------------------------------------------------------------Sos_simple_list<T>::last

template< class T >
inline Sos_simple_list_node<T>* Sos_simple_list<T>::last()
{
    return this->node_ptr()->last();
}

//--------------------------------------------------------------------------Sos_simple_list<T>::add

template< class T >
inline void Sos_simple_list<T>::add( const T& object )
{
    _node_ptr = new Sos_simple_list_node<T>( object, _node_ptr );
}

//-----------------------------------------------------------------------Sos_simple_list<T>::append

template< class T >
inline void Sos_simple_list<T>::append( const T& object )
{
    if( empty() ) {
        _node_ptr = new Sos_simple_list_node<T>( object );
    } else {
        this->node_ptr()->append( object );
    }
}

//-------------------------------------------------------------------Sos_simple_list<T>::delete_list

template< class T >
inline void Sos_simple_list<T>::delete_list()
{
    sos::delete_list( &_node_ptr );
}

//----------------------------------------------------------------------Sos_simple_list<T>::node_ptr

template< class T >
inline Sos_simple_list_node<T>*& Sos_simple_list<T>::node_ptr()
{
    return _node_ptr;
}

//--------------------------------------------------------------------------Sos_simple_list<T>::map
#ifndef SYSTEM_GNU

template< class T >
inline void Sos_simple_list<T>::map( void f( T* ) )
{
    sos::map( _node_ptr, f );
}

#endif
//-----------------------------------------------------------------Sos_simple_list<T>::object_store

template< class T >
inline void Sos_simple_list<T>::object_store( std::ostream& s )
{
    sos::object_store( this->node_ptr(), s );
}

//------------------------------------------------------------------Sos_simple_list<T>::object_load

template< class T >
inline Sos_simple_list<T>& Sos_simple_list<T>::object_load( std::istream& s )
{
    _object_load( s );
    return *this;
}

//-----------------------------------------------------------------Sos_simple_list<T>::_object_load

template< class T >
inline void Sos_simple_list<T>::_object_load( std::istream& s )
{
    delete_list();
    sos::object_load( &_node_ptr, s );
}


} //namespace sos

#endif
