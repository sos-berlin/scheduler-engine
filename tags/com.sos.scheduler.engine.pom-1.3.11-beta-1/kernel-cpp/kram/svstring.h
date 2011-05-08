// svstring.h

#ifndef __SVSTRING_H
#define __SVSTRING_H

#ifndef _STRING_HXX
#define _STRING_HXX

#if !defined _TOOLS_HXX

#define BOOL Starview_BOOL

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////// Ausschnitt für String aus StarView tools.hxx
///////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _TOOLS_H
#endif

//#include <solar.h>   Aus solar.h:
typedef unsigned char       BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      USHORT;
typedef unsigned long       ULONG;


#ifndef _TOOLS_H
#endif

class _export ResId;

struct StringData
{
    USHORT      nLen;
    USHORT      nRefCount;
    char        aStr[1];
};

#define STRING_NOTFOUND    ((USHORT)0xFFFF)
#define STRING_MATCH       ((USHORT)0xFFFF)
#define STRING_LEN         ((USHORT)0xFFFF)
#define STRING_MAXLEN      ((USHORT)0xFFFF-sizeof(StringData))

enum CharSet { CHARSET_DONTKNOW, CHARSET_ANSI, CHARSET_MAC,
               CHARSET_IBMPC_437, CHARSET_IBMPC_850, CHARSET_IBMPC_860,
               CHARSET_IBMPC_861, CHARSET_IBMPC_863, CHARSET_IBMPC_865,
               CHARSET_SYSTEM, CHARSET_SYMBOL,
               CHARSET_IBMPC = CHARSET_IBMPC_850 };
enum StringCompare { COMPARE_EQUAL, COMPARE_LESS, COMPARE_GREATER };
enum LineEnd  { LINEEND_CR, LINEEND_LF, LINEEND_CRLF };

CharSet GetSystemCharSet();
LineEnd GetSystemLineEnd();
short   svstrnicmp( const char* pStr1, const char* pStr2, USHORT nLen );

class _export String
{
    void            InitStringRes( const char* pCharStr, USHORT nLen );

private:
    StringData*     pData;

public:
                    String();
                    String( const ResId& rResId );
                    String( const String& rStr );
                    String( const String& rStr, USHORT nPos, USHORT nLen );
                    String( const char* pCharStr );
                    String( const char* pCharStr, USHORT nLen );
                    String( char c );
                    String( int n );
                    String( unsigned int n );
                    String( short n );
                    String( USHORT n );
                    String( long n );
                    String( ULONG n );
                    ~String();
/*jz 15.5.96
    operator        const char*() const { return pData->aStr; }
    operator        char() const { return pData->aStr[0]; }
    operator        int() const;
    operator        unsigned int() const;
    operator        short() const;
    operator        USHORT() const;
    operator        long() const;
    operator        ULONG() const;
*/
    String&         operator =  ( const String& rStr );
    String&         operator =  ( const char* pCharStr );

    String&         operator += ( const String& rStr );
    String&         operator += ( const char* pCharStr );
    String&         operator += ( char c );
/*jz 15.5.96
    String&         operator += ( int n );
    String&         operator += ( unsigned int n );
    String&         operator += ( short n );
    String&         operator += ( USHORT n );
    String&         operator += ( long n );
    String&         operator += ( ULONG n );

    BOOL            operator !  () const { return !pData->nLen; }
*/
    char&           operator [] ( USHORT nIndex );
    char            operator [] ( USHORT nIndex ) const
                        { return pData->aStr[nIndex]; }

    String          operator() ( USHORT n1, USHORT n2 ) const
                        { return Copy( n1, n2 ); }
    char&           operator() ( USHORT n )
                        { return String::operator[](n); }

    USHORT          Len() const { return pData->nLen; }

    String&         Insert( const String& rStr, USHORT nIndex = STRING_LEN );
    String&         Insert( const String& rStr, USHORT nPos, USHORT nLen,
                            USHORT nIndex = STRING_LEN );
    String&         Insert( const char* pCharStr, USHORT nIndex = STRING_LEN );
    String&         Insert( char c, USHORT nIndex = STRING_LEN );
    String&         Replace( const String& rStr, USHORT nIndex = 0 );
    String&         Erase( USHORT nIndex = 0, USHORT nCount = STRING_LEN );
    String          Cut( USHORT nIndex = 0, USHORT nCount = STRING_LEN );
    String          Copy( USHORT nIndex = 0, USHORT nCount = STRING_LEN ) const;

    String&         Fill( USHORT nCount, char cFillChar = ' ' );
    String&         Expand( USHORT nCount, char cExpandChar = ' ' );
    String&         EraseLeadingChars( char c = ' ' );
    String&         EraseTrailingChars( char c = ' ' );
    String&         EraseAllChars( char c = ' ' );
    String&         Reverse();

    String&         Convert( CharSet eSource,
                             CharSet eTarget = CHARSET_SYSTEM );
    static char     Convert( char c, CharSet eSource,
                             CharSet eTarget = CHARSET_SYSTEM );
    String&         ConvertLineEnd();
    String&         ConvertLineEnd( LineEnd eLineEnd );

