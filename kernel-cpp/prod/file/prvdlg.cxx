/*******************************************************************
*  PRVDLG.CXX
*  (c) 1992-1994 STAR DIVISION
*******************************************************************/

#include "sv.hxx"

#include "prvdlg.hrc"
#include "prvdlg.hxx"

// --- PreviewBox::PreviewBox() ------------------------------------

PreviewBox::PreviewBox( Window* pParent, const ResId& rResId ) :
                Control( pParent, rResId )
{
    ChangeBackgroundBrush( Brush( Color( COL_GRAY ) ) );
}

// --- PreviewDialog::PreviewDialog() ------------------------------

PreviewDialog::PreviewDialog( Window* pParent, JobSetup* pSetup ) :
                   ModalDialog( pParent, ResId( DLG_PREVIEW ) ),
                   aPVBox( this, ResId( DP_BOX ) ),
                   aVScroll( this, ResId( DP_VSCROLL ) ),
                   aHScroll( this, ResId( DP_HSCROLL ) ),
                   aZoomIn( this, ResId( DP_ZOOMIN ) ),
                   aZoomOut( this, ResId( DP_ZOOMOUT ) ),
                   aNormalPage( this, ResId( DP_NORMALPAGE ) ),
                   aFullPage( this, ResId( DP_FULLPAGE ) ),
                   aDoublePage( this, ResId( DP_DOUBLEPAGE ) ),
                   aPrevPage( this, ResId( DP_PREVPAGE ) ),
                   aNextPage( this, ResId( DP_NEXTPAGE ) ),
                   aCancel( this, ResId( DP_CANCEL ) ),
                   aZoom( 0, 1 )
{
    FreeResource();

    aVScroll.ChangeEndScrollHdl(
        LINK( this, PreviewDialog, VScrollEndHdl ) );
    aHScroll.ChangeEndScrollHdl(
        LINK( this, PreviewDialog, HScrollEndHdl ) );

    aZoomIn.ChangeClickHdl(
        LINK( this, PreviewDialog, ZoomInHdl ) );
    aZoomOut.ChangeClickHdl(
        LINK( this, PreviewDialog, ZoomOutHdl ) );
    aNormalPage.ChangeClickHdl(
        LINK( this, PreviewDialog, NormalPageHdl ) );
    aFullPage.ChangeClickHdl(
        LINK( this, PreviewDialog, FullPageHdl ) );
    aDoublePage.ChangeClickHdl(
        LINK( this, PreviewDialog, DoublePageHdl ) );
    aPrevPage.ChangeClickHdl(
        LINK( this, PreviewDialog, PrevPageHdl ) );
    aNextPage.ChangeClickHdl(
        LINK( this, PreviewDialog, NextPageHdl ) );

    nPage              = 1;
    pPreview           = new Preview( &aPVBox, 0 );
    pSecondPreview     = NULL;
    aMaxSize           = aPVBox.GetSizePixel();
    PreviewDialog::pSetup = pSetup;
    pPreview->SetJobSetup( pSetup );
    ((Window*)pPreview)->ChangeBackgroundBrush(
        Brush( Color( COL_WHITE ) ) );

    pPreview->SetPageQueueSize( 10 );
    pPreview->ChangeRequestPageHdl(
        LINK( this, PreviewDialog, PrintPage ) );
    CalcPreviewSize();
    CheckScrollRange();

    pPreview->Show();
}

// --- PreviewDialog::~PreviewDialog() -----------------------------

PreviewDialog::~PreviewDialog()
{
#ifndef RS6000
    if ( pPreview != NULL )
        delete pPreview;

    if ( pSecondPreview != NULL )
        delete pSecondPreview;
#endif
}

// --- PreviewDialog::CalcPreviewSize() ----------------------------

Size PreviewDialog::CalcPreviewSize()
{
    Size    aPreviewSize;
    Point   aPreviewPos;
    Point   aPageOffset;

    aPreviewSize = pPreview->CalcWindowSizePixel( aMaxSize );

    if ( aPreviewSize.Width() < aMaxSize.Width() )
        aPreviewPos.X() = (aMaxSize.Width()>>1) -
                          (aPreviewSize.Width()>>1);
    if ( aPreviewSize.Height() < aMaxSize.Height() )
        aPreviewPos.Y() = (aMaxSize.Height()>>1) -
                          (aPreviewSize.Height()>>1);

    pPreview->ChangePosPixel( aPreviewPos );
    pPreview->ChangeSizePixel( aPreviewSize );

    return aPreviewSize;
}

