// sosobj.h                                                © 1995 SOS GmbH Berlin

#ifndef __SOSFILTR_H
#define __SOSFILTR_H

#if !defined __SOSOBJ_H
#   include "sosobj.h"
#endif

#if !defined __SOSMSG_H
#   include "sosmsg.h"
#endif

namespace sos
{

struct Sos_read_blocker;

//-------------------------------------------------------------------------------Sos_msg_filter

struct Sos_msg_filter : Sos_object   // Filter-Objekt mit Standard-Eingabe und -Ausgabe
{
    BASE_CLASS( Sos_object )
                              //Sos_msg_filter          ( Sos_msg_filter* input = 0, Sos_msg_filter* output = 0 );
                               ~Sos_msg_filter          ();
/*
    void                        obj_pipe_out            ( Sos_msg_filter* );
    void                        obj_pipe_in             ( Sos_msg_filter* );

    DECLARE_PUBLIC_MEMBER( Sos_msg_filter*, obj_input_ptr  )
    DECLARE_PUBLIC_MEMBER( Sos_msg_filter*, obj_output_ptr )
*/
  //void                        obj_pipe_out            ( Sos_object* );

    const Sos_object_ptr&       obj_output_ptr          () const                    { return _obj_output_ptr; }
    void                        obj_output_ptr          ( const Sos_object_ptr& p ) { _obj_output_ptr = p; }
    Sos_msg_filter*             obj_reverse_filter      ( Sos_msg_filter* client = 0 );
  //Sos_read_blocker*           obj_read_blocker        () const            { return (Sos_read_blocker*)+_obj_read_blocker; }

  protected:
    Bool                       _obj_is_type             ( Sos_type_code t ) const { return t == tc_Sos_msg_filter || Base_class::_obj_is_type( t ); }
    void                       _obj_msg                 ( Sos_msg* );

    void                        obj_created             ()                  { obj_reply_ack(); }
    void                        obj_not_created         ( const Xc& x )     { obj_reply_error( x ); }
  //Bool                        obj_reblock             ( Data_msg* );
    void                        obj_send                ( const Const_area& );    // Data_msg
    void                        obj_send                ( const Const_area&, uint4 seek_pos );    // Data_msg
    void                        obj_send                ( const Const_area&, const Const_area_handle& key );    // Data_msg
    void                        obj_request_end         ();
    void                        obj_reply_ack           ();
    void                        obj_reply_error         ( const Xc& );
  //void                        obj_send_ack            ();         // VORSICHT, REKURSION!

    virtual Const_area_handle  _obj_client_name         () const;

    virtual Sos_ptr<Sos_msg_filter> _obj_create_reverse_filter( Sos_msg_filter* client = 0 );

    Sos_object_ptr             _obj_client_ptr;
    Sos_ptr<Sos_msg_filter>    _obj_reverse_filter;
    Sos_object_ptr             _obj_output_ptr;

  private:
  //Sos_object_ptr             _obj_read_blocker;
};

//==========================================================================================inlines

//---------------------------------------------------------------------Sos_msg_filter::obj_pipe_out
/*
inline void Sos_msg_filter::obj_pipe_out( Sos_object* out )
{
    obj_output_ptr( out );
    //out->obj_input_ptr( this );
}
*/
//----------------------------------------------------------------------Sos_msg_filter::obj_pipe_in
/*
inline void Sos_msg_filter::obj_pipe_in( Sos_msg_filter* in )
{
    obj_input_ptr( in );
    in->obj_output_ptr( this );
}
*/
//-------------------------------------------------------------------Sos_msg_filter::Sos_msg_filter
/*
inline Sos_msg_filter::Sos_msg_filter( Sos_msg_filter* in, Sos_msg_filter* out )
:
    _obj_output_ptr    ( out ),
    _obj_read_blocker_ptr ( 0   )
{
}
*/


} //namespace sos

#endif
