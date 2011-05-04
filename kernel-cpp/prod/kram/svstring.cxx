#include "precomp.h"
//#define MODULE_NAME "svstring.cxx"
//#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"


#include "sosstrng.h"
#include "sysdep.h"

#if defined SOS_STRING_STARVIEW

#include "svstring.h"

/*inline*/ Sos_string expanded_string( const Sos_string& str, int len, char c )
{
    String erg = str;
    return erg.Expand( len, c );
}


/*inline*/ String operator + ( const String& rStr1, const String& rStr2 )
{
    String aTmpStr( rStr1 );
    aTmpStr += rStr2;
    return aTmpStr;
}

/*inline*/ String operator + ( const String& rStr, const char* pCharStr )
{
    String aTmpStr( rStr );
    aTmpStr += pCharStr;
    return aTmpStr;
}

/*inline*/ String operator + ( const char* pCharStr, const String& rStr )
{
    String aTmpStr( pCharStr );
    aTmpStr += rStr;
    return aTmpStr;
}

#endif
