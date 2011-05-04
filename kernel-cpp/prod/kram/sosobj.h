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

//---------------------------------------------------------------------SOS_BEGIN_MSG_DISPATCHER

#define SOS_DECLARE_MSG_DISPATCHER    void _obj_msg( Sos_msg* m );

#define SOS_BEGIN_MSG_DISPATCHER( TYPE )                                                    \
    void TYPE::_obj_msg( Sos_msg* m )                                                       \
    {                                                                                       \
        switch( m->type() )                                                                 \
        {

#define SOS_DISPATCH_MSG( MSG )                                                             \
            case msg_##MSG: _obj_##MSG##_msg( (T_##MSG##_msg*)m ); break;

#define SOS_END_MSG_DISPATCHER_DEFAULT( FUNCT )                                             \
            default: FUNCT( m );                                                            \
        }                                                                                   \
    }

#define SOS_END_MSG_DISPATCHER  SOS_END_MSG_DISPATCHER_DEFAULT( Base_class::_obj_msg )

//-------------------------------------------------------------------------------------forwards

//class ostream;
//class istream;

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

                                Sos_object              () : _obj_request_semaphore( 1 ), _owner(0) {}

    Sos_object*                 obj_possession          ( int i/*0..*/ ) const      { return _obj_possession( i ); }

    void                        obj_print_spec_msg      ( ::std::ostream* s, const Spec_msg& msg ) const { _obj_print_spec_msg( s, msg ); }

    uint                        obj_input_block_size    ()                          { return _obj_input_block_size(); }
    Area                        obj_input_buffer        ();
    const Record_type*          obj_spec_msg_field_type ( int spec_code ) const     { return _obj_spec_msg_field_type( spec_code ); }

    void                        obj_run                 ();
    void                        obj_end                 ( Close_mode = close_normal );
    void                        obj_put                 ( const Const_area& );
    Const_area_handle           obj_put_and_wait_data   ( const Const_area& );      // (File-)Server-Anfrage und Antwort
    Const_area_handle           obj_get                 ( Const_area_handle* pos = 0 );
    Const_area_handle           obj_read                ( uint length = 0 );
    Const_area_handle           obj_client_name         () const;
    Sos_client*                 obj_client              ()                          { return _obj_client(); }
    Sos_object*			        obj_owner               () const;
    void                        obj_owner               ( Sos_object* );

    virtual void                close                   ( Close_mode = close_normal );
    void                        obj_print_status        ( ::std::ostream* s ) const { _obj_print_status( s ); } 
    int                         obj_request_semaphore   () const                    { return _obj_request_semaphore; }

  protected:
    void                        obj_busy                ();
#   if !defined SYSTEM_RTTI
        void                   _obj_print               ( ::std::ostream* ) const;
#   endif
    virtual void               _obj_print_status        ( ::std::ostream* ) const;
    Bool                       _obj_is_type             ( Sos_type_code t ) const   { return t == tc_Sos_object || Base_class::_obj_is_type( t ); }
    virtual Sos_object*        _obj_possession          ( int /*0..*/ ) const       { return 0; }
    virtual uint               _obj_input_block_size    ();
    virtual Area               _obj_input_buffer        ();
    virtual void               _obj_print_spec_msg      ( ::std::ostream* s, const Spec_msg& ) const;
    const Record_type*         _obj_spec_msg_field_type ( int ) const     { return 0; }
    virtual Const_area_handle  _obj_client_name         () const;
    virtual Sos_client*        _obj_client              ();
    virtual void               _obj_msg                 ( Sos_msg* );     // Alle Anforderungen

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

  //Sos_object_register*       _object_register_ptr;
  //int4                       _obj_index;              // Index in der Tabelle für global bekannte Objekte oder 0

    static Bool                _busy;                   // für obj_request() und obj_busy()

    Sos_object*                _owner;
};

//-----------------------------------------------------------------------------------Busy_error

struct Busy_error : Xc { Busy_error( const Sos_object* o = 0, const char* e = "SOS-1148" ) : Xc( e ) { insert( o ); } };

void throw_busy_error( const Sos_object* o = 0, const char* e = "SOS-1148" );

//-------------------------------------------------------------------------------Sos_object_ptr

typedef Sos_ptr<Sos_object>     Sos_object_ptr;

//----------------------------------------------------------------------------------------Sos_owner
/*
struct Sos_owner : Sos_object
{
                                Sos_owner               ()              {}
                               ~Sos_owner               ()              { remove_possession(); }

    void                        add                     ( Sos_object* );
    void                        remove                  ( Sos_object* p )       { remove( index( p )); }
    void                        remove                  ( int i )               { _possession_array[ i ]->remove_possessor(); _possession_array[ i ] = 0; }
    void                        remove_possession       ();
    int                         count                   () const;
    int                         index                   ( Sos_possession* );

  private:
    int                        _possession_count;
    Sos_simple_array<Sos_object*> _possession_array;
};
*/

//======================================================================================inlines

//-----------------------------------------------------------------Sos_object::obj_input_buffer

inline Area Sos_object::obj_input_buffer()
{
    Area area = _obj_input_buffer();
    area.length( 0 );
    return area;
}

} //namespace sos

#endif
