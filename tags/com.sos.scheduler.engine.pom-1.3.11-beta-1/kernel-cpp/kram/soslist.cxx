#include "precomp.h"
//#define MODULE_NAME "soslist"
/* soslist.cpp                                          (c) SOS GmbH Berlin
                                                        Joacim Zschimmer
*/

//#pragma implementation

#include "sos.h"
#include "xception.h"
#include "soslist.h"

using namespace std;
namespace sos {

//---------------------------------------------------------------------Sos_simple_list_node_0::last

Sos_simple_list_node_0* Sos_simple_list_node_0::last() /*const*/
{
    Sos_simple_list_node_0* p = this;

    while( !empty( p->tail() ) ) {
        p = p->tail();
    }

    return p;
}

//------------------------------------------------------------------------------------delete_list()

void delete_list( Sos_simple_list_node_0** node_ptr )
{
    Sos_simple_list_node_0* p = *node_ptr;  *node_ptr = 0;

    while( !empty( p ) ) {
        Sos_simple_list_node_0* p2 = p->tail();
        p->_tail = 0;
        delete p;
        p = p2;
    }

    *node_ptr = 0;
}


} //namespace sos
