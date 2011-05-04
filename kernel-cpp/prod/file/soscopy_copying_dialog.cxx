// Soscopy_copying_dialog.cxx : implementation file
//

#include "afxwin.h"
#include <sos.h>
#include <msec.h>
#include "soscopy.h"
#include <soscopy.hrc>
#include "Soscopy_copying_dialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Soscopy_copying_dialog dialog

const int timer_id = 1;

Soscopy_copying_dialog::Soscopy_copying_dialog(CWnd* pParent /*=NULL*/)
	: CDialog(Soscopy_copying_dialog::IDD, pParent),
    _zero_(this+1)
{
	//{{AFX_DATA_INIT(Soscopy_copying_dialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Soscopy_copying_dialog::stop_timer()
{
    KillTimer( timer_id );
}


BEGIN_MESSAGE_MAP(Soscopy_copying_dialog, CDialog)
	//{{AFX_MSG_MAP(Soscopy_copying_dialog)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Soscopy_copying_dialog message handlers

BOOL Soscopy_copying_dialog::OnInitDialog() 
{
    CenterWindow();
	CDialog::OnInitDialog();
	
    SetTimer( timer_id, 100, NULL );
    _start_msec = elapsed_msec();
	
	return FALSE;  // return TRUE unless you set the focus to a control
	               // EXCEPTION: OCX Property Pages should return FALSE
}


void Soscopy_copying_dialog::OnDestroy() 
{
    KillTimer( timer_id );
	CDialog::OnDestroy();
}



void Soscopy_copying_dialog::OnCancel() 
{
    _abort = true;

    SendDlgItemMessage( IDCANCEL, BN_PUSHED );
	
	//CDialog::OnCancel();
}


void Soscopy_copying_dialog::OnTimer(UINT nIDEvent) 
{
    SetDlgItemInt( IDC_COPYING_SEC, ( elapsed_msec() - _start_msec ) / 1000 );
    SetDlgItemInt( IDC_COPYING_COPIED, _copied );
	CDialog::OnTimer(nIDEvent);
}



