#include <precomp.h>
#define MODULE_NAME "sosdde"

#include <sysdep.h>
#if defined SYSTEM_WIN


#define __MINMAX_DEFINED        // wg. bc4 stdlib.h:527 (min/max Templates)
#include <stdlib.h>

#include <cstring.h>
#include <except.h>

#define STRICT
#include <windows.h>
#include <ddeml.h>

#include <sos.h>
#include <xception.h>
#include <sosarray.h>
#include <soswin.h>
#include <sosmswin.h>
#include <sosdde.h>
#include <log.h>




#if 0
struct Reference_counted        // ????????????????????
{
                                Reference_counted       () : _reference_count( 1 ) {}

                                operator delete         ()  { if( --_reference_count == 0 )  delete this; }
    void                        incr_reference          ()  { _reference_count++; }

  private:
    int                        _reference_count;
};
#endif


struct Sos_system_dde_conversation : Sos_dde::Conversation, Sos_dde
{
                                Sos_system_dde_conversation( Sos_dde::Server* );

    virtual void                request_event           ( Sos_dde::Hsz item, Area&, UINT format );

  private:
    Sos_dde::String_handle      _system_topic_handle;
    Sos_dde::String_handle      _topics_item_handle;
    Sos_dde::String_handle      _formats_item_handle;
    Sos_dde::String_handle      _sysitems_item_handle;
    Sos_dde::String_handle      _error_item_handle;
};

static FNCALLBACK dde_callback;

//------------------------------------------------------------------------------------------statics

Sos_dde::Ddeml_instance   Sos_dde::_ddeml_instance;
Mswin::Callback_instance  callback_instance   ( (FARPROC) dde_callback );

//-------------------------------------------------------------------------------------dde_callback

