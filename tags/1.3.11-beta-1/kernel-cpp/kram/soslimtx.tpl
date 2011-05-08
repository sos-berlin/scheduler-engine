// soslimtx.tpl

//-----------------------------------------------------Sos_limited_text<SIZE>::Sos_limited_text
#if defined SYSTEM_SOLARIS

namespace sos
{

template< int SIZE >
Sos_limited_text<SIZE>::Sos_limited_text()
:
    String0_area( _text, SIZE )
{
    _length = 0;
}


} //namespace sos

#else
    // inline in soslimtx.h
#endif
