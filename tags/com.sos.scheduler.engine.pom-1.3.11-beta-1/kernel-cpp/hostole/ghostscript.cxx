// $Id$          Joacim Zschimmer

#include "precomp.h"
#include "../kram/sysdep.h"
#include "../kram/sos.h"
#include "../zschimmer/com.h"
#include "../kram/com_simple_standards.h"
#include "../kram/com_server.h"
//#include "../kram/olestd.h"
#   include "hostole2.h"

#ifndef SYSTEM_WIN

 namespace sos {
    STDMETHODIMP Hostware::Ghostscript( BSTR parameters_bstr )
    {
        return E_NOTIMPL;
    }
 };

#else

extern "C"
{
#   define __PROTOTYPES__
#   define _Windows
#   include "../misc/ghostscript/iapi.h"
#   include "../misc/ghostscript/errors.h"
}

#include <vector>
#include "../kram/thread_semaphore.h"

//-------------------------------------------------------------------------------------------------

namespace sos {

using std::vector;

const char start_string[] = "systemdict /start get exec\n";
const char* error_texts[] = { "no error", LEVEL1_ERROR_NAMES, LEVEL2_ERROR_NAMES };

//--------------------------------------------------------------------------------------------static

static string                   stdout_buffer;              // Nicht thread-sicher, aber das ist GhostScript sowieso nicht.
static bool                     revision_showed             = false;

//------------------------------------------------------------------------------Hostware_ghostscript

struct Hostware_ghostscript: Ighostscript, Sos_ole_object
{
    void*                       operator new                ( size_t size )                         { return sos_alloc( size, "Ghostscript" ); }
    void                        operator delete             ( void* ptr )                           { sos_free( ptr ); }


                                Hostware_ghostscript        ();
                               ~Hostware_ghostscript        ();

    USE_SOS_OLE_OBJECT

    STDMETHODIMP                get_Collect_stdout          ( VARIANT_BOOL* b )                         { *b = _collect_stdout; return NOERROR; }
    STDMETHODIMP                put_Collect_stdout          ( VARIANT_BOOL );
    STDMETHODIMP                get_Stdout                  ( BSTR* text )                              { *text = SysAllocString_string( stdout_buffer ); return NOERROR; }
    STDMETHODIMP                Run                         ( BSTR );
    STDMETHODIMP                Init                        ();
    STDMETHODIMP                Close                       ();

  private:

