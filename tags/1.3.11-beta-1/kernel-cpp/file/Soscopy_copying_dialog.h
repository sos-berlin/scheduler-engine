// Soscopy_copying_dialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Soscopy_copying_dialog dialog

class Soscopy_copying_dialog : public CDialog
{
// Construction
public:
	Soscopy_copying_dialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Soscopy_copying_dialog)
	enum { IDD = IDD_COPYING_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Soscopy_copying_dialog)
	//}}AFX_VIRTUAL

// Implementation

  public:
    void                        stop_timer              ();

    Fill_zero                  _zero_;
    uint4                      _start_msec;
    uint4                      _copied;
    Bool                       _abort; 

protected:

	// Generated message map functions
	//{{AFX_MSG(Soscopy_copying_dialog)
	virtual void OnCancel();
	afx_msg void OnTimer(UINT nIDEvent);
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
