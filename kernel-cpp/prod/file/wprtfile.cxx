//#define MODULE_NAME "wprtfile"
// wprtfile.cpp                                         (c) SOS GmbH Berlin
//                                                         Joacim Zschimmer

#include "precomp.h"
#include "../kram/sysdep.h"


#ifdef SYSTEM_BORLAND

#if defined SYSTEM_STARVIEW


#include <stdlib.h>
#include <except.h>

#if defined( SYSTEM_WIN )
#   include <svwin.h>           // StarView und Windows
#   define _INC_WINDOWS
#endif

#include <sos.h>
#include <sosopt.h>
#include <xception.h>

#include <sv.hxx>
#include <file.hrc>

#include <frmpage.h>
#include <sosstrea.h>

#include <ctype.h>
#include <wprtfile.h>

#include "file.hrc"

struct Print_job_params {
                    Print_job_params() { _init(); }

    //enum { print_even, print_odd, print_all } Print_job_mode;

    void            _init();

    int             _page_count;
    int             _start_page;
    int             _end_page;
    int             _copy_count;
    //Print_job_mode  _mode;
    Sos_string      _job_name;
};

void Print_job_params::_init() {
    _page_count     = -1; // d.h. alle
    _start_page     = 1;
    _end_page       = -1; // s.o.
    //_mode           = print_all;
    _copy_count     = 1;
    _job_name       = "Frame Druck";
}

class AbortDialog : public ModelessDialog
{
public:
                    AbortDialog       ( const ResId& rResId );

  FixedText         aPrintText;
  FixedText         aPagesText;
  CancelButton      aCancelPB;
};

AbortDialog::AbortDialog( const ResId& rResId ) :
     ModelessDialog( pApp->GetAppWindow(), rResId ),
     aPrintText( this, ResId( ABORT_DLG_PRINT_TEXT ) ),
     aPagesText( this, ResId( ABORT_DLG_PAGES_TEXT ) ),
     aCancelPB(  this, ResId( ABORT_DLG_CANCEL_PB ) )
{
    FreeResource();
};


static const int PAGES_UNKNOWN = -1;

struct Print_file_sv : Print_file, LinkHdl
{
                        Print_file_sv   ();
                        Print_file_sv   ( const char* filename, Open_mode );
                       ~Print_file_sv   ();

    void                open            ( const char* filename, Open_mode, const File_spec& );
    void                close           ( Close_mode = close_normal );
    void                flush           ();

    // StarView-Handler
    void                StartPrintHdl( Printer* );
    void                PrintPageHdl( Printer* );
    void                EndPrintHdl( Printer* );
    void                AbortJobHdl( CancelButton* );


  protected:
    virtual void        put_record      ( const Const_area& );
//    virtual void        write_area      ( const Const_area& );
    //virtual void        read_area       ( Area& );


  private:
    Frame_esc_parser*   _parser;

    void                _init            ();


    Printer*            _printer;
    AbortDialog         _abort_dlg;
    Bool                _aborting;

    Bool                _printing;
    Bool                _direct_print;
    Bool                _opened                : 1;
    Bool                _opened_by_constructor : 1;
    Bool                _preview               : 1;

    int                 _pages;
};


struct Print_file_type : Abs_file_type
{
    Print_file_type() : Abs_file_type() {};

    virtual const char* name      () const { return "printer"; }
    virtual const char* alias_name() const { return "drucker"; }

    virtual Sos_ptr<Abs_file> create_base_file() const
    {
        Sos_ptr<Print_file_sv> f = SOS_NEW_PTR( Print_file_sv );
        return +f;
    }
};



struct Fontwidth_file_type : Abs_file_type
{
    Fontwidth_file_type() : Abs_file_type() {};

    virtual const char* name      () const { return "fontwidth"; }
    virtual const char* alias_name() const { return "dickten"; }

    virtual Sos_ptr<Abs_file> create_base_file() const
    {
        Sos_ptr<Fontwidth_file> f = SOS_NEW_PTR( Fontwidth_file );
        return +f;
    }
};

void Fontwidth_file::_init() {
    _key_len = 80;
};