    Fill_zero                  _zero_;
    gs_main_instance*          _gs;
    bool                       _collect_stdout;
    bool                       _used;
};

//-------------------------------------------------------------------------------------------------

DESCRIBE_CLASS_CREATABLE( &hostole_typelib, Hostware_ghostscript, ghostscript, CLSID_Ghostscript, "hostWare.Ghostscript", "1.0" );

//--------------------------------------------------------------------------------throw_ghostscript

static void throw_ghostscript( int error )
{
    const char* error_text = NULL;

    if( -error >= 1  &&  -error < NO_OF( error_texts ) )  error_text = error_texts[ -error ];

    throw_xc( Msg_code( "GHOSTSCRIPT", error ), error_text );
}

//-------------------------------------------------------------------------------------gsdll_stdout

int __stdcall gsdll_stdout( void*, const char* text, int len )
{
    stdout_buffer.append( text, len );
    return len;
}

//-------------------------------------------------------------------------------------------------

STDMETHODIMP Hostware::Ghostscript( BSTR parameters_bstr )
{
    HRESULT hr = NOERROR;

    gs_main_instance* gs = NULL;
    int               argc = 0;
    char**            argv = NULL;

    stdout_buffer = "";

    try
    {
        int                 ret;
        int                 exit_code = 0;
        gsapi_revision_t    revision;
        string              parameters = bstr_as_string( parameters_bstr );
        vector<string>      argv_array;

        argv_array.push_back( "" );

        LOGI( "Hostware::ghostscript " << parameters << '\n' );

        for( Sos_option_iterator o = parameters; !o.end(); o.next() )
        {
            argv_array.push_back( o.complete_parameter( '\0', '\0' ) );
        }

        argc = argv_array.size();
        argv = new char*[argc];
        for( int i = 0; i < argc; i++ )  argv[i] = strdup( argv_array[i].c_str() );

        ret = gsapi_revision( &revision, sizeof revision );
        if( ret == 0 )
        {
            LOG( "gsapi_revision product=" << revision.product << ", revision=" << revision.revision << ", date=" << revision.revisiondate << ", copyright=" << revision.copyright << '\n' );
        }

        ret = gsapi_new_instance( &gs, NULL );
        if( ret != 0 )  throw_ghostscript( ret );

        //if( stdout_output )  gsapi_set_stdio( gs, NULL, gsdll_stdout, NULL );

        ret = gsapi_init_with_args( gs, argc, argv );
        if( ret != 0  &&  ret != e_Quit )  throw_ghostscript( ret );


	    ret = gsapi_run_string( gs, start_string, 0, &exit_code );
        if( ret != e_Quit )  throw_ghostscript( ret );
    }
    catch( const Xc&         x )  { hr = _set_excepinfo( x, "hostWare::ghostscript" ); }
    catch( const xmsg&       x )  { hr = _set_excepinfo( x, "hostWare::ghostscript" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "hostWare::ghostscript" ); }

    if( gs )
    {
        gsapi_exit( gs );
        gsapi_delete_instance( gs );
        gs = NULL;
    }

    //if( stdout_output )  *stdout_output = SysAllocString_string( stdout_buffer );  stdout_buffer = "";

    for( int i = 0; i < argc; i++ )  delete argv[i];
    delete argv;

    return hr;
}

//-------------------------------------------------------------------------------------------------

Hostware_ghostscript::Hostware_ghostscript()
:
    Sos_ole_object( &ghostscript_class, this ),
    _zero_(this+1)
{
}

//-------------------------------------------------------------------------------------------------

Hostware_ghostscript::~Hostware_ghostscript()
{
    if( _gs )
    {
        gsapi_exit( _gs );
        gsapi_delete_instance( _gs );
        _gs = NULL;
    }
}

//-------------------------------------------------------------------------------------------------

STDMETHODIMP Hostware_ghostscript::Close()
{
    HRESULT hr = NOERROR;

    if( _gs )
    {
        gsapi_exit( _gs );
        gsapi_delete_instance( _gs );
        _gs = NULL;
    }

    return hr;
}

//-------------------------------------------------------------------------------------------------

STDMETHODIMP Hostware_ghostscript::Init()
{
    HRESULT     hr = NOERROR;
    int         ret;

    try
    {
        if( _used )  Close();
        if( _gs )  return hr;

        if( !revision_showed )
        {
            revision_showed = true;
            gsapi_revision_t    revision;

            ret = gsapi_revision( &revision, sizeof revision );
            if( ret == 0 )
            {
                LOG( "gsapi_revision product=" << revision.product << ", revision=" << revision.revision << ", date=" << revision.revisiondate << ", copyright=" << revision.copyright << '\n' );
            }
        }

        ret = gsapi_new_instance( &_gs, NULL );
        if( ret != 0 )  throw_ghostscript( ret );

        _used = false;
    }
    catch( const Xc&         x )  { hr = _set_excepinfo( x, "hostWare.Ghostscript.init" ); }
    catch( const xmsg&       x )  { hr = _set_excepinfo( x, "hostWare.Ghostscript.init" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "hostWare.Ghostscript.init" ); }

    return hr;
}

//---------------------------------------------------------Hostware_ghostscript::put_collect_stdout

STDMETHODIMP Hostware_ghostscript::put_Collect_stdout( VARIANT_BOOL b )
{
    HRESULT hr = NOERROR;
    int     ret;

    try
    {
        _collect_stdout = b != 0;

        hr = Init();  if( FAILED(hr) )  return hr;

        ret = gsapi_set_stdio( _gs, NULL, _collect_stdout? gsdll_stdout : NULL, NULL );
        if( ret != 0 )  throw_ghostscript( ret );
    }
    catch( const Xc&         x )  { hr = _set_excepinfo( x, "hostWare.Ghostscript.collect_stdout" ); }
    catch( const xmsg&       x )  { hr = _set_excepinfo( x, "hostWare.Ghostscript.collect_stdout" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "hostWare.Ghostscript.collect_stdout" ); }

    return hr;
}

//-------------------------------------------------------------------------------------------------

HRESULT Hostware_ghostscript::Run( BSTR parameters_bstr )
{
    HRESULT         hr = NOERROR;
    vector<string>  argv_array;
    char**          argv = NULL;
    int             argc = 0;
    int             exit_code;
    int             ret;

    try
    {
        LOG( "Ghostscript::run " << parameters_bstr << '\n' );

        hr = Init();  if( FAILED(hr) )  return hr;

        argv_array.push_back( "" );
        for( Sos_option_iterator o = bstr_as_string( parameters_bstr ); !o.end(); o.next() )
        {
            argv_array.push_back( o.complete_parameter( '\0', '\0' ) );
        }

        argc = argv_array.size();
        argv = new char*[argc];
        for( int i = 0; i < argc; i++ )  argv[i] = strdup( argv_array[i].c_str() );

        _used = true;

        stdout_buffer = "";
        ret = gsapi_init_with_args( _gs, argc, argv );
        if( ret != 0  &&  ret != e_Quit )  throw_ghostscript( ret );

	    ret = gsapi_run_string( _gs, start_string, 0, &exit_code );
        if( ret != e_Quit )  throw_ghostscript( ret );
    }
    catch( const Xc&         x )  { hr = _set_excepinfo( x, "hostWare.Ghostscript.run" ); }
    catch( const xmsg&       x )  { hr = _set_excepinfo( x, "hostWare.Ghostscript.run" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "hostWare.Ghostscript.run" ); }

    for( int i = 0; i < argc; i++ )  delete argv[i];
    delete argv;

    return hr;
}

//-------------------------------------------------------------------------------------------------

} //namespace sos

#endif
