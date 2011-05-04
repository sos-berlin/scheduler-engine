// sosdumcl.h                               © 1995 SOS GmbH Berlin

#ifndef __SOSDUMCL_H
#define __SOSDUMCL_H

namespace sos
{

//-----------------------------------------------------------------------------Sos_dummy_client

#if !defined __SOSSTRNG_H   // Borland C++ 5.00
    typedef Sos_string;
#endif
struct Sos_dummy_client_mailbox;
struct Sos_msg;
struct Request_msg;
struct Data_msg;
struct Data_reply_msg;
struct Error_msg;
struct Object_ref_msg;

struct Sos_dummy_client : Sos_object
{
    BASE_CLASS( Sos_object )
                                Sos_dummy_client        () : _object_ptr ( 0 ), _mailbox_ptr ( 0 ) { obj_const_name( "Sos_dummy_client" ); }
                               ~Sos_dummy_client        ()                 { /*delete _object_ptr; ???*/ }

    Const_area_handle           send_and_await_data     ( Request_msg*, Const_area_handle* key = 0 );
    void                        send_and_await_ack      ( Request_msg* );
    Sos_object_ptr              send_and_await_object_ref( Request_msg* );
    Const_area_handle           send_and_await_data_request( Data_msg* );  // für den Fileserver
    Sos_object_ptr              create                  ( const Sos_string& name, Sos_object* owner );
    void                        await_ack               ();

    Sos_object*                 object_ptr              () const           { return _object_ptr; }
    void                        object_ptr              ( Sos_object* o )  { _object_ptr = o; }

  protected:
    void                       _obj_print               ( std::ostream* s ) const;

  private:
    friend struct               Sos_dummy_client_mailbox;

    void                       _obj_data_msg            ( Data_msg* );          // für wait_data()
    void                       _obj_other_msg           ( Sos_msg* );           // für send_and_await_xxx()
    void                       _obj_data_reply_msg      ( Data_reply_msg* );    // für send_and_await_data()
    void                       _obj_error_msg           ( Error_msg* );         // für send_and_await_xxx()
    void                       _obj_object_ref_msg      ( Object_ref_msg* );    // für send_and_await_object_ref()

    SOS_DECLARE_MSG_DISPATCHER

    void                        send_and_await_msg      ( Request_msg* );
    void                        await_msg               ();

    Sos_object*                _object_ptr;
    Sos_dummy_client_mailbox*  _mailbox_ptr;
};


} //namespace sos


#endif
