#include "precomp.h"
#if 0
//#define MODULE_NAME "sosdde"

#include <stdio.h>              // sprintf

#if defined __BORLANDC__
#   include <borstrng.h>
#endif

#include "../kram/sosstrng.h"           // Zieht windows.h für ddeml.h ein
#include "../kram/sysdep.h"

#if defined SYSTEM_WIN


#define __MINMAX_DEFINED        // wg. bc4 stdlib.h:527 (min/max Templates)
#include <stdlib.h>
#define __USELOCALES__          // posix
#include <time.h>

#include "sysxcept.h"

#if defined SYSTEM_WIN
#   include <windows.h>
#endif

#include <ddeml.h>

#include "../kram/sos.h"
#include "../kram/xception.h"
#include "../kram/sosarray.h"
#include "../kram/soswin.h"
#include "../kram/sosmswin.h"
#include "../kram/sosstat.h"
#include "../kram/log.h"
#include "../kram/sosdde.h"

using namespace std;
namespace sos {


#define DDE_LOG( TEXT )                                                                     \
{                                                                                           \
    ostream* s = ddeml_instance_ptr()->log();                                               \
    if( s )  *s << TEXT << flush;                                                           \
}

DEFINE_SOS_DELETE_ARRAY( Sos_dde::Conversation* )

//----------------------------------------------------------------------Dde_increment_semaphore

struct Dde_increment_semaphore
{
            Dde_increment_semaphore( int * p );
   virtual ~Dde_increment_semaphore();

  private:
    int* _ptr;
};

Dde_increment_semaphore::Dde_increment_semaphore( int * p ) : _ptr ( p ) {} //{ (*p)++; LOG( "ddelock=" << *p << '\n' ); }
Dde_increment_semaphore::~Dde_increment_semaphore()                      {} // { (*_ptr)--; LOG( "ddelock=" << *_ptr << '\n' ); }

//------------------------------------------------------------------Sos_system_dde_conversation

struct Sos_system_dde_conversation : Sos_dde::Conversation, Sos_dde
{
                                Sos_system_dde_conversation( Sos_dde::Server* );

    virtual Const_area_handle   request_event           ( const Sos_dde::Hsz& item, UINT format );

