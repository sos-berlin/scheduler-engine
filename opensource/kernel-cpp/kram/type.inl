// type.inl                                             (c) SOS GmbH Berlin



#if 0  // input_value() ist noch nicht definiert
inline void Type::save_value( ostream& s, const void* object_ptr ) const
{
    print_value( s, object_ptr );
}

inline void Type::restore_value( istream& s, void* object_ptr ) const
{
    input_value( s, object_ptr );
}
#endif

//--------------------------------------------------------------------------------------Typed::name

inline const char* Type::name() const
{
    return _name_ptr;
}

//----------------------------------------------------------------------Type_fundamental::is_signed

inline Bool Type_fundamental::is_signed() const
{
    return _signed;
}

//-----------------------------------------------------------------------Type_array::no_of_elements

inline int Type_array::no_of_elements() const
{
    return _no_of_elements;
}

//---------------------------------------------------------------------Type_array::element_distance

inline int Type_array::element_distance() const
{
    return _element_distance;
}

//-------------------------------------------------------------------------Type_array::element_type

inline const Type& Type_array::element_type() const
{
    return *_type_ptr;
}

//-----------------------------------------------------------------------------Member_descr::offset

inline int Member_descr::offset() const
{
    return _offset;
}

//-----------------------------------------------------------------------Type_struct::no_of_members

inline int Type_struct::no_of_members() const
{
    return _no_of_members;
}

//-----------------------Typed_not_virtual_without_destructor::Typed_not_virtual_without_destructor

inline Typed_not_virtual_without_destructor::Typed_not_virtual_without_destructor( const Type* type_ptr )
{
    Typed::insert_object( this, type_ptr );
}

//------------------------------------------------------------Typed_not_virtual::~Typed_not_virtual

inline Typed_not_virtual::~Typed_not_virtual()
{
    Typed::remove_object( this );
}

//-------------------------------------------------------------Typed_not_virtual::Typed_not_virtual

inline Typed_not_virtual::Typed_not_virtual( const Type* type_ptr )
  : Typed_not_virtual_without_destructor( type_ptr )
{}

//-------------------------------------------------------------------------------------Typed::print

inline void Typed::object_print( ostream& s ) const
{
    object_type().print_value( s, this );
}

//-----------------------------------------------------------Object_window_ref
