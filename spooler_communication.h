// $Id: spooler_communication.h,v 1.11 2003/02/18 21:38:09 jz Exp $

#ifndef __SPOOLER_COMMUNICATION_H
#define __SPOOLER_COMMUNICATION_H

#ifdef __GNUC__
#   include <errno.h>
#   include <sys/socket.h>

#   ifndef SYSTEM_HPUX
#       include <sys/select.h>
#   endif

#   include <netdb.h>
#   include <netinet/in.h>   // gethostbyname()
#   include <arpa/inet.h>    // inet_addr()
#   include <unistd.h>       // close()

    typedef int SOCKET;
    const SOCKET SOCKET_ERROR = (SOCKET)-1;
#endif

namespace sos {
namespace spooler {


//inline bool operator < ( const in_addr& a, const in_addr& b )  { return a.s_addr < b.s_addr; }  // Für map<>


//---------------------------------------------------------------------------------------------Host

struct Host
{
                                Host                        ()                              { _ip.s_addr = 0; }
                                Host                        ( const in_addr& ip )           { set_ip(ip.s_addr); }
                                Host                        ( uint32 ip )                   { set_ip(ip); }

    void                        operator =                  ( const in_addr& ip )           { set_ip(ip.s_addr); }
                                operator in_addr            () const                        { return _ip; }

    bool                        operator ==                 ( const Host& h ) const         { return _ip.s_addr == h._ip.s_addr; }
    bool                        operator !=                 ( const Host& h ) const         { return _ip.s_addr != h._ip.s_addr; }
    bool                        operator <                  ( const Host& h ) const         { return _ip.s_addr <  h._ip.s_addr; }  // Für map<>

    uint32                      netmask                     () const;                       // Network byte order
    Host                        net                         () const;

    virtual string              as_string                   ()                              { return inet_ntoa( _ip ); }    // In Unix nicht thread-sicher?
    virtual ostream&            operator <<                 ( ostream& s )                  { s << inet_ntoa( _ip ); return s; }    // In Unix nicht thread-sicher?

    void                        set_ip                      ( uint32 ip )                   { _ip.s_addr = ip; set_name(); }
    virtual void                set_name                    ()                              {}

    static set<Host>            get_host_set_by_name        ( const string& name );

  protected:
    in_addr                    _ip;                         // IP-Nummer. Muss vielleicht für IPv6 angepasst werden?
};

//---------------------------------------------------------------------------------------Named_host

struct Named_host : Host
{
                                Named_host                  ()                              {}
                                Named_host                  ( const in_addr& ip )           { set_ip(ip.s_addr); }
                                Named_host                  ( uint32 ip )                   { set_ip(ip); }
    virtual                    ~Named_host                  ()                              {}                      // gcc 3.2 zuliebe

    void                        operator =                  ( const in_addr& ip )           { set_ip(ip.s_addr); }

    void                        set_name                    ();
    
    string                      as_string                   ()                              { return _name + " " + Host::as_string(); }
    ostream&                    operator <<                 ( ostream& s )                  { s <<  _name << " " + Host::as_string(); return s; }

  protected:
    string                     _name;
};

//-----------------------------------------------------------------------------------Xml_end_finder

const int xml_end_finder_token_count = 2;

struct Xml_end_finder
{
    // Findet das Ende eines XML-Dokuments

    enum Tok { cdata_tok, comment_tok };


    struct Tok_entry
    {
                                Tok_entry                   ()                          : _index(0),_active(false) {}

        void                    reset                       ()                          { _index = 0; _active = false; }
        bool                    step_begin                  ( char );
        void                    step_end                    ( char );

        int                    _index;
        bool                   _active;
        const char*            _begin;
        const char*            _end;
    };


                                Xml_end_finder              ();

    bool                        is_complete                 ( const char* p, int length );

    Fill_zero                  _zero_;

    int                        _open_elements;              // Anzahl der offenen Elemente (ohne <?..?> und <!..>)
    bool                       _at_start_tag;               // Letztes Zeichen war '<'
    bool                       _in_special_tag;             // <?, <!
    bool                       _in_tag;                 
    bool                       _in_end_tag;             
    bool                       _xml_is_complete;
    char                       _last_char;
    Tok_entry                  _tok [xml_end_finder_token_count];
};

//------------------------------------------------------------------------------------Communication

struct Communication : zschimmer::Thread
{                                                 
    struct Channel : Sos_self_deleting
    {

                                Channel                     ( Spooler* );
                               ~Channel                     ();


        bool                    do_accept                   ( SOCKET listen_socket );
        void                    do_close                    ();
        bool                    do_recv                     ();
        bool                    do_send                     ();

        void                    recv_clear                  ();


        Fill_zero              _zero_;
        Spooler*               _spooler;
        SOCKET                 _socket;
        Named_host             _host;

        string                 _text;

        bool                   _receive_at_start;
        bool                   _receive_is_complete;
        bool                   _eof;

        Xml_end_finder         _xml_end_finder;

        bool                   _send_is_complete;
        int                    _send_progress;
        Prefix_log             _log;
    };

    typedef list< Sos_ptr<Channel> >  Channel_list;


                                Communication               ( Spooler* );
                               ~Communication               ();

    void                        init                        ();
    void                        start_or_rebind             ();
    void                        start_thread                ();
    void                        close                       ( double wait_time = 0.0 );
    void                        bind                        ();
    void                        rebind                      ()                                      { bind(); }
    int                         thread_main                 ();
    bool                        started                     ()                                      { return _started; }

  private:
    int                         run                         ();
    bool                        handle_socket               ( Channel* );
    int                         bind_socket                 ( SOCKET, struct sockaddr_in* );
    void                       _fd_set                      ( SOCKET, fd_set* );

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    SOCKET                     _listen_socket;
    Channel_list               _channel_list;
    SOCKET                     _udp_socket;
    int                        _nfds;
    fd_set                     _read_fds;
    fd_set                     _write_fds;
    Thread_semaphore           _semaphore;
    bool                       _terminate;
    int                        _tcp_port;
    int                        _udp_port;
    bool                       _rebound;
    int                        _initialized;
    bool                       _started;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
