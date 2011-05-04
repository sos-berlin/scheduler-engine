#include "precomp.h"
//#define MODULE_NAME "exp10"
//#define COPYRIGHT   "(c) 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"


#include "../kram/sos.h"
#include "exp10.h"

using namespace std;
namespace sos {


Big_int exp10_table [] = {                  1L,        // 1e0
                                           10L,
                                          100L,
                                         1000L,
                                        10000L,
                                       100000L,
                                      1000000L,
                                     10000000L,
                                    100000000L,
                                   1000000000L         // 1e9
#if defined SYSTEM_INT64
                           , I64( 10000000000 ),
                            I64( 100000000000 ), 
                           I64( 1000000000000 ), 
                          I64( 10000000000000 ), 
                         I64( 100000000000000 ), 
                        I64( 1000000000000000 ), 
                       I64( 10000000000000000 ), 
                      I64( 100000000000000000 ), 
                     I64( 1000000000000000000 )         // 1e18
#endif
};

} //namespace sos

