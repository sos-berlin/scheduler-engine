// Soscopyw.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Soscopy_dialog dialog

class Soscopy_dialog : public CDialog
{
// Construction
public:
	Soscopy_dialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Soscopy_dialog)
	enum { IDD = IDD_SOSCOPY };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Soscopy_dialog)
	//}}AFX_VIRTUAL

// Implementation
    Fill_zero                      _zero_;
    Bool                           _abort;
/*
    CEdit                          _input_edit;
    CEdit                          _output_edit;
    CStatic                        _count_static;
    CButton                        _abort_button;
    CButton                        _ok_button;
*/

protected:

	// Generated message map functions
	//{{AFX_MSG(Soscopy_dialog)
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
