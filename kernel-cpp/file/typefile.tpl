// typefile.tpl                                      © 1995 SOS GmbH Berlin

namespace sos {

//-------------------------------------------------------Typed_file::Typed_file
#if 0
template< class OBJECT >
Typed_file<OBJECT>::Typed_file()
:
    _object_type ( +OBJECT::_type )
{
}
#endif
//-------------------------------------------------------Typed_file::Typed_file

template< class OBJECT >
Typed_file< OBJECT >::Typed_file( const Sos_ptr<Record_type>& o )
:
    _object_type ( +o )
{
}

//-------------------------------------------------------Typed_file::~Typed_file

template< class OBJECT >
Typed_file< OBJECT >::~Typed_file()
{
    try { close(); } catch(...) {}
}

//---------------------------------------------------------Typed_file<OBJECT>::open

template< class OBJECT >
void Typed_file<OBJECT>::open( const char* filename, Any_file::Open_mode open_mode, const File_spec& spec )
{
    open( as_string( filename ), open_mode, spec );
}

//---------------------------------------------------------Typed_file<OBJECT>::open

template< class OBJECT >
void Typed_file<OBJECT>::open( const Sos_string& filename, Any_file::Open_mode open_mode, const File_spec& spec_ )
{
    File_spec spec ( spec_ );
    spec._field_type_ptr = _object_type;
    Base_class::open( filename, open_mode, spec );
}

//-------------------------------------------------------Typed_indexed_file::Typed_indexed_file
#if 0
template< class OBJECT, class KEY >
Typed_indexed_file<OBJECT,KEY>::Typed_indexed_file( /*const OBJECT_FIELD& o, const KEY_FIELD_DESCR& k*/ )
:
    _object_type ( +OBJECT::_type ),
    _key_type    ( +KEY::_type )
{
}
#endif
//-------------------------------------------------------Typed_indexed_file::Typed_indexed_file

template< class OBJECT, class KEY >
Typed_indexed_file<OBJECT,KEY>::Typed_indexed_file( const Sos_ptr<Record_type>& o, const Sos_ptr<Field_descr>& k )
:
    _object_type ( +o ),
    _key_type    ( +k )
{
}

//-------------------------------------------------------Typed_indexed_file::~Typed_indexed_file

template< class OBJECT, class KEY >
Typed_indexed_file< OBJECT,KEY >::~Typed_indexed_file()
{
    try { close(); } catch(...) {}
}


//---------------------------------------------------------Typed_indexed_file<OBJECT,KEY>::open

template< class OBJECT, class KEY >
void Typed_indexed_file<OBJECT,KEY>::open( const char* filename, File_base/*gcc2.5.8*/::Open_mode open_mode, const File_spec& spec )
{
    open( as_string( filename ), open_mode, spec );
}

//---------------------------------------------------------Typed_indexed_file<OBJECT,KEY>::open

template< class OBJECT, class KEY >
void Typed_indexed_file<OBJECT,KEY>::open( const Sos_string& filename, File_base/*gcc2.5.8*/::Open_mode open_mode, const File_spec& spec_ )
{
    File_spec spec ( spec_ );
    spec._field_type_ptr = _object_type;
    spec._key_specs._key_spec._field_descr_ptr = _key_type;
    // ??? spec._key_specs[0]._key_position    = f->offset();
    // ??? spec._key_specs[0]._key_length      = f->type().field_size();
    Base_class::open( filename, open_mode, spec );
}

//---------------------------------Typed_indexed_file<OBJECT,KEY,OBJECT_FIELD,KEY_FIELD_DESCR>::get
/*
template< class OBJECT, class KEY, class OBJECT_FIELD, class KEY_FIELD_DESCR >
void Typed_indexed_file<OBJECT,KEY,OBJECT_FIELD,KEY_FIELD_DESCR>::get( OBJECT* object_ptr )
{
    Dynamic_area record;
    uint         field_size = _object_field_descr.field_size();

    record.allocate_min( field_size );  xc;

    Base_class::get( &record );  xc;

#   if defined JZ_ISO_TEST
        if( record.byte_ptr()[ 0 ] < 0x40 ) {
            xlat( record.byte_ptr(), record.byte_ptr(), record.length(), iso2ebc );
        }
#   endif

    ::read( _object_field_descr, record.byte_ptr(), record.byte_ptr() + record.length(), object_ptr );
}

//----------------------------------------------------------Typed_indexed_file<OBJECT,KEY>::put

template< class OBJECT, class KEY >
void Typed_indexed_file<OBJECT,KEY>::put( const OBJECT& object )
{
    Dynamic_area record;
    build_put_record( object, &record );
    Base_class::put( record );
}

//--------------------------------------------------------Typed_indexed_file<OBJECT,KEY>::store

template< class OBJECT, class KEY >
void Typed_indexed_file<OBJECT,KEY,OBJECT_FIELD,KEY_FIELD_DESCR>::store( const OBJECT& object )
{
    Dynamic_area record;
    build_put_record( object, &record );
    Base_class::store( record ); xc;

  exceptions
}

//-------------------------------------------------------Typed_indexed_file<OBJECT,KEY>::update

template< class OBJECT, class KEY >
void Typed_indexed_file<OBJECT,KEY>::update( const OBJECT& object )
{
    Dynamic_area record;
    build_put_record( object, &record );
    Base_class::update( record );
}

//---------------------------------Typed_indexed_file<OBJECT,KEY,OBJECT_FIELD,KEY_FIELD_DESCR>::insert

template< class OBJECT, class KEY >
void Typed_indexed_file<OBJECT,KEY>::insert( const OBJECT& object )
{
    Dynamic_area record;
    build_put_record( object, &record );
    Base_class::insert( record );
}


// ---------------------Typed_indexed_file<OBJECT,KEY,OBJECT_FIELD,KEY_FIELD_DESCR>::build_put_record

template< class OBJECT, class KEY, class OBJECT_FIELD, class KEY_FIELD_DESCR >
void Typed_indexed_file<OBJECT,KEY,OBJECT_FIELD,KEY_FIELD_DESCR>::build_put_record( const OBJECT& object, Area* area_ptr )
{
    uint         field_size = _object_field_descr.field_size();

    area_ptr->allocate_min( field_size );  xc;

    ::write( _object_field_descr, area_ptr->byte_ptr(), area_ptr->byte_ptr() + area_ptr->size(), object );
    area_ptr->length( field_size );

#   if defined JZ_ISO_TEST
        xlat( area_ptr->byte_ptr(), area_ptr->byte_ptr(), area_ptr->length(), ebc2iso );
#   endif
  exceptions
}

// ---------------------Typed_indexed_file<OBJECT,KEY,OBJECT_FIELD,KEY_FIELD_DESCR>::build_key_record

template< class OBJECT, class KEY, class OBJECT_FIELD, class KEY_FIELD_DESCR >
void Typed_indexed_file<OBJECT,KEY,OBJECT_FIELD,KEY_FIELD_DESCR>::build_key_record( const KEY& key, Area* area_ptr )
{
    uint         key_length = _key_field_descr.type().field_size( );

    area_ptr->allocate_min( key_length );

    ::write( _key_field_descr.type(), area_ptr->byte_ptr(), area_ptr->byte_ptr() + key_length, key );
    area_ptr->length( key_length );
}

//---------------------------------Typed_indexed_file<OBJECT,KEY,OBJECT_FIELD,KEY_FIELD_DESCR>::set

template< class OBJECT, class KEY, class OBJECT_FIELD, class KEY_FIELD_DESCR >
void Typed_indexed_file<OBJECT,KEY,OBJECT_FIELD,KEY_FIELD_DESCR>
     ::set( const KEY& key )
{
    Dynamic_area key_buffer;

    build_key_record( key, &key_buffer );
    Base_class::set( key_buffer );  xc;

  exceptions
}

//---------------------------------Typed_indexed_file<OBJECT,KEY,OBJECT_FIELD,KEY_FIELD_DESCR>::del

template< class OBJECT, class KEY, class OBJECT_FIELD, class KEY_FIELD_DESCR >
void Typed_indexed_file<OBJECT,KEY,OBJECT_FIELD,KEY_FIELD_DESCR>
     ::del( const KEY& key )
{
    Dynamic_area key_buffer;

    build_key_record( key, &key_buffer );
    Base_class::del( key_buffer );
}

template< class OBJECT, class KEY, class OBJECT_FIELD, class KEY_FIELD_DESCR >
void Typed_indexed_file<OBJECT,KEY,OBJECT_FIELD,KEY_FIELD_DESCR>
     ::del()
{
    Base_class::del();
}
*/
//---------------------------Typed_indexed_file<OBJECT,KEY,OBJECT_FIELD,KEY_FIELD_DESCR>::file_spec
#if 0
template< class OBJECT, class KEY, class OBJECT_FIELD, class KEY_FIELD_DESCR >
File_spec Typed_indexed_file<OBJECT,KEY,OBJECT_FIELD,KEY_FIELD_DESCR>::file_spec()
{
    //static const OBJECT_FIELD           object_field_type;
    //static const KEY_FIELD_DESCR::Type  key_field_type;

    /*static*/ File_spec          spec ( _object_field_descr.field_size(),
                                         Key_spec( _key_field_descr.offset(),
                                                   _key_field_descr.type().field_size(),
                                                   Key_spec::ka_none,
                                                   &_key_field_descr ) );

    spec.field_type_ptr( &_object_field_descr );

    return spec;
}
#endif

} //namespace sos
