// save.h                                       (c) SOS GmbH Berlin
//                                              Joacim Zschimmer

#ifndef __SAVE_H
#define __SAVE_H

/* Sichert im Konstruktor ein Objekt und stellt es im Destruktor wieder her.

   Verwendung:

   Typ object;
   Typ object2 = object;
   {
       Save<Typ> save ( &object );
       object = ...;
   }
   assert( object == object2 );

*/

template< class T >
struct Save
{
                 Save( T* ptr )
                 :
                     _ptr ( ptr ),
                     _copy ( *ptr )
                 {}

                ~Save()
                 {
                     *_ptr = _copy;
                 }

  private:
                 Save( const Save& );  // Nicht implementiert

    T* const    _ptr;
    T  const    _copy;
};

#endif
