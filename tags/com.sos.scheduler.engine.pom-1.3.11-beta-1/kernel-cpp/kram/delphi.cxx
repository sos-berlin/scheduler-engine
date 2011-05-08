#include "precomp.h"
//#define MODULE_NAME "delphi"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "sos.h"
#include "sosfield.h"
#include "sosdate.h"
#include "delphi.h"

using namespace std;
namespace sos {

//---------------------------------------------------------------------------------------static

Delphi_string_type     delphi_string_type;
//Delphi_tdatetime_type  delphi_tdatetime_type;

Type_info Delphi_string_type::_type_info;

SOS_INIT( Delphi_string )
{
    Delphi_string_type::_type_info._std_type = std_type_char;
    Delphi_string_type::_type_info._name     = "Delphi_string";
    Delphi_string_type::_type_info._max_size = 255;
    Delphi_string_type::_type_info.normalize();
}

//---------------------------------------------------------------Delphi_string_type::write_text

void Delphi_string_type::write_text( const Byte* p, Area* buffer, const Text_format& ) const
{
    buffer->assign( (const char*)p + 1, (int)(Byte)*p );
}

//----------------------------------------------------------------Delphi_string_type::read_text

void Delphi_string_type::read_text( Byte* p, const char* t, const Text_format& ) const
{
    Area area ( p + 1, _field_size - 1 );
    area.assign( t );
    *p = area.length();
}

//------------------------------------------------------------Delphi_tdatetime_type::write_text
/*
static const int4       tage_bis_1900_01_01 = 0;
static const double     rundung             = 0.0;  // 0.5e0L / ( 24.0L * 3600.0L );

void Delphi_tdatetime_type::write_text( const Byte* p, Area* buffer, const Text_format& f ) const
{
    Sos_date datum ( 1900, 1, 1 );
    datum += (int4) ( *(double*)p + rundung ) - tage_bis_1900_01_01;

    sos_optional_date_type.write_text( (const Byte*)&datum, buffer, f );
}

//-------------------------------------------------------------Delphi_tdatetime_type::read_text

void Delphi_tdatetime_type::read_text( Byte* p, const Const_area& area, const Text_format& f ) const
{
    Sos_date datum;
    sos_optional_date_type.read_text( (Byte*)&datum, area, f );

    *(double*)p = datum - Sos_date( 1900, 1, 1 ) + tage_bis_1900_01_01;
}
*/

} //namespace sos
