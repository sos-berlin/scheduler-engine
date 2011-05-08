// $Id$

// MSVC users on Windows: compile the JS Engine with linker flag /OPT:NOICF !!!
// Here's why: http://www.mozilla.org/js/spidermonkey/release-notes/NOICF.html

/*
    WAS FEHLT?

    ??? Scheduler und com_remote: 2 x GetIDsOfNames und Invoke zusammenfassen?

    Wie in Java die Objekte nur für einen Aufruf gültig machen und danach entfernen?

    Nicht thread-sicher!
        Spidermonkey mit JS_THREADSAFE übersetzen (dazu fehlen Header-Dateien)
        Jsobject_idispatch sollte in anderen Threads benutzbar sein
        -> Problem, wenn Spidermonkey beendet wird während ein anderer Thread Jsobject_idispatch aufruft -> Locks

    Spidermonkey mit STRICT übersetzen!

    Fehlermeldungen testen: Syntax, throw, Spooler- und Hostware-Fehler, Java-Fehler, catch von spooler-Fehler

    Einige Z_LOGs rausnehmen?

    *** msvcr71.dll wird benötigt!

    Garbage Collector beachten: http://www.mozilla.org/js/spidermonkey/gctips.html


    jvm.dll wird automatisch ermittelt aus Registrierung

    
    Fehlermeldungen mit Methodennamen versehen: COM-Fehler bei spooler_log.info() sollte info erwähnen.
*/

/*
    SPIDERMONKEY
        Visual C++ 2003: Nur die Debug-Version funktioniert!
        Sie kann mit /O2 usw. übersetzt werden, aber es muss die Debug-Variante sein.

        Die Release-Variante erkennt Funktions-Parameter nicht.
        Mit #ifdef DEBUG führt ein assert() zum Abbruch.
*/

/*
    KLASSEN

    Spidermonkey : IActiveScript

    Jsobject_idispatch : IDispatch
        Ein JavaScript-Objekt (auch das globale) als IDispatch.
        Eigentümer ist Javascript.
        ~Jsobject_idispatch() gibt JS_Object frei (außer dem globalen Objekt)

    Idispatch_jsobject
        Nur für das globale Objekt in Javascript!
        Wenn andere Javascript-Objekte als IDispatch ansprechbar sein sollen:
            Referenzzählung beachten
            finalizer -> Release(), _jsobject_to_idispatch.erase( jsobject )

        Ein IDispatch als JS_Object
        Eigentümer ist die COM-Anwendung
        finalizer -> IDispatch()::Release()


    Java_liveconnect_interface
        Callbacks für LiveConnect



    OBJEKTE
        
        IDispatch an Javascript übergeben (z.B. spooler_log)

            
        JSObject an COM übergeben (erstmal nur das globale Objekt)
            Jsobject_idispatch( jsobject )




    ÄNDERUNGEN IN SPIDERMONKEY 1.7

    jsscan.c, Zeile 113-116:

        #ifdef WIN32                        //2008-01-10 Joacim Zschimmer
        #   include "jsautokw_windows.h"    //2008-01-10 Joacim Zschimmer
        #else                               //2008-01-10 Joacim Zschimmer
        #   include "jsautokw.h"            //2008-01-10 Joacim Zschimmer
        #endif                              //2008-01-10 Joacim Zschimmer


    jsapi.c, Zeile 4992-4995:
        DllMain auskommentiert.


    liveconnect/jsj_JavaPackage.c, Zeilen 552-555 angehängt:

        JSBool JSJ_pre_define_java_packages(JSContext *cx, JSObject *global_obj, JavaPackageDef *predefined_packages)   //2008-01-10 Joacim Zschimmer
        {                                                                                                               //2008-01-10 Joacim Zschimmer
            return pre_define_java_packages( cx, global_obj, predefined_packages );                                     //2008-01-10 Joacim Zschimmer
        }                                                                                                               //2008-01-10 Joacim Zschimmer



    Veraltet:
    ÄNDERUNGEN IN SPIDERMONKEY 1.5

    jsapi.c, Zeile 4196-4199:
        DllMain auskommentiert.


    jscntxt.h, Zeile 327 eingefügt:
        jsbytecode*         exception_pc;           //jz 27.9.04
        char*               exception_script;       //jz 27.9.04


    jscntxt.c, Zeile 613 eingefügt:
        if( cx->exception_script )                                                              //jz 27.9.04
        {                                                                                       //jz 27.9.04
            report.filename = cx->exception_script->filename;                                   //jz 27.9.04
            report.lineno   = js_PCToLineNumber(cx, cx->exception_script, cx->exception_pc );   //jz 27.9.04
        }                                                                                       //jz 27.9.04


    jsinterp.c, Zeile 1414:
        cx->exception_script = NULL;     //jz 27.09.04
        cx->exception_pc     = NULL;     //jz 27.09.04


    jsinterp.c, Zeile 4257:
        cx->exception_script = cx->fp->script;   //jz 27.09.04
        cx->exception_pc     = pc;               //jz 27.09.04


    liveconnect/jsjava.h:
        Alle JS_EXPORT_API gegen JS_PUBLIC_API ausgetauscht


    liveconnect/jsj_JavaPackage.c, Zeilen 530-503 eingefügt:

        JSJ_pre_define_java_packages(JSContext *cx, JSObject *global_obj, JavaPackageDef *predefined_packages)  //jz 17.10.04
        {                                                                                                       //jz 17.10.04
            return pre_define_java_packages( cx, global_obj, predefined_packages );                             //jz 17.10.04
        }                                                                                                       //jz 17.10.04

*/

//-------------------------------------------------------------------------------------------------

//Und vc 2005  #ifdef __GNUC__     //test
#   define DONT_USE_HASH_SET        // gcc 3.3.3, 3.4.2: hash_set<Bstr> benutzt BSTR, den Typ von operator&().
//#endif

#include <zschimmer/zschimmer.h>
#include <zschimmer/z_com.h>
#include <zschimmer/com_server.h>
#include <zschimmer/z_com_server.h>
#include <zschimmer/java.h>
#include <zschimmer/java_com.h>
#include <zschimmer/scripting_engine.h>
#include "spidermonkey.h"
#include "version.h"
#include <math.h>

#ifdef Z_WINDOWS
#   define snprintf _snprintf
#endif

#ifdef Z_AIX
#   define AIX
#   define HAVE_SYS_INTTYPES_H
#endif

//-----------------------------------------------------------------------------------------jsapi.j

#ifdef Z_WINDOWS
#   define XP_WIN
#else
#   define XP_UNIX
#endif

#ifdef Z_AIX
#   define AIX
#   define HAVE_SYS_INTTYPES_H
#endif

#define JS_FILE
//#define JS_THREADSAFE  // JavaScript mit JS_THREADSAFE übersetzen!   Geht nicht und brauchen wir auch nicht.
#define EXPORT_JS_API   // Wir binden Spidermonkey als .lib ein

#include "../js/src/jsapi.h"
#include "../js/src/jsdate.h"
#include "../js/src/liveconnect/jsjava.h"
#include "../js/src/jscntxt.h"                  // Für *cx->down->fp->pc == JSOP_SETCALL 
#include "../js/src/jsopcode.h"                 // Für *cx->down->fp->pc == JSOP_SETCALL 


extern "C" JSBool JSJ_pre_define_java_packages( JSContext*, JSObject*, JavaPackageDef* );  //jz

//-------------------------------------------------------------------------------------------------

#ifdef Z_WINDOWS
    struct __declspec( uuid( "{BB1A2AE1-A4F9-11cf-8F20-00805F2CD064}" ) )  IActiveScript;       // Das hat Microsoft vergessen
#endif

//-------------------------------------------------------------------------------------------------

using namespace std;
using namespace stdext;
using namespace zschimmer;
using namespace zschimmer::com;

//-------------------------------------------------------------------------------------------------
#ifdef __GNUC__

    namespace __gnu_cxx 
    {
        template<>
        struct hash< IDispatch* >
        {
            size_t operator() ( const IDispatch* pointer ) const { return (size_t)pointer ^ 0xfeeefeee; }
        };

        template<>
        struct hash< JSObject* >
        {
            size_t operator() ( const JSObject* pointer ) const { return (size_t)pointer ^ 0xfeeefeee; }
        };
    }

#endif
//------------------------------------------------------------------------------operator << jschar*
/*
inline ostream& operator << ( ostream& s, const jschar* str )
{
    s << static_cast<const OLECHAR*>( str );
    return s;
}
*/
//-------------------------------------------------------------------------------------------------

inline const OLECHAR* olechar_from_jschar( const jschar* jstr )
{
    return reinterpret_cast<const OLECHAR*>( jstr );
}

//-------------------------------------------------------------------------------------------------

inline const jschar* jschar_from_olechar( const OLECHAR* olechar_string )
{
    return reinterpret_cast<const jschar*>( olechar_string );
}

//-------------------------------------------------------------------------------------------------

namespace spidermonkey {

//-------------------------------------------------------------------------------------------------

using zschimmer::int16;
using zschimmer::int32;
using zschimmer::int64;
using zschimmer::uint;
using zschimmer::uint16;
using zschimmer::uint32;
using zschimmer::uint64;

//-------------------------------------------------------------------------------------------------

struct Spidermonkey;

//--------------------------------------------------------------------------------------------const

const string uncaught_exception_prefix = "uncaught exception: ";
const string error_prefix              = "Error: ";

static Message_code_text error_codes[] =
{
    { "SPIDERMONKEY-1", "Reference to C++-object is not longer valid" },
    NULL
};

Z_INIT(Spidermonkey) { add_message_code_texts(error_codes); }

//-------------------------------------------------------------------------------------Error_report

struct Error_report
{
    Error_report() : _zero_(this+1) {}
    

    Error_report& assign ( const JSErrorReport& e, const char* error_message )
    {
        _message = error_message;
        if( string_begins_with( _message, uncaught_exception_prefix ) )  _message.erase( 0, uncaught_exception_prefix.length() );      // von: throw new Error("...")

        _filename       = e.filename? e.filename : "";
        _line_number    = e.lineno;
        _line           = e.linebuf? e.linebuf : "";
        _line_bstr      = e.uclinebuf;
        _column_number  = (int)( e.tokenptr - e.linebuf );
        _error_number   = e.errorNumber;

        if( _column_number < 0  ||  _column_number >= (int)_line.length() )  _column_number = 0;

        return *this;
    }


    void clear()
    {
        _error_number = 0;
        _line = "";
    }


    bool filled() const 
    { 
        return _error_number != 0  &&  _line != ""; 
    }


    operator string()
    {
        //"JAVASCRIPT-" << _error_number << "  ";
        S result;

        result << _message;
        if( _filename != "" )  result << ", in "     << _filename;
        if( _line_number    )  result << ", line "   << _line_number;
        if( _column_number  )  result << ", column " << _column_number;

        return result;
    }


