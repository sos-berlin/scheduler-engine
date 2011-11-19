/* sossock.h                                    (c) SOS GmbH Berlin
                                                    Joacim Zschimmer

   Klasse Sos_socket für Sockets für BSD und Windows winsock.dll.
*/

#ifndef __SOSSOCK_H
#define __SOSSOCK_H

#include "sossock1.h"

#if !defined __SOSFILTR_H
#   include "sosobj.h"
#   include "sosmsg.h"
#   include "sosfiltr.h"
#endif

#if defined SYSTEM_WIN
#   if defined SYSTEM_WIN16
        SOS_DECLARE_MSWIN_HANDLE( HWND )
#   endif
#endif

struct sockaddr;    // in winsock.h
struct sockaddr_in; // in winsock.h

namespace sos
{


struct SOS_CLASS Sos_socket;


struct Sos_socket_reverse : Sos_msg_filter
{
                                Sos_socket_reverse      ( Sos_socket* s ) : _sos_socket_ptr ( s ) {}
  protected:
    void                       _obj_msg                 ( Sos_msg* );
#   if !defined SYSTEM_RTTI
        void                   _obj_print               ( ostream* s ) const  { *s << "Sos_socket_reverse"; }
#   endif

  private:
    Sos_socket*                _sos_socket_ptr;
};


struct Sos_socket : Sos_msg_filter
{
    BASE_CLASS( Sos_msg_filter )

    enum Error_code
    {
        eintr            =  4,
        ebadf            =  9,
        eacces           = 13,
        efault           = 14,
        einval           = 22,
        emfile           = 24,
        ewouldblock      = 35,
        einprogress      = 36,
        ealready         = 37,
        enotsock         = 38,
        edestaddrreq     = 39,
        emsgsize         = 40,
        eprototype       = 41,
        enoprotoopt      = 42,
        protonosupport   = 43,
        esocktnosupport  = 44,
        eopnotsupp       = 45,
        epfnosupport     = 46,
        eafnosupport     = 47,
        eaddrinuse       = 48,
        eaddrnotavail    = 49,
        enetdown         = 50,
        enetunreach      = 51,
        enetreset        = 52,
        econnaborted     = 53,
        econnreset       = 54,
        enobufs          = 55,
        eisconn          = 56,
        enotconn         = 57,
        eshutdown        = 58,
        etoomanyrefs     = 59,
        etimedout        = 60,
        econnrefused     = 61,
        eloop            = 62,
        enametoolong     = 63,
        ehostdown        = 64,
        ehostunreach     = 65,
        sysnotready      = 91,
        vernotsupported  = 92,
        notinitialised   = 93,
        host_not_found   = 1001,
        try_again        = 1002,
        no_recovery      = 1003,
        no_data          = 1004,
        unknown_error    = 9999
    };

    enum Receive_flags  { recv_none, recv_out_of_band, recv_peek };
    enum Send_flags     { send_none, send_out_of_band, send_dont_route };
    enum Status         { sta_none       = 0,
                          sta_listening  = 0x01,
                          sta_connecting = 0x02,
                          sta_receiving  = 0x04,
                          sta_sending    = 0x08,
                          sta_closing    = 0x10,
                          sta_ending     = 0x20 };

                                Sos_socket              ( int socket_type );
    virtual                    ~Sos_socket              ();

    SOCKET                      socket                  () const;
    Bool                        close_socket            ( Close_mode );
    void                        send                    ( const Const_area&, Send_flags = send_none );
    void                        receive                 ( Area&, Receive_flags = recv_none );
    void                        flush                   ();                     // In MS-Windows Dummy

#   if defined SOSSOCK_WND
        int                     mswin_message_event     ( uint2 msg, uint2 wParam, uint4 lParam );
#   endif

    static void                 select                  ();
    void                        event                   ( Status );


    static int                  open_event_count        ();
    static Sos_string           my_host_name            ();
    static Error_code           normalized_error        ( int system_error_code );

    void                        try_send                ();
    void                        try_receive             ();

    static void                 init                    ();

  protected:
    void                       _obj_print               ( ostream* s ) const;
    Sos_ptr<Sos_msg_filter>    _obj_create_reverse_filter( Sos_msg_filter* client = 0 );

    SOS_DECLARE_MSG_DISPATCHER

  private:
    friend struct               Sos_socket_manager;
    friend struct               Sos_socket_reverse;
    friend struct               Sos_socket_descr;

    void                       _obj_open_msg            ( Open_msg* );
    void                       _obj_run_msg             ( Run_msg* );
    void                       _obj_data_msg            ( Data_msg* );
    void                       _obj_ack_msg             ( Ack_msg* );
    void                       _obj_end_msg             ( End_msg* );
    void                       _obj_get_msg             ( Get_msg* );

    virtual Const_area_handle  _obj_client_name         () const;

    void                        make_socket             ( int address_format, int type  );
    void                        save_peername           ( struct sockaddr_in peer_addr );
    SOCKET                      accept_connection       ();
    void                        connected               ();
    void                        status                  ( Status );
    void                        status_clear            ( Status );

    Fill_zero                  _zero_;
    Sos_socket_manager*        _socket_manager_ptr;
    int                        _socket_type;
    Sos_object_ptr             _runner;
    Sos_object_ptr             _get_requestor_ptr;
    Status                     _status;
    SOCKET                     _socket;
    uint2                      _msg;
    Dynamic_area               _send_data;
    uint                       _sent_length;
    Send_flags                 _send_flags;
    Area*                      _receive_area_ptr;
    Receive_flags              _receive_flags;
    int                        _unuseable_errno;
    Dynamic_area               _receive_buffer;
    uint                       _receive_length;
    uint                       _buffer_size;        // if !_sam3
    Sos_socket_reverse         _reverse;
    Bool                       _continue;           // open() ok, trotz fehlendem accept()
    Bool                       _collecting_length;  // js
    Bool                       _sam3;               // jz 5.1.96
    Area                       _receive_sam3_area;  // js
    Byte                       _length_bytes [ 3 ]; // jz 5.1.96
    Bool                       _nl;
    Bool                       _send_nl;
    Dynamic_area               _overflow_buffer;    // Rest nach '\n', bei _nl
    Sos_string                 _peer_name;
};


int socket_errno();

//==========================================================================================inlines

//-------------------------------------------------------------------------------Sos_socket::socket

inline SOCKET Sos_socket::socket() const
{
    return _socket;
}


} //namespace sos

#endif
