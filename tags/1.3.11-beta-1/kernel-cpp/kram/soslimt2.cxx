#include "precomp.h"
//#define MODULE_NAME "soslimt2"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

//#pragma implementation

#include "sosstrng.h"         // Für storable.h: Storable_as<Sos_string>
#include "sos.h"
#include "sosfield.h"
#include "soslimtx.h"

using namespace std;
namespace sos {

// Aus soslimtx.cxx herausgelöst, damit Solaris C++ 4.0.1 zufrieden ist

//------------------------------------------------------------Sos_limited_text_type::_type_info

Type_info Sos_limited_text_type::_type_info;

SOS_INIT( sos_limited_text )
{
    Sos_limited_text_type::_type_info._std_type      = std_type_varchar;
    Sos_limited_text_type::_type_info._name          = "Sos_limited_text";
    Sos_limited_text_type::_type_info._max_size      = 32000;
    Sos_limited_text_type::_type_info._max_precision = 32000;
    Sos_limited_text_type::_type_info.normalize();
};

//-------------------------------------------------Sos_limited_text_type::Sos_limited_text_type

Sos_limited_text_type::Sos_limited_text_type( int size )
:
    _size ( size )
{
    Sos_limited_text<1>* t = NULL;
    _field_size = (Byte*)&t->_text[0] + sizeof t->_text - 1 - (Byte*)t + size;
  //_field_size = sizeof (Sos_limited_text<1>) - 1 + size;
}

//------------------------------------------------------------Sos_limited_text_type::_get_param

void Sos_limited_text_type::_get_param( Type_param* param ) const
{
    param->_info_ptr     = &_type_info;
    param->_precision    = _size;
}

//-------------------------------------------------------------Sos_limited_text_type::construct

void Sos_limited_text_type::construct( Byte* p ) const
{
    new (p) String0_area( ((Sos_limited_text<1>*)p)->_text, _size );
    ((String0_area*)p)->_length = 0;
}

//------------------------------------------------------------Sos_limited_text_type::field_copy

void Sos_limited_text_type::field_copy( Byte* p, const Byte* s ) const
{
    construct( p );
    ((String0_area*)p)->length( ((String0_area*)s)->length() );
    memcpy( ((Sos_limited_text<1>*)p)->_text, ((Sos_limited_text<1>*)s)->_text,
            ((Sos_limited_text<1>*)s)->length() );
}

//-------------------------------------------------------------------add_sos_limited_text_field

void add_sos_limited_text_field(
    Record_type* t, const String0_area* offset, const char* name, int size,
    const Bool* null_offset, uint flags )
{
    Sos_ptr<Sos_limited_text_type> ft = SOS_NEW( Sos_limited_text_type( size ) );
    t->add_field( ft, name, (long)offset, (long)null_offset, flags );
}

} //namespace sos