/* jz 3.3.96
inline Fontwidth_file::Fontwidth_file( const char* filename, Open_mode open_mode, const File_spec& file_spec )
{
    _init();
    open( filename, open_mode, file_spec ); xc;
exceptions
};
*/

inline void Fontwidth_file::open( const char*, Open_mode, const File_spec& file_spec )
{
    if( file_spec.key_specs().key_length() ) {
        _key_len = file_spec.key_specs().key_length();
    }
};


void Fontwidth_file::get_record_key( Area& area, const Key& key )
{
    int                 i;
    char                _fontname[file_max_key_length+1];
    Window              _window( pApp->GetAppWindow(), WB_HIDE ); // temp. Window
    Sos_binary_ostream  s;

#if 1
    strcpy( _fontname, key.char_ptr() );
#else
    read_string_ebcdic( &Sos_binary_istream( key ), _fontname,  key.length() );
    _fontname[key.length()] = 0;
#endif

    Format_status status;
    status.font().ChangeSize( Size(0,10) ); // 10 pt => twips
    LOG( "Fontwidth_file::get_record_key" << _fontname << '\n' );
    status.parse_font_name( _fontname );
    _window.ChangeFont( status.font() );

    Font f = status.font();
    f.ChangeSize( Size(0, status.size() ) );
    status.window_char_sizes( status._fontwidth_array, f ); xc;

    area.allocate_min( 256 * sizeof(uint2) + 38 ); xc;
    s.reset( area );

    for ( i= 0; i < 256; i++ )   // Schleife über alle EBCDIC-Zeichen
    {
        s.write_uint2( status._fontwidth_array[(uchar)ebc2iso[i]] );
    }

    s.write_int4( 300                       );
    write_string_ebcdic( &s, "WIN",     4    );
    write_string_ebcdic( &s, _fontname, 30   );

    area.length( s.length() );

}

/*jz 22.12.97
Record_length Fontwidth_file::key_length( Key::Number )
{
    return _key_length;
}
*/
//----------------------------------------------------------------------statics

static Print_file_type      _print_file_type;
const Print_file_type&       print_file_type = _print_file_type;

static Fontwidth_file_type  _fontwidth_file_type;
const Fontwidth_file_type&   fontwidth_file_type = _fontwidth_file_type;

Printer* _static_printer = NULL;

//-------------------------------------------------Print_file::static_file_type
/*
const Abs_file_type& Print_file::static_file_type ()
{
    return _Print_file_type;
}
*/
//-----------------------------------------------------------Print_file::create

Sos_ptr<Print_file> Print_file::create()
{
    Sos_ptr<Print_file_sv> o;
    o = SOS_NEW( Print_file_sv );
    return +o;
}

//-------------------------------------------------Print_file_sv::Print_file_sv

Print_file_sv::Print_file_sv()
  : _abort_dlg( ResId( ABORT_DLG ) )
{
    _init();
}

//------------------------------------------------Print_file_sv::~Print_file_sv

Print_file_sv::~Print_file_sv()
{
    if ( pApp->GetAppWindow() ) pApp->GetAppWindow()->Enable();
    //if (_opened) close( _opened_by_constructor? close_normal : close_error );
    _abort_dlg.Hide();
    SOS_DELETE( _parser );
    //if ( _printer != NULL )  SOS_DELETE( _printer );
}

//---------------------------------------------------------Print_file_sv::_init

void Print_file_sv::_init()
{
    _preview                = false;
    _direct_print           = true;
    _printing               = false;
    _aborting               = false;
    _opened                 = false;
    _opened_by_constructor  = false;
    _pages                  = PAGES_UNKNOWN;

    _printer        = NULL;
    _parser         = NULL;
}

//----------------------------------------------------------Print_file_sv::open

