#include "precomp.h"
//#define MODULE_NAME "iso2ebc"
// iso2ebc.cpp                                         (c) SOS GmbH Berlin

#include <stdlib.h>

#if defined __BORLANDC__
#   include <mem.h>
#else
#   include <string.h>
#endif

#include "sos.h"

using namespace std;
namespace sos {


#define nix 0x41

Byte iso2ebc [256];
Byte iso2ebc_german [256];
char ebc2iso_german [256];
char ebc2iso_german_0_is_blank [256];

extern char mvs2iso [256];  // in ebc2iso.cxx
       Byte iso2mvs [256];
       Byte iso2mvs_german [256];
       char mvs2iso_german [256];
       char mvs2iso_german_0_is_blank [256];


/* = {
  nix , nix , nix , nix , nix , nix , nix , nix ,  // 00  Ascii-Steuerzeichen
  nix , nix , nix , nix , nix , nix , nix , nix ,  // 08
  nix , nix , nix , nix , nix , nix , nix , nix ,  // 10
  nix , nix , nix , nix , nix , nix , nix , nix ,  // 18

  0x40, 0x5A, 0x7F, 0x7B, 0x5B, 0x6C, 0x50, 0x7D,  // 20   !#"$%&'
  0x4D, 0x5D, 0x5C, 0x4E, 0x6B, 0x60, 0x4B, 0x61,  // 28  ()*+,-./
  0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,  // 30  01234567
  0xF8, 0xF9, 0x7A, 0x5E, 0x4C, 0x7E, 0x6E, 0x6F,  // 38  89:;<=>?
  0x7C, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,  // 40  @ABCDEFG
  0xC8, 0xC9, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6,  // 48  HIJKLMNO
  0xD7, 0xD8, 0xD9, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6,  // 50  PQRSTUVW
  0xE7, 0xE8, 0xE9, 0xBB, 0xBC, 0xBD, 0x6A, 0x6D,  // 58  XYZ[\]^_
  0x4A, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,  // 60  `abcdefg
  0x88, 0x89, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96,  // 68  hijklmno
  0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6,  // 70  pqrstuvw
  0xA7, 0xA8, 0xA9, 0xFB, 0x4F, 0xfd, 0xFF, 0x7,   // 78  xyz{|}~±

  0x68, 0xDC, 0x51, 0x42, 0x43, 0x44, 0x47, 0x48,  // 80  ÄÅÇÉÑÖÜá
  0x52, 0x53, 0x54, 0x57, 0x56, 0x58, 0x63, 0x67,  // 88  àâäãåçéè
  0x71, 0x9C, 0x9E, 0xCB, 0xCC, 0xCD, 0xDB, 0xC0,  // 90  êëíìîïñó
  0xDF, 0xEC, 0xFC, 0xB0, 0xB1, 0xB2, nix , nix ,  // 98  òôöõúùûü
  0x45, 0x55, 0xCE, 0xDE, 0x49, 0x69, 0x9A, 0x9B,  // A0 †°¢£§•¶ß
  0xAB, nix , 0xBA, 0xB8, 0xB7, nix , 0x8A, 0x8B,  // A8  ®©™´¨≠ÆØ
  nix , nix , nix , nix , nix , nix , nix , nix ,  // B0  ∞±≤≥¥µ∂∑
  nix , nix , nix , nix , nix , nix , nix , nix ,  // B8  ∏π∫ªºΩæø
  nix , nix , nix , nix , 0xBB, nix , nix , nix ,  // C0  ¿¡¬√ƒ≈∆«
  nix , nix , nix , nix , nix , nix , nix , nix ,  // C8  »… ÀÃÕŒœ
  nix , nix , nix , nix , nix , nix , 0xBC, nix ,  // D0  –—“”‘’÷◊
  nix , nix , nix , nix , 0xBD, nix , nix , 0xFF,  // D8  ÿŸ⁄€‹›ﬁﬂ
  nix , 0x59, nix , nix , 0xFB, nix , 0xA0, nix ,  // E0  ‡·‚„‰ÂÊÁ
  nix , nix , nix , nix , nix , 0x70, nix , nix ,  // E8  ËÈÍÎÏÌÓÔ
  nix , nix , nix , nix , nix , nix , 0x4F, nix ,  // F0  ÒÚÛÙıˆ˜
  0x90, 0xB3, nix , nix , 0xFC, nix , 0xAE, 0xDF   // F8  ¯˘˙˚¸˝˛
};*/

// static const car *AsciiCars = "[\\]{|}~";
// static const car *AnsiCars  = "ƒ÷ ‹‰ˆ¸ﬂ";


struct Iso2ebc_init
{
    Iso2ebc_init();
};

static Iso2ebc_init iso2ebc_init;


Iso2ebc_init::Iso2ebc_init()
{
    int i;

    memset( (void*)iso2ebc, nix, 256 );

    for( i = 0; i < 256; i++ ) {
        iso2ebc[ (Byte)ebc2iso[ i ] ] = i;
    }

#   ifdef _DEBUGxxx
    {
        for( int i = 0; i < 256; i++ ) {
            if( (Byte)ebc2iso[ (Byte)iso2ebc[ i ] ] != i )  {
                SHOW_MSG( "Zeichen " << (char)i << " (" << hex << i << dec << ") in iso2ebc wird nicht eindeutig umgesetzt." );
            }
        }
    }
#   endif

    // ebc2iso_text und iso2ebc_text kopieren
    memcpy( ebc2iso_german, ebc2iso, 256 );
    memcpy( iso2ebc_german, iso2ebc, 256 );

    // ebc2iso_german anpassen
    ebc2iso_german[iso2ebc[ (Byte) '[' ]] = 'ƒ';
    ebc2iso_german[iso2ebc[ (Byte) '\\']] = '÷';
    ebc2iso_german[iso2ebc[ (Byte) ']' ]] = '‹';
    ebc2iso_german[iso2ebc[ (Byte) '{' ]] = '‰';
    ebc2iso_german[iso2ebc[ (Byte) '|' ]] = 'ˆ';
    ebc2iso_german[iso2ebc[ (Byte) '}' ]] = '¸';
    ebc2iso_german[iso2ebc[ (Byte) '~' ]] = 'ﬂ';

    iso2ebc_german[(Byte)'ƒ'] = iso2ebc[(Byte)'[' ];
    iso2ebc_german[(Byte)'÷'] = iso2ebc[(Byte)'\\'];
    iso2ebc_german[(Byte)'‹'] = iso2ebc[(Byte)']' ];
    iso2ebc_german[(Byte)'‰'] = iso2ebc[(Byte)'{' ];
    iso2ebc_german[(Byte)'ˆ'] = iso2ebc[(Byte)'|' ];
    iso2ebc_german[(Byte)'¸'] = iso2ebc[(Byte)'}' ];
    iso2ebc_german[(Byte)'ﬂ'] = iso2ebc[(Byte)'~' ];

    memcpy( ebc2iso_german_0_is_blank, ebc2iso_german, 256 );
    ebc2iso_german_0_is_blank[ 0 ] = ' ';

// MVS:
    memset( (void*)iso2mvs, nix, 256 );

    for( i = 0; i < 256; i++ ) {
        iso2mvs[ (Byte)mvs2iso[ i ] ] = i;
    }

#   ifdef _DEBUGxxx
    {
        for( int i = 0; i < 256; i++ ) {
            if( (Byte)mvs2iso[ (Byte)iso2mvs[ i ] ] != i )  {
                SHOW_MSG( "Zeichen " << (char)i << " (" << hex << i << dec << ") in iso2mvs wird nicht eindeutig umgesetzt." );
            }
        }
    }
#   endif


    // mvs2iso_text und iso2mvs_text kopieren
    memcpy( mvs2iso_german, mvs2iso, 256 );
    memcpy( iso2mvs_german, iso2mvs, 256 );

    // mvs2iso_german anpassen
    mvs2iso_german[iso2mvs[ (Byte) '[' ]] = 'ƒ';
    mvs2iso_german[iso2mvs[ (Byte) '\\']] = '÷';
    mvs2iso_german[iso2mvs[ (Byte) ']' ]] = '‹';
    mvs2iso_german[iso2mvs[ (Byte) '{' ]] = '‰';
    mvs2iso_german[iso2mvs[ (Byte) '|' ]] = 'ˆ';
    mvs2iso_german[iso2mvs[ (Byte) '}' ]] = '¸';
    mvs2iso_german[iso2mvs[ (Byte) '~' ]] = 'ﬂ';
    mvs2iso_german[iso2mvs[ (Byte) '@' ]] = 'ß';

    mvs2iso_german[iso2mvs[ (Byte) 'ƒ' ]] = '[';
    mvs2iso_german[iso2mvs[ (Byte) '÷' ]] = '\\';
    mvs2iso_german[iso2mvs[ (Byte) '‹' ]] = ']';
    mvs2iso_german[iso2mvs[ (Byte) '‰' ]] = '{';
    mvs2iso_german[iso2mvs[ (Byte) 'ˆ' ]] = '|';
    mvs2iso_german[iso2mvs[ (Byte) '¸' ]] = '}';
    mvs2iso_german[iso2mvs[ (Byte) 'ﬂ' ]] = '~';
    mvs2iso_german[iso2mvs[ (Byte) 'ß' ]] = '@';

    iso2mvs_german[ (Byte)'ƒ' ] = iso2mvs[ (Byte)'[' ];
    iso2mvs_german[ (Byte)'÷' ] = iso2mvs[ (Byte)'\\'];
    iso2mvs_german[ (Byte)'‹' ] = iso2mvs[ (Byte)']' ];
    iso2mvs_german[ (Byte)'‰' ] = iso2mvs[ (Byte)'{' ];
    iso2mvs_german[ (Byte)'ˆ' ] = iso2mvs[ (Byte)'|' ];
    iso2mvs_german[ (Byte)'¸' ] = iso2mvs[ (Byte)'}' ];
    iso2mvs_german[ (Byte)'ﬂ' ] = iso2mvs[ (Byte)'~' ];
    iso2mvs_german[ (Byte)'ß' ] = iso2mvs[ (Byte)'@' ];

    iso2mvs_german[ (Byte)'[' ] = iso2mvs[ (Byte)'ƒ' ];
    iso2mvs_german[ (Byte)'\\'] = iso2mvs[ (Byte)'÷' ];
    iso2mvs_german[ (Byte)']' ] = iso2mvs[ (Byte)'‹' ];
    iso2mvs_german[ (Byte)'{' ] = iso2mvs[ (Byte)'‰' ];
    iso2mvs_german[ (Byte)'|' ] = iso2mvs[ (Byte)'ˆ' ];
    iso2mvs_german[ (Byte)'}' ] = iso2mvs[ (Byte)'¸' ];
    iso2mvs_german[ (Byte)'~' ] = iso2mvs[ (Byte)'ﬂ' ];
    iso2mvs_german[ (Byte)'@' ] = iso2mvs[ (Byte)'ß' ];

    memcpy( mvs2iso_german_0_is_blank, mvs2iso_german, 256 );
    mvs2iso_german_0_is_blank[ 0 ] = ' ';
}

} //namespace sos
