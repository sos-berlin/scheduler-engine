/* video.inl                                                (c) SOS GmbH Berlin
                                                                Joacim Zschimmer
                                                                Jörg Schwiemann
*/

namespace sos {

inline Video_attr::Video_attr()
 : _attr( 0x08 /* half_light */ )
{
}

inline Video_attr::Video_attr( unsigned char attr )
{
    _attr = attr;
}

inline Video_attr::operator unsigned char() const
{
    return _attr;
}

inline void     Video_attr::dark       ( Bool b ) { if( b )  _attr &= ~0x04; else _attr |= 0x04; }
inline Bool     Video_attr::dark       () const   { return (_attr & 0x04) == 0; }
inline void     Video_attr::half_light ( Bool b ) { if( b )  _attr &= ~0x08; else _attr |= 0x08; }
inline Bool     Video_attr::half_light () const   { return (_attr & 0x08) == 0; }
inline void     Video_attr::blinking   ( Bool b ) { if( b )  _attr |= 0x80; else _attr &= ~0x80; }
inline Bool     Video_attr::blinking   () const   { return (_attr & 0x80) != 0; }
inline void     Video_attr::underlined ( Bool b ) { if( b )  _attr |= 0x01; else _attr &= ~0x01; }
inline Bool     Video_attr::underlined () const   { return (_attr & 0x01) != 0; }
inline int      Video_attr::operator== ( const Video_attr& a ) const { return _attr == a._attr; }
inline int      Video_attr::operator!= ( const Video_attr& a ) const { return _attr != a._attr; }

inline          Video_char::Video_char() {}
inline          Video_char::Video_char( char c, Video_attr a )  { _char = c; _attr = a; }
inline char     Video_char::chr() const  { return _char; }
inline Video_attr Video_char::attr() const  { return _attr; }

inline          Video_pos::Video_pos() {}
inline          Video_pos::Video_pos(int offset)  { _offset = offset; }

inline Video_pos::Video_pos(int column0, int line0)
{
    _offset = line0 * video_line_length + column0;
}

inline Video_pos::Video_pos( const Video_pos& vp )
{
    _offset = vp._offset;
}

inline Video_pos Video_pos::operator++( int )
{
    return (int) _offset++;
}

inline int Video_pos::offset() const
{
    return _offset;
}

inline int Video_pos::column0() const
{
     return _offset % video_line_length;
}

inline int Video_pos::line0() const
{
     return _offset / video_line_length;
}


inline char& Video::Buffer::chr( Video_pos pos )
{
     return _char_buffer[ pos.offset() ];
}

inline Video_attr& Video::Buffer::attr( Video_pos pos )
{
     return _attr_buffer[ pos.offset() ];
}

} //namespace sos