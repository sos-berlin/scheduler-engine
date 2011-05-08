// stream.inl                                           (c) SOS GmbH Berlin

#ifndef __XCEPTION_H
#   include <xception.h>
#endif

inline void Stream_base::set_eor_char( int c1, int c2 )
{
    _eor_char_1 = c1;
    _eor_char_2 = c2;
}

inline Input_stream::Input_stream()  {}

inline Output_stream& Output_stream::operator << ( Const_area area )  { write( area );  return *this; }
inline Output_stream& Output_stream::operator << (          char p )  { return *this << CONST_AREA( p ); }
inline Output_stream& Output_stream::operator << ( unsigned char p )  { return *this << CONST_AREA( p ); }
inline Output_stream& Output_stream::operator << (   signed char p )  { return *this << CONST_AREA( p ); }
inline Output_stream& Output_stream::operator << ( const          char* p )  { return *this << Const_area( p, strlen( p ) ); }
//inline Output_stream& Output_stream::operator << ( const unsigned char* p )  { return *this << Const_area( p, strlen( p ) ); }
//inline Output_stream& Output_stream::operator << ( const   signed char* p )  { return *this << Const_area( p, strlen( p ) ); }
inline Output_stream& Output_stream::operator << ( Output_stream& (*p)(Output_stream&) ) { return p( *this ); }

inline Input_stream& Input_stream::operator >> ( Area& area       )  { read( area );  return *this; }
inline Input_stream& Input_stream::operator >> (          char& p )  { return *this >> AREA( p ); }
inline Input_stream& Input_stream::operator >> ( unsigned char& p )  { return *this >> AREA( p ); }
inline Input_stream& Input_stream::operator >> (   signed char& p )  { return *this >> AREA( p ); }
inline Input_stream& Input_stream::operator >> ( Input_stream& (*p)(Input_stream&) ) { return p( *this ); }

inline void Output_stream::put( unsigned char c )  { *this << c; }

inline void Output_stream::write( Const_area area )            
{
    write_area( area ); 
    _write_position += area.length();
}

inline void Output_stream::write( const void* p, unsigned int length )
{
    write( Const_area( p, length )); 
}

inline void Output_stream::write_end_of_record()
{
    _write_end_of_record();
    _write_record_position = _write_position;
}

inline int4 Output_stream::write_position() const
{
    return _write_position;
}

inline int4 Output_stream::write_record_position() const
{
    return _write_record_position;
}

inline int4 Output_stream::written_record_length() const
{
    return _write_position - _write_record_position;
}

//----------------------------------------------------------------Manipulatoren

inline Output_stream& end_of_record( Output_stream& s )
{
    s.write_end_of_record();
    return s;
}

inline Input_stream& end_of_record( Input_stream& s )
{
    s.skip_end_of_record();
    return s;
}

//-----------------------------------------------------------------------------

inline Streambuf_stream::Streambuf_stream( streambuf* streambuf_ptr )
 :  _streambuf_ptr                   ( streambuf_ptr ),
    _delete_streambuf                ( false ),
    _ignore_read_until_end_of_record ( false )
{}


inline void Streambuf_stream::delete_streambuf( Bool b )
{
    _delete_streambuf = b;
}



