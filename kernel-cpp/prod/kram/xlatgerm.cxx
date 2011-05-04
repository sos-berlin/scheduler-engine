#include "precomp.h"
//#define MODULE_NAME "xlatgerm"
//#define COPYRIGHT   "(c) SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "../kram/sos.h"
#include "../kram/xlat.h"
#include "../kram/xlatgerm.h"

using namespace std;
namespace sos {

char german_to_iso_table[ 256 ];

static void init()
{
    for( int i = 0; i < 256; i++ ) {
        german_to_iso_table[ i ] = i;
    }

    german_to_iso_table[ (uchar) '{' ] = 'ä';
    german_to_iso_table[ (uchar) '|' ] = 'ö';
    german_to_iso_table[ (uchar) '}' ] = 'ü';
    german_to_iso_table[ (uchar) '[' ] = 'Ä';
    german_to_iso_table[ (uchar) '\\'] = 'Ö';
    german_to_iso_table[ (uchar) ']' ] = 'Ü';
    german_to_iso_table[ (uchar) '~' ] = 'ß';
}

struct Xlatgerm
{
    Xlatgerm()
    {
        init();
    }
};

static Xlatgerm xlatgerm;


} //namespace sos