static HDDEDATA CALLBACK dde_callback( UINT wType, UINT wFmt, HCONV hConv,
                                       HSZ hsz1, HSZ hsz2, HDDEDATA hData,
                                       DWORD dwData1, DWORD dwData2 )
{
    return Sos_dde::ddeml_instance_ptr()->event( wType, wFmt, hConv,
                                                 hsz1, hsz2,
                                                 hData,
                                                 dwData1, dwData2 );
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Hsz::assign( Ddeml_instance* ddeml_instance_ptr, const char* string )
{
    del();

    _ddeml_instance = *ddeml_instance_ptr;
    _handle         = DdeCreateStringHandle( _ddeml_instance, string, CP_WINANSI );

    assert( _handle );
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

Sos_dde::Hsz::operator string() const
{
    uint size = length() + 1;

    Auto_simple_array_ptr<char> p; p = new char[ size ];

    *p = 0;
    DdeQueryString( _ddeml_instance, _handle, p, size, CP_WINANSI );
    p[ size - 1 ] = 0;

    return string( (const char*) p );
}

//------------------------------------------------------------------------------------------------

const char* Sos_dde::Hsz::c_str() const
{
    return ((string)*this).c_str();
}

//------------------------------------------------------------------------------------------------

uint Sos_dde::Hsz::length() const
{
    return DdeQueryString( _ddeml_instance, _handle, 0, 0, CP_WINANSI );
}

//------------------------------------------------------------------------------------------------

Sos_dde::String_handle::String_handle( Ddeml_instance* ddeml_instance_ptr,
                                       const string& str )
// wegen #include <cstring.h> nicht inline.
:
    _destroy ( true )
  //_destroy ( false )      // wirklich???
{
    assign( ddeml_instance_ptr, str.c_str() );
}

//------------------------------------------------------------------------------------------------

Sos_dde::String_handle::~String_handle()
{
    if( _destroy ) {
        del();
    }
}

//------------------------------------------------------------------------------------------------

Sos_dde::Ddeml_instance::~Ddeml_instance()
{
    if( !_ddeml_instance )  return;

    DdeUninitialize( _ddeml_instance );
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Ddeml_instance::init()
{
    if( _ddeml_instance )  return;

    int  rc;

    rc = DdeInitialize( &_ddeml_instance,
                        (PFNCALLBACK) (FARPROC) callback_instance,
                        APPCLASS_STANDARD        |
                        APPCMD_FILTERINITS       |
                        CBF_FAIL_ADVISES         |
                        CBF_SKIP_REGISTRATIONS   |
                        CBF_SKIP_UNREGISTRATIONS,
                        0L );
    if( rc ) {
        throw Dde_error( rc, "DdeInitialize" );
    }
}

//-------------------------------------------------------------------------------------dde_callback

HDDEDATA Sos_dde::Ddeml_instance::event( UINT wType, UINT wFmt, HCONV hConv,
                                         HSZ hsz1, HSZ hsz2, HDDEDATA hData,
                                         DWORD dwData1, DWORD dwData2 )
{
    if( !ddeml_instance_ptr()->server_ptr() ) {
        LOG( "dde_callback: ddeml_instance_ptr()->server_ptr() ist 0!\n" );
        return (HDDEDATA) 0;
    }

    return ddeml_instance_ptr()->server_ptr()->event(
                                      wType, wFmt, hConv,
                                      Sos_dde::Hsz( ddeml_instance_ptr(), hsz1 ),
                                      Sos_dde::Hsz( ddeml_instance_ptr(), hsz2 ),
                                      hData,
                                      dwData1, dwData2 );
}

//------------------------------------------------------------------------------------------------

Sos_dde::Server::Server( const char* server_name )
:
    _ddeml_instance_ptr ( Sos_dde::ddeml_instance_ptr() )
{
    if( ddeml_instance_ptr()->server_ptr() )  throw Sos_dde_error( 1 );

    _service_name_handle.assign( _ddeml_instance_ptr, server_name );

    HDDEDATA data_handle = DdeNameService( *_ddeml_instance_ptr, _service_name_handle,
                                           NULL, DNS_REGISTER | DNS_FILTERON );
    if( !data_handle )  throw Dde_error( DdeGetLastError( *_ddeml_instance_ptr ), "DdeNameService" );

    add( new Sos_system_dde_conversation( this ));

    ddeml_instance_ptr()->server_ptr( this );
}

//------------------------------------------------------------------------------------------------

Sos_dde::Server::~Server()
{
    ddeml_instance_ptr()->server_ptr( 0 );
    DdeNameService( *_ddeml_instance_ptr, _service_name_handle, NULL, DNS_UNREGISTER | DNS_FILTERON );
    _service_name_handle.del();
}

//-------------------------------------------------------------------------------------------------

#define DDE_LOCK    { Sos_use_lock __use_lock__ ( &_lock );
#define DDE_UNLOCK  }


HDDEDATA Sos_dde::Server::event( UINT type, UINT format, HCONV conv_handle,
                                 Hsz topic_handle, Hsz item_handle, HDDEDATA data_handle,
                                 DWORD data_1, DWORD data_2 )
{ try {
    LOG( "Sos_dde::Server::event( " << hex << type << ", " << format << ", " << (uint*)&conv_handle << ", \"" << item_handle << "\", \"" << topic_handle << "\", ... )\n" );
    if( _lock.locked() ) return (HDDEDATA) DDE_FBUSY; // ???

    switch( type )
    {
    case XTYP_CONNECT: // Aufwachen, jemand klopft an!
        LOG( "XTYP_CONNECT\n" );
        add( connect_event( topic_handle, (const CONVCONTEXT*) data_1, (Bool) data_2 ));
        return (HDDEDATA) TRUE;

  //case XTYP_WILDCONNECT: // Ein ganz Neugieriger...
  //    LOG( "XTYP_WILDCONNECT, _locked " << (int) _locked << "!" << endl );
  //    return (HDDEDATA) NULL;

    case XTYP_CONNECT_CONFIRM:
        LOG( "XTYP_CONNECT_CONFIRM" << endl );
        conv_ptr( topic_handle )->connect_confirm_event( conv_handle );
        return (HDDEDATA) FALSE;  // Keine Rückgabe

  //case XTYP_ADVREQ:
  //    return NULL;

    case XTYP_REQUEST:
    {
        LOG( "XTYP_REQUEST" << endl );

        Dyn_area<char>  area;

        DDE_LOCK
            conv_ptr( conv_handle )->request_event( item_handle, area, format );
        DDE_UNLOCK

        HDDEDATA data_handle = DdeCreateDataHandle( *_ddeml_instance_ptr,
                                                    area.char_ptr(), area.length(), 0,
                                                    item_handle, format, 0 );
        if( !data_handle ) {
            throw Dde_error( DdeGetLastError( *_ddeml_instance_ptr ), "DdeCreateDataHandle" );
        }

        return data_handle;
    }

  //case XTYP_ADVSTART: // Client wünscht Advise!
  //     return (HDDEDATA)FALSE;

  //case XTYP_ADVSTOP: // Advise beenden!
  //     break;

    case XTYP_EXECUTE: // Ein Kommando: aye aye, Sir!
        DDE_LOCK
            conv_ptr( conv_handle )->execute_event( data_handle, format );
            return (HDDEDATA)DDE_FACK;
        DDE_UNLOCK

    case XTYP_POKE:
        DDE_LOCK
            conv_ptr( conv_handle )->poke_event( item_handle, data_handle, format );
            return (HDDEDATA)DDE_FACK;
        DDE_UNLOCK

    case XTYP_DISCONNECT: // Ciao!
    {
        int i = index( conv_handle );
        delete _conv_ptr_array[ i ];  _conv_ptr_array[ i ] = 0;
        break;
    }

    default: ;
    }

    return (HDDEDATA) 0;
}
catch( Xc x ) {
    return (HDDEDATA) DDE_FNOTPROCESSED;
}
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Server::error_event( uint2 code )
{
    LOG( "Sos_dde::Server::error_event(" << code << ") ignoriert.\n" );
    throw Sos_dde::Sos_dde_error( 2 );
}

//-------------------------------------------------------------------------------------------------

Sos_dde::Conversation* Sos_dde::Server::conv_ptr( Hsz topic_handle )
{
    for( int i = _conv_ptr_array.first_index(); i < _conv_ptr_array.last_index(); i++ ) {
        if( _conv_ptr_array[ i ]  &&  _conv_ptr_array[ i ]->_topic_handle == topic_handle )  break;
    }
    if( i > _conv_ptr_array.last_index() )  throw_xc( "Sos_dde::Server::index", "Conversation unbekannt" );

    return _conv_ptr_array[ i ];
}

//-------------------------------------------------------------------------------------------------

int Sos_dde::Server::index( HCONV conv_handle )
{
    for( int i = _conv_ptr_array.first_index(); i < _conv_ptr_array.last_index(); i++ ) {
        if( _conv_ptr_array[ i ]  &&  _conv_ptr_array[ i ]->_handle == conv_handle )  break;
    }
    if( i > _conv_ptr_array.last_index() )  throw_xc( "Sos_dde::Server::index", "Conversation unbekannt" );

    return i;
}

//-------------------------------------------------------------------------------------------------

Sos_dde::Conversation* Sos_dde::Server::conv_ptr( HCONV conv_handle )
{
    return _conv_ptr_array[ index( conv_handle ) ];
}

//------------------------------------------------------------------------------------------------

Sos_dde::Conversation::Conversation( Server* server_ptr, const char* topic )
 :  _server_ptr   ( server_ptr ),
    _topic_handle ( server_ptr->ddeml_instance_ptr(), topic )
{
}

//------------------------------------------------------------------------------------------------

Sos_dde::Conversation::Conversation( Server* server_ptr, Hsz topic )
 :  _server_ptr   ( server_ptr ),
    _topic_handle ( topic )
{
}

//------------------------------------------------------------------------------------------------

Sos_system_dde_conversation::Sos_system_dde_conversation( Sos_dde::Server* dde_server_ptr )
:
    Conversation ( dde_server_ptr, SZDDESYS_TOPIC )
{
    Ddeml_instance* ddeml_instance_ptr = dde_server_ptr->ddeml_instance_ptr();

  //_topics_item_handle  .assign( ddeml_instance_ptr, SZDDESYS_ITEM_TOPICS );
  //_formats_item_handle .assign( ddeml_instance_ptr, SZDDESYS_ITEM_FORMATS );
  //_sysitems_item_handle.assign( ddeml_instance_ptr, SZDDESYS_ITEM_SYSITEMS );
  //_error_item_handle   .assign( ddeml_instance_ptr, "Error" );
}

//------------------------------------------------------------------------------------------------

Sos_dde::Conversation::~Conversation()
{
}

//------------------------------------------------------------------------------------------------

void Sos_system_dde_conversation::request_event( Hsz item_handle, Area& area, UINT format )
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
        throw Sos_dde::Dde_error( 2 );
#endif
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Conversation::connect_confirm_event( HCONV conv_handle )
{
    _handle = conv_handle;
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Conversation::connect_confirm_event()
{
    LOG( "Sos_dde::Conversation::connect_confirm_event() ignoriert.\n" );
    throw Sos_dde::Sos_dde_error( 2 );
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Conversation::execute_event( HDDEDATA, UINT )
{
    LOG( "Sos_dde::Conversation::execute_confirm_event() ignoriert.\n" );
    throw Sos_dde::Sos_dde_error( 2 );
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Conversation::request_event( Hsz, Area&, UINT )
{
    LOG( "Sos_dde::Conversation::request_event() ignoriert.\n" );
    throw Sos_dde::Sos_dde_error( 2 );
}

//------------------------------------------------------------------------------------------------

void Sos_dde::Conversation::poke_event( Hsz, HDDEDATA, UINT )
{
    LOG( "Sos_dde::Conversation::poke_event() ignoriert.\n" );
    throw Sos_dde::Sos_dde_error( 2 );
}

//------------------------------------------------------------------------------------------------

Sos_dde::Server* Sos_dde::Conversation::server_ptr() const
{
    return _server_ptr;
}