void Print_file_sv::open( const char* filename, Open_mode, const File_spec& )
{
    LOG( "Print_file_sv::open\n" );
    Print_job_params params;
    Bool _debug = false;
    Bool _print_direct_dlg = false;

    if ( System::GetPrinterCount() == 0 )   throw_xc( "NOPRINTER" );//raise ( "NOPRINTER", "NOPRINTER" );

    for ( Sos_option_iterator opt(filename); !opt.end(); opt.next() ) {
        if      ( opt.flag(       'd', "debug"      ) ) _debug = opt.set();
        else if ( opt.with_value( 'c', "copy-count" ) ) { Sos_string s = opt.value(); params._copy_count = as_int(s); }
        else if ( opt.with_value(      "start-page" ) ) { Sos_string s = opt.value(); params._start_page = as_int(s); }
        else if ( opt.with_value(      "end-page"   ) ) { Sos_string s = opt.value(); params._end_page   = as_int(s); }
        else if ( opt.with_value(      "page-count" ) ) { Sos_string s = opt.value(); params._page_count = as_int(s); }
        else if ( opt.with_value(      "job"        ) ) params._job_name = opt.value();
        else if ( opt.param()                         ) {
            Sos_string s = opt.value();
            if      ( strcmpi( c_str(s), "preview") == 0 )  _preview          = true;
            else if ( strcmpi( c_str(s), "std"    ) == 0 )  _print_direct_dlg = true;
        }
        else throw_sos_option_error( opt );
    }
    _direct_print = !_preview;

#if 0 // alter Kram
    const char* _filename = (const char *) filename;

    if ( strncmpi( _filename, "preview", 7 ) == 0 ) {
        _preview = true;
        if ( strcmpi(_filename, "preview -d" ) == 0 ) {
            _debug = true;
        }
    } else {
        _direct_print = true;
        if ( ! strcmp( _filename, "std" ) == 0 ) _print_direct_dlg = true;
    }
#endif

    if ( _preview ) {
        _parser = new Frame_page_list();
                if (!_parser)  throw_no_memory_error();
        _parser->debug( _debug );
        _parser->open();
    } else if ( _direct_print ) {
        String print_text;
        if ( !_static_printer ) {
            _static_printer = new Printer;   // JobSetup ???
                if (!_static_printer)  throw_no_memory_error();
        }
        if ( _print_direct_dlg )
        {
            PrintDialog aPrintDlg( pApp->GetAppWindow(), WB_SVLOOK );

            aPrintDlg.ChangePrinter( _static_printer );
            aPrintDlg.ChangeCopyCount( params._copy_count );
            aPrintDlg.EnableSelection( TRUE );
            aPrintDlg.ChangeFirstPage( params._start_page );
            if ( params._start_page != 1 || params._end_page != PAGES_UNKNOWN ) {
                aPrintDlg.ChangeFirstPage( params._start_page );
                aPrintDlg.ChangeLastPage( params._end_page );
            }
            aPrintDlg.EnablePageFields( TRUE ); // wir wissen nicht wieviele Seiten es sind!
            aPrintDlg.EnableCollate( TRUE );
            aPrintDlg.CheckCollate( FALSE );

            if ( aPrintDlg.Execute() == RET_OK )
            {
              // alles ok, Drucker ausgwählt
              // params aktualisieren
              params._copy_count = aPrintDlg.GetCopyCount();
              if ( aPrintDlg.IsSelectionChecked() ) {
                params._start_page = aPrintDlg.GetFirstPage();
                params._end_page   = aPrintDlg.GetLastPage();
              } else {
                params._start_page = 1;
                params._end_page   = PAGES_UNKNOWN;
              }
            } else {
                //SHOW_MSG( "Job wird abgebrochen" );
                _aborting = true;
                return;
            }
         }

        _static_printer->ChangeStartPrintHdl(          LINK( this, Print_file_sv, StartPrintHdl    ) );
        _static_printer->ChangeEndPrintHdl  (          LINK( this, Print_file_sv, EndPrintHdl      ) );
        _static_printer->ChangePrintPageHdl (          LINK( this, Print_file_sv, PrintPageHdl     ) );
        _abort_dlg.aCancelPB.ChangeClickHdl(    LINK( this, Print_file_sv, AbortJobHdl      ) );

        print_text  = "Aktueller Druckjob: ";
        print_text += c_str(params._job_name);
        _abort_dlg.aPrintText.SetText( print_text );
        _abort_dlg.aPagesText.SetText( "" );

        _parser = new Frame_esc_print( _static_printer, Format_status() );
                if (!_parser) throw_no_memory_error();
        _parser->open();
    }

    _printing = true;

    if ( pApp->GetAppWindow() ) pApp->GetAppWindow()->Disable();

    _opened = true;

  exceptions
//    if ( _printer != NULL ) SOS_DELETE( _printer );
    if ( _parser  != NULL ) SOS_DELETE( _parser );
}

