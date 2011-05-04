//#define MODULE_NAME "filedde"
/*                                                  (c) Sos GmbH Berlin
*/

#include "precomp.h"
#if 0
#include <stdio.h>

#include "../kram/sysdep.h"
#if defined SYSTEM_WIN

#include <stdlib.h>

#if defined __WIN32__
//#   if defined SYSTEM_MICROSOFT
//#       include <afx.h>
//#    else
#       include <windows.h>
//#   endif
#endif

#include <ddeml.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/log.h"
#include "../kram/sosmswin.h"
#include "../kram/sosdde.h"
#include "../kram/sosopt.h"
#include "../file/absfile.h"
#include "../file/filedde.h"

#ifdef JZ_TEST
//#   include <e370file.h>
#endif

namespace sos {

//-------------------------------------------------------------------------------------------------

struct File_dde_server_impl : Sos_dde::Server, Sos_dde
{
                                File_dde_server_impl    ( const char* name = "Sos_file_dde_server" );
  //                           ~File_dde_server_impl    ();

    //DECLARE_PUBLIC_MEMBER( ostream*, log_stream_ptr )
    void                        log_stream_ptr          ( ostream* );
    DECLARE_PUBLIC_MEMBER( Bool    , log            )

  protected:
    virtual Conversation*       xtyp_connect            ( const Hsz&, const CONVCONTEXT FAR*, Bool same_instance );
};


struct File_dde_conversation : Sos_dde::Conversation, Sos_dde
{
                                File_dde_conversation   ( File_dde_server_impl*, const Hsz& topic );
                               ~File_dde_conversation   ();

    virtual void                xtyp_disconnect         ();
    virtual void                xtyp_execute            ( HDDEDATA command, UINT format );
    virtual void                xtyp_request            ( Const_area_handle*, const Hsz&, UINT format );
    virtual void                xtyp_advstart           ( const Hsz& item, uint format );
    virtual void                xtyp_advstop            ( const Hsz& item );
    virtual void                xtyp_advreq             ( Const_area_handle*, const Hsz& item, uint format, uint );
    virtual void                xtyp_poke               ( const Hsz&, HDDEDATA data, UINT format );
    virtual Bool                ignore_error            ()     { return _ignore_error; }
    virtual Sos_string          error_value             ()     { return _error_value; }

