// $Id$

#include "../kram/sos.h"
#include "../kram/com_simple_standards.h"
#include "../kram/com_server.h"
#include "../zschimmer/file.h"
#include "../zschimmer/z_com.h"
#include "../zschimmer/z_com_server.h"

#if defined _WIN32

#   if defined _DEBUG
#       import "debug/hostole.tlb" no_namespace raw_interfaces_only named_guids
#   else
#       import "release/hostole.tlb" no_namespace raw_interfaces_only named_guids
#   endif

#else

#   include "hostole.h"

#endif

using namespace std;
using namespace zschimmer::com;

namespace sos
{

extern Typelib_descr   hostole_typelib;

//--------------------------------------------------------------------------------------------Rerun
/*
    VERWENDUNG

    set file = createobject( "hostware.File" )
    file.open "-in tab -csv | meinedatensätze.csv"

    set rerun_file = createobject( "hostware.Rerun" )
    rerun_file.open "c:/tmp/my_rerun_file"
    
    while not file.eof()
        set record = file.get()

        if rerun_file.process_next_record() then
            try
                processor.process record
                rerun.processing_was_ok(true)
                if processor.document_written then rerun_file.commit 
            catch
                rerun.processing_was_ok(false)
                        
    wend

    processor.close
    if processor.document_written then rerun_file.commit 
    rerun_file.close
    file.close
*/


struct Rerun : Irerun, Sos_ole_object
{
                                Rerun                       ();
                               ~Rerun                       ();

    USE_SOS_OLE_OBJECT

    STDMETHODIMP                Open                        ( BSTR );
    STDMETHODIMP                Close                       ();
    STDMETHODIMP                Process_next_record         ( VARIANT_BOOL* );
  //STDMETHODIMP                Processing_was_ok           ( VARIANT_BOOL );
  //STDMETHODIMP                Commit                      ();
    STDMETHODIMP                Set_record_ok               ( int, VARIANT_BOOL );
    STDMETHODIMP            get_Rerunning                   ( VARIANT_BOOL* result )                { *result = _rerun;     return S_OK; }
    STDMETHODIMP            get_Record_number               ( int* result )                         { *result = _record_nr; return S_OK; }
    STDMETHODIMP            get_Ok                          ( VARIANT_BOOL* result )                { *result = _rerun_ok;  return S_OK; }


    void                        read_range                  ();
    int                         get_char_from_rerun_file    ();
    int                         get_int_from_rerun_file     ();
    bool                        set_next_rerun_record_nr    ();
    void                        end_reading_rerun_file      ();
    void                        begin_writing_rerun_file    ();
    void                        write_rest_is_bad           ( int rest_record_number );
    void                        write_bad                   ( int record_number );


    Fill_zero                  _zero_;
    string                     _rerun_filename;
    zschimmer::file::Stream_file _rerun_file;
    int64                      _rerun_pos;                 

