#include "zschimmer.h"
#include "perl.h"

using namespace zschimmer;

int main( int, char** )
{
    Perl perl;

    perl.init();
    perl.parse( "$a='Hallo, hier ist Perl\n'; print $a;" );
    perl.close();
    return 0;
}
