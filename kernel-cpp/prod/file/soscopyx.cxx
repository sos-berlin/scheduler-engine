// Soscopyw.cxx : implementation file
//

#include "resource.h"
//#include "stdafx.h"
#include <afxwin.h>         // MFC core and standard components
//#include <afxext.h>         // MFC extensions
//#include <windowsx.h>
//#include <vfw.h>

//#include "soscopy.h"
#include <sos.h>
#include "soscopyw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


extern int _argc;
extern char** _argv;
extern int soscopy( int, char** );


/////////////////////////////////////////////////////////////////////////////
// Soscopy_dialog dialog


Soscopy_dialog::Soscopy_dialog(CWnd* pParent /*=NULL*/)
: 
    CDialog(Soscopy_dialog::IDD, pParent),
    _zero_ (this+1)
{
	//{{AFX_DATA_INIT(Soscopy_dialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
/*
    _input_edit.Create( WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP, CRect(1,1,1,1), this, IDC_INPUT );
    _output_edit.Create( WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP, CRect(2,2,2,2), this, IDC_OUTPUT );
    _ok_button.Create( WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP, CRect(3,3,3,3), this, IDOK );
    _abort_button.Create( WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP, CRect(4,4,4,4), this, IDABORT );
*/
}


BEGIN_MESSAGE_MAP(Soscopy_dialog, CDialog)
	//{{AFX_MSG_MAP(Soscopy_dialog)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Soscopy_dialog message handlers

void Soscopy_dialog::OnOK() 
{
    // TODO: Add extra validation here
	
    CDialog::OnOK();
    soscopy( _argc, _argv );
}

void Soscopy_dialog::OnCancel() 
{
    // TODO: Add extra cleanup here

    _abort = true;
    CDialog::OnCancel();
}

void Soscopy_dialog::OnClose() 
{
	// TODO: Add your message handler code here and/or call default

    _abort = true;
	
	//? CDialog::OnClose();
}