  private:
    Fill_zero                  _zero_;
    File_dde_server_impl*      _server_ptr;
    Any_file                   _file;
    Sos_string                 _current_item;
    Const_area_handle          _current_record;
    Sos_string                 _advise_item;
    Const_area_handle          _advise_record;
    Bool                       _advise_active;
    Bool                       _current_item_valid;
    Bool                       _delphi;
    Bool                       _open_with_poke;
    Bool                       _ignore_error;
    int4                       _current_record_no;
    Sos_string                 _error_value;
};

//-------------------------------------------------------------------------------------------------

File_dde_server::File_dde_server( const char* name )
:
    _file_dde_server_impl_ptr ( new File_dde_server_impl( name ))
{
}

//-------------------------------------------------------------------------------------------------

File_dde_server::~File_dde_server()
{
    delete _file_dde_server_impl_ptr;
}

//-------------------------------------------------------------------------------------------------

void File_dde_server::log_stream_ptr( ostream* s )
{
    _file_dde_server_impl_ptr->log_stream_ptr( s );
}

//-------------------------------------------------------------------------------------------------

void File_dde_server::log( Bool value )
{
    _file_dde_server_impl_ptr->log( value );
}

//=================================================================================================

File_dde_server_impl::File_dde_server_impl( const char* name )
:
  //_log_stream_ptr ( 0 ),
    _log            ( false ),
    Server          ( name )
{
}

//-------------------------------------------------------------------------------------------------

void File_dde_server_impl::log_stream_ptr( ostream* s )
{
    Sos_dde::Server::ddeml_instance_ptr()->log( s );
}

//-------------------------------------------------------------------------------------------------

Sos_dde::Conversation* File_dde_server_impl::xtyp_connect( const Hsz& topic, const CONVCONTEXT FAR*,
                                                           Bool /*same_instance*/ )
{
    return new File_dde_conversation( this, topic );
}

//-------------------------------------------------------------------------------------------------

File_dde_conversation::File_dde_conversation( File_dde_server_impl* server_ptr, const Hsz& topic )
:
    Sos_dde::Conversation ( server_ptr, topic ),
    _zero_                ( this+1            ),
    _server_ptr           ( server_ptr        )
{
    Sos_string topic_string = topic;
    Sos_option_iterator o ( c_str( topic_string ) );
    for( ; !o.end(); o.next() )
    {
        if( o.with_value( "dde-error"    ) )  _error_value = o.value();
        else
        if( o.flag( "dde-ignore-error"   ) )  _ignore_error = true;
        else
        if( o.flag( "dde-open-with-poke" ) )  _open_with_poke = true;
        else break;
    }

    Sos_string filename = o.rest();
/*
    if( _server_ptr->_xxxx ) {
        // [hostdde shortcuts]
        // filmadr=select $TOPIC from filmadr where helpkey = $ITEM
    }
*/
    if( memcmp( c_str( filename ), "(delphi)", 8 ) == 0 )
    {
        filename = as_string( c_str( filename ) + 8 );
        _delphi = true;
    }

    if( empty( filename ) ) {
        filename = "file_as_key:";
    }

    if( !_open_with_poke ) {
        LOG( "File_dde_conversation::File_dde_conversation _file.open(\"" << filename << "\")\n" );
        _file.open( filename, Abs_file_base::inout );
    }
}

//-------------------------------------------------------------------------------------------------

File_dde_conversation::~File_dde_conversation()
{
}

//-------------------------------------------------------------------------------------------------

void File_dde_conversation::xtyp_disconnect()
{
    _file.close( close_error );   // Achtung: Evtl. Exception im Destruktor!
}

//-------------------------------------------------------------------------------------------------

void File_dde_conversation::xtyp_request( Const_area_handle* handle_ptr, const Hsz& item, UINT format )
{
    if( format && format != CF_TEXT )  throw_xc( "SOS-DDE-004" );

    Sos_string item_name = item;

    if( _advise_active  &&  item_name == _advise_item )
    {                                           // REQUEST folgt auf ADVSTART
        *handle_ptr = _advise_record;
    }
    else
    {
        Dynamic_area record;

        if( _current_item_valid  &&  item_name == _current_item )  {
            *handle_ptr = _current_record;
            return;
        }

        if( strcmpi( c_str( item_name ), "*dde*close" ) == 0 ) {
            _file.close( close_error );
            *handle_ptr = record;
        }
        else
        if( strncmpi( c_str( item_name ), "*dde*record#", 8 ) == 0
         || empty( item_name )
         || ( length( item_name ) == 1  &&  item_name[ 0 ] == ' ' ) /*Visual Basic 3.0*/ )
        {
            if( !_delphi ) {
                _file.get( &record );
            }
            else
            try {
                _file.get( &record );
            }
            catch( const Eof_error& )
            {
                record.assign( "(EOF)" );
            }
            _current_record_no++;
            char buffer [ 20 ]; sprintf( buffer, "*dde*record#%ld", (long)_current_record_no );
            _current_item = buffer;
            _current_item_valid = true;
        }
        else
        {
          /*if( _server_ptr->_is_shortcut ) {
                // [hostdde shortcuts]
                // filmadr=select $TOPIC from filmadr where helpkey = $ITEM
                Sos_string filename = "select " + _topic + " from filmadr where helpkey = $ITEM
                _file.open(
            } else*/ {
                _file.get_key( &record, Key( c_str( item_name ) ));
                _current_item = item_name;
                _current_item_valid = true;
            }
        }

        _current_record = record;
        *handle_ptr = record;
    }
}

//------------------------------------------------------------------------------------------------

void File_dde_conversation::xtyp_advstart( const Hsz& item, uint format )
{
    if( format && format != CF_TEXT )  throw_xc( "SOS-DDE-004" );

    if( _advise_active )  throw_xc( "DOUBLE_ADVISE" );

    xtyp_request( &_advise_record, item, format );

    _advise_active = true;
    _advise_item = item;

    DdePostAdvise( *ddeml_instance_ptr(), _topic_handle, item );
}

//------------------------------------------------------------------------------------------------

void File_dde_conversation::xtyp_advstop( const Hsz& )
{
    _advise_active = false;
    _advise_item = "";
}

//------------------------------------------------------------------------------------------------

void File_dde_conversation::xtyp_advreq( Const_area_handle* handle_ptr, const Hsz& item, UINT format, uint )
{
    if( format && format != CF_TEXT )  throw_xc( "SOS-DDE-004" );

    *handle_ptr = _advise_record;
}

//-------------------------------------------------------------------------------------------------

void File_dde_conversation::xtyp_execute( HDDEDATA data_handle, UINT format )
{
    if( format && format != CF_TEXT )  throw_xc( "SOS-DDE-004" );

    // Für Delphi -> e370.film, bei poke wartet Delphi die Quittung nicht ab
    xtyp_poke( Hsz( server_ptr()->ddeml_instance_ptr(), HSZ(0) ), data_handle, format );
}

//-------------------------------------------------------------------------------------------------

void File_dde_conversation::xtyp_poke( const Hsz& item, HDDEDATA data_handle, UINT format )
{
    if( format && format != CF_TEXT )  throw_xc( "SOS-DDE-004" );

    Sos_string item_name = item;
    DWORD      len;
    BYTE*      p = DdeAccessData( data_handle, &len );
    if( !p )  throw_dde_error( DdeGetLastError( *ddeml_instance_ptr() ), "DdeAccessData" ); //throw_dde_error( "DdeAccessData" );

    if( format == 0  &&  len > 0 ) {    // was ist format == 0?  0-terminierter String wird angenommen. jz 11.4.97
        const Byte* z = (const Byte*) memchr( p, '\0', len );
        if( z )  len = z - p;
    }
    else
    if( format == CF_TEXT  &&  len > 0  /*&&  p[ len - 1 ] == '\0'*/ ) {
        //len--; // 0-Byte entfernen
        len = strlen( (char*)p );    // Hinter dem 0-Byte kann Schmutz stehen (?)
        LOG( "File_dde_conversation::xtyp_poke " << p << '\n' );
    }
    else
    if( format == server_ptr()->ddeml_instance_ptr()->_format_sos_binary ) {
        uint l = ( (uint4)p[0] << 16 )  |  ( (uint4)p[1] << 8 )  |  (uint4)p[2];
        if( l > len )  throw_xc( "SOS-1138" );
        len = l;
        p += 3;
    }

    if( _open_with_poke ) {
        Sos_string filename = as_string( (const char*)p, len );
        LOG( "File_dde_conversation::xtyp_poke _file.open(\"" << filename << "\")\n" );
        _file.open( filename, Abs_file_base::inout );
        _open_with_poke = false;
        return;
    }

    if( empty( item_name )
     || ( length( item_name ) == 1  &&  ( item_name[ 0 ] == ' ' /*Visual Basic 3.0*/
                                 || item_name[ 0 ] == '\0' /*Delphi?*/ )))
    {
        if( format == CF_TEXT ) {
            BYTE* p_end = p + len;
            do {
                BYTE* q = (BYTE*) memchr( p, '\r', p_end - p );        // "\r\n" trennt Sätze
                if( !q  ||  q+1 == p_end  ||  *(q+1) != '\n' )  q = p_end;
                Const_area rec ( p, q - p );
                LOG( "File_dde_conversation::xtyp_poke CF_TEXT _file.put(\"" << rec << "\")\n" );
                _file.put( rec );
                p = q + 2;
            } while( p < p_end );
        }
        else {
            Const_area rec ( p, len );
            LOG( "File_dde_conversation::xtyp_poke " << Sosdde_format(format) << " _file.put(\"" << rec << "\")\n" );
            _file.put( rec );  // Nicht CF_TEXT
        }
    } else {
        Const_area rec ( p, len );
        LOG( "File_dde_conversation::xtyp_poke " << Sosdde_format(format) << " _file.store_key(\"" << rec << "\",\"" << item_name << "\")\n" );
        _file.store_key( rec, Key( c_str( item_name ) ));
    }

    //if( exception() )  throw_xc( _XC.error_code() );
}

} //namespace sos

#endif
#endif
