/* e9750hlp.inl                                     (c) SOS GmbH Berlin
                                                        Joacim Zschimmer
*/

//-----------------------------------------------Video_simple_field_descr::Video_simple_field_descr

inline Video_simple_field_descr::Video_simple_field_descr()
 : _pos    ( Video_pos( 0, 0 ) ),
   _length ( 0                 )
{}

//-----------------------------------------------Video_simple_field_descr::Video_simple_field_descr

inline Video_simple_field_descr::Video_simple_field_descr( const Video_pos& pos, int length )
 : _pos    ( pos    ),
   _length ( length )
{}

//--------------------------------------------------------------------Video_simple_field_descr::pos

inline Video_pos Video_simple_field_descr::pos() const
{
    return _pos;
}

//-----------------------------------------------------------------Video_simple_field_descr::length

inline int Video_simple_field_descr::length() const
{
    return _length;
}

//-----------------------------------------------------------------Video_rectangle::Video_rectangle

inline Video_rectangle::Video_rectangle()
 :  _pos  ( Video_pos( 0, 0 ) ),
    _pos2 ( Video_pos( 0, 0 ) )
{}

//-----------------------------------------------------------------Video_rectangle::Video_rectangle

inline Video_rectangle::Video_rectangle( const Video_pos& links_oben, const Video_pos& rechts_unten )
 :  _pos  ( links_oben ),
    _pos2 ( rechts_unten )
{}

//---------------------------------------------------------------Video_rectangle::field_descr_count

inline int Video_rectangle::field_descr_count() const
{
    return _pos2.line0() - _pos.line0() + 1;
}

//---------------------------------------------Video_mask_identification::Video_mask_identification

inline Video_mask_identification::Video_mask_identification()
{
}

//-----------------------------------------------------------Video_mask_identification::field_descr

inline void Video_mask_identification::add( const Video_simple_field_descr& field_descr )
{
    prepend( field_descr, &_field_descr_list );
}

//-----------------------------------------------------Video_mask_identification::field_descr_count

inline int Video_mask_identification::field_descr_count() const
{
    return length( _field_descr_list );
}

//-------------------------------------------------------------------Sos_help_entry::Sos_help_entry

inline Sos_help_entry::Sos_help_entry()
 :  _help_id        ( 0  )
{}

//-------------------------------------------------------------------Sos_help_entry::Sos_help_entry

inline Sos_help_entry::Sos_help_entry( const char* filename, int help_id )
 :  _filename       ( filename ),
    _help_id        ( help_id )
{
}

//-------------------------------------------------------Video_help_rectangle::Video_help_rectangle

inline Video_help_rectangle::Video_help_rectangle()
{
}

//-------------------------------------------------------Video_help_rectangle::Video_help_rectangle

inline Video_help_rectangle::Video_help_rectangle( const Video_rectangle& rectangle, const Sos_help_entry& help_entry )
 : _rectangle  ( rectangle  ),
   _help_entry ( help_entry )
{
}


