//$Id: sosodbc.h 13577 2008-06-07 10:03:02Z jz $
// sosodbc.h           ©1996 SOS GmbH Berlin, Joacim Zschimmer
// Übernommen von Gorta, s. gortafaq.txt

#ifndef __SOSODBC_H
#define __SOSODBC_H

#define VENDOR_FULL_NAME  "SOS GmbH Berlin"
#define DRIVER_FULL_NAME  "hostODBC Driver"
#define DRIVER_SHORT_NAME "hostODBC"
#define SOS_ODBC_VER      0x0200

//#include <optimize.h>

#ifndef STRICT
#	define STRICT
#endif

#if defined SYSTEM_WIN
#if defined __WIN32__
#   include <windef.h>
# else
#   include <windows.h>
#endif
#endif
//	-	-	-	-	-	-	-	-	-

#include "sql.h"
#include "sqlext.h"

#if !defined __SOSSTRNG_H
#   include "../kram/sosstrng.h"
#endif

#if !defined __SOSFIELD_H
#   include "../kram/sosfield.h"
#endif

#if !defined __ANYFILE_H
#   include "../file/anyfile.h"
#endif

#if !defined __SOSLIMTX_H
#   include "../kram/soslimtx.h"
#endif

#include "../kram/log.h"
#include "sosodbc.hrc"
#include "../file/odbctype.h"