#   if 0
        // Info-Mode
        for (i=0; i < System::GetPrinterCount(); i++ ) {
            printers += "\n" + System::GetPrinterName(i);
        }
        InfoBox( NULL, printers ).Execute();
#   endif



//---------------------------------------------------------Print_file_sv::close


void Print_file_sv::close( Close_mode )
{
    LOG( "Print_file_sv::close\n" );
try {
    if( !_opened)  return;
    _opened = false;

    LOGI( "Print_file_sv::close: start ...\n" );

    if ( _preview ) {
        int MODAL_PREVIEW_WIRD_NICHT_DELETED;

        LOG( "Print_file_sv::close: preview-mode ...\n" );
        pApp->Reschedule();
        _parser->close();

        // Auswahl der Dialoggröße (evtl. skalierbaren Preview-Dialog!) = > TESTEN !!!
        Size  desktop_size = System::GetScreenSizePixel();
        USHORT resid;

        if ( desktop_size.Width() >= 1020 &&  desktop_size.Height() >= 760 )
        {
            resid = DLG_PREVIEW;
        } else if ( desktop_size.Width() >= 780 &&  desktop_size.Height() >= 580 )
        {
            resid = DLG_PREVIEW_800_600;
        } else resid = DLG_PREVIEW_640_480;

    LOG( "Print_file_sv::close: before dialog\n" );
        Frame_modal_preview* dialog_ptr = new Frame_modal_preview( pApp->GetAppWindow(), (Frame_page_list*) _parser, ResId(resid) );
                                  if ( !dialog_ptr ) throw_no_memory_error();
    LOG( "Print_file_sv::close: after dialog\n" );

        dialog_ptr->GrabFocus();
        dialog_ptr->ToTop();
        dialog_ptr->Execute();
        LOG( "Print_file_sv::close: Ende preview-mode ...\n" );

        SOS_DELETE( dialog_ptr );
    } else if ( _direct_print ) {
        LOG( "Print_file_sv::close: Direct-Print ...\n" );
        _abort_dlg.Hide();
        _parser->close();
        // wirklich noch noetig ???
        while ( _static_printer->IsJobActive() || _static_printer->IsPrinting() ) {
            pApp->Reschedule();
        }
        LOG( "Print_file_sv::close: Ende Direct-Print ...\n" );
        // StarView-Handler zurücksetzen ???
        // _static_printer->ChangeStartPrintHdl( (const Link&)*((PFUNC)((void*)NULL)) );
        // _static_printer->ChangeEndPrintHdl  ( (PFUNC)NULL );
        // _static_printer->ChangePrintPageHdl ( (PFUNC)NULL );
    }

    if ( pApp->GetAppWindow() ) pApp->GetAppWindow()->Enable();

    SOS_DELETE( _parser );
    LOG( "Print_file_sv::close: after delete parser ...\n" );
    //if ( _printer != NULL ) SOS_DELETE( _printer );
    //LOG( "Print_file_sv::close: after delete printer ...\n" );
}
catch ( const Xc& x )
{
    if ( pApp->GetAppWindow() ) pApp->GetAppWindow()->Enable();
    LOG( "Print_file_sv::close : " << x << endl ); // ???
}
catch ( ... )
{
    if ( pApp->GetAppWindow() ) pApp->GetAppWindow()->Enable();
    LOG( "Unknown Exception: " << __throwExceptionName << endl );
    throw;
}

    SOS_DELETE( _parser );
    LOG( "Print_file_sv::close: after delete parser ...\n" );
    //if ( _printer != NULL ) SOS_DELETE( _printer );
    //LOG( "Print_file_sv::close: after delete printer ...\n" );
    _opened = false;
    LOG( "Print_file_sv::close: Ende ...\n" );
}

//-------------------------------------------------Print_file_sv::StartPrintHdl

