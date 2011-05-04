// sosdll.h                  © 1995 SOS GmbH Berlin

#ifndef __SOSDLL_H
#define __SOSDLL_H

/*
    Statt erst beim Aufruf sollten die Adressen bei der Initialisierung des
    Objekts geholt werden. Dann wird weniger Code generiert.
    Am besten wahlweise. jz 15.5.96
*/

namespace sos
{

#if 1 //defined SYSTEM_WIN

#if !defined SOSDLL_NAME_PREFIX
#   define SOSDLL_NAME_PREFIX ""
#endif

#if !defined SOSDLL_CALL
#   if defined SYSTEM_WIN
#       define SOSDLL_CALL __stdcall       
#    else
#       define SOSDLL_CALL 
#   endif
#endif

#if defined SYSTEM_WIN
    typedef int (SOSDLL_CALL* Sosdll_proc_ptr)();           // wo ist das definiert?
#else
    typedef void (*Sosdll_proc_ptr)();
#endif



#ifdef SYSTEM_WIN
    typedef HINSTANCE       Library_handle;
# else
    typedef void*           Library_handle;
#endif


        

void* _dll_proc_ptr( Library_handle, const char* name );

inline Sosdll_proc_ptr dll_proc_ptr( Library_handle h, const char* name ) 
{ 
    return (Sosdll_proc_ptr)_dll_proc_ptr( h, name ); 
}



struct Sos_dll : Has_static_ptr
{
                                Sos_dll                 () : _library_handle(0) {} //(  const char* name );
                               ~Sos_dll                 ();     // Nicht in WEP rufen!!

    void                        init                    ( const char* name );
    void                        init                    ( const string& name )              { init( name.c_str() ); }
    Sosdll_proc_ptr             proc_ptr                ( const char* name ) const;
    Library_handle              handle                  () const                            { return _library_handle; }
    void                        set_handle              ( Library_handle h )                { _library_handle = h; }

  private:
    Library_handle             _library_handle;
};



struct Auto_loading_dll_proc
{
                                Auto_loading_dll_proc   () : _ptr ( 0 ) {}
  //Sosdll_proc_ptr             ptr                     ( const Sos_dll*, const char* name_ptr );
    void*                       ptr                     ( const Sos_dll*, const char* name_ptr );
  //void                        get_ptr                 ( const char* name );

  private:
    Sosdll_proc_ptr            _ptr;
};


template< class FUNC_TYPE >
struct Auto_loading_dll_proc_as : Auto_loading_dll_proc
{
    FUNC_TYPE                   ptr                     ( const Sos_dll* d, const char* name_ptr ) { return (FUNC_TYPE)Auto_loading_dll_proc::ptr( d, name_ptr ); }
};

#if !defined SOSDLL_NAME_PREFIX
#   define SOSDLL_NAME_PREFIX ""
#endif

#define DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, PARAM_DECL, PARAM_LIST )          \
    typedef RESULT_TYPE ( SOSDLL_CALL* T_##NAME ) PARAM_DECL;                               \
    Auto_loading_dll_proc_as< T_##NAME> _##NAME;                                            \
    RESULT_TYPE NAME PARAM_DECL                                                             \
    {                                                                                       \
        T_##NAME function_ptr = _##NAME.ptr( this, SOSDLL_NAME_PREFIX #NAME );              \
        return (*function_ptr) PARAM_LIST ;                                                 \
    }


#else   // !defined SYSTEM_WIN

// statisch einbinden:

#define DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, PARAM_DECL, PARAM_LIST )          \
    RESULT_TYPE NAME PARAM_DECL { return ::NAME PARAM_LIST; }

#endif


#define DECLARE_AUTO_LOADING_DLL_PROC_0( RESULT_TYPE, NAME )  \
    DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, (), () )

//#define DECLARE_AUTO_LOADING_DLL_PROC_0( RESULT_TYPE, NAME )  
//    DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, (), () )

#define DECLARE_AUTO_LOADING_DLL_PROC_1( RESULT_TYPE, NAME, TYPE1 )  \
    DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, ( TYPE1 p1 ), ( p1 ) )

#define DECLARE_AUTO_LOADING_DLL_PROC_2( RESULT_TYPE, NAME, TYPE1, TYPE2 )  \
    DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, ( TYPE1 p1, TYPE2 p2 ), ( p1, p2 ) )

#define DECLARE_AUTO_LOADING_DLL_PROC_3( RESULT_TYPE, NAME, TYPE1, TYPE2, TYPE3 )  \
    DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, ( TYPE1 p1, TYPE2 p2, TYPE3 p3 ), ( p1, p2, p3 ) )

#define DECLARE_AUTO_LOADING_DLL_PROC_4( RESULT_TYPE, NAME, TYPE1, TYPE2, TYPE3, TYPE4 )  \
    DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, ( TYPE1 p1, TYPE2 p2, TYPE3 p3, TYPE4 p4 ), ( p1, p2, p3, p4 ) )

#define DECLARE_AUTO_LOADING_DLL_PROC_5( RESULT_TYPE, NAME, TYPE1, TYPE2, TYPE3, TYPE4, TYPE5 )  \
    DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, ( TYPE1 p1, TYPE2 p2, TYPE3 p3, TYPE4 p4, TYPE5 p5 ), ( p1, p2, p3, p4, p5 ) )

#define DECLARE_AUTO_LOADING_DLL_PROC_6( RESULT_TYPE, NAME, TYPE1, TYPE2, TYPE3, TYPE4, TYPE5, TYPE6 )  \
    DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, ( TYPE1 p1, TYPE2 p2, TYPE3 p3, TYPE4 p4, TYPE5 p5, TYPE6 p6 ), ( p1, p2, p3, p4, p5, p6 ) )