namespace sos {

//	Definitions to be used in function prototypes.
//	The SQL_API is to be used only for those functions exported for driver
//		manager use.
//	The EXPFUNC is to be used only for those functions exported but used
//		internally, ie, dialog procs.
//	The INTFUNC is to be used for all other functions.
#ifdef WIN32
#   define INTFUNC  __stdcall
#   define EXPFUNC  __stdcall
#else
#   define INTFUNC PASCAL
#   define EXPFUNC __export CALLBACK
#endif

//#define ODBC_LOG( x )  do;while(0)
#define ODBC_LOG( x )  LOG( x )

typedef void* Unknown_param; // Typ für Default-Parametergröße für Aufruf von func(...)

#define DEFINE_ODBC_CALL( CLASS, FUNC, PARAM_COUNT, PARAM_DECLS, PARAMS )                   \
    /* HENV/HDBC/HSTMT handle muß der erste Parameter sein */                               \
                                                                                            \
    static RETCODE __cdecl call_##FUNC PARAM_DECLS                                          \
    {                                                                                       \
        return ((CLASS*)handle)->FUNC PARAMS;                                               \
    }                                                                                       \
                                                                                            \
    extern "C"                                                                              \
    RETCODE SQL_API FUNC PARAM_DECLS                                                        \
    {                                                                                       \
        ((CLASS*)handle)->_func_code = fc_##FUNC;                                           \
        ((CLASS*)handle)->_func_ptr  = (Sos_odbc_base::Sos_odbc_func_ptr)&call_##FUNC;      \
        ((CLASS*)handle)->_param_byte_count = PARAM_COUNT;                                  \
        return ((CLASS*)handle)->call PARAMS;                                               \
    }

// parameter size:
#define OPS( PARAM ) (( sizeof (PARAM) + ( sizeof(int) - 1 ) ) / sizeof(int) * sizeof(int) )

#define DEFINE_ODBC_CALL_0( CLASS, FUNC, HANDLE_TYPE )                                      \
    DEFINE_ODBC_CALL( CLASS, FUNC, OPS(handle),                                             \
        ( HANDLE_TYPE handle ), ( handle ) )

#define DEFINE_ODBC_CALL_1( CLASS, FUNC, HANDLE_TYPE, P1 )                                  \
    DEFINE_ODBC_CALL( CLASS, FUNC, OPS(handle)+OPS(p1),                                     \
        ( HANDLE_TYPE handle, P1 p1 ), ( handle, p1 ) )

#define DEFINE_ODBC_CALL_2( CLASS, FUNC, HANDLE_TYPE, P1, P2 )                              \
    DEFINE_ODBC_CALL( CLASS, FUNC, OPS(handle)+OPS(p1)+OPS(p2),                             \
        ( HANDLE_TYPE handle, P1 p1, P2 p2 ),                                               \
        ( handle, p1, p2 ) )

#define DEFINE_ODBC_CALL_3( CLASS, FUNC, HANDLE_TYPE, P1, P2, P3 )                          \
    DEFINE_ODBC_CALL( CLASS, FUNC, OPS(handle)+OPS(p1)+OPS(p2)+OPS(p3),                     \
        ( HANDLE_TYPE handle, P1 p1, P2 p2, P3 p3 ),                                        \
        ( handle, p1, p2, p3 ) )

#define DEFINE_ODBC_CALL_4( CLASS, FUNC, HANDLE_TYPE, P1, P2, P3, P4 )                      \
    DEFINE_ODBC_CALL( CLASS, FUNC,                                                          \
        OPS(handle)+OPS(p1)+OPS(p2)+OPS(p3)+OPS(p4),                                        \
        ( HANDLE_TYPE handle, P1 p1, P2 p2, P3 p3, P4 p4 ),                                 \
        ( handle, p1,    p2,    p3,    p4 ) )

#define DEFINE_ODBC_CALL_5( CLASS, FUNC, HANDLE_TYPE, P1, P2, P3, P4, P5 )                  \
    DEFINE_ODBC_CALL( CLASS, FUNC,                                                          \
        OPS(handle)+OPS(p1)+OPS(p2)+OPS(p3)+OPS(p4)+OPS(p5),                                \
        ( HANDLE_TYPE handle, P1 p1, P2 p2, P3 p3, P4 p4, P5 p5 ),                          \
        ( handle, p1,    p2,    p3,    p4,    p5 ) )

#define DEFINE_ODBC_CALL_6( CLASS, FUNC, HANDLE_TYPE, P1, P2, P3, P4, P5, P6 )              \
    DEFINE_ODBC_CALL( CLASS, FUNC,                                                          \
          OPS(handle)+OPS(p1)+OPS(p2)+OPS(p3)+OPS(p4)+OPS(p5)+OPS(p6),                      \
        ( HANDLE_TYPE handle, P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6 ),                   \
        ( handle            ,    p1,    p2,    p3,    p4,    p5,    p6 ) )

#define DEFINE_ODBC_CALL_7( CLASS, FUNC, HANDLE_TYPE, P1, P2, P3, P4, P5, P6, P7 )          \
    DEFINE_ODBC_CALL( CLASS, FUNC,                                                          \
          OPS(handle)+OPS(p1)+OPS(p2)+OPS(p3)+OPS(p4)+OPS(p5)+OPS(p6)+OPS(p7),              \
        ( HANDLE_TYPE handle, P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7 ),            \
        ( handle, p1,    p2,    p3,    p4,    p5,    p6,    p7 ) )

#define DEFINE_ODBC_CALL_8( CLASS, FUNC, HANDLE_TYPE, P1, P2, P3, P4, P5, P6, P7, P8 )      \
    DEFINE_ODBC_CALL( CLASS, FUNC,                                                          \
          OPS(handle)+OPS(p1)+OPS(p2)+OPS(p3)+OPS(p4)+OPS(p5)+OPS(p6)+OPS(p7)+OPS(p8),      \
        ( HANDLE_TYPE handle, P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8 ),     \
        ( handle, p1,    p2,    p3,    p4,    p5,    p6,    p7,    p8 ) )

#define DEFINE_ODBC_CALL_9( CLASS, FUNC, HANDLE_TYPE, P1, P2, P3, P4, P5, P6, P7, P8, P9 )  \
    DEFINE_ODBC_CALL( CLASS, FUNC,                                                          \
          OPS(handle)+OPS(p1)+OPS(p2)+OPS(p3)+OPS(p4)+OPS(p5)+OPS(p6)+OPS(p7)+OPS(p8)+OPS(p9),\
        ( HANDLE_TYPE handle, P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8,P9 p9),\
        ( handle, p1,    p2,    p3,    p4,    p5,    p6,    p7,    p8, p9 ) )

enum Odbc_func_code
{
    fc_SQLAllocStmt,
    fc_SQLBindCol,
    fc_SQLBindParameter,
    fc_SQLColAttributes,
    fc_SQLColumns,
    fc_SQLConnect,
    fc_SQLDisconnect,
    fc_SQLDescribeCol,
    fc_SQLDriverConnect,
    fc_SQLExecDirect,
    fc_SQLExecute,
    fc_SQLFetch,
    fc_sos_SQLFreeStmt,
    fc_SQLGetConnectOption,
    fc_SQLGetData,
    fc_SQLGetInfo,
    fc_SQLGetStmtOption,
    fc_SQLGetTypeInfo,
    fc_SQLNumResultCols,
    fc_SQLParamData,
    fc_SQLPutData,
    fc_SQLPrepare,
    fc_SQLRowCount,
    fc_SQLSetConnectOption,
    fc_SQLSetStmtOption,
    fc_SQLSpecialColumns,
    fc_SQLStatistics,
    fc_SQLTables,
    odbc_func_code_last
};


//--------------------------------------------------------------------------------Sos_odbc_base
// Nur für Sos_odbc_env, Sos_odbc_connection und Sos_odbc_stmt

struct Sos_odbc_base : Has_static_ptr
{
//#   if defined SYSTEM_MICROSOFT
    //typedef RETCODE (Sos_odbc_base::* __stdcall Sos_odbc_func_ptr)( ... );
    typedef RETCODE (__cdecl   *Sos_odbc_func_ptr)(...); 

