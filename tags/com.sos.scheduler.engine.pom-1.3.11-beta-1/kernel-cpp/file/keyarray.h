#ifndef __KEYARRAY_H
#define __KEYARRAY_H

//-----------------------------------------------------------------------------------Key_buffer

struct Fixed_length_key_array : Sos_self_deleting
{
                                    Fixed_length_key_array( const Sos_ptr<Field_type>& );
                                   ~Fixed_length_key_array();

    Byte*                           operator []         ( int );
    int                             operator []         ( const Byte* );
    void                            add                 ( const Byte* );
    void                            last_index          ( int );

    Fill_zero                      _zero_;
  //DECLARE_PRIVATE_MEMBER( int,    first_index )
    void                            first_index         ( int i )   { _array.first_index( i ); }
    int                             first_index         () const    { return _array.first_index(); }
  //DECLARE_PRIVATE_MEMBER( int,    allocation_increment )

  private:
    //Dynamic_area                   _buffer;
    Sos_simple_array<Byte*>        _array;
    Sos_ptr<Field_type>            _type;
    int                            _entry_size;
};

//-----------------------------------------------------------Fixed_length_key_array::operator[]

inline Byte* Fixed_length_key_array::operator[] ( int i )
{
    //return _buffer.byte_ptr() + ( i - _first_index ) * _entry_size;
    return _array[ i ];
}

#endif
