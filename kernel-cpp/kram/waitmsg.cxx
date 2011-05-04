#include <precomp.h>
#define MODULE_NAME "waitmsg"
#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
#define AUTHOR      "Joacim Zschimmer"

#if !defined SYSTEM_STARVIEW

#include <sos.h>
#include <waitmsg.h>

int WAITMSG_NUR_MIT_STARVIEW_IMPLEMENTIERT;

Wait_msg_box::Wait_msg_box( Bool*, int4 timeout, const char* )  {}
Wait_msg_box::~Wait_msg_box()  {}

#else

#include <sv.hxx>
#include <svstring.h>
#include <sosstrng.h>
#include <sos.h>
#include <log.h>
#include <sosctrl.hrc>
#include <kram.hrc>
#include <waitmsg.h>

struct Wait_msg_box_dialog : ModelessDialog
{
                                Wait_msg_box_dialog     ()
                                :
                                    ModelessDialog( pApp->GetAppWindow(),
                                                    ResId( res_wait_msg_box_dialog ) ),
                                    _cancel_button( this, ResId( sos_cancel_button ) )
                                {}

    CancelButton               _cancel_button;
};


struct Wait_msg_box_impl : LinkHdl
{
                                Wait_msg_box_impl       ( Bool*, int4 timeout = -1,
                                                          const char* title=0 );
                               ~Wait_msg_box_impl       ();

  private:
    void                        timeout_handler         ( Timer* );
    void                        button_click            ( CancelButton* );

    Wait_msg_box_dialog*        _dialogbox;
    Bool*                      _abbruch_ptr;
    int4                       _timeout;
    Timer                      _timer;
    Sos_string                 _title;
};



Wait_msg_box::Wait_msg_box( Bool* abbruch, int4 timeout, const char* title )
:
    _impl_ptr ( 0 )
{
    static Bool resource_checked = false;
    static Bool resource_valid   = false;

    if( !resource_checked ) {
        resource_checked = true;
        Resource* p = 0;
        resource_valid = Resource::GetResManager()->IsAvailable( RSC_MODELESSDIALOG,
                                                            res_wait_msg_box_dialog, 0, p );
        LOG( "waitmsg.cxx: Ressource nicht eingebunden\n" );
    }

    if( resource_valid )
    {
        _impl_ptr = new Wait_msg_box_impl( abbruch, timeout, title );
    }
}

Wait_msg_box::~Wait_msg_box()
{
    delete _impl_ptr;
}


Wait_msg_box_impl::Wait_msg_box_impl( Bool* abbruch, int4 timeout, const char* title )
:
    _dialogbox   ( 0 ),
    _abbruch_ptr ( abbruch ),
    _timeout     ( timeout ),
    _title       ( title )
{
    *_abbruch_ptr = false;

    _timer.ChangeTimeout( timeout * 1000L );
    _timer.ChangeTimeoutHdl( LINK( this, Wait_msg_box_impl, timeout_handler ) );
    _timer.Start();
}


Wait_msg_box_impl::~Wait_msg_box_impl()
{
    _timer.Stop();
    delete _dialogbox;
}


void Wait_msg_box_impl::timeout_handler( Timer* )
{
    _dialogbox = new Wait_msg_box_dialog();
    if( !empty( _title ) )  _dialogbox->SetText( as_sv_string( _title ) );
    _dialogbox->_cancel_button.ChangeClickHdl( LINK( this, Wait_msg_box_impl, button_click ) );
    _dialogbox->Show();
}

void Wait_msg_box_impl::button_click( CancelButton* )
{
    *_abbruch_ptr = true;
}

#endif
