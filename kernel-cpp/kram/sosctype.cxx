#include "precomp.h"
//#define MODULE_NAME "sosctype"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

//#include "../kram/ctype.h>
#include "../kram/sos.h"
#include "tabucase.h"
#include "sosctype.h"
#include "log.h"

using namespace std;
namespace sos {



char           tabucase_   [256];
char           tablcase_   [256];
unsigned char  sosctype_tab[1+256];     // incl. EOF (-1)


static
struct Sosctype_init
{
    Sosctype_init();
} x;


Sosctype_init::Sosctype_init()
{
    for( int i = 0  ; i <= 255; i++ )  {
        //13.1.97 if( isspace( i ) )  sosctype_tab[ 1 + i ] |= SOS_IS_SP;
        tabucase_[ i ] = (char)i;
        tablcase_[ i ] = (char)i;
    }

    sosctype_tab[ 1 + (unsigned)' '  ] |= SOS_IS_SP;
    sosctype_tab[ 1 + (unsigned)'\t' ] |= SOS_IS_SP;
    sosctype_tab[ 1 + (unsigned)'\v' ] |= SOS_IS_SP;
    sosctype_tab[ 1 + (unsigned)'\r' ] |= SOS_IS_SP;
    sosctype_tab[ 1 + (unsigned)'\n' ] |= SOS_IS_SP;
    sosctype_tab[ 1 + (unsigned)'\f' ] |= SOS_IS_SP;


    const unsigned char* k;

  //sosctype_tab[ (unsigned)'ß' &= ~SOS_IS_HIGH;   // ???


    k = (unsigned char*) "0123456789";
    while( *k )  sosctype_tab[ 1 + *k++ ] |= SOS_IS_DIG | SOS_IS_HEX | SOS_IS_GRAPH;

    k = (unsigned char*) "aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ"
                         "àÀáÁâÂãÃäÄåÅæÆçÇèÈéÉêÊëËìÌíÍîÎïÏðÐñÑòÒóÓôÔõÕöÖøØùÙúÚûÛüÜýÝþÞ";
                       //"ãÃäÄæÆçÇèÈéÉêÊëËìÌíÍîÎïÏðÐñÑòÒóÓôÔõÕöÖøØùÙúÚûÛüÜýÝþÞ";
                       //"šŠœŒÿŸ";   // ist das auch ISO-8859-1? (0x81..0x9F)

    while( *k ) {
        sosctype_tab[ 1 + *k    ] |= SOS_IS_LOW | SOS_IS_GRAPH;
        sosctype_tab[ 1 + *(k+1)] |= SOS_IS_UPP | SOS_IS_GRAPH;
        tabucase_[ *k     ] = (char)*(k+1);
        tablcase_[ *(k+1) ] = (char)*k;
        k += 2;
    }

    sosctype_tab[ 1 + (unsigned char)'ß' ] |= SOS_IS_LOW;

    k = (unsigned char*) "abcdefABCDEF";
    while( *k )  sosctype_tab[ 1 + *k++ ] |= SOS_IS_HEX;

    for( int m = 0; m < (unsigned)' '; m++ )  sosctype_tab[ 1 + m ] |= SOS_IS_CTL;

    k = (unsigned char*) "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
                         //"‚ƒ„…†‡ˆ‰‹‘’“”•–—˜™›šŠœŒŸ"       // ist das auch ISO-8859-1? (0x81..0x9F)
                         "¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¼½¾¿×÷ßÿ¸¹º»¼½";

    while( *k )  sosctype_tab[ 1 + *k++ ] |= SOS_IS_GRAPH;

    sosctype_tab[ 1 + (unsigned)' ' ] |= SOS_IS_BLK;
}

} //namespace sos