    BOOL            IsPrintable( CharSet eCharSet = CHARSET_SYSTEM ) const;
    static BOOL     IsPrintable( char c, CharSet eCharSet = CHARSET_SYSTEM );

    String&         SpaceToZero();
    String&         ZeroToSpace();

    String&         ToUpper();
    String&         ToLower();

    String          Upper() const;
    String          Lower() const;

    BOOL            IsUpper() const;
    BOOL            IsLower() const;
    BOOL            IsAlpha() const;
    BOOL            IsNumeric() const;
    BOOL            IsAlphaNumeric() const;

    StringCompare   Compare( const String& rStr,
                             USHORT nLen = STRING_LEN ) const;
    StringCompare   Compare( const char* pCharStr,
                             USHORT nLen = STRING_LEN ) const;
    StringCompare   ICompare( const String& rStr,
                              USHORT nLen = STRING_LEN ) const;
    StringCompare   ICompare( const char* pCharStr,
                              USHORT nLen = STRING_LEN ) const;

    USHORT          Match( const String& rStr ) const;
    USHORT          Match( const char* pCharStr ) const;

    USHORT          Search( const String& rStr, USHORT nIndex = 0 ) const;
    USHORT          Search( const char* pCharStr, USHORT nIndex = 0 ) const;
    USHORT          Search( char c, USHORT nIndex = 0 ) const;

    USHORT          GetTokenCount( char cTok = ';' ) const;
    String          GetToken( USHORT nToken, char cTok = ';' ) const;

    char*           AllocStrBuf( USHORT nLen );
    const char*     GetStr() const { return pData->aStr; }

#ifdef __BORLANDC__
    friend          String operator+( const String& rStr1,  const String& rStr2  );
    friend          String operator+( const String& rStr,   const char* pCharStr );
    friend          String operator+( const char* pCharStr, const String& rStr   );
#else
    friend inline   String operator+( const String& rStr1,  const String& rStr2  );
    friend inline   String operator+( const String& rStr,   const char* pCharStr );
    friend inline   String operator+( const char* pCharStr, const String& rStr   );
#endif

    friend BOOL     operator == ( const String& rStr1,  const String& rStr2  );
    friend BOOL     operator == ( const String& rStr,   const char* pCharStr );
    friend BOOL     operator == ( const char* pCharStr, const String& rStr   );
    friend BOOL     operator != ( const String& rStr1,  const String& rStr2  );
    friend BOOL     operator != ( const String& rStr,   const char* pCharStr );
    friend BOOL     operator != ( const char* pCharStr, const String& rStr   );
    friend BOOL     operator <  ( const String& rStr1,  const String& rStr2  );
    friend BOOL     operator <  ( const String& rStr,   const char* pCharStr );
    friend BOOL     operator <  ( const char* pCharStr, const String& rStr   );
    friend BOOL     operator >  ( const String& rStr1,  const String& rStr2  );
    friend BOOL     operator >  ( const String& rStr,   const char* pCharStr );
    friend BOOL     operator >  ( const char* pCharStr, const String& rStr   );
    friend BOOL     operator <= ( const String& rStr1,  const String& rStr2  );
    friend BOOL     operator <= ( const String& rStr,   const char* pCharStr );
    friend BOOL     operator <= ( const char* pCharStr, const String& rStr   );
    friend BOOL     operator >= ( const String& rStr1,  const String& rStr2  );
    friend BOOL     operator >= ( const String& rStr,   const char* pCharStr );
    friend BOOL     operator >= ( const char* pCharStr, const String& rStr   );
};

// SOS: manche inlines in svstring.cxx, weil
// Warning S:\SV\INC\TOOLS.HXX 2091: Functions containing some return statements are not expanded inline in function operator +(const String &,const String &)

///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////Ende des Ausschnitts für String aus StarView tools.hxx
///////////////////////////////////////////////////////////////////////////////////////////////

#undef BOOL
#endif
#endif


inline String        as_sv_string( const String& s )                      { return s; }
inline String        as_sv_string( const char* s )                        { return String( s ); }
inline String        as_sv_string( const char* string, unsigned int length ) { return String( string, length ); }

inline const char*   c_str    ( const String& string )                    { return string.GetStr(); }
inline unsigned int  length   ( const String& string )                    { return string.Len(); }

#if defined __SOSSTRNG_H && !defined SOS_STRING_STARVIEW
    inline String     as_sv_string( const Sos_string& s )                 { return as_sv_string( c_str( s ), length( s ) ); }
    inline Sos_string as_string   ( const String& s )                     { return as_string( c_str( s ), s.Len() ); }
#endif

inline unsigned int position( const String& str, const String& to_find, unsigned int pos = 0 )
{
    unsigned int p = str.Search( to_find, pos );
    return p == STRING_NOTFOUND? length( str ) : p;
}

//inline void assign( Sos_string* str, const char* o, int len )  { *str = as_string( o, len ); }
inline void assign( String* str, const char* o )  { *str = o; }

#if defined __BORLANDC__
#   if !defined _EXPCLASS
#       include <_defs.h>
#   endif
    class _EXPCLASS istream;
    class _EXPCLASS ostream;
# else
    struct istream;
    struct ostream;
#endif

istream& operator>> ( istream&, String& );
ostream& operator<< ( ostream&, const String& );


#endif
