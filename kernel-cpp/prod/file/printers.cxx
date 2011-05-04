// printers.cxx                                ©1998 SOS GmbH Berlin
//                                             Joacim Zschimmer
// $Id$

#include "precomp.h"
#include "../kram/sysdep.h"

#if defined SYSTEM_WIN32

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"
#include "../kram/sosfield.h"
#include "../kram/soslimtx.h"

#include <windows.h>
#include <winspool.h>
/*
    Müssen die Pointer von EnumPrinters nicht freigegeben werden?
*/   

namespace sos {

//-------------------------------------------------------------------------------------Printers_file

struct Printers_file : Abs_file
{
    struct Printer_info_2
    {
                                    // Ein Destruktor wird nicht aufgerufen!
    
        Sos_limited_text<MAX_PATH>  _server_name;
        Sos_limited_text<MAX_PATH>  _printer_name;
        Sos_limited_text<MAX_PATH>  _share_name;
        Sos_limited_text<MAX_PATH>  _port_name;
        Sos_limited_text<MAX_PATH>  _driver_name;
        Sos_limited_text<4096>      _comment;
        Sos_limited_text<4096>      _location;
      //_devmode;
        Sos_limited_text<MAX_PATH>  _sep_file;
        Sos_limited_text<MAX_PATH>  _print_processor;
        Sos_limited_text<MAX_PATH>  _data_type;
        Sos_limited_text<MAX_PATH>  _parameters;
      //security_descriptor;
      //attributes;
      //priority
      //default_priority;
      //start_time;
      //until_time;
      //status;
      //jobs
      //average_ppm;  // pages per minute
    };

    struct Printer_info_4
    {
                                    // Ein Destruktor wird nicht aufgerufen!
    
        Sos_limited_text<MAX_PATH>  _printer_name;
        Sos_limited_text<MAX_PATH>  _server_name;
    };

    struct Printer_info_5
    {
                                    // Ein Destruktor wird nicht aufgerufen!
    
        Sos_limited_text<MAX_PATH>  _printer_name;
        Sos_limited_text<MAX_PATH>  _port_name;
    };

                                Printers_file           ();
                               ~Printers_file           ();

  //void                        prepare_open            ( const char*, Open_mode, const File_spec& );
    void                        open                    ( const char*, Open_mode, const File_spec& );
  //void                        close                   ( Close_mode );

  protected:
    void                        get_record              ( Area& area );

    Fill_zero                  _zero_;
    Sos_ptr<Record_type>       _printers_info_type;
    Bool                       _eof;
    int                        _level;
    void*                      _printer_info;
    void*                      _ptr;
    void*                      _ptr_end;
    DWORD                      _printer_info_size;
    DWORD                      _count;
};


//--------------------------------------------------------------------------------Printers_file_type

struct Printers_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "printers"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Printers_file> f = SOS_NEW( Printers_file );
        return +f;
    }
};

const Printers_file_type       _printers_file_type;
const Abs_file_type&            printers_file_type = _printers_file_type;

//---------------------------------------------------------------------------------------remark

static void remark( Record_type* t, const char* text ) 
{
    t->field_descr_ptr( t->field_count() - 1 )->_remark = text;
}

//---------------------------------------------------------------------------Printers_file::Printers_file

Printers_file::Printers_file()
:
    _zero_ ( this+1 )
{
}

//----------------------------------------------------------------------Printers_file::~Printers_file

Printers_file::~Printers_file()
{
    sos_free( _printer_info );  
    _printer_info = NULL;
}

//---------------------------------------------------------------------------Printers_file::open

