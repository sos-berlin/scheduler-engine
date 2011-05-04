// frmprev.cxx

//#define SOURCE "frmprev.cxx"

#ifdef __BORLANDC__

#include <precomp.h>
#include <sysdep.h>
#include <sos.h>
#include <sv.hxx>

#if defined SYSTEM_WIN  &&  defined SYSTEM_STARVIEW

#include <sosstrng.h>
#include <sosprof.h>
#include <sosopt.h>

#if defined SYSTEM_WIN
#include <svwin.h>
#include <sysdep.hxx>
#define _INC_WINDOWS
#endif

//#include "frmprev.h"
#include "file.hrc"

#include <area.h>
#include <xception.h>
#include <strstrea.h>
#include <log.h>
#include <anyfile.h>


// --- class Frame_preview_app -------------------------------------------------

struct Frame_preview_app : Application
{
                            Frame_preview_app() : _view(true), _print(false), _only_once(false) {}

				long        select          ( Menu* );
				void        do_preview      ();

	 virtual void 			UserEvent		 ( unsigned long nEvent, void* pEventData );
	 virtual void           Main            ( int, char*[] );

private:
				String      filename;
				String      last_filename;
				String      last_filter;
                Bool        _view;
                Bool        _print;
                Bool        _only_once;
};


// --- Frame_preview_app::do_preview() -----------------------------------------

void Frame_preview_app::do_preview()
{
    LOG( "Frame_preview_app::do_preview\n" );
try {
	 char        buffer[256];
	 /*while (1)*/
	 {
		  Any_file input;
		  Any_file output;
		  Area area( buffer, sizeof buffer );

		  if ( filename == "" ) {
				FileDialog aFileDlg( GetAppWindow(), WB_OPEN | WB_SVLOOK | WB_STDMODAL );
                if ( last_filename != "" ) {
                    aFileDlg.SetPath( last_filename );
                } else {
#if defined( SYSTEM_WIN )
				    aFileDlg.SetPath( "c:\\" );
#else
				    aFileDlg.SetPath( "~" );
#endif
                }

				aFileDlg.SetDefaultExt( "esc" );
				aFileDlg.AddFilter( "Frame-ESC-Dateien (*.esc)", "*.esc" );
				aFileDlg.AddFilter( "Frame-ESC-Dateien (*.frm)", "*.frm" );
				aFileDlg.AddFilter( "Alle Dateien (*.*)", "*.*" );
                if ( last_filter != "" ) {
                    aFileDlg.SetCurFilter( last_filter);
                }

				if ( aFileDlg.Execute() )
				{
					filename = aFileDlg.GetPath();
                    last_filename = filename;
                    last_filter   = aFileDlg.GetCurFilter();
				} else return;
		  }

          pApp->Wait( TRUE );
		  // filename = "file: " + filename;
		  input.open( (const char*)filename, File_base::in );
		  output.open( "printer: preview", File_base::out );

		  while (1) {
				try { input.get( area ); } catch ( const Eof_error& ) { break; }
				output.put( (Const_area) area );
		  }
          pApp->Wait( FALSE );
		  filename = "";

		  input.close();
		  output.close();  // hier wird der Preview angezeigt

          if ( _only_once ) pApp->Quit();
	 }
} catch ( const Xc& x ) {
	 SHOW_ERR( "Fehler aufgetreten beim Lesen von " << filename << ": " << x );
	 filename = "";
     if ( _only_once ) pApp->Quit();
     if ( pApp->GetAppWindow() ) {
        pApp->GetAppWindow()->Enable();
     }
}
     pApp->Wait( FALSE );
}

// --- Frame_preview_app::UserEvent() -----------------------------------------------

void Frame_preview_app::UserEvent( unsigned long nEvent, void* /*pEventData*/ )
{
	 switch ( nEvent ) {
		case 1: 	do_preview(); break;
		default: break;
	 }
}

// --- Frame_preview_app::select() -----------------------------------------------

long Frame_preview_app::select ( Menu* pMenu )
{
	 switch ( pMenu->GetCurItemId() ) {
		  case FRMPREV_PREVIEW : filename = ""; do_preview(); break;
		  case FRMPREV_QUIT:     Quit(); break;
		  default:   break;
	 }
	 return 0;
}

// --- Frame_preview_app::Main() -----------------------------------------------
#if defined SYSTEM_WIN
HINSTANCE _hinstance = NULL;
#endif
void Frame_preview_app::Main( int argc, char* argv[] )
{
#if defined SYSTEM_WIN
	 _hinstance = Sysdepen::GethInst();
#endif
try {
	 Bool mit_menubar = false;

	 Sos_option_iterator opt ( argc, argv );

	 opt.max_params( 1 );
	 for(; !opt.end(); opt.next() ) {
		  if( opt.with_value( "sos.ini" ))  set_sos_ini_filename( opt.value() );
		  else
		  if( opt.with_value( "log"   ))   log_start( opt.value() );
		  else
		  if( opt.flag( 'v', "view"   ))   _view = as_bool( opt.set() );
		  else
		  if( opt.flag( 'p', "print"  ))   _print = as_bool( opt.set() );
		  else
		  if( opt.param     ( 1       ))   filename = opt.value();
		  else {
				SHOW_ERR( "usage: FRVIEW16 <Esc-datei>" );
				return;
		  }
	 }

	 EnableSVLook();
     _only_once = filename != "" && (_view||_print);

	 if ( 1 /*it_menubar || filename == ""*/ ) {
         String titel;

         LOG( "before WorkWindow()\n" );
         int style_bits = WB_APP|WB_STDWORK;
         //if ( _only_once ) style_bits |= WB_HIDE;

		 WorkWindow _main_window( NULL, style_bits );
		 MenuBar _menubar( ResId( FRMPREV_MENU ) );

		 _menubar.GetPopupMenu( FRMPREV_FILE )->
		 PushSelectHdl( LINK(this, Frame_preview_app, select ) );

         if ( _only_once ) {
            _main_window.Minimize(); // minimiert starten ...
         }
         ChangeAppMenu( &_menubar );

         if ( filename != "" ) {
            titel = filename + " --- ";
         }
         titel += "Frame Preview";

         _main_window.SetText( titel );
         _main_window.Show();

		 // Datei sofort öffnen
		 if ( filename != "" ) PostUserEvent( 1, NULL );
		 // Programm-Events verarbeiten
		 Execute();
	 } else do_preview(); // Modalen Dialog direkt ausführen ...
} catch ( const Xc& x ) {
    SHOW_ERR( "Fehler aufgetreten: " << x );
}

}

// --- Frame_preview_app aApp ------------------------------------------------------

Frame_preview_app aApp;

#endif

#endif
