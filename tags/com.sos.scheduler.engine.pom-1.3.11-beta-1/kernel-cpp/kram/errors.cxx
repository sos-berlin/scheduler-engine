#include <precomp.h>
#define MODULE_NAME "errors"
//                                                    (c) SOS GmbH Berlin
//                                                        Joacim Zschimmer

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sos.h>
#include <anyfile.h>
#include <log.h>

int main (void)
{
Any_file                error_file;
FILE*                   in;
char                    text[ 300 ];

error_file.open( "errors.txt",
                 Abs_file_base::Open_mode( Abs_file_base::out | Abs_file_base::trunc | Abs_file_base::unsafe ),
                 File_spec( 512, Key_spec( 0, 32 ) ));   xc;

in = stdin;  //fopen ("errors.txt", "r");
if (in == NULL) {
    perror ("errors.txt läßát sich nichtö ”ffnen");
    exit (1);
}

loop {
    char* p = fgets (text, sizeof text, in);
    if( !p )  break;
    if (text [strlen (text) - 1] == '\n')
        text [strlen (text) - 1] = 0;
    error_file.insert( Const_area( text, strlen (text) ));  xc;
}

error_file.close();  xc;
return 0;

exception_handler:
   cerr << "Exception " << _XC.name() << " " << _XC.error_code() << endl;
   discard_exception();
   return 1;
} // main

