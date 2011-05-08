// sosarray.tpl                             ©1995 SOS GmbH Berlin

namespace sos
{

//--------------------------------------------------------------------------------sos_new_array

template< class TYPE >
TYPE* sos_new_array( const TYPE*, int count, const char* debug_name )
{
    TYPE* array = (TYPE*) sos_alloc( count * sizeof (TYPE), debug_name );  //sizeof (long) * array_long_size( dummy, count ) );

    TYPE* p     = array;
    TYPE* p_end = p + count;

    while( p < p_end )  new (p++) TYPE;            // Elemente konstruieren mit dummy-new

    return array;
}

//-----------------------------------------------------------------------------sos_delete_array

template< class TYPE >
void sos_delete_array( TYPE* array, int count )
{
    TYPE* p     = array;
    TYPE* p_end = p + count;

    while( p < p_end ) {
        destruct( p );   // Bei einem Syntaxfehler: DEFINE_SOS_DELETE_ARRAY( <TYPE> ) in .h oder .cxx
        p++;
    }

    sos_array_free( array );
}

//--------------------------------------------------------------Sos_array<T>::checked_index_ptr

template< class TYPE >
TYPE* Sos_array<TYPE>::checked_elem_ptr( int4 i ) const
{
	_check_index( i );
	return elem_ptr( i );
}

//----------------------------------------------------------------------------Sos_array<T>::add

template< class T >
void Sos_array<T>::add( const T& object )
{
    add_empty();
    *_elem_ptr( last_index() ) = object;
}

//-------------------------------------------------------Sos_simple_array<T>::~Sos_simple_array

template< class T >
Sos_simple_array<T>::~Sos_simple_array()
{
    if( _size ) {
        sos_delete_array( _array /*+ first_index()*/, _size );
        _array = 0;
        _size = 0;
    }
}

//--------------------------------------------------------------------Sos_simple_array<T>::size

template< class T >
void Sos_simple_array<T>::size( int4 new_size )
{
    new_size = ( new_size + _increment - 1 ) / _increment * _increment;

    if( new_size == _size )  return;

#   ifdef SYSTEM_WIN16
        if( new_size                >= 0xFFFF / sizeof _array[0]
         || new_size + _first_index >= 0xFFFF / sizeof _array[0] )  throw_xc( "SOS-1347" );
#   endif

    T*  new_array = new_size > 0? sos_new_array( (T*)0, (int)new_size, this->_obj_const_name ) : 0;
    int s         = min( new_size, _size );

    if( _size ) {
        //_array += first_index();
		int i;

        for( i = s; i < _size; i++ )  destruct( &_array[ i ] );

        for( i = 0; i < s; i++ ) {
            new_array[ i ] = _array[ i ];
            destruct( &_array[ i ] );
        }

        sos_array_free( _array );
    }

    _array = new_array /*- first_index()*/;
    _size  = new_size;
}

//-------------------------------------------------------Sos_simple_array<T>::~Sos_simple_array

template< class T >
Sos_simple_auto_ptr_array<T>::~Sos_simple_auto_ptr_array()
{
    for( int i = this->first_index(); i <= this->last_index(); i++ ) {
        T*& p = (*this)[ i ];
        delete p; p = 0;
    }
}

//----------------------------------------------------------------------------------------index

template< class TYPE >
int index( const Sos_array<TYPE>& array, const TYPE& o )
{
    for( int i = array.first_index(); i <= array.last_index(); i++ ) {
        if( o == array[ i ] )  return i;
    }

    throw Not_found_error();
}

//------------------------------------------------------------------------------------index_ptr
// Index auf Pointer-Element

template< class TYPE >
int index_ptr( const Sos_array<TYPE>& array, const TYPE& o )
{
    for( int i = array.first_index(); i <= array.last_index(); i++ ) {
        if( *o == *(array[ i ]) )  return i;
    }

    throw Not_found_error();
}


} //namespace sos
