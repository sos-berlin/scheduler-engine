#include "precomp.h"
//#define MODULE_NAME "ebc2iso"
//ebc2iso.cpp                                           (c) SOS GmbH Berlin

#include "sysdep.h"
#include <string.h>

#if defined SYSTEM_WIN
#   define x ((unsigned char)0x80)          // Ein nicht belegter Wert!!
#else
#   define x ((unsigned char) 'ø')
#endif

#include "asciisym.h"

using namespace std;
namespace sos {


/* BS2000-EBCDIC-Steuerzeichen, die nicht im ISO-Zeichensatz enthalten sind, werden in ISO-Zeichen
   0x80 bis 0x9E umgesetzt.
   Auﬂerdem: 0x41 -> 0xA0  '†'
             0x5F -> 0x9F  'ï'
             0xCA -> 0xA1  'Å'

   Alle anderen Zeichen sollten gem‰ﬂ ISO 8859-1 umgesetzt werden
   (s.a. Handbuch BS2000 XHCS 1.0 S.137).

   Neues Euro-Symbol Ä fehlt noch.
*/

char ebc2iso [256] =
{
   _NUL,_SOH,_STX,_ETX, '\x80',_HT ,'\x81',_DEL,  '\x82','\x83','\x84',_VT , _FF , _CR, _SO,_SI,
   _DLE,_DC1,_DC2,_DC3, '\x85','\x86',_BS ,'\x87',  _CAN,_EM ,'\x88','\x89', _IS4,_IS3,_IS2,_IS1,
   '\x8A','\x8B','\x8C','\x8D', '\x8E',_LF ,_ETB,_ESC,  '\x8F','\x90','\x91','\x92', '\x93',_ENQ,_ACK,_BEL,
   '\x94','\x95',_SYN,'\x96', '\x97','\x98','\x99',_EOT,  '\x9A','\x9B','\x9C','\x9D', _DC4,_NAK,'\x9E',_SUB,

  ' ','\xA0','‚','‰', '‡','·','„','Â', 'Á','Ò','`'   ,'.', '<' ,'(','+','|',   // 40
  '&','È'   ,'Í','Î', 'Ë','Ì','Ó','Ô', 'Ï','ﬂ','!'   ,'$', '*' ,')',';','\x9F',  // 50
  '-','/'   ,'¬','ƒ', '¿','¡','√','≈', '«','—','^'   ,',', '%' ,'_','>','?',   // 60
  '¯','…'   ,' ','À', '»','Õ','Œ','œ', 'Ã','®',':'   ,'#', '@' ,'\'','=','"',  // 70
                                                           
  'ÿ','a'   ,'b','c', 'd','e','f','g', 'h','i','´'   ,'ª', '' ,'˝','˛','±',   // 80
  '∞','j'   ,'k','l', 'm','n','o','p', 'q','r','™'   ,'∫', 'Ê' ,'∏','∆','§',   // 90
  'µ','Ø'   ,'s','t', 'u','v','w','x', 'y','z','°'   ,'ø', '–' ,'›','ﬁ','Æ',   // A0
  '¢','£'   ,'•','∑', '©','ß','∂','º', 'Ω','æ','¨'   ,'[', '\\',']','¥','◊',  // B0

  '˘','A'   ,'B','C', 'D','E','F','G', 'H','I','\xA1','Ù', 'ˆ', 'Ú','Û','ı',   // C0
  '¶','J'   ,'K','L', 'M','N','O','P', 'Q','R','π'   ,'˚', '¸', '€','˙','ˇ',   // D0
  'Ÿ','˜'   ,'S','T', 'U','V','W','X', 'Y','Z','≤'   ,'‘', '÷', '“','”','’',   // E0
  '0','1'   ,'2','3', '4','5','6','7', '8','9','≥'   ,'{', '‹', '}','⁄','~'    // F0

};

char ebc2iso_0_is_blank [256];
char mvs2iso_0_is_blank [256];

char e2a_printable [256];

/* Das ist wohl die deutsche Variante:
extern char mvs2iso [256] =
{
   _NUL  ,_SOH  ,_STX  ,_ETX  , '\x80',_HT   ,'\x81',_DEL,   '\x82','\x83','\x84',_VT   , _FF   , _CR, _SO  ,_SI,
   _DLE  ,_DC1  ,_DC2  ,_DC3  , '\x85','\x86',_BS   ,'\x87',   _CAN,_EM   ,'\x88','\x89', _IS4  ,_IS3,_IS2  ,_IS1,
   '\x8A','\x8B','\x8C','\x8D', '\x8E',_LF   ,_ETB  ,_ESC,   '\x8F','\x90','\x91','\x92', '\x93',_ENQ,_ACK  ,_BEL,
   '\x94','\x95',_SYN  ,'\x96', '\x97','\x98','\x99',_EOT,   '\x9A','\x9B','\x9C','\x9D', _DC4  ,_NAK,'\x9E',_SUB,

  ' ','\xA0','‚','{', '‡','·','„','Â', 'Á','Ò','ƒ','.', '<' ,'(' ,'+','!',      // 40
  '&','È'   ,'Í','Î', 'Ë','Ì','Ó','Ô', 'Ï','~','‹','$', '*' ,')' ,';','^',      // 50
  '-','/'   ,'¬','[', '¿','¡','√','≈', '«','—','ˆ',',', '%' ,'_' ,'>','?',      // 60
  '¯','…'   ,' ','À', '»','Õ','Œ','œ', 'Ã','`',':','#', 'ß' ,'\'','=','"',      // 70

  'ÿ','a'   ,'b','c', 'd','e','f','g', 'h','i','´','ª', '' ,'˝' ,'ﬁ','±',      // 80
  '∞','j'   ,'k','l', 'm','n','o','p', 'q','r','™','∫', 'Ê' ,'∏' ,'∆','§',      // 90
  'µ','ﬂ'   ,'s','t', 'u','v','w','x', 'y','z','°','ø', '–' ,'›' ,'˛','Æ',      // A0
  '¢','£'   ,'•','∑', '©','@','∂','º', 'Ω','æ','¨','|', 'Ø' ,'®' ,'¥','◊',      // B0

  '‰','A'   ,'B','C', 'D','E','F','G', 'H','I','@','Ù', '¶' ,'Ú' ,'Û','ı',      // C0
  '¸','J'   ,'K','L', 'M','N','O','P', 'Q','R','π','˚', '}' ,'˘' ,'˙','ˇ',      // D0
  '÷','˜'   ,'S','T', 'U','V','W','X', 'Y','Z','≤','‘', '\\','“' ,'”','’',      // E0
  '0','1'   ,'2','3', '4','5','6','7', '8','9','≥','€', ']' ,'Ÿ' ,'⁄','\xA1'    // F0
};                                    
*/


/* MVS-EBCDIC-Steuerzeichen, die nicht im ISO-Zeichensatz enthalten sind, werden in ISO-Zeichen
   0x80 bis 0x9E umgesetzt.
   Auﬂerdem: 0x41 -> 0xA0  '†'
             0x5F -> 0x9F  'ï'
             0xCA -> 0xA1  'Å'

   Alle anderen Zeichen sollten gem‰ﬂ ISO 8859-1 umgesetzt werden
   Die Tabelle ist noch nicht bijektiv!

   Neues Euro-Symbol Ä fehlt noch.

  ---------------------------------------------------------------------------

   SMS/MVS Version 1 Release 3
   Macro Instructions for Data Sets
   Document Number SC26-4913-02
   Program Number
   5695-DF1
   5645-001
   File Number S370/S390-30

   Tabellen f¸r den Zugriff auf ASCII-B‰nder:

   APPENDIX1.3.3.2 Translating from ASCII to EBCDIC
 
            0 1 2 3 4 5 6 7  8 9 A B C D E F
    00-0F  00010203372D2E2F 1605250B0C0D0E0F
    10-1F  101112133C3D3226 18193F271C1D1E1F
    20-2F  404F7F7B5B6C507D 4D5D5C4E6B604B61
    30-3F  F0F1F2F3F4F5F6F7 F8F97A5E4C7E6E6F
    40-4F  7CC1C2C3C4C5C6C7 C8C9D1D2D3D4D5D6
    50-5F  D7D8D9E2E3E4E5E6 E7E8E94AE05A5F6D
    60-6F  7981828384858687 8889919293949596
    70-7F  979899A2A3A4A5A6 A7A8A9C06AD0A107
    80-8F  3F3F3F3F3F3F3F3F 3F3F3F3F3F3F3F3F
    90-9F  3F3F3F3F3F3F3F3F 3F3F3F3F3F3F3F3F
    A0-AF  3F3F3F3F3F3F3F3F 3F3F3F3F3F3F3F3F
    B0-BF  3F3F3F3F3F3F3F3F 3F3F3F3F3F3F3F3F
    C0-CF  3F3F3F3F3F3F3F3F 3F3F3F3F3F3F3F3F
    D0-DF  3F3F3F3F3F3F3F3F 3F3F3F3F3F3F3F3F
    E0-EF  3F3F3F3F3F3F3F3F 3F3F3F3F3F3F3F3F
    F0-FF  3F3F3F3F3F3F3F3F 3F3F3F3F3F3F3F3F
 

   APPENDIX1.3.3.1 Translating from EBCDIC to ASCII
 
            0 1 2 3 4 5 6 7  8 9 A B C D E F
    00-0F  000102031A091A7F 1A1A1A0B0C0D0E0F
    10-1F  101112131A1A081A 18191A1A1C1D1E1F
    20-2F  1A1A1A1A1A0A171B 1A1A1A1A1A050607
    30-3F  1A1A161A1A1A1A04 1A1A1A1A14151A1A
    40-4F  201A1A1A1A1A1A1A 1A1A5B2E3C282B21
    50-5F  261A1A1A1A1A1A1A 1A1A5D242A293B5E
    60-6F  2D2F1A1A1A1A1A1A 1A1A7C2C255F3E3F
    70-7F  1A1A1A1A1A1A1A1A 1A603A2340273D22
    80-8F  1A61626364656667 68691A1A1A1A1A1A
    90-9F  1A6A6B6C6D6E6F70 71721A1A1A1A1A1A
    A0-AF  1A7E737475767778 797A1A1A1A1A1A1A
    B0-BF  1A1A1A1A1A1A1A1A 1A1A1A1A1A1A1A1A
    C0-CF  7B41424344454647 48491A1A1A1A1A1A
    D0-DF  7D4A4B4C4D4E4F50 51521A1A1A1A1A1A
    E0-EF  5C1A535455565758 595A1A1A1A1A1A1A
    F0-FF  3031323334353637 38391A1A1A1A1A1A
*/

char mvs2iso [256] =
{
   _NUL  ,_SOH  ,_STX  ,_ETX  , '\x80',_HT   ,'\x81',_DEL,   '\x82','\x83','\x84',_VT   , _FF   , _CR, _SO  ,_SI,
   _DLE  ,_DC1  ,_DC2  ,_DC3  , '\x85','\x86',_BS   ,'\x87',   _CAN,_EM   ,'\x88','\x89', _IS4  ,_IS3,_IS2  ,_IS1,
   '\x8A','\x8B','\x8C','\x8D', '\x8E',_LF   ,_ETB  ,_ESC,   '\x8F','\x90','\x91','\x92', '\x93',_ENQ,_ACK  ,_BEL,
   '\x94','\x95',_SYN  ,'\x96', '\x97','\x98','\x99',_EOT,   '\x9A','\x9B','\x9C','\x9D', _DC4  ,_NAK,'\x9E',_SUB,

   ' ' ,'\xA0','‚','‰', '‡','·','„','Â', 'Á','Ò','[','.', '<','(' ,'+','!',
   '&' ,'È'   ,'Í','Î', 'Ë','Ì','Ó','Ô', 'Ï','ﬂ',']','$', '*',')' ,';','^',
   '-' ,'/'   ,'¬','ƒ', '¿','¡','√','≈', '«','—','|',',', '%','_' ,'>','?',
   '¯' ,'…'   ,' ','À', '»','Õ','Œ','œ', 'Ã','`',':','#', '@','\'','=','"',

   'ÿ' ,'a'   ,'b','c', 'd','e','f','g', 'h','i','´','ª', '','˝' ,'ﬁ','±',
   '∞' ,'j'   ,'k','l', 'm','n','o','p', 'q','r','™','∫', 'Ê','∏' ,'∆','§',
   'µ' ,'~'   ,'s','t', 'u','v','w','x', 'y','z','°','ø', '–','›' ,'˛','Æ',
   '¢' ,'£'   ,'•','∑', '©','ß','∂','º', 'Ω','æ','¨','\xA1', 'Ø','®' ,'¥','◊',
             
   '{' ,'A'   ,'B','C', 'D','E','F','G', 'H','I','ß','Ù', 'ˆ','Ú' ,'Û','ı',
   '}' ,'J'   ,'K','L', 'M','N','O','P', 'Q','R','π','˚', '¸','˘' ,'˙','ˇ',
   '\\','˜'   ,'S','T', 'U','V','W','X', 'Y','Z','≤','‘', '÷','“' ,'”','’',
   '0' ,'1'   ,'2','3', '4','5','6','7', '8','9','≥','€', '‹','Ÿ' ,'⁄','¶',
};               



struct Ebc2iso
{
    Ebc2iso()
    {
        memcpy( ebc2iso_0_is_blank, ebc2iso, sizeof ebc2iso );
        ebc2iso_0_is_blank[ 0 ] = ' ';

        memcpy( mvs2iso_0_is_blank, mvs2iso, sizeof mvs2iso );
        mvs2iso_0_is_blank[ 0 ] = ' ';

        memcpy( e2a_printable, ebc2iso, sizeof e2a_printable );
        e2a_printable[ 0 ] = '∑'/*nil*/;
        memset( e2a_printable + 1, x, 63 );
       _dummy = 0;
    }

    char _dummy;
};

const Ebc2iso ebc2iso_dummy;

// AsciiChars = "[\\]{|}~";
// AnsiChars  = "ƒ÷‹‰ˆ¸ﬂ";

} //namespace sos