// --- PreviewDialog::CheckScrollRange() ---------------------------

void PreviewDialog::CheckScrollRange()
{
    if ( aZoom.GetNumerator() == 0 )
    {
        aHScroll.Hide();
        aVScroll.Hide();
        aHScroll.ChangeThumbPos( 0 );
        aVScroll.ChangeThumbPos( 0 );
        pPreview->ChangePageOffset( Point( 0, 0 ) );
        return;
    }

    Size aPaperSize   = pPreview->GetPaperSize();
    Size aViewSize    = pPreview->GetVisibleSize();
    Size aPreviewSize = pPreview->GetSizePixel();

    if ( aPreviewSize.Width() == aMaxSize.Width() )
    {
        short nThumbPos = aHScroll.GetThumbPos();

        aHScroll.ChangeRange( Range( 0, aPaperSize.Width() -
                                        aViewSize.Width() ) );
        aHScroll.ChangeLineSize( aViewSize.Width() / 10 );
        aHScroll.ChangePageSize( aViewSize.Width() / 3  );

        aHScroll.ChangeThumbPos( nThumbPos );
        aHScroll.Show();
    }
    else
    {
        aHScroll.Hide();
        aHScroll.ChangeThumbPos( 0 );
        Point aOfs = pPreview->GetPageOffset();
        aOfs.X() = 0;
        pPreview->ChangePageOffset( aOfs );
    }

    if ( aPreviewSize.Height() == aMaxSize.Height() )
    {
        short nThumbPos = aVScroll.GetThumbPos();

        aVScroll.ChangeRange( Range( 0, aPaperSize.Height() -
                                        aViewSize.Height() ) );
        aVScroll.ChangeLineSize( aViewSize.Height() / 10 );
        aVScroll.ChangePageSize( aViewSize.Height() / 3  );

        aVScroll.ChangeThumbPos( nThumbPos );
        aVScroll.Show();
    }
    else
    {
        aVScroll.Hide();
        aVScroll.ChangeThumbPos( 0 );
        Point aOfs = pPreview->GetPageOffset();
        aOfs.Y() = 0;
        pPreview->ChangePageOffset( aOfs );
    }

    pPreview->ChangePageOffset( Point( aHScroll.GetThumbPos(),
                                       aVScroll.GetThumbPos() ) );
}

// --- PreviewDialog::ChangeRequestPageHdl() -----------------------

Link PreviewDialog::ChangeRequestPageHdl( const Link& rLink )
{
    Link aOldLink = aRequestPageHdl;
    aRequestPageHdl = rLink;
    return aOldLink;
}

// --- PreviewDialog::VScrollEndHdl() ------------------------------

void PreviewDialog::VScrollEndHdl( ScrollBar* )
{
    aOffset = Point( aHScroll.GetThumbPos(),
                     aVScroll.GetThumbPos() );
    pPreview->ChangePageOffset( aOffset );
    pPreview->Update();
}

// --- PreviewDialog::HScrollEndHdl() ------------------------------

void PreviewDialog::HScrollEndHdl( ScrollBar* )
{
    aOffset = Point( aHScroll.GetThumbPos(),
                     aVScroll.GetThumbPos() );
    pPreview->ChangePageOffset( aOffset );
    pPreview->Update();
}

// --- PreviewDialog::ZoomInHdl() ----------------------------------

void PreviewDialog::ZoomInHdl( Button* )
{
    if ( pSecondPreview != NULL )
        pSecondPreview->Hide();

    if ( aZoom.GetNumerator() == 0 )
        aZoom = Fraction( 1, 4 );
    else
        aZoom *= Fraction( 3, 2 );

    if ( aZoom > Fraction( 16, 1 ) )
        aZoom = Fraction( 16, 1 );

    pPreview->ChangeZoomFactor( aZoom );
    CalcPreviewSize();
    CheckScrollRange();
}

// --- PreviewDialog::ZoomOutHdl() ---------------------------------

