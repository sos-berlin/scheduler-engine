// $Id$

#include "precomp.h"
#include "sos.h"

namespace sos
{
    string exception_name( const exception& x )
    { 
        return z::name_of_type( x ); 
    }
}
