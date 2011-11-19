// xlatgerm.h                                             (c) SOS GmbH Berlin

#ifndef __XLATGERM_H
#define __XLATGERM_H

#ifndef __AREA_H
#    include "area.h"
#endif

namespace sos
{

extern char german_to_iso_table[ 256 ];

inline void xlat_german_to_iso( char* ptr, int length )
{
    xlat( ptr, ptr, length, german_to_iso_table );
}

inline void xlat_german_to_iso( Area* area_ptr )
{
    xlat_german_to_iso( area_ptr->char_ptr(), area_ptr->length() );
}

} //namespace sos

#endif