  private:
    Sos_dde::String_handle      _system_topic_handle;
    Sos_dde::String_handle      _topics_item_handle;
    Sos_dde::String_handle      _formats_item_handle;
    Sos_dde::String_handle      _sysitems_item_handle;
    Sos_dde::String_handle      _error_item_handle;
};

static FNCALLBACK dde_callback;

//--------------------------------------------------------------------------------------statics

//Sos_dde::Ddeml_instance   Sos_dde::_ddeml_instance;
Mswin::Callback_instance  sosdde_callback_instance   ( (FARPROC) dde_callback );

//---------------------------------------------------------------------------------dde_callback

static HDDEDATA CALLBACK dde_callback( UINT wType, UINT wFmt, HCONV hConv,
                                       HSZ hsz1, HSZ hsz2, HDDEDATA hData,
                                       DWORD dwData1, DWORD dwData2 )
{
    return Sos_dde::ddeml_instance_ptr()->event( wType, wFmt, hConv,
                                                 hsz1, hsz2,
                                                 hData,
                                                 dwData1, dwData2 );
}

//------------------------------------------------------------------------ Dde_error::Dde_error
/*
Dde_error::Dde_error( int code, const char* function_name )
:
    Xc ( Msg_code( "MSWIN-DDE-%04X", code ), Msg_insertions ( function_name ) )
{
}

//------------------------------------------------------------------------ Dde_error::Dde_error

Dde_error::Dde_error( const char* function_name )
:
    Xc ( Msg_code( "MSWIN-DDE-%04X", DdeGetLastError( *ddeml_instance_ptr() ) ), Msg_insertions ( function_name ) )
{
}
*/
//------------------------------------------------------------------------------throw_dde_error

void throw_dde_error( int code, const char* dde_function_name, const char* ins1, const char* ins2 )
{
    //Dde_error x ( dde_function_name );
    Xc x ( Msg_code( "MSWIN-DDE-%04X", code ) );
    x.insert( dde_function_name );
    x.insert( ins1 );
    x.insert( ins2 );
    throw x;
}

//------------------------------------------------------------------------------throw_dde_error
/*
void throw_dde_error( const char* dde_function_name )
{
    throw_dde_error( dde_function_name, 0 );
}
*/
//------------------------------------------------------------------------------throw_dde_error

void throw_dde_error( int code, const char* dde_function_name )
{
    //throw Dde_error( code, dde_function_name );
    Xc x ( Msg_code( "MSWIN-DDE-%04X", code ) );
    x.insert( dde_function_name );
    throw x;
}

//-----------------------------------------------------------------------------throw_dde_locked

void throw_sos_dde_locked()
{
    throw Sos_dde_locked();
}

//--------------------------------------------------------------------------throw_sos_dde_error
/*
void throw_sos_dde_error( int code )
{
    throw Sos_dde_error( code );
}
*/
//---------------------------------------------------------------------------Sosdde_format

ostream& operator<< ( ostream& s, HDDEDATA h )
{
    switch( (int)h )
    {
        case DDE_FACK        : s << "FACK"; break;
        case DDE_FBUSY       : s << "FBUSY"; break;
      //case DDE_FDEFERUPD   : s << "FDEFERUPD"; break;
      //case DDE_FACKREQ     : s << "FACKREQ"; break;
        case DDE_FRELEASE    : s << "FRELEASE"; break;
        case DDE_FREQUESTED  : s << "FREQUESTED"; break;
        case DDE_FACKRESERVED: s << "FACKRESERVED"; break;
        case DDE_FADVRESERVED: s << "FADVRESERVED"; break;
        case DDE_FDATRESERVED: s << "FDATRESERVED"; break;
        case DDE_FPOKRESERVED: s << "FPOKRESERVED"; break;
        case DDE_FAPPSTATUS  : s << "FAPPSTATUS"; break;
        case DDE_FNOTPROCESSED: s << "FNOTPROCESSED"; break;
        case TRUE             : s << "TRUE"; break;
        default:
        {
            s << hex << (int)h << dec;
        }
    }
    return s;
}

ostream& operator<< ( ostream& s, const Sosdde_format& f )
{
    char name [ 50+1 ];
    const char* t;

    switch( f._format )
    {
        case CF_TEXT             : t = "TEXT"; break;
        case CF_BITMAP           : t = "BITMAP"; break;
        case CF_METAFILEPICT     : t = "METAFILEPICT"; break;
        case CF_SYLK             : t = "SYLK"; break;
        case CF_DIF              : t = "DIF"; break;
        case CF_TIFF             : t = "TIFF"; break;
        case CF_OEMTEXT          : t = "OEMTEXT"; break;
        case CF_DIB              : t = "DIB"; break;
        case CF_PALETTE          : t = "PALETTE"; break;
        case CF_PENDATA          : t = "PENDATA"; break;
        case CF_RIFF             : t = "RIFF"; break;
        case CF_WAVE             : t = "WAVE"; break;
        case CF_OWNERDISPLAY     : t = "OWNERDISPLAY"; break;
        case CF_DSPTEXT          : t = "DSPTEXT"; break;
        case CF_DSPBITMAP        : t = "DSPBITMAP"; break;
        case CF_DSPMETAFILEPICT  : t = "DSPMETAFILEPICT"; break;
        default:
        {
            int len = GetClipboardFormatName( f._format, name, sizeof name - 1 );
            if( len )  name[ len ] = '\0';
                 else  sprintf( name, "%X", f._format );
            t = name;
        }
    }

    s << t;
    return s;
}
/*
struct Sos_use_lock
{
                                Sos_use_lock            ( Sos_lock* )  throw( Sos_locked );
    virtual                    ~Sos_use_lock            ();

  private:
    Sos_lock*                  _lock_ptr;
};

Sos_use_lock::Sos_use_lock( Sos_lock* lock_ptr )  throw( Sos_locked )
:
    _lock_ptr ( lock_ptr )
{
    if( _lock_ptr->locked() )  throw Sos_locked();
    _lock_ptr->lock();
}

Sos_use_lock::~Sos_use_lock()
{
    _lock_ptr->unlock();
}
*/
//------------------------------------------------------------------------------------------------
/*
Sos_dde::Hsz::Hsz( Ddeml_instance* dde_server_ptr, HSZ hsz )
 :  _ddeml_instance ( *ddeml_instance_ptr() ),
    _handle         ( hsz )
{
}

//------------------------------------------------------------------------------------------------

Bool Sos_dde::Hsz::operator== ( const Sos_dde::Hsz& hsz2 ) const
{
    return _ddeml_instance == hsz2._ddeml_instance
           && _handle == hsz2._handle;
}

//------------------------------------------------------------------------------------------------

Sos_dde::String_handle::String_handle( Ddeml_instance* ddeml_instance_ptr,
                                              const char* string )
 :  _destroy ( true )
{
    assign( ddeml_instance_ptr, string );
}

//------------------------------------------------------------------------------------------------

Sos_dde::String_handle::String_handle( const Hsz& hsz )
 :  Hsz      ( hsz ),
    _destroy ( false )
{
}

//------------------------------------------------------------------------------------------------

Sos_dde::Ddeml_instance::Ddeml_instance()
:
    _ddeml_instance ( 0 ),
    _server_ptr     ( 0 ),
    _log            ( 0 ),
    _locked         ( 0 )
{
}

//------------------------------------------------------------------------------------------------

Sos_dde::Ddeml_instance::operator uint4()
{
    if( !_ddeml_instance ) {
        init();                               // DDE automatisch initiieren
    }
    return _ddeml_instance;
}
*/
//-----------------------------------------------------------------------------make_data_handle

HDDEDATA make_data_handle( Sos_dde::Ddeml_instance* ddeml_instance_ptr, Const_area& record,
                           const Sos_dde::Hsz& item_handle, uint format )
{
    HDDEDATA          data_handle;

    if( format == CF_TEXT
     && ( record.length() == 0  ||  record.char_ptr()[ record.length() - 1 ] != '\0' ) )
    {
        data_handle = DdeCreateDataHandle( *ddeml_instance_ptr, 0,
                                           record.length() + 1, 0,
                                           item_handle, format, 0 );
        data_handle = DdeAddData( data_handle, (Dde_data*)record.ptr(), record.length(), 0 );
        data_handle = DdeAddData( data_handle, (Dde_data*)"\0", 1, record.length() );
    }
    else
    if( format == ddeml_instance_ptr->_format_sos_binary )
    {
        data_handle = DdeCreateDataHandle( *ddeml_instance_ptr, 0,
                                           3 + record.length(), 0,
                                           item_handle, format, 0 );
        Byte l [ 3 ];
        l[ 0 ] = (Byte)( record.length() >> 16 );
        l[ 1 ] = (Byte)( record.length() >> 8 );
        l[ 2 ] = (Byte)  record.length();
        data_handle = DdeAddData( data_handle, l, 3, 0 );
        data_handle = DdeAddData( data_handle, (Dde_data*)record.ptr(), record.length(), 3 );
    }
    else
    {
        data_handle = DdeCreateDataHandle( *ddeml_instance_ptr,
                                           (Dde_data*)record.ptr(), record.length(), 0,
                                           item_handle, format, 0 );
    }

    if( !data_handle )  throw_dde_error( DdeGetLastError( *ddeml_instance_ptr ), "DdeCreateDataHandle" );
    return data_handle;
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Hsz::assign( Ddeml_instance* ddeml_instance_ptr, const char* string )
{
    del();

    _ddeml_instance = *ddeml_instance_ptr;
    _handle         = DdeCreateStringHandle( _ddeml_instance, string, CP_WINANSI );

    if( !_handle ) {
        int e = DdeGetLastError( *ddeml_instance_ptr );
        if( e ) throw_dde_error( e, "DdeCreateStringHandle" );
    }
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Hsz::del()
{
    if( _handle ) {
        BOOL ok = DdeFreeStringHandle( _ddeml_instance, _handle );
        _handle         = 0;
        if( !ok )  SHOW_ERR( "DdeFreeStringHandle() meldet Fehler" );
    }
}

//------------------------------------------------------------------------------------------------

Sos_dde::Hsz::operator Sos_string() const
{
    //uint size = length() + 1;

    char value [ 255+1 ];

    DdeQueryString( _ddeml_instance, _handle, value, sizeof value, CP_WINANSI );
    value[ sizeof value - 1 ] = 0;

    return Sos_string( value );
}

//------------------------------------------------------------------------------------------------
/*
const char* Sos_dde::Hsz::c_str() const
{
    return ((string)*this).c_str();
}
*/
//------------------------------------------------------------------------------------------------

uint Sos_dde::Hsz::length() const
{
    return DdeQueryString( _ddeml_instance, _handle, 0, 0, CP_WINANSI );
}

//------------------------------------------------------------------------------------------------

Sos_dde::String_handle::String_handle( Ddeml_instance* ddeml_instance_ptr,
                                       const Sos_string& str )
// wegen #include <cstring.h> nicht inline.
:
    _destroy ( true )
  //_destroy ( false )      // wirklich???
{
    assign( ddeml_instance_ptr, c_str( str ) );
}

//------------------------------------------------------------------------------------------------

Sos_dde::String_handle::~String_handle()
{
    if( _destroy ) {
        try {
            del();
        }
        catch(...) {}
    }
}

//------------------------------------------------------------------------------------------------

Sos_dde::Ddeml_instance::~Ddeml_instance()
{
    if( !_ddeml_instance )  return;

    //try {
        DdeUninitialize( _ddeml_instance );
        _ddeml_instance = 0;
    //}
    //catch(...) {}
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Ddeml_instance::init()
{
    if( _ddeml_instance )  return;

    int  rc;

    rc = DdeInitialize( &_ddeml_instance,
                        (PFNCALLBACK) (FARPROC) sosdde_callback_instance,
                        APPCLASS_STANDARD        |
                        APPCMD_FILTERINITS       |
                      /*CBF_FAIL_ADVISES         |*/
                        CBF_SKIP_REGISTRATIONS   |
                        CBF_SKIP_UNREGISTRATIONS,
                        0L );
    if( rc ) {
        throw_dde_error( rc, "DdeInitialize" );
    }

    _format_sos_binary = RegisterClipboardFormat( "sos_binary" );
}

//---------------------------------------------------------------Sos_dde::Ddeml_instance::event

HDDEDATA Sos_dde::Ddeml_instance::event( UINT wType, UINT wFmt, HCONV hConv,
                                         HSZ hsz1, HSZ hsz2, HDDEDATA hData,
                                         DWORD dwData1, DWORD dwData2 )
{
  try {
    return event( wType, wFmt, hConv,
                  Sos_dde::Hsz( ddeml_instance_ptr(), hsz1 ),
                  Sos_dde::Hsz( ddeml_instance_ptr(), hsz2 ),
                  hData, dwData1, dwData2 );
  }
  catch( const Xc& x )
  {
    LOG( "SOSDDE: Zu spät abgefangene Exception: " << x << '\n' );
    DDE_LOG( "SOSDDE: Zu spät abgefangene Exception: " << x << '\n' );
    return (HDDEDATA)DDE_FNOTPROCESSED;
  }
  catch( const xmsg& x )
  {
    LOG( "SOSDDE: Zu spät abgefangene Exception: " << exception_text( x ) << '\n' );
    DDE_LOG( "SOSDDE: Zu spät abgefangene Exception: " << exception_text( x ) << '\n' );
    return (HDDEDATA)DDE_FNOTPROCESSED;
  }
  catch( ... )
  {
#   if defined SYSTEM_BORLAND    
        LOG( "SOSDDE: Zu spät abgefangene unbekannte Exception " << __throwExceptionName << ' ' );
        DDE_LOG( "SOSDDE: Zu spät abgefangene unbekannte Exception " << __throwExceptionName << ' ' );
        if( __throwFileName  &&  __throwFileName[ 0 ] ) {
            const char* f = __throwFileName + strlen( __throwFileName ) - 1;
            while( f >= __throwFileName  &&  *f != '/'  &&  *f != '\\'  &&  *f != ':' )  f--;
            LOG( " [" << (f+1) << '/' << __throwLineNumber << ']' << '\n' );
            DDE_LOG( " [" << (f+1) << '/' << __throwLineNumber << ']' << '\n' );
        }
#   else
        LOG( "SOSDDE: Zu spät abgefangene unbekannte Exception\n" );
        DDE_LOG( "SOSDDE: Zu spät abgefangene unbekannte Exception\n" );
#   endif

    return (HDDEDATA)DDE_FNOTPROCESSED;
  }
}

//---------------------------------------------------------------Sos_dde::Ddeml_instance::event

HDDEDATA Sos_dde::Ddeml_instance::event( UINT wType, UINT wFmt, HCONV hConv,
                                         const Hsz& hsz1, const Hsz& hsz2, HDDEDATA hData,
                                         DWORD dwData1, DWORD dwData2 )
{
    HDDEDATA ret;

    const char* log_string;
    switch( wType )
    {
        case XTYP_CONNECT        : log_string = "XTYP_CONNECT"; break;
        case XTYP_WILDCONNECT    : log_string = "XTYP_WILDCONNECT"; break;
        case XTYP_CONNECT_CONFIRM: log_string = "XTYP_CONNECT_CONFIRM"; break;
        case XTYP_ADVREQ:          log_string = "XTYP_ADVREQ"; break;
        case XTYP_REQUEST:         log_string = "XTYP_REQUEST"; break;
        case XTYP_ADVSTART:        log_string = "XTYP_ADVSTART"; break;
        case XTYP_ADVSTOP:         log_string = "XTYP_ADVSTOP"; break;
        case XTYP_EXECUTE:         log_string = "XTYP_EXECUTE"; break;
        case XTYP_POKE:            log_string = "XTYP_POKE"; break;
        case XTYP_DISCONNECT:      log_string = "XTYP_DISCONNECT"; break;
        case XTYP_XACT_COMPLETE:   log_string = "XTYP_XACT_COMPLETE"; break;
        default:                   log_string = "XTYP_unknown??";
    }

    char time_buffer[ 9+1 ]; time_t t = time(0);
    strftime( time_buffer, 9+1, "%T ", localtime( &t ) );
    LOGI( time_buffer << log_string << " conv=" << (uint*)hConv << ", format=" << Sosdde_format( wFmt ) << ", \"" << Sos_string( hsz1 ) << "\", \"" << Sos_string( hsz2 ) << "\"\n" << dec );
    DDE_LOG( time_buffer << log_string << " conv=" << (uint*)hConv << ", format=" << Sosdde_format( wFmt ) << ", \"" << Sos_string( hsz1 ) << "\", \"" << Sos_string( hsz2 ) << "\"\n" << dec );

    if( wType == XTYP_CONNECT  ||  wType == XTYP_WILDCONNECT  ||  wType == XTYP_DISCONNECT )
    {
        if( !ddeml_instance_ptr()->server_ptr() ) {
            LOG( "dde_callback: ddeml_instance_ptr()->server_ptr() ist 0!\n" );
            ret = (HDDEDATA) 0;
        }
        else {
            ret = ddeml_instance_ptr()->server_ptr()->event( wType, wFmt, hConv, hsz1, hsz2,
                                                             hData, dwData1, dwData2 );
        }
    }
    else
    if( wType == XTYP_CONNECT_CONFIRM )
    {
        ret = ddeml_instance_ptr()->conv_ptr( hsz1 )->event( wType, wFmt, hConv, hsz1, hsz2,
                                                             hData, dwData1, dwData2 );
    }
    else
    {
        ret =  ddeml_instance_ptr()->conv_ptr( hConv )->event( wType, wFmt, hConv, hsz1, hsz2,
                                                               hData, dwData1, dwData2 );
    }

    LOG( "DDE-Server liefert " << ret << '\n' );
    return ret;
}

//-----------------------------------------------------------------Sos_dde::Ddeml_instance::add

void Sos_dde::Ddeml_instance::add( Conversation* conv_ptr )
{
    //_conv_ptr_array.add( conv_ptr );
    for( int i = _conv_ptr_array.first_index(); i <= _conv_ptr_array.last_index(); i++ )
    {
        if( !_conv_ptr_array[ i ] )  break;
    }
    if( i > _conv_ptr_array.last_index() )  _conv_ptr_array.last_index( i );

    _conv_ptr_array[ i ] = conv_ptr;
}

//-----------------------------------------------------------------Sos_dde::Ddeml_instance::del
/*
void Sos_dde::Ddeml_instance::remove( HCONV conv_handle )
{
    Conversation*& c = _conv_ptr_array[ index( conv_handle ) ];
    DELETE( c );
}
*/
//------------------------------------------------------------Sos_dde::Ddeml_instance::conv_ptr

Sos_dde::Conversation* Sos_dde::Ddeml_instance::conv_ptr( const Hsz& topic_handle )
{
    for( int i = _conv_ptr_array.first_index(); i <= _conv_ptr_array.last_index(); i++ )
    {
        if( _conv_ptr_array[ i ]
         && _conv_ptr_array[ i ]->_topic_handle == topic_handle
         && _conv_ptr_array[ i ]->_handle == 0 )  break;
    }
    if( i > _conv_ptr_array.last_index() )  throw_xc( "SOS-DDE-003" );//sos_dde_error( 3 );

    return _conv_ptr_array[ i ];
}

//---------------------------------------------------------------Sos_dde::Ddeml_instance::index

int Sos_dde::Ddeml_instance::index( HCONV conv_handle )
{
    for( int i = _conv_ptr_array.first_index(); i <= _conv_ptr_array.last_index(); i++ ) {
        if( _conv_ptr_array[ i ]  &&  _conv_ptr_array[ i ]->_handle == conv_handle )  break;
    }
    if( i > _conv_ptr_array.last_index() )  throw_xc( "SOS-DDE-004" );//throw_sos_dde_error( 4 );

    return i;
}

//------------------------------------------------------------Sos_dde::Ddeml_instance::conv_ptr

Sos_dde::Conversation*& Sos_dde::Ddeml_instance::conv_ptr( HCONV conv_handle )
{
    return _conv_ptr_array[ index( conv_handle ) ];
}

//----------------------------------------------------------------------Sos_dde::Server::Server

Sos_dde::Server::Server( const char* server_name )
:
    _ddeml_instance_ptr ( Sos_dde::ddeml_instance_ptr() )
{
    if( _ddeml_instance_ptr->server_ptr() )  throw_xc( "SOS-DDE-001" );//throw_sos_dde_error( 1 );

    _service_name_handle.assign( _ddeml_instance_ptr, server_name );

    HDDEDATA data_handle = DdeNameService( *_ddeml_instance_ptr, _service_name_handle,
                                           NULL, DNS_REGISTER | DNS_FILTERON );
    if( !data_handle )  throw_dde_error( DdeGetLastError( *_ddeml_instance_ptr ), "DdeNameService" );

    new Sos_system_dde_conversation( this );    // meldet sich bei ddeml_instance an
    ddeml_instance_ptr()->server_ptr( this );
}

//---------------------------------------------------------------------Sos_dde::Server::~Server

Sos_dde::Server::~Server()
{
    if( ddeml_instance_ptr()  &&  _ddeml_instance_ptr ) {
        try {
            ddeml_instance_ptr()->server_ptr( 0 );
            DdeNameService( *_ddeml_instance_ptr, _service_name_handle, NULL, DNS_UNREGISTER | DNS_FILTERON );
            _ddeml_instance_ptr = NULL;
          //delete ddeml_instance_ptr()->conv_ptr( _service_name_handle );
        }
        catch( const Xc& ) {}
    }
}

//-----------------------------------------------------------------------Sos_dde::Server::event

//#define DDE_LOCK    { Sos_use_lock __use_lock__ ( &ddeml_instance_ptr()->_lock );
//#define DDE_UNLOCK  }

HDDEDATA Sos_dde::Server::event( UINT type, UINT format, HCONV conv_handle,
                                 const Hsz& topic_handle, const Hsz& item_handle, HDDEDATA data_handle,
                                 DWORD data_1, DWORD data_2 )
{
    Bool locking = false;
    HDDEDATA ret = 0;

    try {
    try {

        switch( type )
        {
        case XTYP_CONNECT: // Aufwachen, jemand klopft an!
        {
            if( ddeml_instance_ptr()->_locked )  throw_sos_dde_locked();
            ddeml_instance_ptr()->_locked++; locking = true;

            Conversation* c = xtyp_connect( topic_handle, (const CONVCONTEXT*) data_1, data_2 != 0 );
            ddeml_instance_ptr()->add( c );
            ret = (HDDEDATA) TRUE;
            break;
        }

        case XTYP_WILDCONNECT: // Ein ganz Neugieriger...
            //LOG( "XTYP_WILDCONNECT\n" );
            break;

        case XTYP_DISCONNECT:
        {
            //if( ddeml_instance_ptr()->_locked )  throw_sos_dde_locked();
            ddeml_instance_ptr()->_locked++; locking = true;

            Conversation*& c = ddeml_instance_ptr()->conv_ptr( conv_handle );
            try {
                try {
                    c->xtyp_disconnect();
                }
                CATCH_AND_THROW_XC
            }
            catch( const Xc& x ) {
                LOG_ERR( "XTYP_DISCONNECT: " << x << '\n' );
            }
            catch(...) {}

            //ddeml_instance_ptr()->remove( conv_handle );
            SOS_DELETE( c );
            break;
        }

        default: ;
        }
    }
    catch( const Sos_dde_locked& x )
    {
        LOG( x << '\n' );
        DDE_LOG( "    " << x << '\n' );
    }
    CATCH_AND_THROW_XC
    }
    catch( const Xc& x ) {
        LOG( x << '\n' );
        DDE_LOG( "    " << x << '\n' );
        ret = DDE_FNOTPROCESSED;
    }

    if( locking ) {
        ddeml_instance_ptr()->_locked--;
    }

    return ret;
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Server::error_event( uint2 code )
{
    LOG( "Sos_dde::Server::error_event(" << code << ") ignoriert.\n" );
    throw_xc( "SOS-DDE-002" ); //throw_sos_dde_error( 2 );
}

//----------------------------------------------------------Sos_dde::Conversation::Conversation

Sos_dde::Conversation::Conversation()
:
    _server_ptr   ( 0 ),
    _handle       ( 0 ),
    _xc           ( 0 )
{
    //ddeml_instance_ptr()->add( this );
}

//----------------------------------------------------------Sos_dde::Conversation::Conversation

Sos_dde::Conversation::Conversation( Server* server_ptr, const char* topic )
:
    _server_ptr   ( server_ptr ),
    _topic_handle ( server_ptr->ddeml_instance_ptr(), topic ),
    _handle       ( 0 ),
    _xc           ( 0 )
{
    //ddeml_instance_ptr()->add( this );
}

//----------------------------------------------------------Sos_dde::Conversation::Conversation

Sos_dde::Conversation::Conversation( Server* server_ptr, const Hsz& topic )
:
    _server_ptr   ( server_ptr ),
    _topic_handle ( topic ),
    _handle       ( 0 ),
    _xc           ( 0 )
{
    //ddeml_instance_ptr()->add( this );
}

//-------------------------------------Sos_system_dde_conversation::Sos_system_dde_conversation

Sos_system_dde_conversation::Sos_system_dde_conversation( Sos_dde::Server* dde_server_ptr )
:
    Conversation ( dde_server_ptr, SZDDESYS_TOPIC )
{
  //_topics_item_handle  .assign( ddeml_instance_ptr, SZDDESYS_ITEM_TOPICS );
  //_formats_item_handle .assign( ddeml_instance_ptr, SZDDESYS_ITEM_FORMATS );
  //_sysitems_item_handle.assign( ddeml_instance_ptr, SZDDESYS_ITEM_SYSITEMS );
  //_error_item_handle   .assign( ddeml_instance_ptr, "Error" );
}

//---------------------------------------------------------Sos_dde::Conversation::~Conversation

Sos_dde::Conversation::~Conversation()
{
    delete _xc;
}

//-----------------------------------------------------------------Sos_dde::Conversation::event

HDDEDATA Sos_dde::Conversation::event( UINT type, UINT format, HCONV conv_handle,
                                       const Hsz& topic_handle, const Hsz& item_handle, HDDEDATA data_handle,
                                       DWORD data_1, DWORD data_2 )
{
Bool locking = false;
HDDEDATA ret = 0;
Bool delete_error = true;

try {
    switch( type )
    {
    case XTYP_CONNECT_CONFIRM:
    {
        _handle = conv_handle;
        xtyp_connect_confirm();
        ret = (HDDEDATA) FALSE;  // Keine Rückgabe
        break;
    }

    case XTYP_REQUEST:
    {
        Const_area_handle record;

        if( ddeml_instance_ptr()->_locked )  { ret = (HDDEDATA)DDE_FBUSY; throw_sos_dde_locked(); }
        ddeml_instance_ptr()->_locked++; locking = true;
        Sos_string item = item_handle;

        if( strcmpi( c_str( item ), "*dde*error" ) == 0 ) {
            Dynamic_area e;
            if( _xc )  e.assign( _xc->code() );
            record = e;
            delete_error = false;
        }
        else
        if( strcmpi( c_str( item ), "*dde*errortext" ) == 0 ) {
            Dynamic_area e ( 200 );
            if( _xc ) {
                ostrstream s ( e.char_ptr(), e.size() );
                s << *_xc << '\0';
                e.length( s.pcount() );
                delete_error = false;
            }
            record = e;
        }
        else
        {
            xtyp_request( &record, item_handle, format );
        }

        ret = make_data_handle( ddeml_instance_ptr(), record, item_handle, format );
        break;
    }

    case XTYP_ADVSTART:
    {
        if( ddeml_instance_ptr()->_locked )  throw_sos_dde_locked();
        ddeml_instance_ptr()->_locked++; locking = true;

        xtyp_advstart( item_handle, format );

        ret = (HDDEDATA)1;
        break;
    }

    case XTYP_ADVSTOP:
    {
        xtyp_advstop( item_handle );
        ret = (HDDEDATA)1;
        break;
    }

    case XTYP_ADVREQ:
    {
        if( ddeml_instance_ptr()->_locked )  throw_sos_dde_locked();
        ddeml_instance_ptr()->_locked++; locking = true;
        Const_area_handle record;

        xtyp_advreq( &record, item_handle, format, LOWORD( data_1 ) );

        ret = make_data_handle( ddeml_instance_ptr(), record, item_handle, format );
        break;
    }

    case XTYP_EXECUTE: // Ein Kommando: aye aye, Sir!
    {
        if( ddeml_instance_ptr()->_locked )  { ret = (HDDEDATA)DDE_FBUSY; throw_sos_dde_locked(); }
        ddeml_instance_ptr()->_locked++; locking = true;

        xtyp_execute( data_handle, format );

        ret = (HDDEDATA)DDE_FACK;
        break;
    }

    case XTYP_POKE:
    {
        if( ddeml_instance_ptr()->_locked )  { ret = (HDDEDATA)DDE_FBUSY; throw_sos_dde_locked(); }
        ddeml_instance_ptr()->_locked++; locking = true;

        xtyp_poke( item_handle, data_handle, format );
        ret = (HDDEDATA)DDE_FACK;
        break;
    }

    case XTYP_XACT_COMPLETE:
    {
        xtyp_xact_complete( item_handle, data_handle, format, data_1 );
        ret = (HDDEDATA)0;
        break;
    }

    default: ;
    }
}
catch( const Sos_dde_locked& x )
{
    LOG( x << '\n' );
    DDE_LOG( "    " << x << '\n' );
}
catch( const Xc& x )
{
    LOG( x << '\n' );
    DDE_LOG( "    " << x << '\n' );

    if( type == XTYP_REQUEST  &&  error_value() != "" )
    {
        char buffer [ 256 ];
        Area error_text ( buffer, sizeof buffer );
        ostrstream s ( error_text.char_ptr(), error_text.size() );
        s << error_value() << ' ';
        if( empty( x.name() ) )  s << ' ';  else s << x.name();
        s << ' ';
        x.print_text( s );
        s << '\0'; buffer[ sizeof buffer - 1 ] = '\0';
        error_text.length( strlen( error_text.char_ptr() ) );
        ret = make_data_handle( ddeml_instance_ptr(), error_text, item_handle, format );
    } else {
        ret = DDE_FNOTPROCESSED;
    }

    delete _xc; _xc = 0;
    _xc = new Xc( x );
    delete_error = false;
}
catch( const xmsg& x )
{
    LOG( "xmsg: " << exception_text( x ) << '\n' );
    DDE_LOG( "    xmsg: " << exception_text( x ) << '\n' );
    delete _xc; _xc = 0;
    _xc = new Xc( x );
    delete_error = false;
    ret = DDE_FNOTPROCESSED;
}

if( delete_error )  { delete _xc; _xc = 0; }


if( locking )  ddeml_instance_ptr()->_locked--;

if( ret == DDE_FNOTPROCESSED  &&  ignore_error() )  ret = (HDDEDATA)DDE_FACK; //???

return ret;
}

//------------------------------------------------------------------------------------------------

Const_area_handle Sos_system_dde_conversation::request_event( const Hsz& item_handle, UINT format )
{
#if 0
    if( DdeCmpStringHandles( hszItem, aTopicsItem ) == 0 )
        sprintf( lpsz, "%s\t<File>\n", SZDDESYS_TOPIC);
    else if (DdeCmpStringHandles(hszItem,aFormatsItem)==0)
        sprintf(lpsz,"Text\n");
    else if (DdeCmpStringHandles(hszItem,aSysItemsItem)==0)
        sprintf(lpsz,"%s\t%s\t%s\t%s\n", SZDDESYS_ITEM_TOPICS, SZDDESYS_ITEM_FORMATS,
                                         SZDDESYS_ITEM_SYSITEMS, "Error" );
    else if (DdeCmpStringHandles(hszItem,aErrorItem)==0)
        sprintf(lpsz,"!ERR:%s! %s", _XC.error_code(), _XC.name() );
    else
        throw_sos_dde_error( 2 );
#endif
    return Const_area();
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Conversation::xtyp_connect_confirm()
{
    //LOG( "XTYP_CONNECT_CONFIRM nicht implementiert\n" );
    //throw Sos_dde::Sos_dde_error( 2 );
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Conversation::xtyp_execute( HDDEDATA, UINT )
{
    LOG( "XTYP_EXECUTE nicht implementiert\n" );
    throw_xc( "SOS-DDE-002", "EXECUTE" );  //throw_sos_dde_error( 2 );
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Conversation::xtyp_request( Const_area_handle*, const Hsz&, UINT )
{
    LOG( "XTYP_REQUEST nicht implementiert\n" );

    throw_xc( "SOS-DDE-002", "REQUEST" );  //throw_sos_dde_error( 2 );
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Conversation::xtyp_advstart( const Hsz&, UINT )
{
    LOG( "XTYP_ADVSTART nicht implementiert\n" );

    throw_xc( "SOS-DDE-002", "ADVSTART" );  //throw_sos_dde_error( 2 );
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Conversation::xtyp_advstop( const Hsz& )
{
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Conversation::xtyp_advreq( Const_area_handle*, const Hsz&, uint, uint )
{
    LOG( "XTYP_ADVREQ nicht implementiert\n" );

    throw_xc( "SOS-DDE-002", "ADVREQ" );  //throw_sos_dde_error( 2 );
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Conversation::xtyp_poke( const Hsz&, HDDEDATA, UINT )
{
    LOG( "XTYP_POKE nicht implementiert" );
    throw_xc( "SOS-DDE-002", "POKE" );  //throw_sos_dde_error( 2 );
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Conversation::xtyp_xact_complete( const Hsz&, HDDEDATA, uint, int4 )
{
    LOG( "XTYP_XACT_COMPLETE nicht implementiert\n" );
    throw_xc( "SOS-DDE-002", "XACT_COMPLETE" );  //throw_sos_dde_error( 2 );
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Conversation::xtyp_advdata( const Hsz&, HDDEDATA, uint )
{
    LOG( "XTYP_ADVDATA nicht implementiert\n" );
    throw_xc( "SOS-DDE-002", "ADVDATA" );  //throw_sos_dde_error( 2 );
}

//------------------------------------------------------------------------------------------------

Sos_dde::Server* Sos_dde::Conversation::server_ptr() const
{
    return _server_ptr;
}

//----------------------------------------------------------------------Sos_dde::ddeml_instance_ptr

struct Ddeml_instance_2 : Sos_dde::Ddeml_instance {}; // MSVC++ Fehler bei Sos_ptr<Class::Class2>

Sos_dde::Ddeml_instance* Sos_dde::ddeml_instance_ptr()
{
    //return &_ddeml_instance;            // static!
#   if defined __BORLANDC__
        TYPED_AUTO_PTR( Ddeml_instance, sos_static_ptr()->_ddeml_instance_ptr );
#   else
        if( !sos_static_ptr()->_ddeml_instance_ptr ) {
#           if defined SYSTEM_MICROSOFT
                Sos_ptr<Ddeml_instance_2> p = SOS_NEW( Ddeml_instance_2 );
#           else
                Sos_ptr<Ddeml_instance> p = SOS_NEW( Ddeml_instance );
#           endif
            sos_static_ptr()->_ddeml_instance_ptr = +p;
        }
#   endif
    return (Ddeml_instance*) +sos_static_ptr()->_ddeml_instance_ptr;
}

} //namespace sos

#endif
#endif