    int                        _record_nr;
  //int                        _first_bad_record_nr;        // 0, wenn kein Fehler aufgetreten ist (seit Commit)
  //int                        _last_bad_record_nr;         // Wenn alles ok ist  = _last_processing_was_ok + 1
  //int                        _last_processing_was_ok;     // Merken, dass Processing_was_ok() gerufen worden ist.
    bool                       _rerun;                      // _rerun_file wird gelesen
    int                        _range_begin;                // Nummer des Satzes, bis zu dem aufbereitet werden soll
    int                        _range_end;                  // Nummer des Satzes, bis zu dem aufbereitet werden soll
    bool                       _rerun_file_was_filled;
    bool                       _rerun_ok;                   // Bei einem Fehler im Wiederanlauf die Wiederanlaufdatei unverändert lassen
  //bool                       _was_process_next_record;    // Ergebnis des letzten Process_next_record()
    bool                       _eof;
};

//-------------------------------------------------------------------------------------------static

DESCRIBE_CLASS_CREATABLE( &hostole_typelib, Rerun, rerun, CLSID_Rerun, "hostWare.Rerun" , "1.0" );

//----------------------------------------------------------------------------------Rerun::_methods
#ifndef SYSTEM_HAS_COM

const Com_method Rerun::_methods[] =
{ 
    { DISPATCH_METHOD     ,  1, "Open"                      , (Com_method_ptr)&Rerun::Open                  , VT_EMPTY     , { VT_BSTR } },
    { DISPATCH_METHOD     ,  2, "Close"                     , (Com_method_ptr)&Rerun::Close                 , VT_EMPTY     },
    { DISPATCH_METHOD     ,  3, "Process_next_record"       , (Com_method_ptr)&Rerun::Process_next_record   , VT_BOOL      },
  //{ DISPATCH_METHOD     ,  4, "Processing_was_ok"         , (Com_method_ptr)&Rerun::Processing_was_ok     , VT_EMPTY     , { VT_BOOL } },
    { DISPATCH_PROPERTYGET,  5, "Rerunning"                 , (Com_method_ptr)&Rerun::get_Rerunning         , VT_BOOL      },
    { DISPATCH_PROPERTYGET,  6, "Record_number"             , (Com_method_ptr)&Rerun::get_Record_number     , VT_INT       },
    { DISPATCH_PROPERTYGET,  7, "Ok"                        , (Com_method_ptr)&Rerun::get_Ok                , VT_BOOL      },
  //{ DISPATCH_METHOD     ,  8, "Commit"                    , (Com_method_ptr)&Rerun::Commit                , VT_EMPTY     },
    { DISPATCH_METHOD     ,  9, "Set_record_ok"             , (Com_method_ptr)&Rerun::Set_record_ok         , VT_EMPTY     , { VT_INT, VT_BOOL} },
    {}
};

#endif
//----------------------------------------------------------------------------create_hostware_rerun
/*
HRESULT create_hostware_rerun( const CLSID& clsid, IUnknown** result )
{                                                                            
    if( clsid != CLSID_Rerun )  return CLASS_E_CLASSNOTAVAILABLE;
    *result = (IUnknown*)(Irerun*) new sos::Rerun_file;
    return S_OK;
}
*/
//-------------------------------------------------------------------------------------Rerun::Rerun

Rerun::Rerun()
:
    Sos_ole_object( &rerun_class, (Irerun*)this ),
    _zero_(this+1)
{
}

//------------------------------------------------------------------------------------Rerun::~Rerun

Rerun::~Rerun()
{
}

//--------------------------------------------------------------------------------------Rerun::Open

STDMETHODIMP Rerun::Open( BSTR filename_bstr )
{
    LOGI( Z_FUNCTION << "(" << filename_bstr << ")\n" );

    HRESULT hr = S_OK;

    try 
    {
        _rerun_filename = string_from_bstr( filename_bstr );

        _rerun_file.try_open( _rerun_filename, "r" ); 
        if( !_rerun_file.opened()  &&  _rerun_file.last_errno() != ENOENT )  _rerun_file.throw_error( "try_open" );

        if( _rerun_file.opened()  &&  _rerun_file.length() == 0 )  _rerun_file.close();

        if( _rerun_file.opened() )  
        {
            _rerun                 = true;
            _rerun_file_was_filled = false;    // Bis zum Beweis des Gegenteils

            LOG( "Rerun Wiederanlauf\n" );
        }
        else
        {
            //_first_bad_record_nr = 1;
            begin_writing_rerun_file();
        }

        _rerun_ok = true;
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "hostWare.Rerun::open" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "hostWare.Rerun::open" ); }
                                                         
    return hr;
}

//-------------------------------------------------------------------------------------Rerun::Close

STDMETHODIMP Rerun::Close()
{
    LOGI( Z_FUNCTION << "()\n" );

    HRESULT hr = NOERROR;

    try 
    {
        _rerun_file.close();

        if( _rerun_ok )  unlink( _rerun_file.path().c_str() );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "hostWare.Rerun::close" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "hostWare.Rerun::close" ); }
                                                         
    return hr;
}

//--------------------------------------------------------------------Rerun::get_Next_record_number

STDMETHODIMP Rerun::Process_next_record( VARIANT_BOOL* result )
{ 
    LOGI( Z_FUNCTION << "()  next record_number=" << (_record_nr+1) << "\n" );

    HRESULT hr = S_OK;

    try 
    {
      //if( _was_process_next_record  &&  _last_processing_was_ok < _record_nr )  Processing_was_ok( false );      // Processing_was_ok() vergessen aufzurufen?

        _record_nr++;

        if( _record_nr > _range_end )
        {
            read_range();
        }

        bool process_it = _range_begin <= _record_nr  &&  _record_nr <= _range_end;
        *result = process_it? VARIANT_TRUE : VARIANT_FALSE;
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "hostWare.Rerun::get_next_record_number" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "hostWare.Rerun::get_next_record_number" ); }
                                                         
    return hr;
}

