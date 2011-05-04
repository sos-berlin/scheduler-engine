/* soswncom.h                                    (c) SOS GmbH Berlin
                                                     Joacim Zschimmer

   Klasse Sos_mswin_comm für Kommunikation über MS-Windows-Botschaften (SOS-spezifisch).
*/

#ifndef __SOSWNCOM_H
#define __SOSWNCOM_H

#if !defined __SOSOBJ_H
#   include <sosobj.h>
#endif

#if defined SYSTEM_WIN
    //#ifdef __WINDOWS_H
    //    const sos_socket_msg = WM_USER;
    //#endif
    SOS_DECLARE_MSWIN_HANDLE( HWND )
#endif

struct Mswin_comm_buffer;

struct Sos_mswin_comm : Sos_msg_filter, Has_mswin_message_handler
{
    BASE_CLASS( Sos_msg_filter )

                                Sos_mswin_comm          ( Sos_object* creator );
  //virtual                    ~Sos_mswin_comm          ();

    static int                  open_event_count        ();

    void                        try_send                ();
    void                        try_receive             ();

  protected:
    void                       _obj_run_msg             ( Run_msg* );
    void                       _obj_data_msg            ( Data_msg* );
    void                       _obj_ack_msg             ( const Ack_msg* );
  //void                       _obj_end_msg             ( End_msg* );
  //void                       _obj_get_msg             ( Get_msg* );
  //void                       _obj_print               ( ostream* s ) const;

  private:
    friend class                Sos_mswin_comm_manager;
    long                        mswin_message_handler   ( HWND, UINT, WPARAM, LPARAM, Bool* );

    Sos_mswin_comm_manager*    _manager_ptr;
    Sos_object*                _factory_ptr;        // Nur für Konstruktion
    Sos_object*                _input_ptr;

    HWND                       _hwnd;
    Mswin_comm_buffer*         _send_buffer_ptr;
    HWND                       _remote_hwnd;
    Mswin_comm_buffer*         _remote_data_ptr;
    Bool                       _run_mode;
    uint                       _msg;
};

#endif
