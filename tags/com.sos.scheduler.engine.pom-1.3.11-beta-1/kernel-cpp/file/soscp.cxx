#define MODULE_NAME "soscp"
#define COPYRIGHT   "©1996 SOS GmbH Berlin"
#define AUTHOR      "Joacim Zschimmer"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosopt.h"
#include "../file/anyfile.h"
#include "../kram/licence.h"
#include "../kram/msec.h"
#include "../kram/sosappl.h"
#include "../kram/sosprof.h"
#include "../kram/log.h"
#include "../file/hostcopy_version.h"


#include "soscopy.h"

#if defined SOSCOPY_DIALOG
#   include "soscopy.hrc"
#   include <Soscopy_dialog.h>
#   include <Soscopy_copying_dialog.h>
#endif

#ifdef SYSTEM_WIN16
#   define HOSTCOPY_NAME "hostcp16"
# else
#   define HOSTCOPY_NAME "hostcopy"
#endif


using namespace std;
namespace sos {

//---------------------------------------------------------------------------------------------

extern const Bool _dll = false;
extern Bool     sossock_reschedule;
Soscopy_app     app;
Sos_program*    app_ptr = &app;

//---------------------------------------------------------------------Soscopy_app::Soscopy_app

Soscopy_app::Soscopy_app()
:
    _zero_(this+1),
    _count ( -1 )               // alle

{
}

//----------------------------------------------------------------------------Soscopy_app::main

int Soscopy_app::main( int argc, char** argv )
{
    Bool reverse = false;

#if 0
    NET_API_STATUS ret;
    void**         info;
    CString         netname;
    
    netname = "desktop";

    ret = NetShareGetInfo( NULL, (LPTSTR)((LPCTSTR)netname), 2, (Byte**)info );
    if ( ret == NERR_Success ) {
        SHOW_ERR( "NetShareGetInfo => " << ((SHARE_INFO_2*)*info)->shi2_path << '\n' );
        NetApiBufferFree( info );
    } else {
        Sos_string s;
        switch( ret ) {
        case ERROR_ACCESS_DENIED:       s = "ERROR_ACCESS_DENIED";      break; // The user does not have access to the requested information. 
        case ERROR_INVALID_LEVEL:       s = "ERROR_INVALID_LEVEL";      break; // The value specified for the Level parameter is invalid.  
        case ERROR_INVALID_PARAMETER:   s = "ERROR_INVALID_PARAMETER";  break; // The specified parameter is invalid. 
        case ERROR_MORE_DATA:           s = "ERROR_MORE_DATA";          break; // More entries are available with subsequent calls. 
        case ERROR_NOT_ENOUGH_MEMORY:   s = "ERROR_NOT_ENOUGH_MEMORY";  break; // Insufficient memory is available. 
        case NERR_BufTooSmall:          s = "NERR_BufTooSmall";         break; // The supplied buffer is too small. 
        case NERR_NetNameNotFound:      s = "NERR_NetNameNotFound";     break; // The sharename does not exist. 
        }
        SHOW_ERR( "NetShareGetInfo( '" << netname << "' ) failed => " << s << '\n');
    }

#endif

#   if defined SOSCOPY_DIALOG
        _dlg =  new Soscopy_dialog();
        _dlg->Create( IDD_SOSCOPY_DIALOG );
        m_pMainWnd = _dlg;
        m_pMainWnd->ShowWindow( m_nCmdShow );
        m_pMainWnd->UpdateWindow();
#   endif

    sossock_reschedule = true;  // Windows-Botschaften können uns nicht irritieren


#   ifdef SYSTEM_WIN
      Sos_option_iterator opt ( GetCommandLine() );      // In Windows(argc,argv) sehen "-type=a 1.txt" und -type="a 1.txt" gleich aus.
      opt.skip_param();
#   else
      Sos_option_iterator opt ( argc, argv );
#   endif

    opt.max_params( 2 );
    for(; !opt.end(); opt.next() ) {
        //if( opt.with_value( "sos.ini" ))  { Sos_string var = "SOS_INI="; var += opt.value(); putenv( c_str( var ) ); }
        if( opt.flag      ( "V"      ) )  fprintf( stderr, "hostcopy %s\n", VER_PRODUCTVERSION_STR );
        else
        if( opt.with_value( "sos.ini" ))  set_sos_ini_filename( opt.value() );
        else
        if( opt.flag( "open-out-first" ))  _open_out_first = opt.set();
        else
        if( opt.with_value( "count" ))  _count = opt.as_int();
        else
        if( opt.with_value( "skip"  ))  _skip = opt.as_int();
        else
        if( opt.flag( 'v', "verbose" ))  _verbose = opt.set();
        else
        if( opt.with_value( 'v', "verbose" ))  _verbose = opt.as_uintK();
        else
        if( opt.flag( 'r', "reverse" ))  reverse = opt.set();
        else
        if( opt.with_value( "log"   ))  log_start( opt.value() );
        else
        if( opt.flag      ( "single-step"   ))  _single_step = opt.set();
        else
        if( opt.with_value( "i"     ))  _input_file_filename = opt.value();
        else
        if( opt.with_value( "o"     ))  _output_file_filename = opt.value();
        else
        if( opt.with_value( "files" ))  _files_filename = opt.value();
        else
        if( opt.param     ( 1       ))  _input_filename = opt.value();
        else
        if( opt.param     ( 2       ))  _output_filename = opt.value();
        else {
            goto USAGE;
        }
    }


    if( !SOS_LICENCE( licence_soscopy ) )  throw_xc( "SOS-1000", "soscopy" );

    if( reverse ) {
        Sos_string str = _output_filename;
        _output_filename = _input_filename;
        _input_filename = str;
    }

    if( !empty( _input_file_filename ) ) {
        _output_filename = _input_filename;
        Dynamic_area fn ( 32000 );
        ifstream f ( c_str( _input_file_filename ), ios::in | IOS_BINARY  );
        while(1) {
            int c = f.get();
            if( c == EOF ) break;
            fn += (char)c;
        }

        if( f.fail() )  throw_errno( errno, c_str( _input_file_filename ) );
        _input_filename = as_string( fn );
    }

    if( !empty( _output_file_filename ) ) {
        Dynamic_area fn ( 32000 );
        ifstream f ( c_str( _output_file_filename ), ios::in | IOS_BINARY  );
        while(1) {
            int c = f.get();
            if( c == EOF ) break;
            fn += (char)c;
        }

        if( f.fail() )  throw_errno( errno, c_str( _output_file_filename ) );
        _output_filename = as_string( fn );
    }


  //if( _input_filename  == "-" )  _input_filename  = "/dev/stdin";  /*SOS-Name*/
  //if( _output_filename == "-" )  _output_filename = "/dev/stdout"; /*SOS-Name*/

{
    int4 start_msec = (int4)elapsed_msec();


    if( !empty( _files_filename ) ) {
        if( !empty( _input_filename ) ) goto USAGE;
        if( !empty( _output_filename ) ) goto USAGE;
/* baustelle...
        Any_file file_list;
        file_list.open( _files_filename, Any_file::in_seq );
        Sos_ptr<Record_type> type 
        while( !file_list.eof() ) {
            Dynamic_area
        }
*/
    }
    else
    {
#       if defined SOSCOPY_DIALOG
            _dlg->SetDlgItemText( IDC_INPUT , _input_filename );
            _dlg->SetDlgItemText( IDC_OUTPUT, _output_filename );
            if( _skip > 0    )  _dlg->SetDlgItemInt ( IDC_SKIP , _skip );
                          else  _dlg->SetDlgItemText( IDC_SKIP , "" );
            if( _count >= 0  )  _dlg->SetDlgItemInt ( IDC_COUNT, _count );
                          else  _dlg->SetDlgItemText( IDC_COUNT, "" );

            //_dlg->ShowWindow( SW_SHOWNA );
            //_dlg->UpdateWindow();
            //MSG msg;
            //while( ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )  PumpMessage();

            _dlg->press_ok();

            while(1) {
                PumpMessage();
                if( _dlg->_abort )  break;
                if( !_dlg->IsWindowVisible() )  break;
            }

#       else

            //int4 copied = ::copy_file( _input_filename, _output_filename, _count, _skip );
            int4 copied = copy_file( NULL );

            if( _verbose ) {
                int4 msec = (int4)elapsed_msec() - start_msec;
                SHOW_MSG( "\nDatei " << _input_filename <<
                          "\nist kopiert nach " << _output_filename <<
                          "\n" << copied << " Sätze in " << ( (double)msec / 1000 ) << "s" );
            }
#       endif
    }

    return 0;
}

USAGE:
    SHOW_ERR( "usage:\n"
              HOSTCOPY_NAME
                      " -v        verbose - schreibt einen Punkt für jeden Satz\n"
              "         -v=N      verbose - schreibt einen Punkt alle N Sätze\n"
              "         -skip=N   number of records to skip\n"
              "         -count=N  number of records to copy\n"
              "         -open-out-first  number of records to copy\n"
              "         -reverse  Vertauscht Eingabe- und Ausgabedateinamen\n"
              "         -files=FILELIST Liste der Dateinamen\n"
              "         INPUTFILE OR -i=INPUTFILEFILE\n"
              "         OUTPUTFILE OR -o=OUTPUTFILEFILE" );
    return 9999;
}

//-----------------------------------------------------------------------Soscopy_app::copy_file

uint4 Soscopy_app::copy_file( Soscopy_copying_dialog* copying_dialog )
{
    Any_file     source;
    Any_file     dest;
    File_spec    spec;
    Dynamic_area record;
    long         copied = 0;

#   if defined SOSCOPY_DIALOG
        //copying_dialog->UpdateWindow();
        ::EnableWindow( copying_dialog->GetDlgItem( IDC_COPYING_INPUT_OPEN )->m_hWnd, TRUE );
#   endif

    if( _open_out_first ) {
        dest.open( _output_filename, File_base::out, spec );
    }

    source.open( _input_filename, File_base::Open_mode( File_base::in | File_base::seq ) );

#   if defined SOSCOPY_DIALOG
        ::EnableWindow( copying_dialog->GetDlgItem( IDC_COPYING_INPUT_OPEN_OK )->m_hWnd, TRUE );
        ::EnableWindow( copying_dialog->GetDlgItem( IDC_COPYING_OUTPUT_OPEN )->m_hWnd, TRUE );
#   endif

    if( !_open_out_first ) {
        spec._field_type_ptr = source.spec()._field_type_ptr;

        if( spec._field_type_ptr  &&  _output_filename == "-" )  _output_filename = "record/tabbed | -";  // etwas Komfort

        dest.open( _output_filename, File_base::out, spec );
    }

#   if defined SOSCOPY_DIALOG
        ::EnableWindow( copying_dialog->GetDlgItem( IDC_COPYING_OUTPUT_OPEN_OK )->m_hWnd, TRUE );
        ::EnableWindow( copying_dialog->GetDlgItem( IDC_COPYING_COPIED_STATIC )->m_hWnd, TRUE );
#   endif

        while( _skip-- ) {
            source.get( &record );    // Vorzeitiges EOF wird als Fehler gemeldet

#           if !defined SOSCOPY_DIALOG && !defined SYSTEM_WIN16
                if( _verbose  &&  copied % _verbose == 0 )  cerr << ",";
#           endif
        }

    while(1)
    {
#       if defined SOSCOPY_DIALOG
            MSG msg;
            //BOOL ok = GetMessage( &msg, 0, 0, 0 )
            //if( !ok )  break;
            BOOL ok = PeekMessage( &msg, 0, 0, 0, PM_REMOVE );
            if( ok ) {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }

            if( copying_dialog->_abort )  break;
#       endif

        if( _count >= 0 )  if( _count-- == 0 )  break;

        try
        {
            try {
                source.get( &record );
            }
            catch( const Eof_error& ) { break; }
           
            dest.put( record );
        }
        catch( Xc& x ) 
        { 
            x.insert( "record #" + as_string( copied+1 ) ); 
            throw; 
        }

        copied++;

#       if !defined SOSCOPY_DIALOG  &&  !defined SYSTEM_WIN16
            if( _verbose  &&  copied % _verbose == 0 )  cerr << ".";
#       endif

        if( _single_step ) {
            SHOW_MSG( copied << " Sätze" );
        }

#       if defined SOSCOPY_DIALOG
            copying_dialog->_copied = copied;
            //copying_dialog->SetDlgItemInt( IDC_COPYING_COPIED, copied );
#       endif
    }
          
    dest.close();
    source.close();

    return copied;
}

//--------------------------------------------------------------------Soscopy_app::ExitInstance
#if defined SYSTEM_MFC

BOOL Soscopy_app::ExitInstance()
{
#   ifdef SOSCOPY_DIALOG
        SOS_DELETE( _dlg );
#   endif

    return Sos_program::ExitInstance();
}


#endif
//-------------------------------------------------------------------------------------sos_main

int sos_main( int argc, char** argv )
{
    return app.main( argc, argv );
}

} //namespace sos