                                Sos_odbc_base           () : _xc(0), _active(false) {}

    void                        clear_error             ();
    void                        delete_xc               ();
    RETCODE                     sql_error               ( const char* sqlstate );                   // liefert SQL_ERROR
    RETCODE                     sql_error               ( const char* sqlstate, const Xc_base& );   // liefert SQL_ERROR
    RETCODE                     sql_error               ( const char* sqlstate, const exception& );      // liefert SQL_ERROR
    RETCODE                     sql_error               ( const char* sqlstate, const char* );      // liefert SQL_ERROR
    RETCODE __cdecl             call                    ( void* handle, ... );

  //const char*                _func_name;              // für call(): Name der Funktion (für LOG)
    Odbc_func_code             _func_code;              // für call()
    Sos_odbc_func_ptr          _func_ptr;               // für call(): Zeiger auf Methode
    int                        _param_byte_count;       // für call(): Länge der Parameterliste
    Sos_limited_text<5>        _sqlstate;
    Xc_base*                   _xc;
    Bool                       _active;
};

//-----------------------------------------------------------------------------assign_odbc_func
/*
#if defined SYSTEM_MICROSOFT

    typedef void* Func_type;
    
    inline void assign_odbc_func( Sos_odbc_base::Sos_odbc_func_ptr* v, Func_type f )
    {
        __asm mov eax, f
        __asm mov v, eax
    }

#else
    
    typedef Sos_odbc_base::Sos_odbc_func_ptr Func_type;

    inline void assign_odbc_func( Sos_odbc_base::Sos_odbc_func_ptr* v, Func_type f )
    {
        v = f;  
    }

#endif
*/
//---------------------------------------------------------------------------------Sos_odbc_env

struct Sos_odbc_env : Sos_odbc_base
{
};

//--------------------------------------------------------------------------Sos_odbc_connection

struct Sos_odbc_connection : Sos_odbc_base
{
                                Sos_odbc_connection     ()          : _zero_(this+1) {}

    RETCODE __cdecl             SQLAllocStmt            ( HDBC, HSTMT* phstmt );      // in sosstmt.cxx
    RETCODE __cdecl             SQLConnect              ( HDBC, UCHAR*, SWORD, UCHAR*, SWORD,
                                                          UCHAR*, SWORD );
    RETCODE __cdecl             SQLDriverConnect        ( HDBC, HWND, UCHAR*, SWORD, UCHAR*, SWORD,
                                                          SWORD*, UWORD );
    RETCODE __cdecl             SQLDisconnect           ( HDBC );

    RETCODE __cdecl             SQLGetInfo              ( HDBC hdbc, UWORD fInfoType,
	                                                      PTR rgbInfoValue, SWORD cbInfoValueMax,
	                                                      SWORD FAR *pcbInfoValue );
    RETCODE __cdecl             SQLGetConnectOption     ( HDBC, UWORD fOption, PTR );
    RETCODE __cdecl             SQLSetConnectOption     ( HDBC, UWORD fOption, UDWORD );

    RETCODE                     connect                 ( const Sos_string& db, const Sos_string& user, const Sos_string& pwd,
                                                          const Sos_string& catalog );

    Fill_zero                  _zero_;
    Sos_odbc_env*              _env;
    Sos_string                 _data_source_name;
  //Sos_string                 _user_name;
    Sos_string                 _dbms_param;
    Any_file                   _file;                   // hält die Datenbanksession offen, sonst nix
    Any_file::Open_mode        _open_mode;
};

//--------------------------------------------------------------------------------Sos_odbc_stmt

struct Sos_odbc_stmt : Sos_odbc_base
{
                                Sos_odbc_stmt           ();

