// clipbrd.cxx                                     ©1998 SOS Software GmbH
// Joacim Zschimmer

#include "precomp.h"
#if defined SYSTEM_WIN32

#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"

#include <windows.h>


namespace sos {

const uint start_size       =   60*1024;
const uint write_increment  = 1024*1024;
const uint read_increment   =    4*1024;

//---------------------------------------------------------------------------------------static

static bool                     clipboard_opened        = false;

//-------------------------------------------------------------------------------Clipboard_file

struct Clipboard_file : Abs_file
{
                                    Clipboard_file      ();
                                   ~Clipboard_file      ();

    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );

  protected:
    void                            get_record          ( Area& );
    void                            put_record          ( const Const_area& );

  private:
    Fill_zero                      _zero_;
    Open_mode                      _open_mode;
    HGLOBAL                        _mem;
    Byte*                          _ptr;
    uint                           _size;
    uint                           _length;
    uint                           _extra;              // Anzahl der Extra-Bytes für cr,lf und '\0'
};

//-------------------------------------------------------------------------Clipboard_file_type

struct Clipboard_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "clipboard"; }
  //virtual const char*         alias_name              () const { return "nl"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Clipboard_file> f = SOS_NEW( Clipboard_file() );
        return +f;
    }
};

const Clipboard_file_type      _clipboard_file_type;
const Abs_file_type&            clipboard_file_type = _clipboard_file_type;

//--------------------------------------------------------------Clipboard_file::Clipboard_file

Clipboard_file::Clipboard_file()
:
    _zero_(this+1)
{
}

//-------------------------------------------------------------Clipboard_file::~Clipboard_file

Clipboard_file::~Clipboard_file()
{
}

//------------------------------------------------------------------------Clipboard_file::open

void Clipboard_file::open( const char* param, Open_mode open_mode, const File_spec& )
{
    BOOL ok;

    if( !( open_mode & inout )  ||  ( open_mode & inout ) == inout )  throw_xc( "SOS-1236" );
    _open_mode = open_mode;

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() )
    {
        //if( opt.pipe() )                                filename = opt.rest();
        //else 
        throw_sos_option_error( opt );
    }

    Z_MUTEX( hostware_mutex )
    {
        if( clipboard_opened )  throw_xc( "SOS-1398" );

        LOG( "OpenClipboard()\n" );
        ok = OpenClipboard( NULL );
        if( !ok )  throw_mswin_error( "OpenClipboard" );

        clipboard_opened = true;
    }

    if( _open_mode & out ) 
    {
        _extra = 1;     // Für abschließendes 0-Byte

        LOG( "EmptyClipboard()\n" );
        ok = EmptyClipboard();
        if( !ok )  throw_mswin_error( "EmptyClipboard" );

        _size = start_size - _extra;

        _mem = GlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE, _size + _extra );
        if( !_mem )  throw_mswin_error( "GlobalAlloc" );

        _ptr = (Byte*)GlobalLock( _mem );
        if( !_ptr )  throw_mswin_error( "GlobalLock" );
    }
    else
    {
        LOG( "GetClipboardData\n" );
        _mem = GetClipboardData( CF_TEXT );
        if( !_mem )  throw_mswin_error( "GetClipboardData" );

        _ptr = (Byte*)GlobalLock( _mem );
        if( !_ptr )  throw_mswin_error( "GlobalLock" );

        _size = GlobalSize( _mem );
        Byte* z = (Byte*) memchr( _ptr, '\0', _size );
        if( z )  _size = z - _ptr;
    }
}

// --------------------------------------------------------------------------Clipboard_file::close

void Clipboard_file::close( Close_mode close_mode )
{
    BOOL ok;

    if( _open_mode & out ) 
    {
        _ptr[ _length++ ] = '\0';

        GlobalUnlock( _mem );

        _mem = GlobalReAlloc( _mem, _length, 0 );
        if( !_mem )  throw_mswin_error( "GlobalRealloc" );

        LOG( "SetClipboardData()\n" );
        if( !SetClipboardData( CF_TEXT, _mem ) )  throw_mswin_error( "SetClipboardData" );
    } 
    else 
    {
        GlobalUnlock( _mem );
    }

    _mem = NULL;
    _ptr = NULL;

    LOG( "CloseClipboard()\n" );

    ok = CloseClipboard();
    if( !ok )  throw_mswin_error( "CloseClipboard" );

    clipboard_opened = false;
}

// ----------------------------------------------------------------------Clipboard_file::put_record

void Clipboard_file::put_record( const Const_area& record )
{
    uint4 need = _length + record.length();

    if( need > _size ) 
    {
        GlobalUnlock( _mem );

        uint4 new_size = ( need + _extra + write_increment - 1 ) / write_increment * write_increment;

        _mem = GlobalReAlloc( _mem, new_size, 0 );
        if( !_mem )  throw_mswin_error( "GlobalRealloc" );

        _size = new_size - _extra;

        _ptr = (Byte*)GlobalLock( _mem );
        if( !_ptr )  throw_mswin_error( "GlobalLock" );
    }

    memcpy( _ptr + _length, record.ptr(), record.length() );
    _length += record.length();
}

// ---------------------------------------------------------------------Clipboard_file::get_record

void Clipboard_file::get_record( Area& buffer )
{
    if( _length >= _size )  throw_eof_error();

    if( buffer.resizable() )  buffer.allocate_min( read_increment );

    uint4 l = min( buffer.size(), _size - _length );
    memcpy( buffer.ptr(), _ptr + _length, l );
    _length += l;
    buffer.length( l );
}


} //namespace sos

#endif

