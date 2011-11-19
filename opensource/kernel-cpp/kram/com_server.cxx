// $Id: com_server.cxx 13404 2008-02-12 08:09:25Z jz $

#include "precomp.h"


#include "sos.h"
#include "sosprof.h"
#include "thread_semaphore.h"
#include "com_simple_standards.h"
#include "olereg.h"
#include "com_server.h"

using namespace std;



namespace sos {

//void set_hostole_exception( const Xc& );    // in hostole/exception.cxx

//--------------------------------------------------------------------------------------------const

const extern bool  _dll;

//-------------------------------------------------------------------------------------------static

zschimmer::long32   com_object_count = 0;
zschimmer::long32   com_lock         = 0;

static bool         module_initialized = false;
static bool         log_queryinterface = false;
bool                hostole_log_all  = false;

//--------------------------------------------------------------------------------------init_static

static void init_static()
{
    if( module_initialized )  return;

    Z_MUTEX( hostware_mutex )
    {
        if( !module_initialized )
        {
            hostole_log_all    = read_profile_bool( "", "hostOLE", "log-all", false );
            log_queryinterface = read_profile_bool( "", "hostOLE", "log-QueryInterface", log_queryinterface );
            
            module_initialized = true;
        }
    }
}

//---------------------------------------------Hostole_ISupportErrorInfo::Hostole_ISupportErrorInfo

Hostole_ISupportErrorInfo::Hostole_ISupportErrorInfo( Sos_ole_object* pObj )
{
    _ref_count  = 0;
    _object     = pObj;
}

//--------------------------------------------Hostole_ISupportErrorInfo::~Hostole_ISupportErrorInfo

Hostole_ISupportErrorInfo::~Hostole_ISupportErrorInfo()
{
}

//--------------------------------------------------------Hostole_ISupportErrorInfo::QueryInterface

STDMETHODIMP Hostole_ISupportErrorInfo::QueryInterface( const IID& riid, void** ppv )
{
    return _object->_iunknown->QueryInterface( riid, ppv );
}

//----------------------------------------------------------------Hostole_ISupportErrorInfo::AddRef

STDMETHODIMP_(ULONG) Hostole_ISupportErrorInfo::AddRef()
{
    InterlockedIncrement( &_ref_count ); 

    return _object->_iunknown->AddRef();
}

//---------------------------------------------------------------Hostole_ISupportErrorInfo::Release

STDMETHODIMP_(ULONG) Hostole_ISupportErrorInfo::Release()
{
    InterlockedDecrement( &_ref_count ); 

    return _object->_iunknown->Release();
}

//--------------------------------------------Hostole_ISupportErrorInfo::InterfaceSupportsErrorInfo

STDMETHODIMP Hostole_ISupportErrorInfo::InterfaceSupportsErrorInfo( const IID& riid )
{
    return _object->_ole_class_descr && riid == _object->_ole_class_descr->_iid? S_OK
                                                                               : S_FALSE;
}

//-----------------------------------------------------------------Sos_ole_object::Sos_ole_object

Sos_ole_object::Sos_ole_object( Ole_class_descr* ole_class_descr, IUnknown* iunknown, IUnknown* pUnkOuter )
:
    _zero_(this+1),
    _iunknown           ( iunknown ),
    _supporterrorinfo   ( this ),               // Andere Oberklasse
    _ole_class_descr    ( ole_class_descr ),
    _pUnkOuter          ( pUnkOuter )          // ?
{
    init_static();

    if( _ole_class_descr )  _class_name = _ole_class_descr->_name;
                      else  _class_name = "Sos_ole_object";
}

//-----------------------------------------------------------------Sos_ole_object::Sos_ole_object

Sos_ole_object::Sos_ole_object( const Sos_ole_object& o )
:
    _zero_(this+1),
    _iunknown         ( NULL ),     // = (IUnknown*)this, muss von der erbenden Klasse gesetzt werden!
    _supporterrorinfo ( this ),
    _pUnkOuter        ( NULL )
{
    *this = o;
}

//----------------------------------------------------------------Sos_ole_object::~Sos_ole_object

Sos_ole_object::~Sos_ole_object()
{
/*
    if( _typeinfo ) {
        _typeinfo->Release();
        _typeinfo = NULL;
    }
*/
}

//-----------------------------------------------------------------Sos_ole_object::Sos_ole_object

Sos_ole_object& Sos_ole_object::operator = ( const Sos_ole_object& o )
{
  //_ref_count bleibt!
    _iunknown           = o._iunknown;
    _supporterrorinfo   = o._supporterrorinfo;
    _ole_class_descr    = o._ole_class_descr;
    _pUnkOuter          = o._pUnkOuter;
  //_pImpIProvideCI     = o._pImpIProvideCI;

  //_connection_point_container = o._connection_point_container;

    return *this;
}

//---------------------------------------------------------------------------Sos_ole_object::init
/*
HRESULT Sos_ole_object::init()
{
  //IUnknown*       iunknown = this;
  //HRESULT         hr;

  //if( _pUnkOuter )  iunknown = _pUnkOuter;

    //hr = _ole_class_descr->load_typelib();
    //if( FAILED( hr ) )  return hr;

     *
     * CreateStdDispatch always returns an IUnknown pointer
     * because such is required in aggregation, which is
     * involved here.  In other words, the hostOLE is aggregating
     * on the OLE-provided "StdDispatch" object in order to
     * directly expose IDispatch from that object.  See the
     * implementation of QueryInterface.
     *

    //hr=CreateStdDispatch( iunknown, (Ihostware_file*)_this, _pITINeutral, &_pIUnkStdDisp );
    //if (FAILED(hr))  return hr;

    return NOERROR;
}
*/
//---------------------------------------------------------------------Sos_ole_object::_obj_print

void Sos_ole_object::_obj_print( ostream* s ) const
{
    *s << _class_name;
}

//-----------------------------------------------------------------Sos_ole_object::fill_excepinfo

void Sos_ole_object::fill_excepinfo( const Xc& x, EXCEPINFO* excepinfo )
{
    LOG( "Fehler: " << x << '\n' );

    if( !excepinfo )  return;

    memset( excepinfo, 0, sizeof excepinfo );

    Dynamic_area text;
    x.get_text( &text );

    if( strncmp( x.code(), "SOS-", 4 ) == 0 )  excepinfo->wCode = atoi( x.code() + 4 );
    if( excepinfo->wCode == 0 )   excepinfo->wCode = 1001;                      // An error code describing the error.

  //excepinfo->wReserved;
    excepinfo->bstrSource       = SysAllocString_string( _class_name );    // Source of the exception.
    excepinfo->bstrDescription  = SysAllocString_string( text.char_ptr() );            // Textual description of the error.
  //excepinfo->bstrHelpFile     = SysAllocString( _ole_class_descr->_helpfilename );
  //excepinfo->dwHelpContext    = ...;                                          // Help context ID.
  //excepinfo->pvReserved;
  //excepinfo->pfnDeferredFillIn = ...;                                         // Pointer to function that fills in Help and description info.
  //RETURN VALUE return value;	// A return value describing the error.
}
    
//-----------------------------------------------------------------Sos_ole_object::fill_excepinfo

void Sos_ole_object::fill_excepinfo( const exception& x, EXCEPINFO* excepinfo )
{
    LOG( "Fehler: " << x << '\n' );

    if( !excepinfo )  return;

    memset( excepinfo, 0, sizeof excepinfo );

    if( excepinfo->wCode == 0 )   excepinfo->wCode = 1002;                      // An error code describing the error.

  //excepinfo->wReserved;
    excepinfo->bstrSource       = SysAllocString_string( _class_name );         // Source of the exception.
    excepinfo->bstrDescription  = SysAllocString_string( exception_text( x ) ); // Textual description of the error.
  //excepinfo->bstrHelpFile     = SysAllocString( _ole_class_descr->_helpfilename );
  //excepinfo->dwHelpContext    = ...;                                          // Help context ID.
  //excepinfo->pvReserved;
  //excepinfo->pfnDeferredFillIn = ...;                                         // Pointer to function that fills in Help and description info.
  //RETURN VALUE return value;	// A return value describing the error.
}
    
//-----------------------------------------------------------------Sos_ole_object::fill_excepinfo

void Sos_ole_object::fill_excepinfo( const _com_error& x, EXCEPINFO* excepinfo )
{
    LOG( "Fehler: " << x.Description() << '\n' );

    if( !excepinfo )  return;

    memset( excepinfo, 0, sizeof excepinfo );

    //if( excepinfo->wCode == 0 )   excepinfo->wCode = 1002;                      // An error code describing the error.

  //excepinfo->wReserved;
    excepinfo->scode            = x.Error();
  //excepinfo->bstrSource       = SysAllocString_string( _class_name );         // Source of the exception.
    excepinfo->bstrDescription  = SysAllocString( x.Description() );
  //excepinfo->bstrHelpFile     = SysAllocString( _ole_class_descr->_helpfilename );
  //excepinfo->dwHelpContext    = ...;                                          // Help context ID.
  //excepinfo->pvReserved;
  //excepinfo->pfnDeferredFillIn = ...;                                         // Pointer to function that fills in Help and description info.
  //RETURN VALUE return value;	// A return value describing the error.
}
    
//-----------------------------------------------------------------Sos_ole_object::_set_excepinfo

HRESULT Sos_ole_object::_set_excepinfo( const Xc& x, const string& method )
{
    HRESULT hr;

    //set_hostole_exception( x );

    string what   = x.what();
    string source = x.name();
    if( source == "OLE" )  source = "";

    hr = _set_excepinfo( source.c_str(), what.c_str(), method );

    //Als hr sollte DISP_E_EXCEPTION zurückgegeben werden, denn sonst verschwindet der Fehlertext (mit Einfügungen) oder manchmal sogar der ganze Fehler (in JScript) jz 24.6.01
    //if( strncmp( x.code(), "MSWIN-", 6 ) == 0 )  try{  hr = hex_as_int32( x.code() + 6 );  }catch(const Xc&){}
    //if( strcmp( x.name(), "OLE" ) == 0  &&  FAILED( ((Ole_error*)&x)->_hresult ) )  hr = ((Ole_error*)&x)->_hresult;

    return hr;
}

//-----------------------------------------------------------------Sos_ole_object::_set_excepinfo

HRESULT Sos_ole_object::_set_excepinfo( const _com_error& x, const string& method )
{
    //set_hostole_exception( Ole_error(x) );

#   ifdef SYSTEM_HAS_COM

        string text = get_mswin_msg_text( x.Error() );
        if( x.Description().length() > 0 )  text += " / " + bstr_as_string( x.Description() );
        _set_excepinfo( c_str( _class_name ), text.c_str(), method );
        return x.Error();

#    else

        // Sollte nicht aufgerufen werden!
        return E_FAIL;

#   endif
}

//-----------------------------------------------------------------Sos_ole_object::_set_excepinfo

HRESULT Sos_ole_object::_set_excepinfo( const exception& x, const string& method )
{
    LOG( "_set_excepinfo(\"" << x.what() << "\",\"" << method << ")\n" );
    //set_hostole_exception( x );


#ifndef SYSTEM_GNU
    if( typeid( x ) == typeid( ::std::ios_base::failure ) )
    {
        try
        {
            throw_errno( errno, "ios" );
        }

        catch( const Xc& xc )
        {
           return _set_excepinfo( xc, method );
        }
    }
    else
#endif


#ifdef DYNAMIC_CAST_CRASHES
    int EINFACHERE_FEHLERBEHANDLUNG_WEGEN_ABSTURZ_IN__DYNAMIC_CAST;	// gcc 3.2.3, bei Hostjava File->open( "-out mail ..." );
#else
    if( dynamic_cast< const Xc* >( &x ) )
    {
        return _set_excepinfo( *(const Xc*)&x, method );
    }
#endif

    return _set_excepinfo( c_str( _class_name ), exception_text( x ), method );
}

//-----------------------------------------------------------------Sos_ole_object::_set_excepinfo

HRESULT Sos_ole_object::_set_excepinfo( const char* source, const char* descr, const string& method )
{
    string description = descr? trim(descr) : "";
    if( method != "" ) {
        if( description == "" )  description = string("Calling ") + method;
                           else  description += string(" (Calling ") + method + ")";
    }

    LOG( "_set_excepinfo(\"" << source << "\",\"" << description << ")\n" );

    HRESULT             hr;
    ICreateErrorInfo*   create_error_info;
    IErrorInfo*         error_info = NULL;

    hr = CreateErrorInfo( &create_error_info );
    if( FAILED( hr ) )  return hr;

  //LPOLESTR helpfile = OLETEXT( "hostOLE.hlp" oder x.help_file() );
  //create_error_info->SetHelpFile( helpfile );
  //create_error_info->SetHelpContext( HELP_HOSTOLE_xxx oder x.help_id() );

    if( _ole_class_descr )  create_error_info->SetGUID( _ole_class_descr->_iid );

    Bstr bstr = source;
    create_error_info->SetSource( bstr );

    bstr = description.c_str();
    create_error_info->SetDescription( bstr );

    hr = create_error_info->QueryInterface( IID_IErrorInfo, (void**)&error_info );
    if( SUCCEEDED(hr) && error_info ) {
        SetErrorInfo( 0, error_info );
        error_info->Release();
        error_info = NULL;
    }

    create_error_info->Release();  //SetErrorInfo holds the object's IErrorInfo
    create_error_info = NULL;

    return DISP_E_EXCEPTION;
}

//--------------------------------------------------------------------Sos_ole_object::add_interface
/*
void Sos_ole_object::com_add_interface( const IID& iid, IUnknown* o ) 
{ 
    _additional_interfaces.push_back( Interface_ptr() );

    Interface_ptr* i = &*_additional_interfaces.rbegin();
    i->_iid = iid;
    i->_iunknown = o;
}
*/
//-----------------------------------------------------------------Sos_ole_object::QueryInterface

STDMETHODIMP Sos_ole_object::QueryInterface( const IID& riid, void** ppv )
{
    if( hostole_log_all )  LOG( *this << ".QueryInterface " << riid );
    *ppv = NULL;

    /* The only calls for IUnknown are either in a nonaggregated
     * case or when created in an aggregation, so in either case
     * always return our IUnknown for IID_IUnknown.
     */
    if( riid == IID_IUnknown
     || riid == IID_IDispatch 
     || _ole_class_descr && riid == _ole_class_descr->_iid )  *ppv = _iunknown;
    else
    if( riid == IID_ISupportErrorInfo )  *ppv = &_supporterrorinfo;
#ifdef SYSTEM_HAS_COM
  //else
  //if( riid == IID_IProvideClassInfo )  *ppv = _ole_class_descr->_provideclassinfo;
  //else
  //if( riid == IID_IConnectionPointContainer )  *ppv = _connection_point_container;
#endif
    else
    {
        //Z_FOR_EACH( Interfaces, _additional_interfaces, it )  if( it->_iid == riid )  { *ppv = it->_iunknown; break; }
    }

    if( !*ppv ) 
    {
        if( hostole_log_all  ||  log_queryinterface  &&  _last_queried_interface != riid )   // Nicht thread-sicher. Nicht schlimm, dann ist eben _last_queried_interface falsch.
        {
            if( !hostole_log_all )  LOG( *this << ".QueryInterface " << riid );
            LOG( " => E_NOINTERFACE\n" );
            _last_queried_interface = riid;
        }
        return E_NOINTERFACE;
    }

    ((IUnknown*)*ppv)->AddRef();    

    if( hostole_log_all | log_queryinterface )  sos_log( "\n" );
    return NOERROR;
}

//-------------------------------------------------------------------------Sos_ole_object::AddRef

STDMETHODIMP_(ULONG) Sos_ole_object::AddRef()
{
    int result = InterlockedIncrement( &_ref_count );
    if( result == 1 )  InterlockedIncrement( &com_object_count );

    //LOG( *this << ".AddRef: _ref_count=" << result <<", com_object_count=" << com_object_count << '\n' );
    return result;
}

//------------------------------------------------------------------------Sos_ole_object::Release

STDMETHODIMP_(ULONG) Sos_ole_object::Release()
{
    if( _ref_count == 0 )  {
        LOG( "***FEHLER*** Sos_ole_object._ref_count ist schon 0!\n" ); 
#       if defined _DEBUG && defined SYSTEM_WIN
            DebugBreak();
#       endif
        return 0; 
    }

    int result = InterlockedDecrement( &_ref_count );
    if( result != 0 )  return result;

    if( hostole_log_all )  LOG( *this << ".Release ref=0\n" );
    //LOG( *this << ".Release: _ref_count=" << _ref_count <<", com_object_count=" << com_object_count << '\n' );

/*  jz 8.4.98
#   ifdef SYSTEM_WIN32
        AFX_MANAGE_STATE( AfxGetStaticModuleState() );
#   endif
*/
    InterlockedDecrement( &com_object_count );

#   ifdef SYSTEM_WIN
        if( !_dll  &&  com_object_count == 0  &&  com_lock == 0 ) 
        {
            PostQuitMessage( 0 );
        }
#   endif

    // Geht nicht, weil IUnknown::~IUnknown nicht virtuell ist:  delete _this;
    return 0;
}

//---------------------------------------------------------------Sos_ole_object::GetTypeInfoCount

STDMETHODIMP Sos_ole_object::GetTypeInfoCount( UINT* pctInfo )
{
    LOG( _class_name << "::GetTypeInfoCount()\n" );
    *pctInfo = 0;   // noch keine Typinformation
    return NOERROR;
}

//--------------------------------------------------------------------Sos_ole_object::GetTypeInfo

STDMETHODIMP Sos_ole_object::GetTypeInfo( UINT /*itinfo*/, LCID, ITypeInfo** pptInfo )
{
    *pptInfo = 0;
    return E_NOTIMPL;
}

//------------------------------------------------------------------Sos_ole_object::GetIDsOfNames

STDMETHODIMP Sos_ole_object::GetIDsOfNames( const IID& riid, OLECHAR** rgszNames, UINT cNames,
                                            LCID lcid, DISPID* rgDispID )
{
    if( log_ptr && hostole_log_all )  log_GetIDsOfNames( riid, rgszNames, cNames, lcid );

    if( riid != IID_NULL )  return DISP_E_UNKNOWNINTERFACE;
    if( !_ole_class_descr ) return DISP_E_UNKNOWNINTERFACE;

#   ifdef SYSTEM_HAS_COM
        if( !_ole_class_descr->_typeinfo ) 
        {
            Z_MUTEX( hostware_mutex )
            {
                if( !_ole_class_descr->_typeinfo ) 
                {
                    HRESULT hr = _ole_class_descr->load_typeinfo();
                    if( FAILED( hr ) )  return hr;
                }
            }
        }

        return _ole_class_descr->_typeinfo->GetIDsOfNames( rgszNames, cNames, rgDispID );
#    else
        return E_NOTIMPL;
#   endif
}

//--------------------------------------------------------------Sos_ole_object::log_GetIDsOfNames

void Sos_ole_object::log_GetIDsOfNames( const IID& riid, OLECHAR** rgszNames, UINT cNames, LCID )
{
    Log_ptr log;  // Hat Semaphore

    if( !log )  return;

    *log << *this << ".GetIDsOfNames " << riid;

    for( int i = 0; i < (int)cNames; i++ ) 
    {
        *log << ( i == 0? ' ' : ',' ); 
        *log << w_as_string( rgszNames[ i ] );
    }

    *log << '\n';
}

//---------------------------------------------------------------------Sos_ole_object::log_invoke

void Sos_ole_object::log_invoke( DISPID dispID, const IID& riid, LCID lcid,
                                         unsigned short wFlags, DISPPARAMS* pDispParams )
{
    Log_ptr log;  // Hat Semaphore

    if( !log )  return;


    *log << *this << ":Invoke(";
    print_dispid( log, dispID );
    *log << ',' << hex << riid << ',' << (ulong)lcid << ',';

    if( wFlags & ~( DISPATCH_METHOD | DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF ) ) {
        *log << ' ' << hex << wFlags << dec;
    }

    if( wFlags & DISPATCH_METHOD )          *log << " METHOD";
    if( wFlags & DISPATCH_PROPERTYGET )     *log << " PROPERTYGET";
    if( wFlags & DISPATCH_PROPERTYPUT )     *log << " PROPERTYPUT";
    if( wFlags & DISPATCH_PROPERTYPUTREF )  *log << " PROPERTYPUTREF";

    *log << ',' << dec;
    *log << pDispParams;
    *log << ") ";
}

//-------------------------------------------------------------------------Sos_ole_object::Invoke

STDMETHODIMP Sos_ole_object::Invoke( DISPID dispID, const IID& riid, LCID,
                                     unsigned short wFlags, DISPPARAMS* pDispParams,
                                     VARIANT* pVarResult, EXCEPINFO* pExcepInfo,
                                     UINT* puArgErr )
{
    HRESULT hr;

    if( hostole_log_all )  LOG( *this << ".Invoke " );

    if( riid != IID_NULL || !_ole_class_descr )  { LOG( "DISP_E_UNKNOWNINTERFACE\n" ); return DISP_E_UNKNOWNINTERFACE; }

#   ifdef SYSTEM_HAS_COM
        if( !_ole_class_descr->_typeinfo ) 
        {
            Z_MUTEX( hostware_mutex )
            {
                if( !_ole_class_descr->_typeinfo ) 
                {
                    hr = _ole_class_descr->load_typeinfo();
                    if( FAILED( hr ) )  return hr;
                }
            }
        }

        if( hostole_log_all && log_ptr ) 
        {
            BSTR name = NULL;

            if( !FAILED( _ole_class_descr->_typeinfo->GetDocumentation( dispID, &name, NULL, NULL, NULL ) ) ) 
            {
                *log_ptr << name;
                SysFreeString( name );
            }

            *log_ptr << '\n';
        }

        SetErrorInfo( 0, NULL );

        hr = _ole_class_descr->_typeinfo->Invoke( _iunknown, dispID, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );

#    else

        hr = E_NOTIMPL;

#   endif

    return hr;
}

//-----------------------------------------------------------------------operator << Sos_ole_object

ostream& operator << ( ostream& s, const Sos_ole_object& o ) 
{
    try {
        o._obj_print( &s );
    }
    catch( const Xc& x )
    {
        s << "(Fehler bei Objektanzeige: " << x << ')';
    }

    return s;
}

Ole_class_descr*    Ole_class_descr::head   = NULL;

//----------------------------------------------------------------------------Ole_class_descr_class

struct Ole_class_descr_class : Ole_class_descr
{
    Ole_class_descr_class()
    :
        Ole_class_descr( NULL, IID_NULL, IID_NULL, "hostOLE Factory", "1.0" )
    {
    }
}
ole_class_descr_class;

//---------------------------------------------------------------------------Ole_server::Ole_server

Ole_server::Ole_server( Typelib_descr* typelib ) 
:
    _zero_(this+1),
    _typelib ( typelib ),
    _appl ( false )
{
}

//--------------------------------------------------------------------------Ole_server::~Ole_server

Ole_server::~Ole_server()
{
    exit();
}

//---------------------------------------------------------------------------------Ole_server::exit

void Ole_server::exit()
{
    if( !_initialized )  return;
    _initialized = false;

    Ole_class_descr* cls = Ole_class_descr::head;
    while( cls ) 
    {
        //if( cls->_creatable )  cls->unregister_class_object();
        if( cls->_class_factory ) {
            cls->_class_factory->unregister_factory();
            cls->_class_factory->Release();
            cls->_class_factory = NULL;
        }
        cls = cls->_next;
    }

    //if( _class_factory )  _class_factory->Release();

    if( !_dll )
    {
        CoUninitialize();
    }

    _appl.exit();       // Sos_static schließen
}

//---------------------------------------------------------------------------------Ole_server::init

void Ole_server::init()
{
    LOGI( "Ole_server::init\n " );

    if( _initialized )  return;

    HRESULT hr;

    _appl.init();   // Sos_static initialisieren

    if( !_dll )
    {
        hr = CoInitialize( NULL );
        if( FAILED( hr ) )  throw_ole( hr, "CoInitialize" );
    }

    _typelib->init();

    Ole_class_descr* cls = Ole_class_descr::head;
    while( cls ) 
    {
        if( cls->_creatable ) 
        {
            if( !_dll ) 
            {
                Ole_factory*  factory = new Ole_factory;

                hr = factory->QueryInterface( IID_IClassFactory, (void**)&cls->_class_factory );
                if( FAILED( hr ) )  throw_ole( hr, "Ole_factory::QueryInterface" );

                factory->set_clsid( cls->_clsid );

                //cls->register_class_object( cls->_class_factory );
                factory->register_factory();
            }
        }
        cls = cls->_next;
    }

    _initialized = true;
}

//-------------------------------------------------------------------------Ole_server::message_loop
#ifdef SYSTEM_WIN

void Ole_server::message_loop()
{
    LOGI( "Ole_server::message_loop\n" );

    MSG msg;

    while( GetMessage( &msg, 0, 0, 0 ) )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
}

#endif
//---------------------------------------------------------------------Typelib_descr::Typelib_descr

Typelib_descr::Typelib_descr( const IID& typelib_id, const char* name, const char* version )
:
    _zero_(this+1),
    _typelib_id ( typelib_id ),
    _name       ( name       ),
    _version    ( version    )
{
}

//--------------------------------------------------------------------Typelib_descr::~Typelib_descr

Typelib_descr::~Typelib_descr()
{
#   ifdef SYSTEM_HAS_COM

        if( _typelib ) 
        {
            _typelib->Release();
            _typelib = NULL;
            //2007-07-30 Wird von statischer Variabeln gerufen, wenn COM-DLL bereits entladen ist: CoUninitialize();
        }

#   endif
}

//------------------------------------------------------------------------------Typelib_descr::init

void Typelib_descr::init()
{
    if( _initialized )  return;

    _initialized = true;

    if( length( _typelib_filename ) == 0 ) 
    {
        _typelib_filename = module_filename();
    }
}

//----------------------------------------------------------------------Typelib_descr::load_typelib
#ifdef SYSTEM_HAS_COM

void Typelib_descr::load_typelib()
{
    HRESULT hr = NOERROR;;
    //static Thread_semaphore loadtypelib_lock ( "loadtypelib" );

    THREAD_LOCK( hostware_mutex )
    {
        if( !_typelib ) 
        {
            init();  //jz 10.1.2001

            //hr = LoadRegTypeLib( _typelib_id, 1, 0, LANG_NEUTRAL, &_typelib );
            //if( FAILED( hr ) ) {
            /* If LoadRegTypeLib fails, try loading directly with
             * LoadTypeLib, which will register the library for us.
             *
             * NOTE:  You should prepend your DIR registry key to the
             * .TLB name so you don't depend on it being it the PATH.
             */
            //com_register_server();
            //hr = LoadRegTypeLib( LIBID_hostware_type_library, 1, 0, LANG_NEUTRAL, &type_lib );
            //if( FAILED( hr ) )  return hr;

            CoInitialize(NULL);     // Gegenstück in ~Typelib_descr(), damit OLE bis Erfüllung von atexit() gehalten wird. 

            //Sos_string typelib_filename = module_filename();
            BSTR typelib_filename_bstr = SysAllocString_string( _typelib_filename );
          //LOG( "LoadTypeLib " << typelib_filename_bstr << '\n' );
            hr = LoadTypeLib( typelib_filename_bstr, &_typelib );
            SysFreeString( typelib_filename_bstr );
            if( FAILED( hr ) )  throw_ole( hr, "LoadTypeLib", c_str( _typelib_filename ) );
        }
    }
}

#endif
//----------------------------------------------------------------Typelib_descr::register_server
#ifdef SYSTEM_HAS_COM

HRESULT Typelib_descr::register_server()
{
    Sos_appl appl ( false );        // Sos_static 

    try {
        HKEY        key = 0;
        Sos_string  k;

        appl.init();                // Sos_static initialisieren

        //if( !SOS_LICENCE( licence_hostole ) )  throw_xc( "SOS-1000", "hostWare OLE-Server" );
    
        try {
            hostole_register_typelib( _typelib_id, c_str( _name ), c_str( _version ), _typelib_filename );
        }
        catch( const Xc& x ) 
        {
            LOG( "hostOLE: Fehler bei der Registrierung: " << x << '\n' );
            SHOW_MSG( "Fehler: " << x );
            return SELFREG_E_TYPELIB;
        }

        try 
        {
            Ole_class_descr* cls = Ole_class_descr::head;
            
            while( cls ) 
            {
                if( cls->_creatable )  cls->register_class();
                cls = cls->_next;
            }
        }
        catch( const Xc& x ) 
        {
            LOG( "Fehler bei der Registrierung: " << x << '\n' );
            SHOW_ERR( "Fehler: " << x );
            return SELFREG_E_CLASS;
        }
    }
    catch( const Xc& x ) 
    {
        SHOW_ERR( "Fehler: " << x );
        return E_FAIL;
    }

    return S_OK;
}


#endif
//---------------------------------------------------------------Typelib_descr::::unregister_server
#ifdef SYSTEM_HAS_COM

HRESULT Typelib_descr::unregister_server()
{
    HRESULT hr;
    HRESULT hr2 = NOERROR;

    try {
        Ole_class_descr* cls = Ole_class_descr::head;
        while( cls ) {
            if( cls->_creatable ) {
                hr = cls->unregister_class();
                if( FAILED( hr ) )  hr2 = hr;
            }
            cls = cls->_next;
        }

        return hr2;
    }
    catch( const Xc& x ) 
    {
        SHOW_ERR( "Fehler beim Entfernen der Registrierung: " << x );
        return (HRESULT)E_FAIL;
    }
}

#endif
//------------------------------------------------------------------Typelib_descr::register_typelib
#ifdef SYSTEM_HAS_COM

void Typelib_descr::register_typelib()
{
    hostole_register_typelib( _typelib_id, _name, _version, _typelib_filename );
}

#endif
//----------------------------------------------------------------Typelib_descr::unregister_typelib
#ifdef SYSTEM_HAS_COM

void Typelib_descr::unregister_typelib()
{
    hostole_unregister_typelib( _typelib_id, _name, _version );
}

#endif
//-----------------------------------------------------------------Ole_class_descr::Ole_class_descr

Ole_class_descr::Ole_class_descr( Typelib_descr* typelib_descr, const CLSID& clsid, const IID& iid, const char* name, const char* version )
:
    //Sos_ole_object( NULL, this ),
    _zero_(this+1),
    _typelib_descr( typelib_descr ),
    _clsid      ( clsid   ),
    _iid        ( iid     ),
    _name       ( name    ),
    _version    ( version )
{
    //LOG( "Ole_class_descr( " << clsid << ", " << name << " )\n" );
    _next = head;
    head = this;
}

//----------------------------------------------------------------Ole_class_descr::~Ole_class_descr

Ole_class_descr::~Ole_class_descr()
{
#   ifdef SYSTEM_HAS_COM
        if( _typeinfo ) {
            _typeinfo->Release();
            _typeinfo = NULL;
        }
#   endif

    if( head == this ) 
    {
        head = _next;
    } 
    else 
    {
        Ole_class_descr* p = head;
        while( p->_next != this )  p = p->_next;
        p->_next = p->_next->_next;
    }
}

//------------------------------------------------------------------Ole_class_descr::dummy_call

void Ole_class_descr::dummy_call()
{
    // Zum Einbinden
}

//-------------------------------------------------------------------Ole_class_descr::load_typeinfo
#ifdef SYSTEM_HAS_COM

HRESULT Ole_class_descr::load_typeinfo()
{
    HRESULT hr;

    _typelib_descr->load_typelib();

    //LOG( "ITypeLib::GetTypeInfoOfGuid " << _iid );

    hr = _typelib_descr->_typelib->GetTypeInfoOfGuid( _iid, &_typeinfo );

    //if( FAILED( hr ) )  throw_ole( hr, "Typelib::GetTypeInfoOfGuid" );
    if( FAILED( hr ) )  LOG( "  hr=" << hex << hr << dec );
    LOG( '\n' );
    return hr;
}

#endif
//------------------------------------------------------------------Ole_class_descr::register_class
#ifdef SYSTEM_HAS_COM

void Ole_class_descr::register_class()
{
    if( !_typelib_descr )  return;

    hostole_register_class( _typelib_descr->_typelib_id, _clsid, _name, _version, _name );
}

#endif
//----------------------------------------------------------------Ole_class_descr::unregister_class
#ifdef SYSTEM_HAS_COM

HRESULT Ole_class_descr::unregister_class()
{
    if( !_typelib_descr )  return NOERROR;

    return hostole_unregister_class( _typelib_descr->_typelib_id, _clsid, _name, _version );
}

#endif
//-----------------------------------------------------------------Ole_class_descr::create_instance

HRESULT Ole_class_descr::create_instance( IUnknown* pUnkOuter, const IID& iid, IUnknown** result )
{
    if( pUnkOuter )  return (HRESULT)CLASS_E_NOAGGREGATION;

    if( iid == _iid  
     || iid == IID_IUnknown 
     || iid == IID_IDispatch
    )  
    { 
        return create_simple( result );
    }
    else
    {
        IUnknown* o;
        
        HRESULT hr = create_simple( &o );
        if( FAILED(hr) )  return hr;

        return o->QueryInterface( iid, (void**)result );
    }

    return (HRESULT)E_NOINTERFACE;
}

//-------------------------------------------------------------------Ole_class_descr::create_simple

HRESULT Ole_class_descr::create_simple( IUnknown** )
{
    LOG( *this << ".create_simple() ist nicht implementiert\n" );
    return (HRESULT)E_NOINTERFACE;
}

//-------------------------------------------------------------------------Ole_factory::Ole_factory

Ole_factory::Ole_factory()
:
    Sos_ole_object( NULL, this, NULL )
{
}

//------------------------------------------------------------------------Ole_factory::~Ole_factory

Ole_factory::~Ole_factory()
{
}

//---------------------------------------------------------------------------Ole_factory::set_clsid

STDMETHODIMP Ole_factory::set_clsid( const IID& clsid )
{
    HRESULT hr = NOERROR;

    Ole_class_descr* cls = Ole_class_descr::head;
    while(1) 
    {
        if( !cls )  { hr = CLASS_E_CLASSNOTAVAILABLE; goto FEHLER; }
        //LOG( cls->_name << ", " << cls->_clsid << '\n' );
        if( cls->_clsid == clsid )  break;
        cls = cls->_next;
    }

    if( !cls->_creatable )  
    {
        LOG( cls->_clsid << " ist nicht kreierbar\n" );
        hr = CLASS_E_CLASSNOTAVAILABLE;
        goto FEHLER;
    }

    _class = cls;

  FEHLER:
    return hr;
}

//----------------------------------------------------------------------Ole_factory::QueryInterface

STDMETHODIMP Ole_factory::QueryInterface( REFIID riid, void** ppv )
{
    //LOGI( *this << ".QueryInterface " << riid << '\n' );

    *ppv = NULL;

    if( riid == IID_IUnknown  
     || riid == IID_IClassFactory )  *ppv = this;

    if( !*ppv ) 
    {
        LOG( "Ole_factory::QueryInterface(" << riid << ") => E_NOINTERFACE\n" );
        return E_NOINTERFACE;
    }

    ((IUnknown*)*ppv)->AddRef();
    return NOERROR;
}

//---------------------------------------------------------------Ole_factory::register_class_object

void Ole_factory::register_factory()
{
    LOG( "CoRegisterClassObject(" << _class->_clsid << ",,CLSCTX_LOCAL_SERVER,REGCLS_MULTIPLEUSE)\n" );
    
#   ifdef SYSTEM_HAS_COM

        HRESULT hr = CoRegisterClassObject( _class->_clsid,
                                            this,
                                            CLSCTX_LOCAL_SERVER,
                                            REGCLS_MULTIPLEUSE,
                                            &_register_id );

        if( FAILED( hr ) )  throw_ole( hr, "CoRegisterClassObject" );

#   endif
}

//-------------------------------------------------------------Ole_factory::unregister_class_object

HRESULT Ole_factory::unregister_factory()
{
    HRESULT hr = NOERROR;

    if( _register_id )  return hr;

#   ifdef SYSTEM_HAS_COM

        hr = CoRevokeClassObject( _register_id );
        _register_id = 0;

#   endif

    return hr;
}

//----------------------------------------------------------------------Ole_factory::CreateInstance

STDMETHODIMP Ole_factory::CreateInstance( IUnknown* pUnkOuter, REFIID iid, void** ppvObj )
{
    LOGI( "Ole_factory(" << _class->_name << ").CreateInstance " << iid << '\n' );

    IUnknown* obj  = NULL;
    HRESULT   hr   = NOERROR;

    *ppvObj = NULL;

    try 
    {
        hr = _class->create_instance( pUnkOuter, iid, &obj );
        if( FAILED( hr ) )  goto FEHLER;
    }
    catch( const Ole_error& x ) { LOG( "Ole_factory::CreateInstance: " << x );  hr = x._hresult   ; goto FEHLER; }
    catch( const Xc& x        ) { LOG( "Ole_factory::CreateInstance: " << x );  hr = E_NOINTERFACE; goto FEHLER; }


    hr = obj->QueryInterface( iid, ppvObj );
    if( FAILED( hr ) )  goto FEHLER;

    return hr;

  FEHLER:
    LOG( " HRESULT=" << hex << hr << dec << "\n" );
    delete obj;
    return hr;
}

//--------------------------------------------------------------------------Ole_factory::LockServer

STDMETHODIMP Ole_factory::LockServer( BOOL fLock )
{
    if( fLock )  InterlockedIncrement( &com_lock );
           else  InterlockedDecrement( &com_lock );

    return NOERROR;
}

//-------------------------------------------------------------------------------------------------

} //namespace sos
