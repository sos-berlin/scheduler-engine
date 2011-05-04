//#define MODULE_NAME "filebase"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "precomp.h"
#include "../kram/sysdep.h"
#include "../kram/sos.h"
#include "../file/filebase.h"
#include "../kram/sosfield.h"

namespace sos {

//--------------------------------------------------------------------Key_spec::field_descr_ptr

Key_spec& Key_spec::field_descr_ptr( const Field_descr* p )
{
    _field_descr_ptr = (Field_descr*)p;
    return *this;
}

//--------------------------------------------------------------------File_spec::field_type_ptr

File_spec& File_spec::field_type_ptr( const Record_type* p )
{
    _field_type_ptr = (Record_type*)p;
    return *this;
}

} //namespace sos