void Printers_file::open( const char* parameter, Open_mode open_mode, const File_spec& spec )
{
    DWORD       flags = 0;
    Sos_string  name;
    DWORD       size = 0;
    DWORD       needed = 0;
    OSVERSIONINFO ver;

    ver.dwOSVersionInfoSize = sizeof ver;
    if( !GetVersionEx( &ver ) )  throw_mswin_error( "GetVersionEx" );

    _level = ver.dwPlatformId == VER_PLATFORM_WIN32_NT? 4 : 5;

    if( open_mode & out )  throw_xc( "D127" );

    for( Sos_option_iterator opt( parameter ); !opt.end(); opt.next() )
    {
        if( opt.flag( "local"       ) )  flags |= PRINTER_ENUM_LOCAL;
        else
        if( opt.flag( "name"        ) )  flags |= PRINTER_ENUM_NAME;
        else
        if( opt.flag( "shared"      ) )  flags |= PRINTER_ENUM_SHARED;
        else                        
        if( opt.flag( "default"     ) )  flags |= PRINTER_ENUM_DEFAULT;
        else
        if( opt.flag( "connections" ) )  flags |= PRINTER_ENUM_CONNECTIONS;
        else
        if( opt.flag( "network"     ) )  flags |= PRINTER_ENUM_NETWORK;
        else
        if( opt.flag( "remote"      ) )  flags |= PRINTER_ENUM_REMOTE;
        else
        if( opt.flag( "2"           ) )  _level = 2;
        else
        if( opt.flag( "4"           ) )  _level = 4;
        else
        if( opt.flag( "5"           ) )  _level = 5;
        else
        if( opt.param( 1 )            )  name = opt.value();
        else throw_sos_option_error( opt );
    }


  TRYAGAIN:
    BOOL ok = EnumPrinters( flags, 
                            (char*)c_str( name ), 
                            _level,
                            (Byte*)_printer_info,
                            _printer_info_size,
                            &needed,
                            &_count );
    if( !ok ) {
        if( _printer_info_size == 0  &&  needed > 0 ) { 
            _printer_info = sos_alloc( needed, "PRINTER_ENUM_i" );
            _printer_info_size = needed; 
            goto TRYAGAIN;
        }
      //if( GetLastError() == ERROR_FILE_NOT_FOUND ) {
      //    _eof = true;
      //} else {
            throw_mswin_error( "EnumPrinters" );
      //}
    }

    _ptr = _printer_info;


    _printers_info_type = Record_type::create();
    Record_type* t = _printers_info_type;

    switch( _level ) 
    {
    case 2:
    {
        Printer_info_2* o = 0;

        t->name( "Printer_info_2" );
        t->allocate_fields( 11 );

        RECORD_TYPE_ADD_LIMTEXT     ( server_name , 0 );   remark( t, "identifying the server that controls the printer. If this string is empty, the printer is controlled locally." );
        RECORD_TYPE_ADD_LIMTEXT     ( printer_name, 0 );   remark( t, "specifies the name of the printer" );
        RECORD_TYPE_ADD_FIELD       ( share_name  , 0 );   remark( t, "identifies the sharepoint for the printer. (This string is used only if the PRINTER_ATTRIBUTE_SHARED constant was set for the Attributes member.)" );
        RECORD_TYPE_ADD_FIELD       ( port_name   , 0 );   remark( t, "identifies the port(s) used to transmit data to the printer. If a printer is connected to more than one port, the names of each port must be separated by commas (for example, “LPT1:,LPT2:,LPT3:”)." );
        RECORD_TYPE_ADD_FIELD       ( driver_name , 0 );   remark( t, "specifies the name of the printer driver" );
        RECORD_TYPE_ADD_FIELD       ( comment     , 0 );   remark( t, "provides a brief description of the printer" );
        RECORD_TYPE_ADD_FIELD       ( location    , 0 );   remark( t, "specifies the physical location of the printer (for example, “Bldg. 38, Room 1164”). " );
        RECORD_TYPE_ADD_FIELD       ( sep_file    , 0 );   remark( t, "specifies the name of the file used to create the separator page. This page is used to separate print jobs sent to the printer." );
        RECORD_TYPE_ADD_FIELD       ( print_processor, 0 );   remark( t, "specifies the name of the print processor used by the printer." );
        RECORD_TYPE_ADD_FIELD       ( data_type   , 0 );   remark( t, "specifies the data type used to record the print job. " );
        RECORD_TYPE_ADD_FIELD       ( parameters  , 0 );   remark( t, "specifies the default print-processor parameters" );

        _ptr_end = (PRINTER_INFO_2*)_ptr + _count;
        break;
    }

    case 4:
    {
        Printer_info_4* o = 0;

        t->name( "Printer_info_4" );
        t->allocate_fields( 2 );

        RECORD_TYPE_ADD_LIMTEXT     ( printer_name, 0 );   remark( t, "specifies the name of the printer (local or remote). " );
        RECORD_TYPE_ADD_LIMTEXT     ( server_name , 0 );   remark( t, "the name of the server. " );

        _ptr_end = (PRINTER_INFO_4*)_ptr + _count;
        break;
    }

    case 5:
    {
        Printer_info_5* o = 0;

        t->name( "Printer_info_5" );
        t->allocate_fields( 2 );

        RECORD_TYPE_ADD_LIMTEXT     ( printer_name, 0 );   remark( t, "specifies the name of the printer" );
        RECORD_TYPE_ADD_LIMTEXT     ( port_name   , 0 );   remark( t, "identifies the port(s) used to transmit data to the printer" );

        _ptr_end = (PRINTER_INFO_5*)_ptr + _count;
        break;
    }

    default: throw_xc( "printers-level" );
    }

    _any_file_ptr->_spec._field_type_ptr = +_printers_info_type;
}

