// sosctain.h                                           ©1996 SOS GmbH Berlin

#ifndef __SOSCTAIN_H
#define __SOSCTAIN_H

//----------------------------------------------------------------------------Sos_key_container
//Benötigt  KEY  key        ( const OBJECT& );
//          Bool operator== ( const KEY&, const KEY& );
//          Bool null       ( const OBJECT& );
//          void close      ( OBJECT* );               // postcondition  null(object) == true

template< class OBJECT, class KEY >
struct Sos_key_container
{
    typedef int                 Id;

                                Sos_key_container       ();
                               ~Sos_key_container       ();

    Id                          add                     ( const TYPE& );
    Id                          store                   ( const TYPE& );
    Id                          search                  ( const KEY& );
    TYPE&                       get_by_key              ( const KEY& );
    TYPE&                       get_by_id               ( Id );

    static Bool                 valid_id                ( Id i )                                { return i != 0; }

  private:
    Sos_simple_array< OBJECT >  _array;
};

//---------------------------------------------Sos_key_container<OBJECT,KEY>::Sos_key_container

template< class OBJECT, class KEY >
Sos_key_container<OBJECT,KEY>::Sos_key_container()
{
    _array.obj_const_name( "Sos_key_container::_array" );
    _array.first_index( 1 );
}

//---------------------------------------------Sos_key_container<OBJECT,KEY>::Sos_key_container

template< class OBJECT, class KEY >
Sos_key_container<OBJECT,KEY>::~Sos_key_container()
{
    for( int i = _array.last_index(); i >= _array.first_index(); i-- ) {
        close( &_array[ i ] );
    }
}

//----------------------------------------------------------Sos_key_container<OBJECT,KEY>::find

template< class OBJECT, class KEY >
Sos_key_container<OBJECT,KEY>::Id Sos_key_container<OBJECT,KEY>::find( const KEY& key )
{
    for( int i = _array.first_index(); i <= _array.last_index(); i++ ) {
        if( key( _array[ i ] ) == key )  return i;
    }

    return -1;
}

//---------------------------------------------------------Sos_key_container<OBJECT,KEY>::store

template< class OBJECT, class KEY >
Sos_key_container<OBJECT,KEY>::Id Sos_key_container<OBJECT,KEY>::store( const OBJECT& o )
{
    Id id  = find( key( o ) );
    return valid_id( id )? id : add( o );
}

//-----------------------------------------------------------Sos_key_container<OBJECT,KEY>::add

template< class OBJECT, class KEY >
Sos_key_container<OBJECT,KEY>::Id Sos_key_container<OBJECT,KEY>::add( const OBJECT& o )
{
    for( i = 0; i <= _array.last_index(); i++ )  if( null( _array[ i ] ) )  break;
    if( i > _array.last_index() )  _array.last_index( _array.last_index() + 1 );
    _array[ i ] = o;
    return i;
}

#endif
