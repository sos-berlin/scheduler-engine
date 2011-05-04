#include "precomp.h"
//#define MODULE_NAME "subrange"
// subrange.cpp

#if 0


//#pragma implementation

#include <stdlib.h>
#include "sos.h"
#include "subrange.h"

using namespace std;
namespace sos {


void subrange_fail(
    const char* source,
    long        value,
    const char* typ,
    long        mini,
    long        maxi
)
{
    SHOW_ERR( source << ": " << value << " ist nicht in Subrange<"
              << typ << ',' << mini << ',' << maxi << ">" );

    char value_text [ 17 ];  itoa( value, value_text, 10 );
    char mini_text [ 17 ];   itoa( mini, mini_text, 10 );
    char maxi_text [ 17 ];   itoa( maxi, maxi_text, 10 );
    //abort();
    throw Subrange_error( "SOS-1139", Msg_insertions( value_text, mini_text, maxi_text ) );
}

} //namespace sos

#endif
