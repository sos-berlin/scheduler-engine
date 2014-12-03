// sosfield.tpl                                         © 1995 SOS GmbH Berlin

#if !defined __IOSTREAM_H  &&  !defined _IOSTREAM_H
//#   include <iostream.h>
#endif

#if !defined __IOMANIP_H  &&  !defined _IOMANIP_H
//#   include <iomanip.h>
#endif

//-----------------------------------------------------Std_field_access_as<FIELD_TYPE,OBJECT>::read
/*
template< class FIELD_TYPE, class OBJECT >
void Std_field_access_as<FIELD_TYPE,OBJECT>::read( const Byte* ptr, OBJECT* object_ptr ) const
{
    //read_field( (const FIELD_TYPE&)*this, ptr, object_ptr );
    read_field( *(const FIELD_TYPE*)this, ptr, object_ptr );
}

//----------------------------------------------------Std_field_access_as<FIELD_TYPE,OBJECT>::write

template< class FIELD_TYPE, class OBJECT >
void Std_field_access_as<FIELD_TYPE,OBJECT>::write( Byte* ptr, const OBJECT& object ) const
{
    //write_field( (const FIELD_TYPE&)*this, ptr, object );
    write_field( *(const FIELD_TYPE*)this, ptr, object );
}

//-----------------------------------------------------------------Field_access_as<TYPE>::print

template< class TYPE >
void Field_access_as<TYPE>::print( const Byte* ptr, ostream* s, const Text_format& ) const
{
    TYPE o;
    read( ptr, &o );
    *s << setw( 0 ) << dec << o;
    if( s->fail() )  { Xc x ( "SOS-1170" ); x.insert( o ); throw x; }
}

//--------------------------------------------------------------Field_access_as<TYPE>::_v_input

template< class TYPE >
void Field_access_as<TYPE>::input( Byte* ptr, istream* s, const Text_format& format ) const
{
    TYPE o;

    *s >> dec >> o;

    if( s->fail() )  {
        char c [30];
        s->read( c, sizeof c );
        Xc x( "SOS-1140" );
        if( s->gcount() ) x.insert( c, s->gcount() );
        else x.insert( "(Leeres Feld)" );
        throw x;
    }

    while( s->peek() == ' ' )  s->get();

    if( s->peek() != EOF && s->peek() != format.separator() )  {
        char c [30];
        s->read( c, sizeof c );
        Syntax_error x ( "SOS-1196" );
        x.insert( c, s->gcount() );
        throw x;
    }

    write( ptr, o );
}

//------------------------------------------------------------Field_access_as<OBJECT>::set_null

template< class OBJECT >
void Field_access_as<OBJECT>::set_null( Byte* ) const
{
    throw Xc( "SOS-1186", "Field_access_as<>" );
}
*/
//------------------------------------------------------------------static Text_x_as<>::_field_type
/*
template<class FIELD_TYPE> const FIELD_TYPE Text_x_as<FIELD_TYPE>::_field_type;
Borland-C++ 4.02: requires run-time initializastion/finalization
*/

//-------------------------------------------------------------------------------Field_descr_as
/*
template< class FIELD_TYPE >
Field_descr_as<FIELD_TYPE>::Field_descr_as( const char* name_ptr, uint offset )
:
    Field_descr ( name_ptr, offset )
{

}

//-------------------------------------------------------------------------------Field_descr_as

template< class FIELD_TYPE >
//NON_INLINE_TEMPLATE
Field_descr_as<FIELD_TYPE>::Field_descr_as( Record_type* record_type_ptr,
                                            const char*  name_ptr,
                                            uint         offset )
:
    Field_descr ( name_ptr, offset )
{
    add_to( record_type_ptr );
}
*/
//----------------------------------------------------Record_type_as<FIELD_COUNT>::_v_add_field
#if 0
template< int FIELD_COUNT >
void Record_type_as<FIELD_COUNT>::_v_add_field( const Sos_ptr<Field_descr>& field_descr_ptr )
{
    assert( field_count() < FIELD_COUNT );
    _field_descr_array[ field_count() ] = field_descr_ptr;
    Record_type::_v_add_field( field_descr_ptr );
}


/*
template< class FIELD_TYPE>
const Field_descr& Array_field_as<FIELD_TYPE>::_field_descr( int index ) const
{
    if( (uint)index >= field_count() )  throw Xc( "Array_field_as" );

    Array_field_as<FIELD_TYPE>* t = (Array_field_as<FIELD_TYPE>*)this;
    char name [ 200 ];
    sprintf( name, _name_format, index+1 ); // beginnend von 1 an !

    delete t->_current_descr_ptr; t->_current_descr_ptr = 0;
    t->_current_descr_ptr = new Field_descr_as<FIELD_TYPE>( name, index * _field_type.field_size() );
    return *_current_descr_ptr;
}
*/

#endif
/*#if defined WITH_STATIC_SOSFIELD

template< class FIELD_TYPE, class OBJECT >
void read_field( const Array_field_as<FIELD_TYPE>& type, const Byte* ptr, OBJECT* object_array_ptr )
{
    const Byte*  p = ptr;
    OBJECT       o = *object_array_ptr;

    for( int i = 0; i < type.field_count(); i++ )
    {
        read_field( type._field_type, p, o );
        p += type._field_type.field_length( p );
        o++;
    }
}

template< class FIELD_TYPE, class OBJECT >
void write_field( const Array_field_as<FIELD_TYPE>& type, Byte* ptr, const OBJECT object_array[] )
{
    Byte*         p = ptr;
    const OBJECT* o = object_array;

    for( int i = 0; i < type.field_count(); i++ )
    {
        write_field( type._field_type, p, *o );
        p += type._field_type.field_length( p );
        o++;
    }
}

#endif*/

//inline DEFINE_SOS_STATIC_PTR( Field_descr )   // Für Solaris 4.0.1
