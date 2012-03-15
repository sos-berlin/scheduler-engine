// sosobj.h                                                © 1995 SOS GmbH Berlin

#ifndef __SOSOBJ_H
#define __SOSOBJ_H

#if !defined __XCEPTION_H
#   include "xception.h"
#endif

#if !defined __AREA_H
#   include "area.h"
#endif

namespace sos
{

struct Sos_client;

//----------------------------------------------------------------------------------------const

const uint obj_default_buffer_size = ( 34*1024 ); //4096;

//-------------------------------------------------------------------------------------forwards

struct Record_type;

struct Sos_msg;
struct Request_msg;
struct Reply_msg;
struct Spec_msg;
struct Sos_object_register;

//-----------------------------------------------------------------------------------Sos_object

struct Sos_object : Sos_self_deleting
{
    BASE_CLASS( Sos_self_deleting )

                                Sos_object              () : _obj_request_semaphore( 1 ) {}

    virtual void                close                   ( Close_mode = close_normal );

  protected:
    Bool                       _obj_is_type             ( Sos_type_code t ) const   { return t == tc_Sos_object || Base_class::_obj_is_type( t ); }
    const Record_type*         _obj_spec_msg_field_type ( int ) const     { return 0; }

    Dynamic_area               _obj_default_input_buffer;
    int                        _obj_request_semaphore;

  private:
    friend inline void          reply                   ( const Reply_msg& );
    friend        void          reply_error_msg         ( Sos_object*, Sos_object*, const Xc& );
    friend inline void          reply                   ( Reply_msg* );
    friend void                 request                 ( Request_msg* );
    friend struct               Sos_proxy;              // ~Sos_proxy setzt _obj_ref_count vorübergehend hoch
    friend struct               Sos_msg_queue;
    friend struct               Sos_object_register;

    void                       _request_msg             ( Request_msg* );
    void                       _reply_msg               ( Reply_msg* );
};

//-------------------------------------------------------------------------------Sos_object_ptr

typedef Sos_ptr<Sos_object>     Sos_object_ptr;


} //namespace sos

#endif
