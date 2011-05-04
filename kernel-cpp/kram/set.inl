// set.inl

inline Bit_set::~Bit_set()  
{ 
    if( _delete ) {
        delete [] _bit_map_ptr;
    }
}

inline Bool Bit_set::valid() const
{
    return Bool( _bit_map_ptr != 0 ); 
}

inline Bool Bit_set::is_elem( uint4 elem ) const
{
    assert (elem / 8 < (uint4) _size);
    return in_set (_bit_map_ptr, elem);
}

inline void Bit_set::include( uint4 elem )
{
    assert (elem / 8 < (uint4) _size);
    set_incl (_bit_map_ptr, elem);
}

inline void Bit_set::exclude( uint4 elem )  
{
    assert (elem / 8 < (uint4) _size);
    set_excl (_bit_map_ptr, elem);
}