    Fill_zero                  _zero_;
    string                     _message;
    string                     _filename;      
    int                        _line_number;   
    string                     _line;          
    int                        _column_number; 
    Bstr                       _line_bstr;
  //const jschar*               uctokenptr;
    uintN                      _flags;    
    uintN                      _error_number;
  //const jschar*               ucmessage;      // the (default) error message
  //const jschar**              messageArgs;    // arguments for the error message
};

//-------------------------------------------------------------------------------------Script_error

struct Script_error : iunknown_implementation< Script_error, IActiveScriptError >,
                      My_thread_only 
{
    STDMETHODIMP GetExceptionInfo( EXCEPINFO* excepinfo )
    {
        excepinfo->wCode             = 0; //_error_report._error_number;
        excepinfo->wReserved         = 0;
        excepinfo->bstrSource        = NULL;
        excepinfo->bstrDescription   = bstr_from_string( _error_report._message );
        excepinfo->bstrHelpFile      = NULL;
        excepinfo->dwHelpContext     = 0;
        excepinfo->pvReserved        = NULL;
        excepinfo->pfnDeferredFillIn = NULL;
        excepinfo->scode             = 0;
        return S_OK;
    }
    

    STDMETHODIMP GetSourcePosition( DWORD* source_context, ULONG* line, LONG* col )
    {
        *source_context = 0;
        *line           = _error_report._line_number;
        *col            = _error_report._column_number;
        return S_OK;
    }
    
    
    STDMETHODIMP GetSourceLineText( BSTR* result )
    {
        return String_to_bstr( _error_report._line_bstr, result );
    }


    Error_report               _error_report;
};

//-------------------------------------------------------------------------------------Spidermonkey

struct Spidermonkey : creatable_iunknown_implementation< CLSID_Spidermonkey, Spidermonkey, IActiveScript >,
                      IActiveScriptParse,
                      My_thread_only
{

    //------------------------------------------------------------------------------Object_register

    struct Object_register
    {
                                    Object_register         ( Spidermonkey* e )                     : _zero_(this+1), _spidermonkey(e), _mutex("Spidermonkey::Object_register") {}


        void                        clear                   ();
        void                        add                     ( JSObject*, IDispatch* );
        void                        remove                  ( JSObject*, IDispatch* );
        void                        enter_temporary_mode    ();
        void                        leave_temporary_mode    ();
        void                        remove_temporaries      ();
        JSObject*                   jsobject_from_iunknown  ( IUnknown* );
        JSObject*                   jsobject_from_idispatch ( IDispatch* );
        ptr<IDispatch>              idispatch_from_jsobject ( JSObject* );
        DISPID                      get_dispid              ( IDispatch*, const Bstr& name );

      private:
        typedef hash_map< IDispatch*, JSObject* >               Idispatch_to_jsobject_map;
        typedef hash_map< JSObject* , IDispatch* >              Jsobject_to_idispatch_map;
        typedef hash_map< IDispatch*, ptr<Idispatch_dispids> >  Idispatch_dispid_map;
        typedef hash_set<IDispatch*>                            Idispatch_set;

        Fill_zero                  _zero_;
        Spidermonkey*              _spidermonkey;
        Idispatch_to_jsobject_map  _idispatch_to_jsobject_map;
        Jsobject_to_idispatch_map  _jsobject_to_idispatch_map;
        Idispatch_set              _temporary_idispatch_set;
        Idispatch_dispid_map       _idispatch_dispid_map;       // Hier merken wir uns die DISPIDs der IDispatchs, die von JavaScript benutzt werden
        Mutex                      _mutex;
        int                        _temporary_mode;
    };

    //----------------------------------------------------------------------------In_temporary_mode

    struct In_temporary_mode {
                                    In_temporary_mode(Object_register* o)   : _object_register(o) { _object_register->enter_temporary_mode(); }
                                   ~In_temporary_mode()                     { _object_register->leave_temporary_mode(); }
    private:
        Object_register* const     _object_register;
    };

    //---------------------------------------------------------------------------------------------

    static Class_descriptor     class_descriptor;



                                Spidermonkey            ();
    virtual                    ~Spidermonkey            ();


    STDMETHODIMP                QueryInterface         ( const IID&, void** );
    STDMETHODIMP_(ULONG)        AddRef                 ()                                           { return Iunknown_implementation::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                ()                                           { return Iunknown_implementation::Release(); }


    // IActiveScript
    STDMETHODIMP                SetScriptSite           ( IActiveScriptSite* );
    STDMETHODIMP                GetScriptSite           ( REFIID iid, void** result )               { if( _site )  return _site->QueryInterface( iid, result );  
                                                                                                      *result = NULL; return S_OK; }
    STDMETHODIMP                SetScriptState          ( SCRIPTSTATE );
    STDMETHODIMP                GetScriptState          ( SCRIPTSTATE* result )                     { *result = _script_state; return S_OK; }
    STDMETHODIMP                Close                   ();
    STDMETHODIMP                AddNamedItem            ( LPCOLESTR, DWORD );
    STDMETHODIMP                AddTypeLib              ( REFGUID, DWORD, DWORD, DWORD )            { return E_NOTIMPL; }
    STDMETHODIMP                GetScriptDispatch       ( LPCOLESTR, IDispatch** );
    STDMETHODIMP                GetCurrentScriptThreadID( SCRIPTTHREADID* )                         { return E_NOTIMPL; }
    STDMETHODIMP                GetScriptThreadID       ( DWORD, SCRIPTTHREADID* )                  { return E_NOTIMPL; }
    STDMETHODIMP                GetScriptThreadState    ( SCRIPTTHREADID, SCRIPTTHREADSTATE* )      { return E_NOTIMPL; }
    STDMETHODIMP                InterruptScriptThread   ( SCRIPTTHREADID, const EXCEPINFO*, DWORD ) { return E_NOTIMPL; }
    STDMETHODIMP                Clone                   ( IActiveScript** )                         { return E_NOTIMPL; }


    // IActiveScriptParse
    STDMETHODIMP                InitNew                 ();
        
    STDMETHODIMP                AddScriptlet            ( LPCOLESTR, // pstrDefaultName,
                                                          LPCOLESTR, // pstrCode,
                                                          LPCOLESTR, // pstrItemName,
                                                          LPCOLESTR, // pstrSubItemName,
                                                          LPCOLESTR, // pstrEventName,
                                                          LPCOLESTR, // pstrDelimiter,
                                                          DWORD    , // dwSourceContextCookie,
                                                          ULONG    , // ulStartingLineNumber,
                                                          DWORD    , // dwFlags,
                                                          BSTR*    , // pbstrName,
                                                          EXCEPINFO* )                              { return E_NOTIMPL; }
        
    STDMETHODIMP                ParseScriptText         ( LPCOLESTR pstrCode,
                                                          LPCOLESTR pstrItemName,
                                                          IUnknown* punkContext,
                                                          LPCOLESTR pstrDelimiter,
                                                          DWORD     dwSourceContextCookie,
                                                          ULONG     ulStartingLineNumber,
                                                          DWORD     dwFlags,
                                                          VARIANT*  pvarResult,
                                                          EXCEPINFO*pexcepinfo );

  //private:
    friend struct               Jsobject_idispatch;

    void                        initialize              ();
    void                        start                   ();
    void                        release_js_scripts      ();

    void                        set_js_exception        ( const string& error_text );
    void                        set_js_exception        ( const exception& );
    void                        set_js_exception        ( const _com_error& );
    void                        throw_spidermonkey      ( const string& function )                  { throw_spidermonkey( function.c_str() ); }
    void                        throw_spidermonkey      ( const char* function );

    string                      string_from_jstype      ( JSType );


    static inline Spidermonkey* spidermonkey            ( JSContext* cx )                                                                        { return (Spidermonkey*)JS_GetContextPrivate( cx ); }


    // Callbacks statisch

    static JS_DLL_CALLBACK JSBool   activeXObject_jsfunction       ( JSContext* cx, JSObject* jsobject, uintN argc, jsval* argv, jsval* result ) { return spidermonkey( cx )->activeXObject_jsfunction       ( jsobject, argc, argv, result ); }
    static JS_DLL_CALLBACK JSBool   declare_java_package_jsfunction( JSContext* cx, JSObject* jsobject, uintN argc, jsval* argv, jsval* result ) { return spidermonkey( cx )->declare_java_package_jsfunction( jsobject, argc, argv, result ); }
  //static JS_DLL_CALLBACK JSBool   create_java_object_jsfunction  ( JSContext* cx, JSObject* jsobject, uintN argc, jsval* argv, jsval* result ) { return spidermonkey( cx )->create_java_object_jsfunction  ( jsobject, argc, argv, result ); }
                                                                                                                                                                                                            
    static JS_DLL_CALLBACK void     error_reporter              ( JSContext* cx, const char *message, JSErrorReport* error_report )              { return spidermonkey( cx )->error_reporter            ( message, error_report ); }
    static JS_DLL_CALLBACK JSBool   global_object_property_get  ( JSContext* cx, JSObject* jsobject, jsval id, jsval* result )                   { return spidermonkey( cx )->global_object_property_get( jsobject, id, result ); }
    static JS_DLL_CALLBACK void     com_finalize                ( JSContext* cx, JSObject* jsobject )                                            {        spidermonkey( cx )->com_finalize              ( jsobject ); }
    static JS_DLL_CALLBACK JSBool   com_property_add            ( JSContext* cx, JSObject* jsobject, jsval id, jsval* result )                   { return spidermonkey( cx )->com_property_add          ( jsobject, id, result ); }
    static JS_DLL_CALLBACK JSBool   com_property_delete         ( JSContext* cx, JSObject* jsobject, jsval id, jsval* result )                   { return spidermonkey( cx )->com_property_delete       ( jsobject, id, result ); }
    static JS_DLL_CALLBACK JSBool   com_property_enumerate      ( JSContext* cx, JSObject* jsobject )                                            { return spidermonkey( cx )->com_property_enumerate    ( jsobject ); }
    static JS_DLL_CALLBACK JSBool   com_property_resolve        ( JSContext* cx, JSObject* jsobject, jsval id )                                  { return spidermonkey( cx )->com_property_resolve      ( jsobject, id ); }
    static JS_DLL_CALLBACK JSBool   com_property_get            ( JSContext* cx, JSObject* jsobject, jsval id, jsval* result )                   { return spidermonkey( cx )->com_property_get          ( jsobject, id, result ); }
    static JS_DLL_CALLBACK JSBool   com_property_set            ( JSContext* cx, JSObject* jsobject, jsval id, jsval* result )                   { return spidermonkey( cx )->com_property_set          ( jsobject, id, result ); }
    static JS_DLL_CALLBACK JSBool   com_convert                 ( JSContext* cx, JSObject* jsobject, JSType jstype, jsval* result )              { return spidermonkey( cx )->com_convert               ( jsobject, jstype, result ); }
    static JS_DLL_CALLBACK JSBool   com_call                    ( JSContext* cx, JSObject* jsobject, uintN argc, jsval* argv, jsval* result )    { return spidermonkey( cx )->com_call                  ( jsobject, argc, argv, result ); }
    static JS_DLL_CALLBACK JSBool   com_no_such_method_call_    ( JSContext* cx, JSObject* jsobject, uintN argc, jsval* argv, jsval* result )    { return spidermonkey( cx )->com_no_such_method_call   ( cx, jsobject, argc, argv, result ); }
    static JS_DLL_CALLBACK JSBool   com_toString                ( JSContext* cx, JSObject* jsobject, uintN argc, jsval* argv, jsval* result )    { return spidermonkey( cx )->com_toString              ( jsobject, argc, argv, result ); }


    static JSJavaThreadState*       java__map_js_context_to_jsj_thread( JSContext* cx, char** error_msg )                                        { return spidermonkey( cx )->java__map_js_context_to_jsj_thread( error_msg ); }


    // Callbacks

    JSBool                      activeXObject_jsfunction       ( JSObject*, uintN argc, jsval* argv, jsval* result );
    JSBool                      declare_java_package_jsfunction( JSObject*, uintN argc, jsval* argv, jsval* result );
  //JSBool                      create_java_object_jsfunction  ( JSObject*, uintN argc, jsval* argv, jsval* result );

    void                        error_reporter              ( const char*, JSErrorReport* );
    JSBool                      global_object_property_get  ( JSObject*, jsval id, jsval* result );
    void                        com_finalize                ( JSObject* );
    JSBool                      com_property_add            ( JSObject*, jsval id, jsval* result );
    JSBool                      com_property_delete         ( JSObject*, jsval id, jsval* result );
    JSBool                      com_property_enumerate      ( JSObject* );
    JSBool                      com_property_resolve        ( JSObject*, jsval id );
    JSBool                      com_property_get            ( JSObject*, jsval id, jsval* result );
    JSBool                      com_property_set            ( JSObject*, jsval id, jsval* result );
    JSBool                      com_convert                 ( JSObject*, JSType, jsval* result );
    JSBool                      com_call                    ( JSObject*, uintN argc, jsval* argv, jsval* jsresult );
    JSBool                      com_no_such_method_call     ( JSContext*, JSObject*, uintN argc, jsval* argv, jsval* jsresult );
    JSBool                      com_toString                ( JSObject*, uintN argc, jsval* argv, jsval* jsresult );

    void                        release_idispatch           (JSObject*, IDispatch*, const string& debug_text);
    JSObject*                   create_intermediate_object  ( IDispatch*, JSObject* arguments );
    void                        invalidate_intermediate_object(JSObject*, IDispatch*);

    JSJavaThreadState*          java__map_js_context_to_jsj_thread( char** error_msg );


    void                        get_intermediate_object_parameters( JSObject*, Dispparams*, int arg_offset = 0 );

    // Konvertierung

    JSString*                   jsstring_from_string        ( const string& );
    jsval                       jsval_from_string           ( const string& );
    jsval                       jsval_from_bstr             ( const BSTR& );
    jsval                       jsval_from_variant          ( const VARIANT& );
    jsval                       jsval_from_jsstring         ( const JSString* jsstring )            { return static_cast<jsval>( STRING_TO_JSVAL( (size_t)jsstring ) ); }
    jsval                       jsval_from_jsobject         ( const JSObject* jsobject )            { return OBJECT_TO_JSVAL( (size_t)jsobject ); }
    jsval                       jsval_from_double           ( double );
    JSString*                   jsstring_from_jsval         ( jsval );
    JSObject*                   jsobject_from_jsval         ( jsval v )                             { return JSVAL_TO_OBJECT( (size_t)v ); }

    HRESULT                     Variant_to_jsval            ( const VARIANT&, jsval* result );
    HRESULT                     Jsval_to_variant            ( jsval, VARIANT* result );

    JSObject*                   jsobject_from_idispatch     ( IDispatch* idispatch )                { return _object_register.jsobject_from_idispatch( idispatch ); }
    ptr<IDispatch>              idispatch_from_jsobject     ( JSObject* jsobject )                  { return _object_register.idispatch_from_jsobject( jsobject ); }

    Idispatch_member            get_idispatch_member        ( JSObject*, const Bstr& name );


  //static Loaded_module        spidermonkey_module;
  //static Loaded_module        liveconnect_module;
  //static Mutex                static_mutex;

    Fill_zero                  _zero_;
    ptr<IActiveScriptSite>     _site;
    SCRIPTSTATE                _script_state;
    JSRuntime*                 _js_runtime;
    JSContext*                 _jscontext;
    JSObject*                  _global_jsobject;
    ptr<Script_error>          _script_error;
    int                        _script_number;
    vector<JSScript*>          _js_scripts;

    typedef hash_map< Bstr, ptr<IDispatch> >    Added_items;
    typedef list    < ptr<Idispatch_dispids> >  Global_members;

    Added_items                _added_items;
    Global_members             _global_members;
    Object_register            _object_register;

#   ifdef DONT_USE_HASH_SET
        typedef hash_map<Bstr,bool>     Javascript_standard_names;
#   else
        typedef hash_set<Bstr>          Javascript_standard_names;
#   endif

    Javascript_standard_names  _javascript_standard_object_names;
    Javascript_standard_names  _javascript_standard_global_object_names;


    JSFunction*                _activexobject_jsfunction;
    JSFunction*                _declare_java_package_jsfunction;
  //JSFunction*                _create_java_object_jsfunction;

    ptr<javabridge::Vm>        _java_vm;
    vector<jclass>             _java_classes;
    JSJavaVM*                  _jsj_java_vm;
    JSJavaThreadState*         _jsj_thread_state;
};

//-------------------------------------------------------------------------------Jsobject_idispatch

#ifdef Z_WINDOWS
    struct __declspec( uuid( "{feee46e8-6c1b-11d8-8103-000476ee8afb}" ) )  Jsobject_idispatch;
#endif

Z_DEFINE_GUID( IID_Jsobject_idispatch, 0xfeee46e8, 0xc1b, 0x11d8, 0x81, 0x03, 0x00, 0x04, 0x76, 0xee, 0x8a, 0xfb );

struct Jsobject_idispatch : Idispatch_base_implementation, 
                            My_thread_only//, Object_adapter
{
    DEFINE_UUIDOF( Jsobject_idispatch )


                                Jsobject_idispatch      ( Spidermonkey*, JSObject* );
    virtual                    ~Jsobject_idispatch      ();

    virtual STDMETHODIMP        QueryInterface          ( const IID&, void** );

    virtual STDMETHODIMP        GetTypeInfoCount        ( UINT* )                                   { return E_NOTIMPL; }
    virtual STDMETHODIMP        GetTypeInfo             ( UINT, LCID, ITypeInfo** )                 { return E_NOTIMPL; }
    virtual STDMETHODIMP        GetIDsOfNames           ( REFIID, LPOLESTR*, UINT, LCID, DISPID* );
    virtual STDMETHODIMP        Invoke                  ( DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT* );

    Fill_zero                  _zero_;
    ptr<Spidermonkey>          _spidermonkey;
    JSObject*                  _jsobject;
    vector<string>             _names;                  // Hier steckt GetIDsOfNames() die Namen rein, um eine dispid zu ermitteln
};

//-------------------------------------------------------------------------------Idispatch_jsobject
/*
struct Idispatch_jsobject : Object
{
                                Idispatch_jsobject      ( Spidermonkey*, IDispatch* );
    virtual                    ~Idispatch_jsobject      ();

    JSBool                      call                    ( uintN argc, jsval* argv, jsval* jsresult );

  //static JS_DLL_CALLBACK void finalize                ( JSContext* jscontext, JSObject* jsobject );

    Fill_zero                  _zero_;
    ptr<Spidermonkey>          _spidermonkey;
    ptr<IDispatch>             _idispatch;
    JSObject*                  _jsobject;
};
*/
//-----------------------------------------------------------------------Java_liveconnect_interface
    
struct Java_liveconnect_interface
{
    static JSContext*           map_jsj_thread_to_js_context( JSJavaThreadState*, JNIEnv*, char** errp );
  //static JSJavaThreadState*   map_js_context_to_jsj_thread( JSContext*, char** errp );
    static JSObject*            map_java_object_to_js_object( JNIEnv*, void *pJavaObject, char **errp );
    static JSPrincipals*        get_JSPrincipals_from_java_caller( JNIEnv*, JSContext*, void** pNSIPrincipaArray, int numPrincipals, void *pNSISecurityContext );
  //static JSBool               enter_js_from_java          ( JNIEnv*, char** errp );
  //static void                 exit_js                     ( JNIEnv*, JSContext* );
    static void                 error_print                 ( const char *error_msg );
    static jobject              get_java_wrapper            ( JNIEnv*, jint jsobject );
    static jint                 unwrap_java_wrapper         ( JNIEnv*, jobject java_wrapper );
    static JSBool               create_java_vm              ( SystemJavaVM**, JNIEnv**, void* initargs );
    static JSBool               destroy_java_vm             ( SystemJavaVM*, JNIEnv* );
    static JNIEnv*              attach_current_thread       ( SystemJavaVM* );
    static JSBool               detach_current_thread       ( SystemJavaVM*, JNIEnv* );
    static SystemJavaVM*        get_java_vm                 ( JNIEnv* jenv );
};

//--------------------------------------------------------------------------------------------extern

extern Typelib_ref              typelib;

//---------------------------------------------------------------------------------------------const

const char                      spidermonkey_module_filename[]  = "spidermonkey.dll";
const char                      liveconnect_module_filename[]   = "liveconnect.dll";
const int                       size_before_garbage_collection  = 50*1024*1024;
const int                       context_stack_size              = 100*1024;
const int                       initial_jsobject_method_count   = 20;                       // Soviele Methoden und Eigenschaften hat ein übliches JSObjekt 
const char                      no_such_method_name[]           = "__noSuchMethod__";

const char* javascript_standard_object_names[] = 
{
    "Function",
    "Object",
    no_such_method_name,
    NULL
};

const char* javascript_standard_global_object_names[] =         // javascript_standard_object_names wird hinzugefügt
{
    "Array",
    "Boolean",
    "Call",
    "Date",
    "Error",
    "Infinity",
    "Math",
    "Number",
    "NaN",
    "RegExp",
    "Script",
    "String",
    "With",
    NULL
};


/*
static JSClass standard_jsclass =
{
    "__Standard_object",
    0,
    JS_PropertyStub, 
    JS_PropertyStub, 
    JS_PropertyStub,
    JS_PropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub, 
    JS_ConvertStub, 
};
*/

static JSClass com_jsclass =
{
    "ActiveXObject",
    JSCLASS_HAS_PRIVATE,
    Spidermonkey::com_property_add,         // JS_PropertyStub,
    Spidermonkey::com_property_delete,      // JS_PropertyStub,
    Spidermonkey::com_property_get,         // JS_PropertyStub,
    Spidermonkey::com_property_set,         // JS_PropertyStub,
    Spidermonkey::com_property_enumerate,   // JS_EnumerateStub,
    Spidermonkey::com_property_resolve,     // JS_ResolveStub,
    Spidermonkey::com_convert,              // JS_ConvertStub,
    Spidermonkey::com_finalize,
    NULL,                                   // Com_class_handler::get_object_ops,
    NULL,
    Spidermonkey::com_call,                 // Objekt wie eine Funktion aufrufen: object(...) (entspricht DISPID_VALUE in COM)
    NULL,                                   // Com_class_handler::construct,
    NULL,
    NULL,
    NULL
};


static JSClass intermediate_com_jsclass =
{
    "__Intermediate_com_object",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, //Spidermonkey::com_property_add,         // JS_PropertyStub,
    JS_PropertyStub, //Spidermonkey::com_property_delete,      // JS_PropertyStub,
    Spidermonkey::com_property_get,         // JS_PropertyStub,
    Spidermonkey::com_property_set,         // JS_PropertyStub,
    JS_EnumerateStub, //Spidermonkey::com_property_enumerate,   // JS_EnumerateStub,
    JS_ResolveStub,   //Spidermonkey::com_property_resolve,     // JS_ResolveStub,
    JS_ConvertStub,   //Spidermonkey::com_convert,              // JS_ConvertStub,
    Spidermonkey::com_finalize,
    NULL,                                   // Com_class_handler::get_object_ops,
    NULL,
    NULL, //Spidermonkey::com_call,                 // Objekt wie eine Funktion aufrufen: object(...) (entspricht DISPID_VALUE in COM)
    NULL,                                   // Com_class_handler::construct,
    NULL,
    NULL,
    NULL
};

/*
static JSClass global_jsclass = 
{ 
    "global",
    0, 
    JS_PropertyStub,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub,
}; 
*/
//-------------------------------------------------------------------------------------------static

static creatable_com_class_descriptor< Spidermonkey, IActiveScript >    class_descriptor ( (Typelib_ref*)NULL, "JavaScript" );


Spidermonkey::Class_descriptor  Spidermonkey::class_descriptor  ( &typelib, "Spidermonkey" );

//Mutex                           Spidermonkey::static_mutex;
//Loaded_module                   Spidermonkey::spidermonkey_module;
//Loaded_module                   Spidermonkey::liveconnect_module;


// Pakete, die Java zugeordnet sind und in new benutzt werden können:

static JavaPackageDef my_java_packages[] =
{
    { "sos", NULL, PKG_USER, 0 },
    { "org", NULL, PKG_USER, 0 },
    {}
};


JSJCallbacks jsj_callbacks =
{
    NULL,       //Java_liveconnect_interface::map_jsj_thread_to_js_context,
    Spidermonkey::java__map_js_context_to_jsj_thread,
    NULL,                     //map_java_object_to_js_object,
    NULL,                     //get_JSPrincipals_from_java_caller,
    NULL,                     //enter_js_from_java,
    NULL,                     //Java_liveconnect_interface::exit_js,
    Java_liveconnect_interface::error_print,
    Java_liveconnect_interface::get_java_wrapper,
    Java_liveconnect_interface::unwrap_java_wrapper,
    Java_liveconnect_interface::create_java_vm,
    Java_liveconnect_interface::destroy_java_vm,
    Java_liveconnect_interface::attach_current_thread,
    Java_liveconnect_interface::detach_current_thread,
    Java_liveconnect_interface::get_java_vm
};

//----------------------------------------------------------------------------------------string_from_jsobject_idispatch

static string string_from_jsobject_idispatch( JSObject* jsobject, IDispatch* idispatch )
{
    return S() << (void*)jsobject << " <--> " << (void*)idispatch << "(" << name_of_type_debug( *idispatch ) << ")";
}

//---------------------------------------------------------------------------Object_register::clear

void Spidermonkey::Object_register::clear()
{
    Z_MUTEX( _mutex )
    {
        Z_FOR_EACH( Jsobject_to_idispatch_map, _jsobject_to_idispatch_map, it )
        {
            if( it->second )
            {
                IDispatch* idispatch = it->second;
                it->second = NULL;
                idispatch->Release();
            }
        }
        
        _idispatch_to_jsobject_map.clear();
        _jsobject_to_idispatch_map.clear();

        _idispatch_dispid_map.clear();

        _temporary_idispatch_set.clear();
    }
}

//---------------------------------------------------------------Spidermonkey::Object_register::add

void Spidermonkey::Object_register::add( JSObject* jsobject, IDispatch* idispatch )
{
    Z_MUTEX( _mutex )
    {
        //Z_LOG2( "spidermonkey.idispatch", "Spidermonkey::Object_register::add " << string_from_jsobject_idispatch( js_object, idispatch ) << "\n" );

        _jsobject_to_idispatch_map[ jsobject ] = idispatch;
        _idispatch_to_jsobject_map[ idispatch ] = jsobject;

        if (_temporary_mode) 
            _temporary_idispatch_set.insert(idispatch);
    }
}

//------------------------------------------------------------Spidermonkey::Object_register::remove

void Spidermonkey::Object_register::remove( JSObject* jsobject, IDispatch* idispatch )
{
    Z_MUTEX( _mutex )
    {
        //Z_LOG2( "spidermonkey.idispatch", "Spidermonkey::Object_register::remove " << string_from_jsobject_idispatch( js_object, idispatch ) << "\n" );

        _jsobject_to_idispatch_map.erase( jsobject );
        _idispatch_to_jsobject_map.erase( idispatch );

        _idispatch_dispid_map.erase( idispatch );

        _temporary_idispatch_set.erase( idispatch );
    }
}

//----------------------------------------------Spidermonkey::Object_register::enter_temporary_mode

void Spidermonkey::Object_register::enter_temporary_mode() {
    Z_LOG2("zschimmer",Z_FUNCTION << "\n" );
    _temporary_mode++;
}

//----------------------------------------------Spidermonkey::Object_register::leave_temporary_mode

void Spidermonkey::Object_register::leave_temporary_mode() {
    Z_LOG2("zschimmer",Z_FUNCTION << "\n" );
    --_temporary_mode;
    if (!_temporary_mode)  remove_temporaries();
}

//------------------------------------------------Spidermonkey::Object_register::remove_temporaries

void Spidermonkey::Object_register::remove_temporaries() 
{
    Idispatch_set copy = _temporary_idispatch_set;

    Z_FOR_EACH (Idispatch_set, copy, it) {
        IDispatch* idispatch = *it;
        JSObject* jsobject = _idispatch_to_jsobject_map[idispatch];
        remove(jsobject, idispatch);
        _spidermonkey->invalidate_intermediate_object(jsobject, idispatch);
    }

    assert(_temporary_idispatch_set.empty());
    _temporary_idispatch_set.clear();
}

//--------------------------------------------Spidermonkey::Object_register::jsobject_from_iunknown

JSObject* Spidermonkey::Object_register::jsobject_from_iunknown( IUnknown* iunknown )
{
    JSObject* result = NULL;

    if( iunknown == NULL )  return NULL;

    if( try_qi_ptr<IDispatch> idispatch = iunknown )
    {
        return jsobject_from_idispatch( idispatch );
    }
    else
        _spidermonkey->throw_spidermonkey( "jsobject_from_iunknown: Unbekannter Objekt-Typ" );

    return result;
}

//-------------------------------------------Spidermonkey::Object_register::jsobject_from_idispatch

JSObject* Spidermonkey::Object_register::jsobject_from_idispatch( IDispatch* idispatch )
{
    JSObject* result = NULL;
    JSBool    ok;

    if( idispatch == NULL )  return NULL;


    Z_MUTEX( _mutex )
    {
        Idispatch_to_jsobject_map::iterator it = _idispatch_to_jsobject_map.find( idispatch );
        if( it != _idispatch_to_jsobject_map.end() )  result = it->second;
    }

    if( !result )
    {
        //if( try_qi_ptr<Jsobject_idispatch> jsobject_idispatch = idispatch )     // idispatch ist ein Jsobject_idispatch, also eigentlich ein JSObject?
        if( Jsobject_idispatch* jsobject_idispatch = static_cast<Jsobject_idispatch*>( static_cast<IDispatch*>( com_query_interface_or_null( idispatch, IID_Jsobject_idispatch ) ) ) )
        {
            result = jsobject_idispatch->_jsobject;
        }
        else
        if( try_qi_ptr<javabridge::Jobject_idispatch> jobject_idispatch = idispatch )
        {
            jsval val;
            ok = JSJ_ConvertJavaObjectToJSValue( _spidermonkey->_jscontext, jobject_idispatch->_jobject, &val );
            if( !ok )  _spidermonkey->throw_spidermonkey( "JSJ_ConvertJavaObjectToJSValue" );

            if( !JSVAL_IS_OBJECT( val ) )  _spidermonkey->throw_spidermonkey( "JSJ_ConvertJavaObjectToJSValue, no JSObject" );

            result = _spidermonkey->jsobject_from_jsval( val );
        }
        else                                                                    // idispatch ist ein echtes COM-Objekt
        {
            JSObject* jsobject = JS_NewObject( _spidermonkey->_jscontext, &com_jsclass, NULL, NULL );
            if( !jsobject )  _spidermonkey->throw_spidermonkey( "JS_NewObject" );

            JSFunction* js_no_such_method = JS_DefineFunction( _spidermonkey->_jscontext, jsobject, no_such_method_name, com_no_such_method_call_, 2, 0 );
            if( !js_no_such_method )  _spidermonkey->throw_spidermonkey( "JS_DefineFunction" );

            JSFunction* toString_jsfunction = JS_DefineFunction( _spidermonkey->_jscontext, jsobject, "toString", com_toString, 0, 0 );
            if( !toString_jsfunction )  _spidermonkey->throw_spidermonkey( "JS_DefineFunction" );

            JSBool ok = JS_SetPrivate( _spidermonkey->_jscontext, jsobject, idispatch );
            if( !ok )  _spidermonkey->throw_spidermonkey( "JS_SetPrivate" );

            add( jsobject, idispatch );
            int count = idispatch->AddRef();

            ptr<Idispatch_dispids> idispatch_dispids = Z_NEW( Idispatch_dispids( idispatch, "" ) );
            idispatch_dispids->add_dispid( "toString", DISPID_VALUE );
            _idispatch_dispid_map[ idispatch ] = idispatch_dispids;

            Z_LOG2( "spidermonkey.idispatch", __FUNCTION__ << "  " << string_from_jsobject_idispatch( jsobject, idispatch ) << ".AddRef() ref=" << count <<"\n" );

            result = jsobject;
        }
    }

    return result;
}

//-------------------------------------------Spidermonkey::Object_register::idispatch_from_jsobject

ptr<IDispatch> Spidermonkey::Object_register::idispatch_from_jsobject( JSObject* jsobject )
{
    ptr<IDispatch> result;

    Z_MUTEX( _mutex )
    {
        Jsobject_to_idispatch_map::iterator it = _jsobject_to_idispatch_map.find( jsobject );
        if( it != _jsobject_to_idispatch_map.end() )  result = it->second;
    }

    if( !result )
    {
        if( jsobject == _spidermonkey->_global_jsobject )
        {
            ptr<Jsobject_idispatch> spidermonkey_idispatch = Z_NEW( Jsobject_idispatch( _spidermonkey, jsobject ) );
            result = +spidermonkey_idispatch;
        }
        else
        {
            _spidermonkey->throw_spidermonkey( "Ein Javascript-Objekt kann nicht an COM übergeben werden" );
            // JS_AddNamedRoot():
            // You should use JS_AddNamedRoot to root only JS objects, JS strings, or JS doubles, 
            // and then only if they are derived from calls to their respective JS_NewXXX creation functions.
        }
    }

    return result;

    /* Array zu SAFEARRAY?
    jsuint length;
    JSBool ok = JS_GetArrayLength( jscontext, args, &length );
    if( !ok )  throw_xc( "JS_GetArrayLength" );

    for( int i = 0; i < length; i++ )
    {
        jsval value;
        ok = JS_GetElement( jscontext, args, i, &value );
        if( !ok )  throw_xc( "JS_GetElement" );

        string text = JS_GetStringBytes( jsstring_from_jsval( value ) );
        printf( "%s, ", text.c_str() );
    }
    printf( "\n" );
    */
}

//----------------------------------------------------------------------Object_register::get_dispid

DISPID Spidermonkey::Object_register::get_dispid( IDispatch* idispatch, const Bstr& name )
{
    DISPID result = DISPID_UNKNOWN;

    Z_MUTEX( _mutex )
    {
        result = _idispatch_dispid_map[ idispatch ]->get_dispid( name );
    }

    return result;
}

//-----------------------------------------------------------Jsobject_idispatch::Jsobject_idispatch

Jsobject_idispatch::Jsobject_idispatch( Spidermonkey* e, JSObject* jsobject )
: 
    _zero_(this+1),
    _spidermonkey(e),
    _jsobject( jsobject ),
    _names( 1 + initial_jsobject_method_count )
{ 
    JSBool ok;

    if( _jsobject != _spidermonkey->_global_jsobject )  
    {
        Z_LOG2( "spidermonkey.idispatch", "spidermonkey::Jsobject_idispatch  " << string_from_jsobject_idispatch( jsobject, this ) << "\n" );
        ok = JS_AddNamedRoot( _spidermonkey->_jscontext, &_jsobject, "Jsobject_idispatch" );
        if( !ok )  _spidermonkey->throw_spidermonkey( "JS_AddNamedRoot" );
    }

    _spidermonkey->_object_register.add( jsobject, this );

    _names.push_back( "" );      // dispid 0 ist reserviert (DISPID_VALUE)
}

//----------------------------------------------------------Jsobject_idispatch::~Jsobject_idispatch

Jsobject_idispatch::~Jsobject_idispatch()
{
    if( _spidermonkey->_jscontext )
    {
        _spidermonkey->_object_register.remove( _jsobject, this );

        if( _jsobject != _spidermonkey->_global_jsobject )
        {
            // Sollte nicht durchlaufen werden, weil Javascript-Objekte noch nicht in IDispatch gekapselt werden.
            if( _spidermonkey->_jscontext )
            {
                Z_LOG2( "spidermonkey.idispatch", "spidermonkey::~Jsobject_idispatch " << string_from_jsobject_idispatch( _jsobject, this ) << "  JS_RemoveRoot()\n" );
                JS_RemoveRoot( _spidermonkey->_jscontext, &_jsobject );
            }
        }
    }
}

//---------------------------------------------------------------Jsobject_idispatch::QueryInterface

STDMETHODIMP Jsobject_idispatch::QueryInterface( const IID& iid, void** result )
{ 
    Z_COM_MY_THREAD_ONLY;
    
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Jsobject_idispatch, result );

    return Idispatch_base_implementation::QueryInterface( iid, result );
}

//----------------------------------------------------------------Jsobject_idispatch::GetIDsOfNames

STDMETHODIMP Jsobject_idispatch::GetIDsOfNames( REFIID, LPOLESTR* names, UINT names_count, LCID, DISPID* dispid )
{ 
    Z_COM_MY_THREAD_ONLY;
    
    if( !_spidermonkey->_jscontext )  return E_ACCESSDENIED;
    if( names_count != 1           )  return DISP_E_NONAMEDARGS;

    string name = string_from_ole( names[0] );
    JSBool ok;
    jsval  value;

    uint i;
    for( i = 0; i < _names.size(); i++ )  if( _names[i] == name )  { *dispid = i;  return S_OK; }

    ok = JS_LookupProperty( _spidermonkey->_jscontext, _jsobject, name.c_str(), &value );
    if( !ok  ||  JSVAL_IS_VOID( value ) )  return DISP_E_UNKNOWNNAME;

    _names.push_back( name );

    *dispid = i;

    return S_OK; 
}

//-----------------------------------------------------------------------Jsobject_idispatch::Invoke

STDMETHODIMP Jsobject_idispatch::Invoke( DISPID dispid, REFIID, LCID, WORD, DISPPARAMS* dispparams, VARIANT* result, EXCEPINFO* excepinfo, UINT* ) 
{ 
    HRESULT hr = S_OK;
    JSBool  ok;

    memset( excepinfo, 0, sizeof *excepinfo );

    Z_COM_MY_THREAD_ONLY;
    
    if( !_spidermonkey->_jscontext  )  return E_ACCESSDENIED;
    if( dispparams->cNamedArgs != 0 )  return DISP_E_NONAMEDARGS;

    try
    {
        Spidermonkey::In_temporary_mode in_temporary_mode ( &_spidermonkey->_object_register );    // Gibt hiernach angeforderte Objekte wieder frei

        if( dispid < 1  ||  dispid >= (int)_names.size() )  return DISP_E_MEMBERNOTFOUND;
        const char* name_ptr = _names[ dispid ].c_str();

        int    argc     = dispparams->cArgs;
        jsval* argv     = new jsval[ dispparams->cArgs ];
        jsval  jsresult;

        for( int i = 0; i < argc; i++ )
        {
            hr = _spidermonkey->Variant_to_jsval( dispparams->rgvarg[  argc - 1 - i ], &argv[i] );
            if( FAILED(hr) )  return hr;
        }

        _spidermonkey->_script_error = NULL;
        ok = JS_CallFunctionName( _spidermonkey->_jscontext, _jsobject, name_ptr, argc, argv, &jsresult );

        delete[] argv;

        if( !ok )  _spidermonkey->throw_spidermonkey( S() << "JS_CallFunctionName " << name_ptr );

        if( result )
        {
            hr = _spidermonkey->Jsval_to_variant( jsresult, result );
            if( FAILED(hr) )  throw_com( hr, "Jsval_to_variant" );
        }
    }
    catch( const exception&  x ) 
    { 
        if( excepinfo )  
        {
            excepinfo->bstrDescription = bstr_from_string( x.what() );       
            excepinfo->bstrSource      = bstr_from_string( "javascript" );
        }

        hr = DISP_E_EXCEPTION; 
    }
    catch( const _com_error& x ) 
    { 
        if( excepinfo )  
        {
            excepinfo->scode           = x.Error();
            excepinfo->bstrDescription = SysAllocString( x.Description() );  
            excepinfo->bstrSource      = bstr_from_string( "javascript" );
        }

        hr = DISP_E_EXCEPTION; 
    }

    //JS_GC( _spidermonkey->_jscontext );
    return hr;
}

//-----------------------------------------------------------------------Spidermonkey::Spidermonkey

Spidermonkey::Spidermonkey()
: 
    _zero_(this+1),
    Iunknown_implementation( &class_descriptor ), 
    _object_register(this)
{
    set_log_category_explicit( "spidermonkey.callback"  );
    set_log_category_explicit( "spidermonkey.idispatch" );

#   ifdef DONT_USE_HASH_SET
        for( const char** name_ptr = javascript_standard_object_names       ; *name_ptr; name_ptr++ )  _javascript_standard_object_names       [ *name_ptr ] = true;
        for( const char** name_ptr = javascript_standard_global_object_names; *name_ptr; name_ptr++ )  _javascript_standard_global_object_names[ *name_ptr ] = true;
#   else
        for( const char** name_ptr = javascript_standard_object_names       ; *name_ptr; name_ptr++ )  _javascript_standard_object_names       .insert( Bstr( *name_ptr ) );
        for( const char** name_ptr = javascript_standard_global_object_names; *name_ptr; name_ptr++ )  _javascript_standard_global_object_names.insert( Bstr( *name_ptr ) );
#   endif

    _javascript_standard_global_object_names.insert( _javascript_standard_object_names.begin(), _javascript_standard_object_names.end() );
}

//----------------------------------------------------------------------Spidermonkey::~Spidermonkey

Spidermonkey::~Spidermonkey()
{
    release_js_scripts();
    Close();
}

//-----------------------------------------------------------------Spidermonkey::string_from_jstype

string Spidermonkey::string_from_jstype( JSType jstype )
{
    const char* result = JS_GetTypeName( _jscontext, jstype );
    if( result )  return result;

    return "JSType(" + as_string( (int)jstype ) + ")";
}

//-------------------------------------------------------------------Spidermonkey::set_js_exception

void Spidermonkey::set_js_exception( const string& error_text )
{
    JSBool    ok;
    jsval     val;
    string    class_name = "Error";
    JSString* error_text_jsstring;
    

    error_text_jsstring = jsstring_from_string( error_text );
    if( !error_text_jsstring )  return;

    jsval argv[1] = { jsval_from_jsstring( error_text_jsstring ) };

    ok = JS_CallFunctionName( _jscontext, _global_jsobject, "Error", 1, argv, &val );
    if( ok )
    {
        JS_SetPendingException( _jscontext, val );
    }
    else
        fprintf( stderr, "Fehler bei JS_CallFunctionName( \"Error\", %s )\n", error_text.c_str() );
}

//-------------------------------------------------------------------Spidermonkey::set_js_exception

void Spidermonkey::set_js_exception( const exception& x )
{
    set_js_exception( string( x.what() ) );
}

//-------------------------------------------------------------------Spidermonkey::set_js_exception

void Spidermonkey::set_js_exception( const _com_error& x )
{
    set_js_exception( string_from_ole( x.Description() ) );
}

//-----------------------------------------------------------------Spidermonkey::throw_spidermonkey

void Spidermonkey::throw_spidermonkey( const char* function )
{
    JSBool ok;
    string error_code  = "JAVASCRIPT";
    string report_text;
    
    if( _script_error )
    {
        error_code += "-";
        error_code += as_string( _script_error->_error_report._error_number );
        report_text = _script_error->_error_report;
    }

    if( JS_IsExceptionPending( _jscontext ) )
    {
        jsval exception;
        ok = JS_GetPendingException( _jscontext, &exception );
        
        string msg = JS_GetStringBytes( JS_ValueToString( _jscontext, exception ) );
        
        if( !string_begins_with( report_text, msg ) 
         && !string_begins_with( report_text, "uncaught exception: " + msg ) )
        {
            if( string_begins_with( msg, error_prefix ) )  msg.erase( 0, error_prefix.length() );      // von: throw new Error("...")
            throw_xc( error_code.c_str(), msg, report_text, function );
        }
    }

    if( string_begins_with( report_text, uncaught_exception_prefix ) )  report_text.erase( 0, uncaught_exception_prefix.length() );      // von: throw new Error("...")

    throw_xc( error_code.c_str(), report_text, function );
}

//-----------------------------------------------------------------------Spidermonkey::com_finalize
// Wird möglicherweise von einem eigenen Thread gerufen 
// (in Version 1.5rc6 nocht nicht)

JS_DLL_CALLBACK void Spidermonkey::com_finalize( JSObject* jsobject )
{
    if( jsobject != _global_jsobject )
    {
        if (IDispatch* idispatch = (IDispatch*)JS_GetPrivate( _jscontext, jsobject )) {
            JS_SetPrivate( _jscontext, jsobject, NULL );

            if( JS_GetClass( jsobject ) != &intermediate_com_jsclass )      // Zwischenobjekte sind nicht registiert
            {
                _object_register.remove( jsobject, idispatch );
            }

            release_idispatch(jsobject, idispatch, Z_FUNCTION);
        }
    }
    //if( try_qi_ptr<Jsobject_idispatch> jsobject_idispatch = idispatch )
    //{
    //}
}

//------------------------------------------------------------------Spidermonkey::release_idispatch

void Spidermonkey::release_idispatch(JSObject* jsobject, IDispatch* idispatch, const string& debug_text)
{
    Z_LOGI2( "spidermonkey.callback", debug_text << "  " << string_from_jsobject_idispatch( jsobject, idispatch ) << ".Release()  " );
    int count = idispatch->Release();
    Z_LOG2( "spidermonkey.callback", "ref=" << count << "\n" );
}

//---------------------------------------------------------------------Spidermonkey::error_reporter

void Spidermonkey::error_reporter( const char *message, JSErrorReport* error_report )
{
    Z_LOG( "*** spidermonkey: " << message << ( error_report->linebuf? string("\n  in: ") + error_report->linebuf : "" ) << "\n" );

    _script_error = Z_NEW( Script_error() );
    _script_error->_error_report.assign( *error_report, message );

    _site->OnScriptError( _script_error );
}

//-------------------------------------------------------------Spidermonkey::com_property_enumerate

JSBool Spidermonkey::com_property_enumerate( JSObject* )
{   
    Z_LOG( __FUNCTION__ << "\n" );
    return false;
}

//---------------------------------------------------------------Spidermonkey::get_idispatch_member

Idispatch_member Spidermonkey::get_idispatch_member( JSObject* jsobject, const Bstr& name_bstr )
{
    Idispatch_member    result;
    HRESULT             hr;

    if( jsobject == _global_jsobject )
    {
        Idispatch_dispids* global_object = NULL;

        Z_FOR_EACH( Global_members, _global_members, it )
        {
            global_object = *it;

            if( !global_object->_idispatch )
            {
                ptr<IUnknown>  iunknown;
                ptr<ITypeInfo> itypeinfo;

                hr = _site->GetItemInfo( global_object->_name, SCRIPTINFO_IUNKNOWN, iunknown.pp(), itypeinfo.pp() );
                if( FAILED(hr) )  throw_com( hr, "IActiveScriptSite::GetItemInfo" );

                global_object->_idispatch = qi_ptr<IDispatch>( iunknown );
            }

            
            hr = global_object->Get_dispid( name_bstr, &result._dispid );
            if( FAILED(hr) )
            {
                if( hr == DISP_E_UNKNOWNNAME    )  continue;
                if( hr == DISP_E_MEMBERNOTFOUND )  continue;
                
                throw_com( hr, "IDispatch::GetIDsOfNames global member", string_from_bstr(name_bstr).c_str(), name_of_type_debug( *global_object->_idispatch ).c_str() );
                //throw_com( hr, "IDispatch::GetIDsOfNames global member", string_from_bstr(name_bstr).c_str() );
            }

            result._idispatch = global_object->_idispatch;
            break;
        }
    }
    else
    {
        if( name_bstr == no_such_method_name )  return result;
        if( name_bstr == "__arguments__"     )  return result;
        if( name_bstr == "toString"          )  return result;

        result._idispatch = static_cast<IDispatch*>( JS_GetPrivate( _jscontext, jsobject ) );
        result._dispid    = _object_register.get_dispid( result._idispatch, name_bstr );
    }


    return result;
}

//---------------------------------------------------------------Spidermonkey::com_property_resolve

JSBool Spidermonkey::com_property_resolve( JSObject* jsobject, jsval id )
{
    JSBool ok = false;

    if( JSVAL_IS_VOID( id )  &&  jsobject == _global_jsobject )     // Für new XML("...")
    {
        ok = true;
    }
    else
    if( JSVAL_IS_STRING( id ) )
    {
        try
        {
            Idispatch_member    idispatch_member;
            const jschar*       name_ptr = JS_GetStringChars( jsstring_from_jsval( id ) );
            Bstr                name_bstr = name_ptr;


            if( jsobject == _global_jsobject ) 
            {
                if( _javascript_standard_global_object_names.find( name_bstr ) != _javascript_standard_global_object_names.end() )  return true;

                Z_LOGI2( "spidermonkey.callback", __FUNCTION__ << " global." << name_bstr << "\n" );

                idispatch_member = get_idispatch_member( jsobject, name_bstr );
                
                if( !idispatch_member )  return true;   // Wird ein Name von Spidermonkey sein.
            }
            else
            {
                if( _javascript_standard_object_names.find( name_bstr ) != _javascript_standard_object_names.end() )  return true;

                Z_LOGI2( "spidermonkey.callback", __FUNCTION__ << " " << jsobject << " " << name_bstr << "\n" );

                IDispatch* idispatch = (IDispatch*)JS_GetPrivate( _jscontext, jsobject );
                if (!idispatch)  throw_xc("SPIDERMONKEY-1");
                idispatch_member._idispatch = idispatch;
                idispatch_member._dispid = _object_register.get_dispid( idispatch_member._idispatch, name_bstr );
            }


            if( idispatch_member )
            {
                // Bei jsobject == _global_jsobject muss die Eigenschaft definiert werden (z.B. für embed() )
                // bei jsobject != _global_jsobject kann das unterbleiben. Dann ruft spidermonkey immer wieder resolve auf.

                ok = JS_DefineUCProperty( _jscontext, jsobject, name_bstr.uint16_ptr(), name_bstr.length(), 
                                          JSVAL_VOID, com_property_get, com_property_set,
                                          JSPROP_ENUMERATE | JSPROP_PERMANENT );
                if( !ok )  throw_spidermonkey( "JS_DefineUCProperty" );
            }

            ok = true;
        }
        catch( const exception&  x )  { set_js_exception( x );  ok = false; }
        catch( const _com_error& x )  { set_js_exception( x );  ok = false; }
    }

    //Z_DEBUG_ONLY( assert( ok ); )
    return ok;
}

//-------------------------------------------------Spidermonkey::get_intermediate_object_parameters

void Spidermonkey::get_intermediate_object_parameters( JSObject* intermediate_jsobject, Dispparams* dispparams, int arg_offset )
{
    // Zwischenobjekt von com_call(): 
    // Die Eigenschaft __arguments__ enthält ein Array mit den Parametern für den Aufruf von DISPID_VALUE

    HRESULT hr;
    JSBool  ok;
    jsuint  n;


    jsval arguments_jsval;
    ok = JS_GetProperty( _jscontext, intermediate_jsobject, "__arguments__", &arguments_jsval );
    if( !ok )  throw_spidermonkey( "JS_SetProperty" );

    if( !JSVAL_IS_OBJECT( arguments_jsval ) )  throw_spidermonkey( "Spidermonkey::get_intermediate_object_parameters" );

    JSObject* arguments_jsobject = jsobject_from_jsval( arguments_jsval );

    ok = JS_GetArrayLength( _jscontext, arguments_jsobject, &n );
    if( !ok )  throw_spidermonkey( "JS_GetArrayLength" );

    dispparams->set_arg_count( arg_offset + n );
    for( uint i = 0; i < n; i++ )  
    {
        jsval value;
        ok = JS_GetElement( _jscontext, arguments_jsobject, i, &value );
        if( !ok )  throw_spidermonkey( "JS_GetElement" );

        hr = Jsval_to_variant( value, &(*dispparams)[ i ] );
        if( FAILED(hr) )  throw_com( hr, "Jsval_to_variant" );
    }
}

//-----------------------------------------------------------------------Spidermonkey::com_property_get

JSBool Spidermonkey::com_property_get( JSObject* jsobject, jsval id, jsval* result )
{
    JSBool ok = false;

    if( JSVAL_IS_VOID( id )  &&  jsobject == _global_jsobject )     // Für new XML("...")
    {
        *result = JSVAL_VOID;
        ok = true;
    }
    else
    if( JSVAL_IS_STRING( id ) )
    {
        try
        {
            Bstr name_bstr = JS_GetStringChars( jsstring_from_jsval( id ) );

            Z_LOGI2( "spidermonkey.callback", __FUNCTION__ << " " << jsobject << " " << name_bstr << "\n" );

            Idispatch_member idispatch_member = get_idispatch_member( jsobject, name_bstr );
            if( !idispatch_member ) 
            {
                return true;
            }

            Dispparams dispparams;
            Excepinfo  excepinfo;
            uint       arg_nr    = (uint)-1;
            HRESULT    hr;

            if( JS_GetClass( jsobject ) == &intermediate_com_jsclass )  
                get_intermediate_object_parameters( jsobject, &dispparams );


            Variant result_variant;
            hr = idispatch_member.Invoke( DISPATCH_PROPERTYGET, &dispparams, &result_variant, &excepinfo, &arg_nr );
            if( FAILED(hr) )
            {
                if( hr != DISP_E_MEMBERNOTFOUND         // Keine Eigenschaft? Dann ist es wohl eine Methode.
                 && hr != DISP_E_BADPARAMCOUNT )        // Eigenschaft mit Parametern? Wie Methode aufrufen
                {
                    throw_com_excepinfo( hr, &excepinfo, "IDispatch::Invoke", string_from_bstr( name_bstr ).c_str() );
                }

                //  Wir ändern result nicht, dann ruft Spidermonkey com_no_such_method_call() auf.
            }
            else
            {
                *result = jsval_from_variant( result_variant );
            }

            ok = true;
        }
        catch( const exception&  x )  { set_js_exception( x );  ok = false; }
        catch( const _com_error& x )  { set_js_exception( x );  ok = false; }
    }

    //Z_DEBUG_ONLY( assert( ok ); )
    return ok;
}

//-------------------------------------------------------------------Spidermonkey::com_property_set

JSBool Spidermonkey::com_property_set( JSObject* jsobject, jsval id, jsval* value )
{
    JSBool ok = false;

    if( jsobject == _global_jsobject )  return true;

    if( JSVAL_IS_STRING( id ) )
    {
        try
        {
            const char* name_ptr  = JS_GetStringBytes( jsstring_from_jsval( id ) );
            if( strcmp( name_ptr, "__arguments__" ) == 0 )  return true;

            HRESULT             hr;
            Bstr                name_bstr;  name_bstr.attach( bstr_from_string( JS_GetStringBytes( jsstring_from_jsval( id ) ) ) );
            Dispparams          dispparams;
            Excepinfo           excepinfo;
            uint                arg_nr    = (uint)-1;
            Idispatch_member    idispatch_member = get_idispatch_member( jsobject, name_bstr );

            Z_LOGI2( "spidermonkey.callback", __FUNCTION__ << " " << jsobject << " " << name_bstr << "\n" );

            if( JS_GetClass( jsobject ) == &intermediate_com_jsclass )  
                get_intermediate_object_parameters( jsobject, &dispparams, 1 );
            else
                dispparams.set_arg_count( 1 );

            dispparams.set_property_put();

            hr = Jsval_to_variant( *value, &dispparams[ dispparams.arg_count() - 1 ] );
            if( FAILED(hr) )  throw_com( hr, "Jsval_to_variant" );


            Variant result;
            hr = idispatch_member._idispatch->Invoke( idispatch_member._dispid, IID_NULL, STANDARD_LCID, DISPATCH_PROPERTYPUT, &dispparams, &result, &excepinfo, &arg_nr );
            if( FAILED(hr) )  throw_com_excepinfo( hr, &excepinfo, "IDispatch::Invoke", string_from_bstr( name_bstr ).c_str() );

            ok = true;
        }
        catch( const exception&  x )  { set_js_exception( x );  ok = false; }
        catch( const _com_error& x )  { set_js_exception( x );  ok = false; }
    }

    //Z_DEBUG_ONLY( assert( ok ); )
    return ok;
}

//-------------------------------------------------------------------Spidermonkey::com_property_add

JSBool Spidermonkey::com_property_add( JSObject* jsobject, jsval id, jsval* )
{
    JSBool ok = false;

    if( JSVAL_IS_VOID( id )  &&  jsobject == _global_jsobject )     // Für new XML("...")
    {
        ok = true;
    }
    else
    if( JSVAL_IS_STRING( id ) )
    {
        try
        {

            if( jsobject == _global_jsobject )
            {
                const jschar* name_ptr = JS_GetStringChars( jsstring_from_jsval( id ) );
                Z_LOG2( "spidermonkey.callback", __FUNCTION__ << " " << jsobject << " " << Bstr(name_ptr) << "\n" );

                #ifdef DONT_USE_HASH_SET
                    _javascript_standard_global_object_names[ name_ptr ] = true;
                #else
                    _javascript_standard_global_object_names.insert( name_ptr );
                #endif
            }

            ok = true;
        }
        catch( const exception&  x )  { set_js_exception( x );  ok = false; }
        catch( const _com_error& x )  { set_js_exception( x );  ok = false; }
    }

    //Z_DEBUG_ONLY( assert( ok ); )
    return ok;
}

//----------------------------------------------------------------Spidermonkey::com_property_delete

JSBool Spidermonkey::com_property_delete( JSObject* jsobject, jsval id, jsval* result )
{
    JSBool ok = false;

    if( JSVAL_IS_STRING( id ) )
    {
        const jschar* name_ptr = JS_GetStringChars( jsstring_from_jsval( id ) );
        Z_LOG2( "spidermonkey.callback", __FUNCTION__ << " " << jsobject << " " << name_ptr << "\n" );

        *result = JSVAL_FALSE;
        ok = true;
    }

    //Z_DEBUG_ONLY( assert( ok ); )
    return ok;
}

//---------------------------------------------------------Spidermonkey::create_intermediate_object

JSObject* Spidermonkey::create_intermediate_object( IDispatch* idispatch, JSObject* arguments_jsobject )
{
    // Zwischenobjekt anlegen, das als Funktionsergebnis (lvalue) zurückgegeben wird.

    JSObject* intermediate_jsobject = NULL;

    if( idispatch )     // 2006-12-20
    {
        JSBool    ok;
        intermediate_jsobject = JS_NewObject( _jscontext, &intermediate_com_jsclass, NULL, NULL );
        if( !intermediate_jsobject  )  throw_xc( "SPIDERMONKEY", "JS_NewObject" );

        ok = JS_SetPrivate( _jscontext, intermediate_jsobject, idispatch );
        if( !ok )  throw_xc( "SPIDERMONKEY", "JS_SetPrivate" );

        idispatch->AddRef();


        // Parameter im Zwischenobjekt ablegen

        jsval arguments_jsval = jsval_from_jsobject( arguments_jsobject );
        ok = JS_SetProperty( _jscontext, intermediate_jsobject, "__arguments__", &arguments_jsval );
        if( !ok )  throw_spidermonkey( "JS_SetProperty" );
    }

    return intermediate_jsobject;
}

//-----------------------------------------------------Spidermonkey::invalidate_intermediate_object

void Spidermonkey::invalidate_intermediate_object(JSObject* intermediate_jsobject, IDispatch* idispatch)
{
    assert(idispatch);
    assert(intermediate_jsobject);
    assert(idispatch == JS_GetPrivate(_jscontext, intermediate_jsobject));

    JS_SetPrivate(_jscontext, intermediate_jsobject, NULL);
    release_idispatch(intermediate_jsobject, idispatch, Z_FUNCTION);
}

//---------------------------------------------------------------------------Spidermonkey::com_call

JSBool Spidermonkey::com_call( JSObject*, uintN argc, jsval* argv, jsval* jsresult )
{
    JSBool ok = true;

    try
    {
        //JSClass* cls = JS_GetClass( global_jsobject );

        Z_LOG2( "spidermonkey.callback", __FUNCTION__ <<  "\n" );

        // com_call() wird gerufen, wenn ein Objekt wie eine Funktion gerufen wird:
        // set v = spooler.create_variable_set()
        // v.set_var( "variable", "wert" )
        // a = v("variable") <-- Hier sind wir
        //    global_jsobject ist das globale Objekt: v("variable") ist ungefähr das gleiche wie global.v("variable")
        //    Unser Objekt v ist in argv[-2], s. js_Call() in jsobj.c.
        // Vorher hat com_convert true für JSTYPE_FUNCTION geliefert.

        JSObject* arguments_jsobject = JS_NewArrayObject( _jscontext, argc, argv );
        if( !arguments_jsobject )  throw_xc( "SPIDERMONKEY", "JS_NewArrayObject" );

        *jsresult = jsval_from_jsobject( create_intermediate_object( (IDispatch*)JS_GetPrivate( _jscontext, jsobject_from_jsval( argv[ -2 ] ) ), arguments_jsobject ) );

        JS_SetCallReturnValue2( _jscontext, JS_GetEmptyStringValue( _jscontext ) );  // Name der Eigenschaft "" zurückgeben
    }
    catch( const exception&  x ) { set_js_exception( x );  ok = false; }
    catch( const _com_error& x ) { set_js_exception( x );  ok = false; }

    return ok;
}

//------------------------------------------------------------Spidermonkey::com_no_such_method_call

JSBool Spidermonkey::com_no_such_method_call( JSContext* cx, JSObject* jsobject, uintN argc, jsval* argv, jsval* jsresult )
{
    JSBool ok = true;

    try
    {
        if( argc != 2 )                    throw_spidermonkey( "com_no_such_method_call: argc != 2" );
        if( !JSVAL_IS_STRING( argv[0] ) )  throw_spidermonkey( "com_no_such_method_call: 1. Parameter ist kein String" );
        if( !JSVAL_IS_OBJECT( argv[1] ) )  throw_spidermonkey( "com_no_such_method_call: 2. Parameter ist kein Array" );


        HRESULT     hr;
        Bstr        name_bstr;
        Dispparams  dispparams;
        Excepinfo   excepinfo;
        uint        arg_nr      = (uint)-1;
        jsuint      array_length;
        JSObject*   args        = jsobject_from_jsval( argv[1] );

        name_bstr.attach( bstr_from_string( JS_GetStringBytes( jsstring_from_jsval(argv[0] ) ) ) );

        Idispatch_member idispatch_member = get_idispatch_member( jsobject, name_bstr );

        Z_LOGI2( "spidermonkey.callback", __FUNCTION__ << "  " << name_bstr << "\n" );

        if( *cx->fp->down->pc == JSOP_SETCALL )     // Funktions- oder Methodenaufruf auf linker Seite obj.name(a) = ...
        {
            // Wird interpretiert als Setzen einer parametrisierten COM-Eigenschaft.
            // Dazu merken wir uns die Parameter und liefern ..........

            *jsresult = jsval_from_jsobject( create_intermediate_object( (IDispatch*)JS_GetPrivate( _jscontext, jsobject ), args ) );

            JS_SetCallReturnValue2( _jscontext, jsval_from_bstr( name_bstr ) );  // Name der Eigenschaft zurückgeben
        }
        else
        {
            ok = JS_GetArrayLength( _jscontext, args, &array_length );
            if( !ok )  throw_xc( "JS_GetArrayLength" );

            dispparams.set_arg_count( array_length );

            for( jsuint i = 0; i < array_length; i++ )
            {
                jsval value;
                ok = JS_GetElement( _jscontext, args, i, &value );
                if( !ok )  throw_xc( "JS_GetElement" );

                /*
                if( JSVAL_IS_OBJECT( value ) )
                {
                    // Nicht in Jsval_to_variant() konvertieren, damit wir SetNamedRoot() nicht rufen müssen.

                    ptr<Jsobject_idispatch> jsobject_idispatch = Z_NEW( Jsobject_idispatch( this, value ) );                
                    dispparams[i] = +jsobject_idispatch;

                    Invoke "toString" soll zu Invoke(DISPID_VALUE) führen
                }
                else
                */
                {
                    hr = Jsval_to_variant( value, &dispparams[i] );
                    if( FAILED(hr) )  throw_com( hr, "Jsval_to_variant" );
                }
            }

            Variant result;
            hr = idispatch_member.Invoke( DISPATCH_METHOD | DISPATCH_PROPERTYGET, &dispparams, &result, &excepinfo, &arg_nr );
            if( FAILED(hr) )  throw_com_excepinfo( hr, &excepinfo, "IDispatch::Invoke", string_from_bstr( name_bstr ).c_str() );

            if( name_bstr == "toString" )
            {
                *jsresult = jsval_from_string( string_from_variant( result ) );
            }
            else
            {
                *jsresult = jsval_from_variant( result );
            }
        }
    }
    catch( const exception&  x ) { set_js_exception( x );  ok = false; }
    catch( const _com_error& x ) { set_js_exception( x );  ok = false; }

    return ok;
}

//-----------------------------------------------------------------------Spidermonkey::com_toString

JSBool Spidermonkey::com_toString( JSObject* jsobject, uintN, jsval*, jsval* jsresult )
{
    Z_LOGI2( "spidermonkey.callback", __FUNCTION__ << "\n" );

    HRESULT hr;
    JSBool  ok = true;

    try
    {
        IDispatch*  idispatch   = (IDispatch*)JS_GetPrivate( _jscontext, jsobject );
        Dispparams  dispparams;
        Excepinfo   excepinfo;
        uint        arg_nr      = (uint)-1;

        Variant result;
        hr = idispatch->Invoke( DISPID_VALUE, IID_NULL, STANDARD_LCID, DISPATCH_PROPERTYGET, &dispparams, &result, &excepinfo, &arg_nr );
        if( FAILED(hr) )  throw_com_excepinfo( hr, &excepinfo, "IDispatch::Invoke", "DISPID_VALUE" );

        *jsresult = jsval_from_string( string_from_variant( result ) );
    }
    catch( const exception&  x ) { set_js_exception( x );  ok = false; }
    catch( const _com_error& x ) { set_js_exception( x );  ok = false; }

    return ok;
}

//-------------------------------------------------------------------Spidermonkey::Jsval_to_variant

HRESULT Spidermonkey::Jsval_to_variant( jsval js_value, VARIANT* result )
{
    HRESULT hr = VariantClear( result );
    if( FAILED(hr) )  return hr;


    if( JSVAL_IS_STRING( js_value ) )
    {
        JSString* js_string = jsstring_from_jsval( js_value );
        
        result->vt = VT_BSTR;
        V_BSTR( result ) = SysAllocStringLen( olechar_from_jschar( JS_GetStringChars( js_string ) ), (uint)JS_GetStringLength( js_string ) );
    }
    else
    if( JSVAL_IS_BOOLEAN( js_value ) )
    {
        result->vt = VT_BOOL;
        V_BOOL( result ) = static_cast<VARIANT_BOOL>( JSVAL_TO_BOOLEAN( js_value ) );
    }
    else
    if( JSVAL_IS_INT( js_value ) )
    {
        result->vt = VT_I4;
        V_I4( result ) = JSVAL_TO_INT( js_value );
    }
    else
    if( JSVAL_IS_DOUBLE( js_value ) )
    {
        result->vt = VT_R8;
        V_R8( result ) = *JSVAL_TO_DOUBLE( (size_t)js_value );
    }
    else
    if( JSVAL_IS_NULL( js_value ) )
    {
        result->vt = VT_DISPATCH;
        V_DISPATCH( result ) = NULL;
    }
    else
    if( JSVAL_IS_VOID( js_value ) )
    {
        result->vt = VT_EMPTY;
    }
    else
    if( JSVAL_IS_OBJECT( js_value ) )
    {
        JSObject* date_jsobject = jsobject_from_jsval( js_value );

        if( strcmp( JS_GetClass( date_jsobject )->name, "Date" ) == 0 )
        {
            try
            {
                double days = 0;
                int64  msec = (int64)js_DateGetMsecSinceEpoch( _jscontext, date_jsobject );     // GMT


                // Zeitzone berücksichtigen (VT_DATE ist lokale Zeit)

                jsval jsresult;
                int ok = JS_CallFunctionName( _jscontext, date_jsobject, "toString", 0, NULL, &jsresult );
                if( !ok )  throw_spidermonkey( "Date.toString()" );

                string date_string = string_from_bstr( Bstr( JS_GetStringChars( jsstring_from_jsval( jsresult ) ) ) );
                size_t z = date_string.find( '-' );
                if( z == string::npos )  z = date_string.find( '+' );
                if( z != string::npos )
                {
                    int zone = as_int( date_string.substr( z + 1, 2 ) ) * 60 + 
                               as_int( date_string.substr( z + 3, 2 ) );
                    if( date_string[ z ] == '-' )  msec -= zone * 60*1000;
                                             else  msec += zone * 60*1000;
                }


                if( msec > 0 )
                {
                    //msec -= (int64)timezone*1000;

                    days = (double)msec / ( 24*3600 * 1000 );

                    days += floor( 70*365.25 );         // -70 Jahre
                    days += 2;                          // -2 Tage
                }

                result->vt = VT_DATE;
                V_DATE( result ) = days;
            }
            catch( exception& )  
            { 
                hr = DISP_E_TYPEMISMATCH; 
            }
        }
        else
        {
            result->vt = VT_DISPATCH;
            V_DISPATCH( result ) = idispatch_from_jsobject( date_jsobject ).take();
        }
    }
    else
        hr = DISP_E_TYPEMISMATCH;

    return hr;
}

//-------------------------------------------------------------------Spidermonkey::Variant_to_jsval

HRESULT Spidermonkey::Variant_to_jsval( const VARIANT& variant, jsval* result )
{
    HRESULT hr = S_OK;

    try
    {
        *result = jsval_from_variant( variant );
    }
    catch( const exception&  x )  { hr = Com_set_error( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = Com_set_error( x, __FUNCTION__ ); }

    return hr;
}

//---------------------------------------------------------------Spidermonkey::jsstring_from_string

JSString* Spidermonkey::jsstring_from_string( const string& str )
{
    return JS_NewStringCopyN( _jscontext, str.data(), str.length() );
}

//------------------------------------------------------------------Spidermonkey::jsval_from_string

jsval Spidermonkey::jsval_from_string( const string& str )
{
    return jsval_from_jsstring( JS_NewStringCopyN( _jscontext, str.data(), str.length() ) );
}

//--------------------------------------------------------------------Spidermonkey::jsval_from_bstr

jsval Spidermonkey::jsval_from_bstr( const BSTR& bstr )
{
    return jsval_from_jsstring( JS_NewUCStringCopyN( _jscontext, jschar_from_olechar( bstr ), SysStringLen( bstr ) ) );
}

//----------------------------------------------------------------------Spidermonkey::make_jsdouble

jsval Spidermonkey::jsval_from_double( double d )
{
    jsval result;
    
    JSBool ok = JS_NewDoubleValue( _jscontext, d, &result );
    if( !ok )  throw_spidermonkey( "JS_NewDoubleValue" );

    return result;
}

//----------------------------------------------------------------Spidermonkey::jsstring_from_jsval

JSString* Spidermonkey::jsstring_from_jsval( jsval v )
{ 
    if( JSVAL_IS_STRING( v ) )
    {
        return JSVAL_TO_STRING( (size_t)v );    // (size_t) gegen Warnung
    }
    else
    //if( JSVAL_IS_NULL( v ) )
    //{
    //    return jsstring_from_string( "" );
    //}
    //else
    //if( JSVAL_IS_VOID( v ) )
    //{
    //    return jsstring_from_string( "" );
    //}
    //else
    {
        // Eine von diesen?
        //#define JSVAL_IS_OBJECT(v)      (JSVAL_TAG(v) == JSVAL_OBJECT)
        //#define JSVAL_IS_NUMBER(v)      (JSVAL_IS_INT(v) || JSVAL_IS_DOUBLE(v))
        //#define JSVAL_IS_INT(v)         (((v) & JSVAL_INT) && (v) != JSVAL_VOID)
        //#define JSVAL_IS_DOUBLE(v)      (JSVAL_TAG(v) == JSVAL_DOUBLE)
        //#define JSVAL_IS_STRING(v)      (JSVAL_TAG(v) == JSVAL_STRING)
        //#define JSVAL_IS_BOOLEAN(v)     (JSVAL_TAG(v) == JSVAL_BOOLEAN)
        //#define JSVAL_IS_NULL(v)        ((v) == JSVAL_NULL)
        //#define JSVAL_IS_VOID(v)        ((v) == JSVAL_VOID)
        //#define JSVAL_IS_PRIMITIVE(v)   (!JSVAL_IS_OBJECT(v) || JSVAL_IS_NULL(v))

        assert(!"JSVAL_IS_STRING");
        throw_xc( Z_FUNCTION, (int)v );
    }
}  

//-------------------------------------------------------------------Spidermonkey::Variant_to_jsval

jsval Spidermonkey::jsval_from_variant( const VARIANT& variant )
{
    JSBool      ok;
    jsval       result;
    const void* ptr     = V_VT( &variant ) & VT_BYREF? variant.pintVal : &variant.intVal;

    switch( V_VT( &variant ) & ~VT_BYREF ) 
    {
        case VT_EMPTY:          result = JSVAL_VOID;  break;
        case VT_NULL:           result = JSVAL_NULL;  break;      // Aber JSVAL_NULL --> (IDispatch*)NULL
        
        case VT_I1:             result = INT_TO_JSVAL( (int)*( int8* )ptr );  break;
        case VT_UI1:            result = INT_TO_JSVAL( (int)*(uint8* )ptr );  break;
        
        case VT_I2:             result = INT_TO_JSVAL( (int)*( int16*)ptr );  break;
        case VT_UI2:            result = INT_TO_JSVAL( (int)*(uint16*)ptr );  break;
        
        case VT_INT:
        case VT_I4:             {
                                    int32 i = *(int32*)ptr; 
                                    result = INT_FITS_IN_JSVAL( i )? INT_TO_JSVAL( i ) 
                                                                   : jsval_from_double( (double)i );  
                                    break; 
                                }
        case VT_UINT:
        case VT_UI4:            {
                                    uint32 i = *(uint32*)ptr; 
                                    result = INT_FITS_IN_JSVAL( i )? INT_TO_JSVAL( i ) 
                                                                   : jsval_from_double( (double)i );  
                                    break; 
                                }
        
        case VT_I8:             result = jsval_from_double( (double)*( int64*)ptr ); break;
        case VT_UI8:            result = jsval_from_double( (double)*(uint64*)ptr ); break;
        
        case VT_R4:             result = jsval_from_double( (double)*(float* )ptr ); break;
        case VT_R8:             result = jsval_from_double( (double)*(double*)ptr ); break;

      //case VT_CY:             

        case VT_DATE:
        {
            double days = *(double*)ptr;        // Tage seit 30. Dezember 1899

            days -= floor( 70*365.25 );         // -70 Jahre
            days -= 2;                          // -2 Tage

            int64 msec = (int64)( days * 24*3600*1000 ) + (int64)timezone*1000;

            JSObject* date_jsobject = js_NewDateObjectMsec( _jscontext, 0 );        // Setzt GMT
            if( !date_jsobject )  throw_spidermonkey( "JS_NewObject Date" );

            char text[50];
            z_snprintf( text, sizeof text, "%" Z_PRINTF_INT64, msec );
            jsval argv[] = { jsval_from_jsstring( JS_NewStringCopyZ( _jscontext, text ) ) };

            jsval jsresult;
            ok = JS_CallFunctionName( _jscontext, date_jsobject, "setMilliseconds", NO_OF( argv ), argv, &jsresult );   // Setzt lokale Zeit
            if( !ok )  throw_spidermonkey( "Date.setMilliseconds()" );

            result = jsval_from_jsobject( date_jsobject );  break;
            break;
        }

        case VT_BSTR:           result = jsval_from_bstr( *(BSTR*)ptr );  break;
        case VT_DISPATCH:       result = jsval_from_jsobject( jsobject_from_idispatch( *(IDispatch**)ptr ) );  break;
      //case VT_ERROR:          
        case VT_BOOL:           result = BOOLEAN_TO_JSVAL( *(VARIANT_BOOL*)ptr? true : false );  break;
      //case VT_VARIANT:
        case VT_UNKNOWN:        result = jsval_from_jsobject( _object_register.jsobject_from_iunknown( *(IUnknown**)ptr ) );  break;
      //case VT_DECIMAL:
      //case VT_VOID:
      //case VT_HRESULT:
      //case VT_PTR:
      //case VT_SAFEARRAY:
      //case VT_CARRAY:
      //case VT_USERDEFINED:
      //case VT_LPSTR:
      //case VT_LPWSTR:
      //case VT_FILETIME:
      //case VT_BLOB:
      //case VT_STREAM:
      //case VT_STORAGE:
      //case VT_STREAMED_OBJECT:
      //case VT_STORED_OBJECT:
      //case VT_BLOB_OBJECT:
      //case VT_CF:
      //case VT_CLSID:

        default:
        {
            string str = string_from_variant( variant );
            result = jsval_from_string( str );
        }
    }

    return result;
}

//---------------------------------------------------------------------create_perl_scripting_engine
/*
HRESULT create_perl_scripting_engine( const CLSID& clsid, const IID& iid, IUnknown** result )
{
    if( clsid != CLSID_PerlScript )  return CLASS_E_CLASSNOTAVAILABLE;

    ptr<Spidermonkey> engine = new Spidermonkey;
    return engine->QueryInterface( iid, (void**)result );
}
*/
//---------------------------------------------------------Spidermonkey::global_object_property_get

JSBool Spidermonkey::global_object_property_get( JSObject* jsobject, jsval id, jsval* result )
{
    JSBool ok = false;

    try
    {
        if( !JSVAL_IS_STRING( id ) )  throw_xc( "global_object_property_get" );

        HRESULT         hr;
        ptr<ITypeInfo>  itypeinfo;
        Bstr            name_bstr = JS_GetStringChars( jsstring_from_jsval( id ) );


        Z_LOGI2( "spidermonkey.callback", __FUNCTION__ << " " << jsobject << " " << name_bstr << "\n" );

        Added_items::iterator it = _added_items.find( name_bstr );
        if( it != _added_items.end() )
        {
            ptr<IUnknown>   iunknown;

            hr = _site->GetItemInfo( name_bstr, SCRIPTINFO_IUNKNOWN, iunknown.pp(), itypeinfo.pp() );
            if( FAILED(hr) )  throw_com( hr, "IActiveScriptSite::GetItemInfo" );

            it->second = qi_ptr<IDispatch>( iunknown );

            *result = jsval_from_jsobject( jsobject_from_idispatch( it->second ) );
        }

        ok = true;
    }
    catch( const exception&  x )  { set_js_exception( x );  ok = false; }
    catch( const _com_error& x )  { set_js_exception( x );  ok = false; }

    return ok;
}

//-----------------------------------------------------------Spidermonkey::activeXObject_jsfunction

JSBool Spidermonkey::activeXObject_jsfunction( JSObject*, uintN argc, jsval* argv, jsval* result )
{
    JSBool ok = true;

    try
    {
        if( argc != 1 )                     throw_spidermonkey( "activeXObject_jsfunction():  argc != 1" );
        if( !JSVAL_IS_STRING( argv[0] ) )   throw_spidermonkey( "1. Parameter ist nicht String" );

        string com_class_name = JS_GetStringBytes( jsstring_from_jsval( argv[0] ) );

        ptr<IDispatch> idispatch;
        idispatch.create_instance( com_class_name );

        *result = jsval_from_jsobject( jsobject_from_idispatch( idispatch ) );
    }
    catch( const exception&  x )  { set_js_exception( x );  ok = false; }
    catch( const _com_error& x )  { set_js_exception( x );  ok = false; }

    return ok;
}

//------------------------------------------------------------------------Spidermonkey::com_convert

JSBool Spidermonkey::com_convert( JSObject*, JSType jstype, jsval* )
{
    JSBool ok;
    
    try
    {
        switch( jstype )
        {
            case JSTYPE_VOID:
                ok = true;
                break;

            case JSTYPE_FUNCTION:
            {
                /*
                //JSObject* function_jsobject = JS_NewObject( _jscontext, &js_FunctionClass, NULL, NULL );
                JSFunction* jsfunction = JS_DefineFunction( _jscontext, NULL, "_dispid_value", com_call, 0, 0 );
                if( !jsfunction )  throw_spidermonkey( "JS_DefineFunction" );

                //ok = JS_ConvertValue( _jscontext, jsval_from_jsobject( jsfunction ), JSTYPE_FUNCTION, jsresult );
                //if( !ok )  throw_spidermonkey( "JS_ConvertValue" );
                *jsresult = jsval_from_jsobject( jsfunction );
                */

                ok = true;
                break;
            }

            default: 
                throw_xc( "Spidermonkey::com_convert", string_from_jstype( jstype ) );
        }

    }
    catch( const exception&  x )  { set_js_exception( x );  ok = false; }
    catch( const _com_error& x )  { set_js_exception( x );  ok = false; }

    return ok;
}

//---------------------------------------------------------------------Spidermonkey::QueryInterface

STDMETHODIMP Spidermonkey::QueryInterface( const IID& iid, void** result )
{ 
    Z_IMPLEMENT_QUERY_INTERFACE2( this, iid, IID_IActiveScript     , IActiveScript     , result );
    Z_IMPLEMENT_QUERY_INTERFACE2( this, iid, IID_IActiveScriptParse, IActiveScriptParse, result );

    return Iunknown_implementation::QueryInterface( iid, result ); 
}

//----------------------------------------------------------------------Spidermonkey::SetScriptSite

HRESULT Spidermonkey::SetScriptSite( IActiveScriptSite* site )
{ 
    HRESULT hr = S_OK;

    try
    {
        _site = site; 
        //_perl->parse( "$" Z_PERL_IDISPATCH_PACKAGE_NAME "::site=" + string_from_jsobject_idispatch((int)(IActiveScriptSite*)_site) + ";" );
    }
    catch( const exception&  x ) { hr = Com_set_error( x, __FUNCTION__ ); }
    catch( const _com_error& x ) { hr = Com_set_error( x, __FUNCTION__ ); }

    return hr;
}

//---------------------------------------------------------------------Spidermonkey::SetScriptState

HRESULT Spidermonkey::SetScriptState( SCRIPTSTATE state )
{
    Z_COM_MY_THREAD_ONLY;
    if( _script_state == state )  return S_FALSE;

    HRESULT hr = E_UNEXPECTED;

    try
    {
        switch( state )
        {
            case SCRIPTSTATE_UNINITIALIZED:
                //hr = Close();
                break;

            case SCRIPTSTATE_INITIALIZED:
                if( _script_state == SCRIPTSTATE_UNINITIALIZED )
                {
                    //initialize();
                    hr = S_OK;
                }
                break;

            case SCRIPTSTATE_STARTED:
                if( _script_state == SCRIPTSTATE_INITIALIZED
                 || _script_state == SCRIPTSTATE_UNINITIALIZED )
                {
                    start();
                    hr = S_OK;
                }
                break;

            case SCRIPTSTATE_CONNECTED:
                break;

            case SCRIPTSTATE_DISCONNECTED:
                break;

            case SCRIPTSTATE_CLOSED:
                hr = Close();
                break;

            default: ;
        }

        if( hr == S_OK )  _script_state = state;
    }
    catch( const exception&  x ) { hr = Com_set_error( x, __FUNCTION__ ); }
    catch( const _com_error& x ) { hr = Com_set_error( x, __FUNCTION__ ); }

    return hr;
}

//-------------------------------------------------------------------------Spidermonkey::initialize

void Spidermonkey::initialize()
{
    JSBool ok;


    if( !_js_runtime )
    {
        _js_runtime = JS_NewRuntime( size_before_garbage_collection );
        if( !_js_runtime )  throw_spidermonkey( "JS_NewRuntime" );
    }

    if( !_jscontext )
    {
        _jscontext = JS_NewContext( _js_runtime, context_stack_size );
        if( !_jscontext )  throw_spidermonkey( "JS_NewContext" );

        JS_SetVersion( _jscontext, JSVERSION_1_7 );
        
        JSVersion version = JS_GetVersion( _jscontext );
        Z_LOG2( "spidermonkey", VER_PRODUCTVERSION_STR ", Spidermonkey JavaScript " << JS_GetImplementationVersion() << ", JS_GetVersion() ==> " << version << "\n" );

        JS_SetErrorReporter ( _jscontext, error_reporter );
        JS_SetContextPrivate( _jscontext, this );
      //JS_ToggleOptions    ( _jscontext, JSOPTION_STRICT );
    }

    if( !_global_jsobject )
    {
        _global_jsobject = JS_NewObject(_jscontext, &com_jsclass, NULL, NULL); 
        if( !_global_jsobject )  throw_spidermonkey( "JS_NewObject(global_jsclass)" );

        JSFunction* js_no_such_method = JS_DefineFunction( _jscontext, _global_jsobject, no_such_method_name, com_no_such_method_call_, 2, 0 );
        if( !js_no_such_method )  throw_spidermonkey( "JS_DefineFunction" );

        ok = JS_InitStandardClasses( _jscontext, _global_jsobject ); 
        if( !ok )  throw_spidermonkey( "JS_InitStandardClasses" );
    }



    // Funktion, die ein COM-Objekt anlegt
    _activexobject_jsfunction = JS_DefineFunction( _jscontext, _global_jsobject, "ActiveXObject", activeXObject_jsfunction, 1, 0 );
    if( !_activexobject_jsfunction )  throw_spidermonkey( "JS_DefineFunction" );


    // Funktion, die eine Java-Klasse lädt, sodass sie in Javascript bekannt ist
    _declare_java_package_jsfunction = JS_DefineFunction( _jscontext, _global_jsobject, "declare_java_package", declare_java_package_jsfunction, 1, 0 );
    if( !_declare_java_package_jsfunction )  throw_spidermonkey( "JS_DefineFunction" );


  //_create_java_object_jsfunction = JS_DefineFunction( _jscontext, _global_jsobject, "create_java_object", create_java_object_jsfunction, 1, 0 );
  //if( !_create_java_object_jsfunction )  throw_spidermonkey( "JS_DefineFunction" );



    // LiveConnect Java-Schnittstelle starten:

    JSJ_Init( &jsj_callbacks );

    ok = JSJ_InitJSContext( _jscontext, _global_jsobject, my_java_packages );
    if( !ok )  throw_xc( "JSJ_InitJSContext" );
}

//------------------------------------------------------------------------------Spidermonkey::start

void Spidermonkey::start()
{
    JSBool ok;


    Z_FOR_EACH( vector<JSScript*>, _js_scripts, s )
    {
        JSScript* js_script = *s;
        
        if( js_script )
        {
            jsval jsresult;
            ok = JS_ExecuteScript( _jscontext, _global_jsobject, js_script, &jsresult );
            if( !ok )  throw_spidermonkey( "JS_ExecuteScript" );
        }

        *s = NULL;
    }

    release_js_scripts();
}


//-----------------------------------------------------------------Spidermonkey::release_js_scripts

void Spidermonkey::release_js_scripts()
{
    if( _jscontext )
    {
        Z_FOR_EACH( vector<JSScript*>, _js_scripts, s )
        {
            if( *s )  JS_DestroyScript( _jscontext, *s ),  *s = NULL;
        }
    }
    else
    {
        if( _js_scripts.size() > 0 )  Z_LOG( "*** ERROR in " << __FUNCTION__ << ": _jscontext ist schon freigegeben ***\n" );
    }

    _js_scripts.clear();
}

//------------------------------------------------------------------------------Spidermonkey::Close

HRESULT Spidermonkey::Close()
{
    Z_COM_MY_THREAD_ONLY;

    HRESULT hr = S_OK;

    try
    {
        if( _jscontext )
        {
            JS_GC( _jscontext );        // Garbage Collector, damit auch wirklich alle finalizer aufgerufen werden.
                                        // Wichtig für JSJ_DisconnectFromJavaVM()!

            release_js_scripts();

            if( _java_vm )
            {
                javabridge::Env java_env ( _java_vm );

                Z_FOR_EACH( vector<jclass>, _java_classes, it )
                {
                    java_env->DeleteGlobalRef( *it );                             
                    *it = NULL;
                }
            }

            if( _jsj_java_vm )  JSJ_DisconnectFromJavaVM( _jsj_java_vm ),  _jsj_java_vm = NULL;


            JS_DestroyContext( _jscontext );   // Ruft auch die finalizer auf
            _jscontext = NULL;

            _object_register.clear();
        }

        if( _js_runtime )
        {
            JS_DestroyRuntime( _js_runtime );
            _js_runtime = NULL;
        }
    }
    catch( const exception&  x ) { hr = Com_set_error( x, __FUNCTION__ ); }
    catch( const _com_error& x ) { hr = Com_set_error( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------------------------------------Spidermonkey::AddNamedItem

HRESULT Spidermonkey::AddNamedItem( LPCOLESTR name_w, DWORD flags )
{
    Z_COM_MY_THREAD_ONLY;

    HRESULT hr = NOERROR;
    JSBool  ok;

    try
    {
        if( ( flags & ~SCRIPTITEM_GLOBALMEMBERS ) != SCRIPTITEM_ISVISIBLE )  return E_INVALIDARG;

        if( flags & SCRIPTITEM_GLOBALMEMBERS )
        {
            if( _global_members.size() > 0 )  return E_INVALIDARG;  // Kann ausgebaut werden.

            _global_members.push_back( Z_NEW( Idispatch_dispids( NULL, name_w ) ) );
        }

        _added_items[ name_w ] = NULL;

        string name = string_from_ole( name_w );

        ok = JS_DefineUCProperty( _jscontext, _global_jsobject, jschar_from_olechar( name_w ), wcslen( name_w ), 
                                  JSVAL_VOID, global_object_property_get, JS_PropertyStub,
                                  JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY );
        if( !ok )  throw_spidermonkey( "JS_DefineUCProperty" );//, string_from_ole(name_w).c_str() );
    }
    catch( const exception&  x ) { hr = Com_set_error( x, __FUNCTION__ ); }
    catch( const _com_error& x ) { hr = Com_set_error( x, __FUNCTION__ ); }

    return hr;
}

//----------------------------------------------------------------------------Spidermonkey::InitNew

HRESULT Spidermonkey::InitNew()
{
    Z_COM_MY_THREAD_ONLY;

    HRESULT hr = NOERROR;

    try
    {
        initialize();
    }
    catch( const exception&  x ) { hr = Com_set_error( x, __FUNCTION__ ); }
    catch( const _com_error& x ) { hr = Com_set_error( x, __FUNCTION__ ); }

    return hr;
}

//--------------------------------------------------------------------Spidermonkey::ParseScriptText

HRESULT Spidermonkey::ParseScriptText( LPCOLESTR script_olestr,
                                       LPCOLESTR name_olestr,
                                       IUnknown* ,//context,
                                       LPCOLESTR delimiter_olestr,
                                       DWORD     ,//source_context_cookie,
                                       ULONG     line_number,
                                       DWORD     flags,
                                       VARIANT*  result,
                                       EXCEPINFO*excepinfo )
{
    //fprintf( stderr, "Spidermonkey::ParseScriptText\n" );
    Z_COM_MY_THREAD_ONLY;
    if( delimiter_olestr )                                                  return E_INVALIDARG;
    if( !( flags & ( SCRIPTTEXT_ISEXPRESSION | SCRIPTTEXT_ISVISIBLE ) ) )   return E_INVALIDARG;
    if( flags & ~( SCRIPTTEXT_ISEXPRESSION | SCRIPTTEXT_ISVISIBLE | SCRIPTTEXT_HOSTMANAGESSOURCE ) )   return E_INVALIDARG;  //? csript.exe setzt SCRIPTTEXT_HOSTMANAGESSOURCE.
    if( name_olestr )                                                       return E_INVALIDARG;


    HRESULT hr = NOERROR;
    JSBool  ok;

    try
    {
        if( !_jscontext )  return E_UNEXPECTED;

        if( result )  VariantInit( result );

        _script_number++;

        string script_name = S() << "Script #" << _script_number;


        if( _script_state == SCRIPTSTATE_STARTED   ||  
            _script_state == SCRIPTSTATE_CONNECTED ||
            flags & SCRIPTTEXT_ISEXPRESSION           )   // //2006-02-23  
        {
            jsval jsresult;
            ok = JS_EvaluateUCScript( _jscontext, _global_jsobject, jschar_from_olechar( script_olestr ), (uintN)wcslen( script_olestr ),
                                      script_name.c_str(), line_number, &jsresult ); 
            if( !ok )  throw_spidermonkey( "JS_EvaluateUCScript" );

            if( result )  hr = Jsval_to_variant( jsresult, result );
        }
        else
        {
            //2006-02-23  if( flags & SCRIPTTEXT_ISEXPRESSION )  return E_UNEXPECTED;

            JSScript* js_script = JS_CompileUCScript( _jscontext, _global_jsobject, jschar_from_olechar( script_olestr ), wcslen( script_olestr ), 
                                                      script_name.c_str(), line_number );
            if( !js_script )  throw_spidermonkey( "JS_CompileUCScript" );

            _js_scripts.push_back( js_script );
        }
    }
    catch( const exception&  x ) 
    { 
        if( excepinfo )  
        {
            excepinfo->bstrDescription = bstr_from_string( x.what() );       
            excepinfo->bstrSource      = bstr_from_string( "javascript" );
        }

        hr = DISP_E_EXCEPTION; 
    }
    catch( const _com_error& x ) 
    { 
        if( excepinfo )  
        {
            excepinfo->scode           = x.Error();
            excepinfo->bstrDescription = SysAllocString( x.Description() );  
            excepinfo->bstrSource      = bstr_from_string( "javascript" );
        }

        hr = DISP_E_EXCEPTION; 
    }

    return hr;
}

//------------------------------------------------------------------Spidermonkey::GetScriptDispatch

HRESULT Spidermonkey::GetScriptDispatch( LPCOLESTR name_w, IDispatch** result )
{
    Z_COM_MY_THREAD_ONLY;
    if( name_w  &&  name_w[0] )  return E_INVALIDARG;

    HRESULT hr = S_OK;

    try
    {
        *result = idispatch_from_jsobject( _global_jsobject ).take();
    }
    catch( const exception&  x ) { hr = Com_set_error( x, __FUNCTION__ ); }
    catch( const _com_error& x ) { hr = Com_set_error( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------Java_liveconnect_interface::map_jsj_thread_to_js_context
/* This callback is invoked when there is no JavaScript execution
   environment (JSContext) associated with the current Java thread and
   a call is made from Java into JavaScript.  (A JSContext is associated
   with a Java thread by calling the JSJ_SetDefaultJSContextForJavaThread()
   function.)  This callback is only invoked when Java spontaneously calls
   into JavaScript, i.e. it is not called when JS calls into Java which
   calls back into JS.
    
   This callback can be used to create a JSContext lazily, or obtain
   one from a pool of available JSContexts.  The implementation of this
   callback can call JSJ_SetDefaultJSContextForJavaThread() to avoid any
   further callbacks of this type for this Java thread. */

JSContext* Java_liveconnect_interface::map_jsj_thread_to_js_context( JSJavaThreadState*, JNIEnv*, char** errp )
{
    *errp = strdup( "map_jsj_thread_to_js_context() ist nicht implementiert" );
    return NULL;
}

//-------------------------------------------------Spidermonkey::java__map_js_context_to_jsj_thread
/* This callback is invoked whenever a call is made into Java from
   JavaScript.  It's responsible for mapping from a JavaScript execution
   environment (JSContext) to a Java thread.  (A JavaContext can only
   be associated with one Java thread at a time.) */

JSJavaThreadState* Spidermonkey::java__map_js_context_to_jsj_thread( char** error_msg )
{
    JSBool  ok;
    JNIEnv* jenv;

    *error_msg = NULL;

    if( !_jsj_thread_state )
    {
        try
        {
            if( !_jsj_java_vm )
            {
                _java_vm = javabridge::Vm::get_vm();

                _jsj_java_vm = JSJ_ConnectToJavaVM( _java_vm->vm(), NULL );  // classpath
                if( !_jsj_java_vm )   throw_xc( "JSJ_ConnectToJavaVM" );
            }

            _jsj_thread_state = JSJ_AttachCurrentThreadToJava( _jsj_java_vm, "main thread", &jenv );
            if( !_jsj_thread_state  )  throw_spidermonkey( "JSJ_AttachCurrentThreadToJava");

            JSJ_SetDefaultJSContextForJavaThread( _jscontext, _jsj_thread_state );
        }
        catch( const exception&  x )  { set_js_exception( x );  ok = false; }
        catch( const _com_error& x )  { set_js_exception( x );  ok = false; }
    }

    return _jsj_thread_state;
}

//---------------------------------------------------------Spidermonkey::declare_java_package_jsfunction

JSBool Spidermonkey::declare_java_package_jsfunction( JSObject*, uintN argc, jsval* argv, jsval* result )
{
    JSBool ok = true;

    try
    {
        if( argc != 1 )                     throw_spidermonkey( "declare_java_package_jsfunction():  argc != 1" );
        if( !JSVAL_IS_STRING( argv[0] ) )   throw_spidermonkey( "1. Parameter ist nicht String" );

        if( !_java_vm )  _java_vm = javabridge::Vm::get_vm();


        string      package_name = JS_GetStringBytes( jsstring_from_jsval( argv[0] ) );
      //javabridge::Env   java_env     = javabridge::Env( _java_vm );

        JavaPackageDef java_packages[] = 
        { 
            { package_name.c_str(), NULL, PKG_USER },       // Variable package_name halten!
            {}
        };


        ok = JSJ_pre_define_java_packages( _jscontext, _global_jsobject, java_packages );
        if( !ok )  throw_spidermonkey( "JSJ_pre_define_java_packages " + package_name );


        *result = JSVAL_VOID;
    }
    catch( const exception&  x )  { set_js_exception( x );  ok = false; }
    catch( const _com_error& x )  { set_js_exception( x );  ok = false; }

    return ok;
}

//------------------------------------------------------Spidermonkey::create_java_object_jsfunction
/*
JSBool Spidermonkey::create_java_object_jsfunction( JSObject*, uintN argc, jsval* argv, jsval* result )
{
    JSBool ok = true;

    try
    {
        if( argc != 1 )                     throw_spidermonkey( "create_java_object_jsfunction():  argc != 1" );
        if( !JSVAL_IS_STRING( argv[0] ) )   throw_spidermonkey( "1. Parameter ist nicht String" );

        if( !_java_vm )  _java_vm = javabridge::Vm::get_vm();


        string      java_class_name = JS_GetStringBytes( jsstring_from_jsval( argv[0] ) );
        javabridge::Env   java_env        = javabridge::Env( _java_vm );
        jobject     java_object     = javabridge::Class( _java_vm, java_class_name ).new_object( "()V" );

        ok = JSJ_ConvertJavaObjectToJSValue( _jscontext, java_object, result );
        if( !ok )  throw_spidermonkey( "JSJ_ConvertJavaObjectToJSValue" );
    }
    catch( const exception&  x )  { set_js_exception( x );  ok = false; }
    catch( const _com_error& x )  { set_js_exception( x );  ok = false; }

    return ok;
}
*/
//-----------------------------------------Java_liveconnect_interface::map_java_object_to_js_object
/* This callback implements netscape.javascript.JSObject.getWindow(),
   a method named for its behavior in the browser environment, where it
   returns the JS "Window" object corresponding to the HTML window that an
   applet is embedded within.  More generally, it's a way for Java to get
   hold of a JS object that has not been explicitly passed to it. */
/*
JSObject* Java_liveconnect_interface::map_java_object_to_js_object( JNIEnv* jEnv, void *pJavaObject, char **errp )
{
}
*/    
//------------------------------------Java_liveconnect_interface::get_JSPrincipals_from_java_caller
/* An interim callback function until the LiveConnect security story is
   straightened out.  This function pointer can be set to NULL. */
/*
JSPrincipals* Java_liveconnect_interface::get_JSPrincipals_from_java_caller( JNIEnv* jEnv, JSContext *pJSContext, void** pNSIPrincipaArray, int numPrincipals, void *pNSISecurityContext)
{
}
*/
//---------------------------------------------------Java_liveconnect_interface::enter_js_from_java
/* The following two callbacks sandwich any JS evaluation performed
   from Java.   They may be used to implement concurrency constraints, e.g.
   by suspending the current thread until some condition is met.  In the
   browser embedding, these are used to maintain the run-to-completion
   semantics of JavaScript.  It is acceptable for either function pointer
   to be NULL. */
/*
JSBool Java_liveconnect_interface::enter_js_from_java( JNIEnv* jEnv, char** errp )
{
}

//--------------------------------------------------------------Java_liveconnect_interface::exit_js

void Java_liveconnect_interface::exit_js( JNIEnv*, JSContext* )
{
}
*/
//----------------------------------------------------------Java_liveconnect_interface::error_print
/* Most LiveConnect errors are signaled by calling JS_ReportError(), but in
   some circumstances, the target JSContext for such errors is not
   determinable, e.g. during initialization.  In such cases any error
   messages are routed to this function.  If the function pointer is set to
   NULL, error messages are sent to stderr. */

void Java_liveconnect_interface::error_print( const char *error_msg )
{
    Z_LOG( "*** liveconnect: " << error_msg << "\n" );
}

//-----------------------------------------------------Java_liveconnect_interface::get_java_wrapper
/* This enables liveconnect to ask the VM for a java wrapper so that VM gets a chance to
   store a mapping between a jsobject and java wrapper. So the unwrapping can be done on the
   VM side before calling nsILiveconnect apis. This saves on a round trip request. */

jobject Java_liveconnect_interface::get_java_wrapper( JNIEnv*, jint )
{
    // jobject <- jsobject 
    return NULL;
}

//--------------------------------------------------Java_liveconnect_interface::unwrap_java_wrapper
/* This allows liveconnect to unwrap a wrapped JSObject that is passed from java to js. 
   This happens when Java code is passing back to JS an object that it got from JS. */

jint Java_liveconnect_interface::unwrap_java_wrapper( JNIEnv*, jobject )
{
    // jsobject <- jobject
    return 0;
}

//-------------------------------------------------------Java_liveconnect_interface::create_java_vm
/* The following set of methods abstract over the JavaVM object. */

JSBool Java_liveconnect_interface::create_java_vm( SystemJavaVM** result, JNIEnv** jenv, void* )
{
    ptr<javabridge::Vm> java_vm = javabridge::Vm::get_vm();

    *result = java_vm->vm();
    *jenv   = java_vm->jni_env();

    return true;
}

//------------------------------------------------------Java_liveconnect_interface::destroy_java_vm

JSBool Java_liveconnect_interface::destroy_java_vm( SystemJavaVM*, JNIEnv* )
{
    return false;
}

//------------------------------------------------Java_liveconnect_interface::attach_current_thread

JNIEnv* Java_liveconnect_interface::attach_current_thread( SystemJavaVM* jvm )
{
    //JavaVM* java_vm = (JavaVM*)jvm;
    void* jenv = NULL;
    jvm->AttachCurrentThread( &jenv, NULL );

    return (JNIEnv*)jenv;
}

//------------------------------------------------Java_liveconnect_interface::detach_current_thread

JSBool Java_liveconnect_interface::detach_current_thread( SystemJavaVM* jvm, JNIEnv* )
{
    jint err = jvm->DetachCurrentThread();
    return err == 0;
}

//----------------------------------------------------------Java_liveconnect_interface::get_java_vm

SystemJavaVM* Java_liveconnect_interface::get_java_vm( JNIEnv* jenv )
{
    JavaVM* java_vm = NULL;
    jenv->GetJavaVM( &java_vm );
    return java_vm;
}

//-------------------------------------------------------------------------------------------------

} //namespace spidermonkey

//---------------------------------------------------------------------Create_perl_scripting_engine

using namespace zschimmer;
using namespace spidermonkey;

HRESULT Create_spidermonkey_scripting_engine( const CLSID& clsid, const IID& iid, void** result )
{
    HRESULT hr;

    if( clsid != CLSID_Spidermonkey )  return CLASS_E_CLASSNOTAVAILABLE;

    #ifdef Z_WINDOWS
        ptr<Spidermonkey> engine = Z_NEW( Spidermonkey() );
        hr = engine->QueryInterface( iid, result );
     #else
        hr = Com_get_class_object( &class_descriptor, clsid, iid, result );
    #endif

    return hr;
}

//-------------------------------------------------------------------------------------------------