//-------------------------------------------------------------------------Rerun::Processing_was_ok
/*
STDMETHODIMP Rerun::Processing_was_ok( int first_record_nr, int last_record_nr )
{
    HRESULT hr = S_OK;

    try 
    {
        //if( _rerun  &&  first_record_nr > _last_good_record_nr )  _rerun_ok = false;

        if( !_rerun )
        {
            if( _last_good_record_nr + 1 != first_record_nr )
            {
                _rerun_ok = false;
                write_bad( _last_good_record_nr + 1, first_record_nr - 1 ); 
            }

            write_rest_is_bad( last_record_nr );
        }
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "hostWare.Rerun::processing_was_ok" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "hostWare.Rerun::processing_was_ok" ); }
                                                         
    return hr;
}
*/
//-------------------------------------------------------------------------Rerun::Processing_was_ok
/*
STDMETHODIMP Rerun::Processing_was_ok( VARIANT_BOOL ok )
{
    HRESULT hr = S_OK;

    try 
    {
        if( !ok )  _rerun_ok = false;

        if( !_rerun )
        {
            if( ok  &&  _first_bad_record_nr == _record_nr )
            {
                // Alles in Folge gut
                _first_bad_record_nr = _record_nr + 1;    // Die nächste unbekannte Satznummer gilt als schlecht.
                _last_bad_record_nr  = 0;
            }
            else  
            {
                // Ist einer schlecht, sind alle schlecht. Bis commit
                _last_bad_record_nr  = _record_nr;        
            }
        }

        _last_processing_was_ok = _record_nr;
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "hostWare.Rerun::processing_was_ok" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "hostWare.Rerun::processing_was_ok" ); }
                                                         
    return hr;
}
*/
//------------------------------------------------------------------------------------Rerun::Commit
/*
STDMETHODIMP Rerun::Commit()
{
    HRESULT hr = S_OK;

    try 
    {
        if( !_rerun && _rerun_file.opened() )
        {
            if( _first_bad_record_nr <= _last_bad_record_nr )  write_bad();

            _first_bad_record_nr = _last_processing_was_ok + 1;
            _last_bad_record_nr  = 0;

            write_rest_is_bad();
        }
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
                                                         
    return hr;
}
*/

//-----------------------------------------------------------------------------Rerun::Set_record_ok