    RETCODE __cdecl             sos_SQLFreeStmt         ( HSTMT, UWORD fOption );
    RETCODE __cdecl             SQLSetStmtOption        ( HSTMT, UWORD fOption, UDWORD	vParam );
    RETCODE __cdecl             SQLGetStmtOption        ( HSTMT, UWORD fOption, PTR pvParam );

    // Einige vordefinierte "SQLExecDirect()", alle in odbccat.cxx
    RETCODE __cdecl             SQLTables               ( HSTMT, UCHAR*, SWORD, UCHAR*, SWORD,
                                                          UCHAR*, SWORD, UCHAR*, SWORD );
    RETCODE __cdecl             SQLColumns              ( HSTMT, UCHAR*, SWORD, UCHAR*, SWORD,
                                                          UCHAR*, SWORD, UCHAR*, SWORD );
    RETCODE __cdecl             SQLStatistics           ( HSTMT, UCHAR*, SWORD, UCHAR*, SWORD,
                                                          UCHAR*, SWORD, UWORD fUnique, UWORD fAccuracy);

    RETCODE __cdecl             SQLPrepare              ( HSTMT, UCHAR* sql_string, SDWORD sql_string_length );
    RETCODE __cdecl             prepare_param_record    ();
    RETCODE __cdecl             SQLExecute              ( HSTMT );
    RETCODE __cdecl             SQLExecDirect           ( HSTMT, UCHAR*, SDWORD );
    RETCODE __cdecl             SQLBindParameter        ( HSTMT, UWORD ipar, SWORD fParamType,
                                                          SWORD fCType, SWORD fSqlType,
                                                          UDWORD cbColDef, SWORD ibScale,
                                                          PTR rgbValue, SDWORD cbValueMax,
                                                          SDWORD* pcbValue );
    RETCODE __cdecl             SQLParamData            ( HSTMT, PTR* prgbValue );
    RETCODE __cdecl             SQLPutData              ( HSTMT, PTR rgbValue, SDWORD cbValue );

    RETCODE __cdecl             SQLRowCount             ( HSTMT, SDWORD FAR*pcrow );
    RETCODE __cdecl             SQLNumResultCols        ( HSTMT, SWORD* pccol );
    RETCODE __cdecl             SQLDescribeCol          ( HSTMT, UWORD icol,
                                                          UCHAR* szColName, SWORD cbColNameMax,
                                                          SWORD*pcbColName,
                                                          /* UNALIGNED */ SWORD* pfSqlType,
	                                                      /* UNALIGNED */ UDWORD* pcbColDef,
	                                                      /* UNALIGNED */ SWORD* pibScale,
                                                          SWORD* pfNullable );
    RETCODE __cdecl             SQLColAttributes        ( HSTMT, UWORD icol, UWORD fDescType,
                                                          PTR rgbDesc, SWORD cbDescMax, SWORD* pcbDesc,
                                                          SDWORD* pfDesc );
    RETCODE __cdecl             SQLBindCol              ( HSTMT, UWORD icol, SWORD fCType,
                                                          PTR rgbValue, SDWORD cbValueMax, SDWORD* pcbValue );
    void                        copy_value_to_odbc      ( const Sos_odbc_binding*, const Field_descr* source );
    RETCODE __cdecl             SQLFetch                ( HSTMT );
    RETCODE __cdecl             SQLGetData              ( HSTMT, UWORD icol, SWORD	fCType,
                                                          PTR rgbValue, SDWORD cbValueMax, SDWORD* pcbValue );
    RETCODE __cdecl             SQLSpecialColumns       ( HSTMT, UWORD fColType, UCHAR* szTableQualifier,
                                                          SWORD cbTableQualifier, UCHAR* szTableOwner,
                                                          SWORD	cbTableOwner, UCHAR* szTableName,
                                                          SWORD cbTableName, UWORD fScope,
                                                          UWORD	fNullable );
    RETCODE __cdecl             SQLGetTypeInfo          ( HSTMT, SWORD fSqlType );

  //void                        make_callers_type       ();
    HSTMT                       hstmt                   ()          { return (HSTMT)this; }

