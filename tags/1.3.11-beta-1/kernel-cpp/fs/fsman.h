// fsman.h      FS-Manager                            (C)1996 SOS GmbH Berlin
//                                                    Joacim Zschimmer

#ifndef __FSMAN_H
#define __FSMAN_H

#ifndef __SOSARRAY_H
#   include "../kram/sosarray.h"
#endif

#ifndef __SOSOBJ_H
#   include "../kram/sosobj.h"
#endif

#ifndef __SOSMSG_H
#   include "../kram/sosmsg.h"
#endif

namespace sos {

//---------------------------------------------------------------------------------------static

extern const int4  max_demo_connections;
extern ostream*    fs_log;

//------------------------------------------------------------------------------------Fs_manager

struct Fs_manager : Sos_object
{
    BASE_CLASS( Sos_object )

                                Fs_manager              ( const Sos_string& port );
                               ~Fs_manager              ();

    enum Conn_status
    {
        sta_none,
        sta_creation,
        sta_running,
        sta_closing
    };

    void                        start                   ();
  protected:
    SOS_DECLARE_MSG_DISPATCHER
    #if !defined SYSTEM_RTTI
        void                   _obj_print               ( ostream* s ) const  { *s << "Fs_manager"; }
    #endif

  private:
    void                       _obj_run_msg             ( Run_msg* );
    void                       _obj_ack_msg             ( Ack_msg* );
    void                       _obj_error_msg           ( Error_msg* );

    void                        request_connection      ();

    struct Connection
    {
                                Connection              ( const Sos_object_ptr& c = Sos_object_ptr( NULL ), Conn_status s=sta_none ) : _conn(c), _status(s) {}

        Sos_object_ptr         _conn;
        Conn_status            _status;
    };

    Fill_zero                  _zero_;
    Sos_string                 _port;
    Sos_simple_array<Connection> _connection_array;
    Bool                       _first_request_logged;

  public:
    Sos_string                 _filename_prefix;
    int4                       _allowed_connections_count;
};


} //namespace sos
#endif