#define DECLARE_AUTO_LOADING_DLL_PROC_7( RESULT_TYPE, NAME, TYPE1, TYPE2, TYPE3, TYPE4, TYPE5, TYPE6, TYPE7 )  \
    DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, ( TYPE1 p1, TYPE2 p2, TYPE3 p3, TYPE4 p4, TYPE5 p5, TYPE6 p6, TYPE7 p7 ), ( p1, p2, p3, p4, p5, p6, p7 ) )

#define DECLARE_AUTO_LOADING_DLL_PROC_8( RESULT_TYPE, NAME, TYPE1, TYPE2, TYPE3, TYPE4, TYPE5, TYPE6, TYPE7, TYPE8 )  \
    DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, ( TYPE1 p1, TYPE2 p2, TYPE3 p3, TYPE4 p4, TYPE5 p5, TYPE6 p6, TYPE7 p7, TYPE8 p8 ), ( p1, p2, p3, p4, p5, p6, p7, p8 ) )

#define DECLARE_AUTO_LOADING_DLL_PROC_9( RESULT_TYPE, NAME, TYPE1, TYPE2, TYPE3, TYPE4, TYPE5, TYPE6, TYPE7, TYPE8, TYPE9 )  \
    DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, ( TYPE1 p1, TYPE2 p2, TYPE3 p3, TYPE4 p4, TYPE5 p5, TYPE6 p6, TYPE7 p7, TYPE8 p8, TYPE9 p9 ), ( p1, p2, p3, p4, p5, p6, p7, p8, p9 ) )

#define DECLARE_AUTO_LOADING_DLL_PROC10( RESULT_TYPE, NAME, TYPE1, TYPE2, TYPE3, TYPE4, TYPE5, TYPE6, TYPE7, TYPE8, TYPE9, TYPE10 )  \
    DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, ( TYPE1 p1, TYPE2 p2, TYPE3 p3, TYPE4 p4, TYPE5 p5, TYPE6 p6, TYPE7 p7, TYPE8 p8, TYPE9 p9, TYPE10 p10 ), ( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10 ) )

#define DECLARE_AUTO_LOADING_DLL_PROC11( RESULT_TYPE, NAME, TYPE1, TYPE2, TYPE3, TYPE4, TYPE5, TYPE6, TYPE7, TYPE8, TYPE9, TYPE10, TYPE11 )  \
    DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, ( TYPE1 p1, TYPE2 p2, TYPE3 p3, TYPE4 p4, TYPE5 p5, TYPE6 p6, TYPE7 p7, TYPE8 p8, TYPE9 p9, TYPE10 p10, TYPE11 p11 ), ( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11 ) )

#define DECLARE_AUTO_LOADING_DLL_PROC12( RESULT_TYPE, NAME, TYPE1, TYPE2, TYPE3, TYPE4, TYPE5, TYPE6, TYPE7, TYPE8, TYPE9, TYPE10, TYPE11, TYPE12 )  \
    DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, ( TYPE1 p1, TYPE2 p2, TYPE3 p3, TYPE4 p4, TYPE5 p5, TYPE6 p6, TYPE7 p7, TYPE8 p8, TYPE9 p9, TYPE10 p10, TYPE11 p11, TYPE12 p12 ), ( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12 ) )

#define DECLARE_AUTO_LOADING_DLL_PROC13( RESULT_TYPE, NAME, TYPE1, TYPE2, TYPE3, TYPE4, TYPE5, TYPE6, TYPE7, TYPE8, TYPE9, TYPE10, TYPE11, TYPE12, TYPE13 )  \
    DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, ( TYPE1 p1, TYPE2 p2, TYPE3 p3, TYPE4 p4, TYPE5 p5, TYPE6 p6, TYPE7 p7, TYPE8 p8, TYPE9 p9, TYPE10 p10, TYPE11 p11, TYPE12 p12, TYPE13 p13 ), ( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13 ) )

#define DECLARE_AUTO_LOADING_DLL_PROC15( RESULT_TYPE, NAME, TYPE1, TYPE2, TYPE3, TYPE4, TYPE5, TYPE6, TYPE7, TYPE8, TYPE9, TYPE10, TYPE11, TYPE12, TYPE13, TYPE14, TYPE15 )  \
    DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, ( TYPE1 p1, TYPE2 p2, TYPE3 p3, TYPE4 p4, TYPE5 p5, TYPE6 p6, TYPE7 p7, TYPE8 p8, TYPE9 p9, TYPE10 p10, TYPE11 p11, TYPE12 p12, TYPE13 p13, TYPE14 p14, TYPE15 p15 ), ( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15 ) )

#define DECLARE_AUTO_LOADING_DLL_PROC21( RESULT_TYPE, NAME, TYPE1, TYPE2, TYPE3, TYPE4, TYPE5, TYPE6, TYPE7, TYPE8, TYPE9, TYPE10, TYPE11, TYPE12, TYPE13, TYPE14, TYPE15, TYPE16, TYPE17, TYPE18, TYPE19, TYPE20, TYPE21 )  \
    DECLARE_AUTO_LOADING_DLL_PROC( RESULT_TYPE, NAME, ( TYPE1 p1, TYPE2 p2, TYPE3 p3, TYPE4 p4, TYPE5 p5, TYPE6 p6, TYPE7 p7, TYPE8 p8, TYPE9 p9, TYPE10 p10, TYPE11 p11, TYPE12 p12, TYPE13 p13, TYPE14 p14, TYPE15 p15, TYPE16 p16, TYPE17 p17, TYPE18 p18, TYPE19 p19, TYPE20 p20, TYPE21 p21 ), ( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19, p20, p21 ) )


} //namespace sos

#endif
