// $Id: sysxcept.cxx 11394 2005-04-03 08:30:29Z jz $

#include "precomp.h"
#include "sos.h"

namespace sos
{
    string exception_name( const exception& x )
    { 
        return z::name_of_type( x ); 
    }
}