STDMETHODIMP Rerun::Set_record_ok( int record_number, VARIANT_BOOL ok )
{
    LOGI( Z_FUNCTION << "(" << record_number << "," << abs(ok) << ")\n" );

    HRESULT hr = S_OK;

    try 
    {
        if( !ok )  _rerun_ok = false;

        if( !_rerun )
        {
            if( ok )
            {
                write_rest_is_bad( record_number+1 );
            }
            else
            {
                write_bad( record_number );
                write_rest_is_bad( record_number+1 );
            }
        }
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
                                                         
    return hr;
}

//-------------------------------------------------------------------------Rerun::write_rest_is_bad

void Rerun::write_rest_is_bad( int rest_record_number )
{
    _rerun_file.seek( _rerun_pos );
    
    int ret = fprintf( _rerun_file, "%d-\n", rest_record_number );  
    if( ret == -1 )  throw_errno( errno, _rerun_file.path().c_str() );

    _rerun_file.syncdata();
}

//---------------------------------------------------------------------------------Rerun::write_bad

void Rerun::write_bad( int record_number )
{
    _rerun_file.seek( _rerun_pos );
    
    int ret = fprintf( _rerun_file, "%d\n", record_number );        
    if( ret == -1 )  throw_errno( errno, _rerun_file.path().c_str() );

    _rerun_file.syncdata();

    _rerun_pos = _rerun_file.tell();
}

//---------------------------------------------------------------------------------Rerun::write_bad
/*
void Rerun::write_bad()
{
    _rerun_file.seek( _rerun_pos );
    
    int ret = fprintf( _rerun_file, "%d", _first_bad_record_nr );        
    if( ret == -1 )  throw_errno( errno, _rerun_file.filename().c_str() );

    if( _first_bad_record_nr < _last_bad_record_nr )
    {
        int ret = fprintf( _rerun_file, "-%d", _last_bad_record_nr );        
        if( ret == -1 )  throw_errno( errno, _rerun_file.filename().c_str() );
    }

    _rerun_file.put_char( '\n' );

    _rerun_file.syncdata();

    _rerun_pos = _rerun_file.tell();
}
*/
//-------------------------------------------------------------------------Rerun::write_rest_is_bad
/*
void Rerun::write_rest_is_bad()
{
    _rerun_file.seek( _rerun_pos );
    
    int ret = fprintf( _rerun_file, "%d-\n", _first_bad_record_nr );  
    if( ret == -1 )  throw_errno( errno, _rerun_file.filename().c_str() );

    _rerun_file.syncdata();
}
*/
//------------------------------------------------------------------Rerun::begin_writing_rerun_file

void Rerun::begin_writing_rerun_file()
{
    _rerun_file.open( _rerun_filename, "w" );

    write_rest_is_bad( _record_nr + 1 );

    _range_end = INT_MAX;
}

//--------------------------------------------------------------------Rerun::end_reading_rerun_file

void Rerun::end_reading_rerun_file()
{
    LOG( "Rerun: Wiederanlaufdatei ist abgearbeitet\n" );

    _rerun_file.close();
    _rerun = false;

    if( _rerun_ok )
    {
      //_first_bad_record_nr = _record_nr + 1;
      //_last_bad_record_nr  = 0;
        begin_writing_rerun_file();
    }
}

//--------------------------------------------------------------------------------Rerun::read_range

void Rerun::read_range()
{
    if( _eof )
    {
        end_reading_rerun_file();
    }
    else
    {
        _range_begin = get_int_from_rerun_file();

        int c = get_char_from_rerun_file();

        if( c == '-' ) 
        {
            c = get_char_from_rerun_file();
            if( c == '\r'  ||  c == '\n'  ||  c == EOF )
            {                                       // "VONNR-"
                ungetc( c, _rerun_file );
                _range_end = INT_MAX;
                c = EOF;
            }
            else                                    // "VONNR-BISNR"
            {
                ungetc( c, _rerun_file );
                _range_end = get_int_from_rerun_file();
            }
        }
        else
        {
            _range_end = _range_begin;               // "NR"
            ungetc( c, _rerun_file );
        }

        if( _range_end != INT_MAX )
        {
            c = get_char_from_rerun_file();
            if( c == '\r' )  c = get_char_from_rerun_file();
            if( c != '\n' && c != EOF )  throw_xc( "FACTORY-303", "Zeilenende erwartet" );

            while( isspace( c ) )  c = get_char_from_rerun_file();      // Toleranz
            if( c != EOF )  ungetc( c, _rerun_file );
        }

        if( c == EOF )  
        {
            _eof = true;
            if( !_rerun_file_was_filled )  _range_end = INT_MAX;
        }
    }
}

//------------------------------------------------------------------Rerun::get_char_from_rerun_file

int Rerun::get_char_from_rerun_file()
{
    int c = fgetc( _rerun_file );
    
    while( c == ' ' || c == '\t' )  c = get_char_from_rerun_file();      // Abweichende Gestalt tolerieren

    if( c == EOF )  if( _rerun_file.eof() || errno == 0 )  ; //throw_eof_error();
                                                     else  throw_errno( errno, _rerun_file.path().c_str() );

    return c;
}

//-------------------------------------------------------------------Rerun::get_int_from_rerun_file

int Rerun::get_int_from_rerun_file()
{
    int n;
    int c = get_char_from_rerun_file();

    while( isspace( c ) )  c = get_char_from_rerun_file();      // Abweichende Gestalt tolerieren

    if( !isdigit(c) )  throw_xc( "FACTORY-303", "Ziffer erwartet" );
    
    _rerun_file_was_filled = true;

    n = c - '0';
    
    while(1)
    {
        c = get_char_from_rerun_file();
        if( !isdigit(c) )  { ungetc( c, _rerun_file ); return n; }
        n = 10*n + c - '0';
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace sos
