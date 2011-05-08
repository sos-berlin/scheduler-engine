// Soscopy_dialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Soscopy_dialog dialog

class Soscopy_copying_dialog;


class Soscopy_dialog : public CDialog
{
// Construction
public:
	Soscopy_dialog(CWnd* pParent = NULL);   // standard constructor
   ~Soscopy_dialog();

// Dialog Data
	//{{AFX_DATA(Soscopy_dialog)
	enum { IDD = IDD_SOSCOPY_DIALOG };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Soscopy_dialog)
	//}}AFX_VIRTUAL

// Implementation

  public:

    void                        press_ok            ()          { OnOK(); }
    void                        mini_title          ( const Sos_string& );

    Fill_zero                  _zero_;
    Bool                       _abort;
    Soscopy_copying_dialog*    _copying_dlg;
    int                        _locked;
    CString                    _ids_copy_finished;
    Bool                       _use_mini_title;
    Sos_string                 _mini_title;

protected:

	// Generated message map functions
	//{{AFX_MSG(Soscopy_dialog)
	afx_msg void OnClose();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