    Fill_zero                  _zero_;
    Sos_odbc_connection*       _conn;
    Sos_string                 _sql_string;
    Any_file                   _file;
    Bool                       _with_result_set;
    Bool                       _eof;
    int4                       _row_no;
    Sos_ptr<Record_type>       _record_type;
    int                        _field_count;
    Dynamic_area               _record;
    Sos_ptr<Field_type>        _key_field;
  //Field_converter            _converter;
    Sos_simple_array<Sos_odbc_binding> _param_bindings;     // von SQLBindParameter
    int                                _param_index;        // Bis hier ok. _param_index == _param_bindings.last_index() + 1: Alle Parameter ok
    Sos_ptr<Record_type>               _param_record_type;
    Sos_simple_array<Sos_odbc_binding> _col_bindings;       // von SQLBindCol
    Sos_simple_array<Sos_odbc_binding> _cached_bindings;    // für SQLGetData()
    Dynamic_area               _hilfspuffer;                // zum Konvertieren        
  //Sos_ptr<Record_type>       _callers_type;
};

//---------------------------------------------------------------------------------------------

extern HINSTANCE    _hinstance;

extern Bool         odbc_make_timing;
extern Sos_string   timing_filename;

void print_odbc_func_timings( ostream* s );  // odbcbase.cxx

struct Odbc_error : Xc
{
                                Odbc_error              ( const char* e )   : Xc( e ) {}
    Sos_limited_text<5>        _sqlstate;
};

void        throw_odbc_error            ( const char* sqlstate, const char*, int );
/*
inline void return_odbc_string( void* rgbOut, SWORD cbMax, SWORD FAR* pcbOut,
                                const char* string, int len )
{
    if( pcbOut )  *pcbOut = len;
    memcpy( rgbOut, string,  min( len, cbMax ) );
}

inline void return_odbc_string( void* rgbOut, SWORD cbMax, SWORD FAR* pcbOut,
                                const Sos_string& string )
{
    return_odbc_string( rgbOut, cbMax, pcbOut, c_str( string ), length( string ) );
}

inline void return_odbc_string( void* rgbOut, SWORD cbMax, SWORD FAR* pcbOut,
                                const char* string )
{
    return_odbc_string( rgbOut, cbMax, pcbOut, c_str( string ), length( string ) );
}
*/

//js
inline bool return_odbc_string0( void* szOut, SWORD cbMax, SWORD FAR* pcbOut,
                                 const char* string, int len )
{
    bool result = false;

    if( pcbOut )  *pcbOut = len;  // die tatsächliche Länge
    
    if( szOut && cbMax >= 1 )
    {
        int length = min( len, cbMax-1 );
        memcpy( szOut, string,  length );
        ((char*)szOut)[ length ] = '\0';
        result = true;
    }

    return result;
}

inline bool return_odbc_string0( void* szOut, SWORD cbMax, SWORD FAR* pcbOut,
                                 const Sos_string& string )
{
    return return_odbc_string0( szOut, cbMax, pcbOut, c_str( string ), length( string ) );
}

inline bool return_odbc_string0( void* szOut, SWORD cbMax, SWORD FAR* pcbOut,
                                 const char* string )
{
    return return_odbc_string0( szOut, cbMax, pcbOut, c_str( string ), length( string ) );
}


//-----------------------------------------------------------------------------Odbc_c_char_type
/*
// Jedes Feld hat einen eigenen Typ, in dem pcbValue gespeichert ist.

struct Odbc_c_char_type : Field_type
{
                                Odbc_c_char_type        ( SDWORD cbValueMax, SDWORD* pcbValue ) : Field_type( cbValueMax ), _pcbValue(pcbValue)  {}

    Bool                        nullable                () const    { return true; }
    Bool                        null                    ( const Byte* ) const;
    void                        set_null                ( Byte* ) const;
    void                        field_copy              ( Byte*, const Byte* ) const;
    void                        write_text              ( const Byte*, Area*, const Text_format& ) const;
    void                        read_text               ( Byte*, const char*, const Text_format& ) const;
    uint                       _v_field_length          ( const Byte*, const Byte* ) const;

  //void                        check_type              ( const Odbc_c_date* ) {}

    static const Type_info     _type_info;
    void                       _get_param               ( Type_param* ) const;

    SDWORD*                    _pcbValue;
};

//extern Odbc_c_char_type odbc_c_char_type;
//DEFINE_ADD_FIELD( Odbc_c_date, odbc_c_char_type )

*/


} //namespace sos

#endif