void PreviewDialog::ZoomOutHdl( Button* )
{
    if ( pSecondPreview != NULL )
        pSecondPreview->Hide();

    if ( aZoom.GetNumerator() == 0 )
        aZoom = Fraction ( 1, 4 );
    else
        aZoom *= Fraction( 2, 3 );

    if( aZoom < Fraction( 1, 16 ) )
        aZoom = Fraction( 1, 16 );

    pPreview->ChangeZoomFactor( aZoom );
    CalcPreviewSize();
    CheckScrollRange();
}

// --- PreviewDialog::NormalPageHdl() ------------------------------

void PreviewDialog::NormalPageHdl( Button* )
{
    if ( pSecondPreview != NULL )
        pSecondPreview->Hide();

    aZoom = Fraction( 1, 1 );
    pPreview->ChangeZoomFactor( aZoom );
    CalcPreviewSize();
    CheckScrollRange();
}

// --- PreviewDialog::FullPageHdl() --------------------------------

void PreviewDialog::FullPageHdl( Button* )
{
    aVScroll.Hide();
    aHScroll.Hide();

    if ( pSecondPreview != NULL )
        pSecondPreview->Hide();

    aZoom = Fraction( 0, 1 );
    pPreview->ChangeZoomFactor( aZoom );
    CalcPreviewSize();
    CheckScrollRange();
}

// --- PreviewDialog::DoublePageHdl() ------------------------------

void PreviewDialog::DoublePageHdl( Button* )
{
    short X, Y;
    Size  aHalfSize;

    aZoom = Fraction( 0, 1 );
    pPreview->ChangeZoomFactor( aZoom );
    CheckScrollRange();

    if ( pSecondPreview == NULL )
    {
        pSecondPreview = new Preview( &aPVBox, 0 );
        pSecondPreview->SetJobSetup( pSetup );
        ((Window*)pSecondPreview)->ChangeBackgroundBrush(
            Brush( Color( COL_WHITE ) ) );
        pSecondPreview->ChangeRequestPageHdl(
            LINK( this, PreviewDialog, PrintPage ) );
        pSecondPreview->ChangeCurPage( nPage + 1 );
    }
    pSecondPreview->ChangeZoomFactor( aZoom );
    pSecondPreview->ChangePageOffset( Point( 0, 0 ) );
    pSecondPreview->Invalidate();
    pSecondPreview->Show();

    aHalfSize = Size( (aMaxSize.Width() / 2) - 7,
                      aMaxSize.Height() - 10 );
    aHalfSize = pPreview->CalcWindowSizePixel( aHalfSize );
    aHalfSize = pSecondPreview->CalcWindowSizePixel( aHalfSize );

    Y = (aMaxSize.Height() - aHalfSize.Height()) / 2;
    X = aMaxSize.Width() / 2 - aHalfSize.Width() - 2;
    pPreview->ChangePosPixel( Point( X, Y ) );
    X = aMaxSize.Width() / 2 + 2;
    pSecondPreview->ChangePosPixel( Point( X, Y ) );

    pPreview->ChangeSizePixel( aHalfSize );
    pSecondPreview->ChangeSizePixel( aHalfSize );
}

// --- PreviewDialog::PrevPageHdl() --------------------------------

void PreviewDialog::PrevPageHdl( Button* )
{
    if ( nPage > 1 )
    {
        pPreview->ChangeCurPage( --nPage );

        if ( pSecondPreview != NULL )
        {
            pPreview->Update();
            pSecondPreview->ChangeCurPage( nPage + 1 );
            if ( pSecondPreview->IsVisible() )
                pSecondPreview->Update();
        }
        else
            pPreview->Update();
    }
}

// --- PreviewDialog::NextPageHdl() --------------------------------

void PreviewDialog::NextPageHdl( Button* )
{
    if ( nPage < 0xFFFF )
    {
        pPreview->ChangeCurPage( ++nPage );

        if ( pSecondPreview != NULL )
        {
            pPreview->Update();
            pSecondPreview->ChangeCurPage( nPage + 1 );
            if ( pSecondPreview->IsVisible() )
                pSecondPreview->Update();
        }
        else
            pPreview->Update();
    }
}

// --- PreviewDialog::PrintPage() ----------------------------------

void PreviewDialog::PrintPage( Preview* pPrev )
{
    aRequestPageHdl.Call( pPrev );
}
