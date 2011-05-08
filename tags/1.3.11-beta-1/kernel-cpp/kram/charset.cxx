#include "precomp.h"
//#define MODULE_NAME "charset"
//#define COPYRIGHT   "(c) SOS GmbH Berlin"
//#define AUTHOR      "Jörg Schwiemann, Joacim Zschimmer"

#include "sos.h"
#include "charset.h"

using namespace std;
namespace sos {


const Char_set_table_entry char_set_table [] =
{
    { "ISO 8859-1"   , "ascii",     "[[" "\\\\" "]]" "{{" "||" "}}" "~~" "@@" "$$" "``" "^^" },  
    { "belgisch"     , "belgian",   "[¨" "\\ç"  "]°" "{é" "|§" "}è"      "@à"                },
    { "dänisch"      , "danish",    "[Æ" "\\Ø"  "]Å" "{æ" "|ø" "}å" "~ü"                "^Ü" },
    { "deutsch"      , "german",    "[Ä" "\\Ö"  "]Ü" "{ä" "|ö" "}ü" "~ß" "@§"                },
    { "französisch"  , "french",    "[°" "\\ç"  "]§" "{é" "|ù" "}è" "~¨" "@à"      "``"      },
    { "schwedisch"   , "swedish",   "[Ä" "\\Ö"  "]Å" "{ä" "|ö" "}å" "~ü" "@É"      "`é" "^Ü" }
};

const int char_set_table_count() 
{
  return NO_OF( char_set_table );
};

} //namespace sos
