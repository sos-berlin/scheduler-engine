// Soscopy_dialog.cxx : implementation file
//

#include "afxwin.h"

#include <sosstrng.h>
#include <sos.h>
#include <msec.h>
#include <soscopy.h>
#include "soscopy.hrc"
#include <Soscopy_copying_dialog.h>
#include "Soscopy_dialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Soscopy_dialog dialog


Soscopy_dialog::Soscopy_dialog(CWnd* pParent /*=NULL*/)
: 
    CDialog(Soscopy_dialog::IDD, pParent),
    _zero_(this+1)

{
	//{{AFX_DATA_INIT(Soscopy_dialog)
	//}}AFX_DATA_INIT

    _ids_copy_finished.LoadString( IDS_COPY_FINISHED );
}


Soscopy_dialog::~Soscopy_dialog()
{
    SOS_DELETE( _copying_dlg );
}


void Soscopy_dialog::mini_title( const Sos_string& title )
{
    if( _use_mini_title )  SetWindowText( title );
}


BEGIN_MESSAGE_MAP(Soscopy_dialog, CDialog)
	//{{AFX_MSG_MAP(Soscopy_dialog)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Soscopy_dialog message handlers

void Soscopy_dialog::OnClose() 
{
    _abort = true;
    DestroyWindow();

	//CDialog::OnClose();
}


void Soscopy_dialog::OnOK() 
{
    if( _locked )  return;
    _locked++;

    EnableWindow( FALSE );
    Sos_string text;

    GetDlgItemText( IDC_INPUT , app._input_filename );
    GetDlgItemText( IDC_OUTPUT, app._output_filename );

    GetDlgItemText( IDC_SKIP  , text );
    try {
        app._skip = empty( text )? 0 : as_long( text );
    }
    catch( const Xc& x ) {
        return;
    }

    GetDlgItemText( IDC_COUNT , text );
    try {
        app._count = empty( text )? -1 : as_long( text );
    }
    catch( const Xc& x ) {
        return;
    }


    Soscopy_copying_dialog copying_dlg ( this );
    copying_dlg.Create( IDD_COPYING_DIALOG, this );
    //copying_dlg.SetFocus();
    //copying_dlg.ShowWindow( SW_SHOW );
    //copying_dlg.UpdateWindow();

    //MSG msg;
    //while( ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )  app.PumpMessage();

    try {
        uint4 start_msec = elapsed_msec();

        uint4 count = app.copy_file( &copying_dlg );

        copying_dlg.stop_timer();
        copying_dlg.EnableWindow( FALSE );

        char buffer [ 200 ];
        sprintf( buffer, c_str( _ids_copy_finished ), count, (double)( elapsed_msec() - start_msec ) / 1000 );
        SetDlgItemText( IDC_STATUS, buffer );
    }
    catch( const Xc& x )
    {
        SHOW_MSG( x );
    }
	//CDialog::OnOK();

    EnableWindow();
    _locked--;
}

void Soscopy_dialog::OnCancel() 
{
    _abort = true;
    //DestroyWindow();
	
	//CDialog::OnCancel();
}


void Soscopy_dialog::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);

    if( nType == SIZE_MINIMIZED  ||  nType == SIZE_MAXHIDE ) {
        _use_mini_title = true;
        SetWindowText( _mini_title );
    }
    else
    if( _use_mini_title ) {
        _use_mini_title = false;
        SetWindowText( "SOSCOPY" );
    }
}
