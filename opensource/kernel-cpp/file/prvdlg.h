/*******************************************************************
*  PRVDLG.HXX
*  (c) 1992-1994 STAR DIVISION
*******************************************************************/

// --- class PreviewBox --------------------------------------------

class PreviewBox : public Control
{
public:
    PreviewBox( Window* pParent, const ResId& rResId );
};

// --- class PreviewDialog -----------------------------------------

class PreviewDialog : public ModalDialog
{
private:
    PreviewBox      aPVBox;
    ScrollBar       aVScroll;
    ScrollBar       aHScroll;
    PushButton      aZoomIn;
    PushButton      aZoomOut;
    PushButton      aNormalPage;
    PushButton      aFullPage;
    PushButton      aDoublePage;
    PushButton      aPrevPage;
    PushButton      aNextPage;
    CancelButton    aCancel;

    USHORT          nPage;
    Fraction        aZoom;
    Point           aOffset;
    Size            aMaxSize;
    Link            aRequestPageHdl;
    Preview*        pPreview;
    Preview*        pSecondPreview;
    JobSetup*       pSetup;

    Size            CalcPreviewSize();
    void            CheckScrollRange();

public:
                    PreviewDialog( Window* pParent,
                                   JobSetup* pSetup );
                    ~PreviewDialog();

    Link            ChangeRequestPageHdl( const Link& rLink );

    void            VScrollEndHdl( ScrollBar* pScrollBar );
    void            HScrollEndHdl( ScrollBar* pScrollBar );
    void            ZoomInHdl( Button* );
    void            ZoomOutHdl( Button* );
    void            NormalPageHdl( Button* );
    void            FullPageHdl( Button* );
    void            DoublePageHdl( Button* );
    void            PrevPageHdl( Button* );
    void            NextPageHdl( Button* );

    void            PrintPage( Preview* );
};