//---------------------------------------------------------------------Printers_file::get_record

void Printers_file::get_record( Area& buffer )
{
    if( _eof )  throw_eof_error();
    if( _ptr == _ptr_end )  throw_eof_error();

    buffer.allocate_length( _printers_info_type->field_size() );
    memset( buffer.ptr(), 0, _printers_info_type->field_size() );

    switch( _level ) 
    {
    case 2:
    {
        PRINTER_INFO_2* p = (PRINTER_INFO_2*)_ptr;
        Printer_info_2* e = (Printer_info_2*)buffer.ptr();
        new (e) Printer_info_2;

        e->_server_name     = p->pServerName ? p->pServerName  : "";
        e->_printer_name    = p->pPrinterName? p->pPrinterName : "";
        e->_share_name      = p->pShareName  ? p->pShareName   : "";
        e->_port_name       = p->pPortName   ? p->pPortName    : "";
        e->_driver_name     = p->pDriverName ? p->pDriverName  : "";
        if( p->pComment )  e->_comment .assign( p->pComment , min( (uint)strlen( p->pComment  ), (uint)e->_comment .size() ) );
        if( p->pLocation ) e->_location.assign( p->pLocation, min( (uint)strlen( p->pLocation ), (uint)e->_location.size() ) );
        e->_sep_file        = p->pSepFile    ? p->pSepFile     : "";
        e->_print_processor = p->pPrintProcessor? p->pPrintProcessor : "";
        e->_data_type       = p->pDatatype   ? p->pDatatype    : "";
        e->_parameters      = p->pParameters ? p->pParameters  : "";

        _ptr = ((PRINTER_INFO_2*)_ptr) + 1;
        break;
    }

    case 4:
    {
        PRINTER_INFO_4* p = (PRINTER_INFO_4*)_ptr;
        Printer_info_4* e = (Printer_info_4*)buffer.ptr();
        new (e) Printer_info_4;

        e->_printer_name    = p->pPrinterName? p->pPrinterName : "";
        e->_server_name     = p->pServerName ? p->pServerName  : "";

        _ptr = ((PRINTER_INFO_4*)_ptr) + 1;
        break;
    }

    case 5:
    {
        PRINTER_INFO_5* p = (PRINTER_INFO_5*)_ptr;
        Printer_info_5* e = (Printer_info_5*)buffer.ptr();
        new (e) Printer_info_5;

        e->_printer_name    = p->pPrinterName? p->pPrinterName : "";
        e->_port_name       = p->pPortName   ? p->pPortName    : "" ;

        _ptr = ((PRINTER_INFO_5*)_ptr) + 1;
        break;
    }

    default: throw_xc( "printers-level" );
    }
}


} //namespace sos
#endif