void Print_file_sv::StartPrintHdl( Printer* )
{
    LOG( "Print_file_sv::StartPrintHdl\n" );
   _abort_dlg.Show();
}

//-------------------------------------------------Print_file_sv::EndPrintHdl

void Print_file_sv::EndPrintHdl( Printer* ) {
    LOGI( "Print_file_sv::EndPrintHdl: start ...\n" );
    _printing = false;
    _abort_dlg.Hide();
    LOG( "Print_file_sv::EndPrintHdl: ende ...\n" );
}

//-------------------------------------------------Print_file_sv::PrintPageHdl

void Print_file_sv::PrintPageHdl( Printer* ) {
   String pages;
   String tmp;

   LOGI( "Print_file_sv::PrintPageHdl: start ...\n" );

   if ( _pages == PAGES_UNKNOWN ) {
     pages = String( "?" );
   } else {
     pages = String( _pages );
   }

   tmp  = "Gedruckt wird Seite ";
   tmp += String( _static_printer->GetCurPrintPage() );
   tmp += " von ";
   tmp += pages;

   _abort_dlg.aPagesText.SetText( tmp );

}

//-------------------------------------------------Print_file_sv::AbortJobHdl

void Print_file_sv::AbortJobHdl( CancelButton* ) {
    LOG( "Print_file_sv::AbortJobHdl\n" );
    _aborting = true;
    _abort_dlg.Hide();
    _printing = false;
    _static_printer->AbortJob();
    pApp->GetAppWindow()->Enable();
}



//-----------------------------------------------------------Print_file_sv::put

void Print_file_sv::put_record( const Const_area& area )
BEGIN

    if ( !_printing || _aborting )  throw_xc( "ABORTJOB" ); // throw_cancel_error( "CAN-???" );
    // ESC-RS abfangen ... (schnelle Lösung)
    if ( area.length() >= 3 && memcmp( area.char_ptr(), "\x1B" "RS", 3 ) == 0 )
    {
        LOG( "ESC-RS entfernt ..." << endl );

        if ( area.length() > 3 )
        {
            Const_area area2( area.char_ptr()+3, area.length()-3 );
            _parser->put( area2 );
        }
    } else {
        _parser->put( area );
    }

  exceptions
END


//---------------------------------------------------------Print_file_sv::flush

void Print_file_sv::flush()
{
}



#ifdef JS_TEST_WPRTFILE__MAIN

class MyApp : Application
{
    void Main( int argc, char* argv[] );
};


void MyApp::Main( int argc, char* argv[] )
{
    char fontname[100];
    fontname[0] = 0;

    _InitEasyWin();

    cout << "Font eingeben: " << flush;
    cin >> fontname;

    if ( strlen(fontname) == 0 ) return;

    WorkWindow _window( NULL, WB_APP|WB_STDWORK );
    _window.Show();

/*    if ( argc != 2 )
    {
        cout << "usage: test <fontname>" << endl; return;
    }
    strcpy( fontname, argv[1] );
*/
    ofstream out( "test.log" );
    log_ptr = &out;

    Fontwidth_file f( "", File_base::out|File_base::trunc );

    int i;
    int j;

    char buffer [1000];
    Area area( buffer, sizeof buffer );

    while ( strlen(fontname)!=0 )
    {
        Sos_binary_istream s( area );
        LOG( endl << "Zeichenbreiten für " << fontname << ": " << endl );

        f.get( area, Key( Const_area(fontname) ) ); xc;


        for ( i=0; i<16; i++ )
        {
            LOG( i*16 << " :  " );
            for ( j=0; j<16; j++ )
            {
                uint2 u;
                s.read_uint2( &u );
//                LOG( "w[" << (char)(i*16+j) << "]=" << u << ", " );
                LOG( (char)(i*16+j) << "=" << u << ", " );
            }
            LOG( endl );
        };
        cout << endl;
        cout << "Neuer Font: " << flush;
        cin >> fontname;
    }

    log_ptr = 0;
    
exceptions
    cout << "Fehler: " << _XC.error_code() << endl;
};

MyApp aApp;

#endif

#endif

#endif // SYSTEM_BORLAND
