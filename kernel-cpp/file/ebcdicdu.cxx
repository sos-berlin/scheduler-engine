#define MODULE_NAME "ebcdicdu"
#define COPYRIGHT   "(c) SOS GmbH Berlin"
#define AUTHOR      "Joacim Zschimmer"

#include <stdio.h>                  // perror()
#include <string.h>

#include <sos.h>

extern Byte e2a_printable [256];

int main( int argc, char** argv )
{
    char                    hex_tab [256] [2];
    const char              hex_t [] = "0123456789ABCDEF";

    const int bytes_per_line    = 32;
    Byte     last_line          [ bytes_per_line ];
    Bool     last_line_valid    = false;
    char     repeat             = ' ';
    long     addr               = 0;

    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            hex_tab [i * 16 + j] [0] = hex_t [i];
            hex_tab [i * 16 + j] [1] = hex_t [j];
        }
    }


    while(1)
    {
        Byte buffer [ bytes_per_line ];

        cin.read( (char*)buffer, sizeof buffer );
        int length = cin.gcount();
        if( length == 0 )  break;

        if( memcmp( buffer, last_line, bytes_per_line ) == 0  &&  last_line_valid ) {
            repeat = '=';
        } else {
            char  line [ 4*bytes_per_line ];
            char* p;

            //memset( line, ' ', sizeof line );

            line[ 0 ] = repeat;
            *(uint2*)(line + 1) = *(uint2*)hex_tab[ (Byte) (addr >> 24) ];
            *(uint2*)(line + 3) = *(uint2*)hex_tab[ (Byte) (addr >> 16) ];
            *(uint2*)(line + 5) = *(uint2*)hex_tab[ (Byte) (addr >>  8) ];
            *(uint2*)(line + 7) = *(uint2*)hex_tab[ (Byte)  addr        ];
            line[ 9 ] = ' ';
            p = line + 10;

            for (int j = 0; j < length; j++) {
                if ((j & 3) == 0)  *p++ = ' ';
                int b = *( buffer + j );
                *(uint2*)p = *(uint2*)hex_tab[ b ];
                p += 2;
            }

            *p++ = ' ';
            *p++ = ' ';

            xlat( p, buffer, length, e2a_printable );
            p += length;
            *p++ = '\n';

            cout.write( line, p - line );
            if( cout.fail() )  goto error;

            memcpy( last_line, buffer, bytes_per_line );
            last_line_valid = true;
            repeat = ' ';
        }
        addr += bytes_per_line; 
    }

    if( cin.fail() )  goto error;
    
    cout << "        " << repeat << '\n';
    repeat = ' ';

    return 0;

  error:
    //? A perror();
    //cerr << "i/o error\n";
    return 1;  
}

