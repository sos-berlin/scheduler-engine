#include "precomp.h"
//#define MODULE_NAME "sosfact"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"


#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/xception.h"
#include "../kram/log.h"
#include "../kram/sosstat.h"
#include "../kram/soslist.h"
#include "../kram/sosarray.h"
#include "../kram/sosobj.h"
#include "../kram/sosmsg.h"
#include "../kram/sosfiltr.h"
#include "../kram/sosdumcl.h"       // Sos_dummy_client
#include "../file/absfile.h"        // File_spec
#include "../kram/sosfact.h"

using namespace std;
namespace sos {

Sos_factory sos_factory;

//----------------------------------------------------------------------------Sos_factory_agent

struct Sos_factory_agent : Sos_object   // Verwaltet das Anlegen von Objekten
{
    BASE_CLASS( Sos_object )

                                    Sos_factory_agent   ()              { _creation_array.obj_const_name( "Sos_factory_agent::_creation_array" ); _creation_array.first_index( 1 ); }
    Sos_object_ptr                  request_create      ( Sos_object* sender, const Sos_string& name, Sos_object* owner );

  protected:
    SOS_DECLARE_MSG_DISPATCHER
#   if !defined SYSTEM_RTTI
        void                       _obj_print           ( ostream* s ) const  { *s << "Sos_factory_agent"; }
#   endif

  private:
    void                           _obj_ack_msg         ( Ack_msg* );
    void                           _obj_error_msg       ( Error_msg* );
    const Sos_object_descr*         object_descr        ( const Sos_string& type_name,
                                                          Sos_object_descr::Subtype_code* );

    struct Entry
    {
                                    Entry() : _object_ptr(0),_first(0),_open_count(0),_xc(0) {}
                                   ~Entry() { delete _xc; }
        Sos_object_ptr             _requestor;
        Sos_object_ptr             _object_ptr;
        int                        _first;
        int                        _open_count;
        //Sos_ptr<Xc>                _xc;
        Xc*                        _xc;
    };

    Sos_simple_array<Entry>        _creation_array;
};

//------------------------------------------------------------------------Sos_factory::_obj_msg

SOS_BEGIN_MSG_DISPATCHER( Sos_factory_agent )
    SOS_DISPATCH_MSG( ack )
    SOS_DISPATCH_MSG( error )
SOS_END_MSG_DISPATCHER

//------------------------------------------------------------------------------sos_factory_ptr

Sos_factory* sos_factory_ptr()
{
    //return auto_new_ptr( &sos_static_ptr()->_factory_ptr );
    return &sos_factory;
}

//--------------------------------------------------------------------Sos_factory::~Sos_factory

Sos_factory::~Sos_factory()
{
    Sos_object_descr_node* p = _object_descr_head; 

    while( p )
    {
        Sos_object_descr_node* next = p->tail();
        delete p;
        p = next;
    }
}

//-----------------------------------------------------------------------------Sos_factory::add

void Sos_factory::add( const Sos_object_descr* descr_ptr )
{
    _object_descr_head = new Sos_object_descr_node( descr_ptr, _object_descr_head );
}

//------------------------------------------------------------------Sos_factory::request_create

Sos_object_ptr Sos_factory::request_create( Sos_object* sender, const Sos_string& name,
                                            Sos_object* owner )
{
    ZERO_RETURN_VALUE( Sos_object_ptr );

    Sos_pointer& f = sos_static_ptr()->_factory_agent;

    if( !f )  {
        Sos_ptr<Sos_factory_agent> p = SOS_NEW_PTR( Sos_factory_agent );
        f = +p;
    }
    return ((Sos_factory_agent*)+f)->request_create( sender, name, owner );
}

//--------------------------------------------------------------------------Sos_factory::create

Sos_object_ptr Sos_factory::create( const Sos_string& name, Sos_object* owner )
{
    ZERO_RETURN_VALUE( Sos_object_ptr );

    Sos_dummy_client dummy_client;
    return dummy_client.create( name, owner );
}

//--------------------------------------------------------------Sos_factory_agent::object_descr

const Sos_object_descr* Sos_factory_agent::object_descr( const Sos_string& type_name,
                                                         Sos_object_descr::Subtype_code* subtype_code_ptr )
{
    Sos_object_descr_node* d = sos_factory._object_descr_head;

    while( !empty( d ) )
    {
        *subtype_code_ptr = d->head()->is_my_name( c_str( type_name ) );
        if( *subtype_code_ptr )  break;
        d = d->tail();
    }
    if( empty( d ) )  throw_xc( "SOS-1147", c_str( type_name ) );

    return d->head();
}

//------------------------------------------------------------------Sos_factory::request_create

Sos_object_ptr Sos_factory_agent::request_create( Sos_object* sender, const Sos_string& complete_name,
                                                  Sos_object* owner )
{
    ZERO_RETURN_VALUE( Sos_object_ptr );

    int            first_index = 0;
    Sos_object_ptr prev_object;
    Sos_string     name = complete_name;

    LOGI( *sender << " creates \"" << name << "\"\n" );

    while(1)
    {
        uint           call_length      = min( position( name, '|' ), position( name, '!'/*e370*/ ) );
        int            type_name_length = min( min( position( name, ' ' ), position( name, ':' ) ),
                                               call_length );
        Sos_string     type_name        = as_string( c_str( name ), type_name_length );
        Sos_string     parameters;
        Sos_object_ptr object;
        Sos_object_descr::Subtype_code subtype_code;

        if ( type_name_length == 1 ) type_name_length = 0; // zur Erkennung von DOS-Laufwerksbuchstaben
                                                           // d.h. File-Typen müssen mindestens 2-buchstabig sein!
        if( !type_name_length || empty( c_str( name ) + type_name_length ) ) {   // Nur Dateiname?
            type_name = "file";                                 // Default-Objekttyp
            name = type_name + " " + name;
            call_length += 5;
            type_name_length = 5;
        }

        {   // Objektparameter:
            const char* p0 = c_str( name );
            const char* p  = p0 + type_name_length;
            while( *p == ' ' )  p++;
            if   ( *p == ':' )  p++;   // trennt Objekttyp von den Parametern
            while( *p == ' ' )  p++;
            parameters = as_string( c_str( name ) + ( p - p0 ), call_length - ( p - p0 ) );
        }

        const Sos_object_descr* descr = object_descr( type_name, &subtype_code );

        /// Objekt konstruieren:
        object = descr->create( subtype_code );
        object->obj_owner( owner );

        if( prev_object )  {
            SOS_CAST( Sos_msg_filter, prev_object )->obj_output_ptr( object );
        }

        if( descr->handles_complete_name() )  call_length = length( name );

		int i;
        for( i = _creation_array.first_index(); i <= _creation_array.last_index(); i++ )
            if( !_creation_array[ i ]._object_ptr )  break;

        if( i > _creation_array.last_index() )  _creation_array.last_index( i );

        if( !prev_object )  first_index = i;
                      else  _creation_array[ i ]._first = first_index;

        _creation_array[ i ]._object_ptr = object;
        _creation_array[ first_index ]._open_count++;

        //LOG( "_creation_array["<<i<<"]=={"<<*_creation_array[ i ]._requestor<<','<<*_creation_array[ i ]._object_ptr<<','<<_creation_array[ i ]._open_count<<','<<_creation_array[ i ]._first<<"}\n" );
        {
            Open_msg open_msg ( object, this, c_str( parameters ) );
            request( &open_msg );

            call_length++;      // '|'
            if( call_length >= length( name ) )  break;

            name = as_string( c_str( name ) + call_length );
            prev_object = object;
        }
    }

    _creation_array[ first_index ]._requestor = sender;
    return _creation_array[ first_index ]._object_ptr;
}

//--------------------------------------------------------------Sos_factory_agent::_obj_ack_msg

void Sos_factory_agent::_obj_ack_msg( Ack_msg* m )
{
	int i;
    for( i = _creation_array.first_index(); i <= _creation_array.last_index(); i++ )
    {
        //LOG( "_creation_array["<<i<<"]._object_ptr == " << *_creation_array[ i ]._object_ptr << '\n' );
        if( +_creation_array[ i ]._object_ptr == m->source_ptr() )  break;
    }

    if( i > _creation_array.last_index() )  {
        Base_class::_obj_msg( m );
        return;
    }

    Entry* e = &_creation_array[ i ];
    Entry* first;
    if( e->_first ) {
        first = &_creation_array[ e->_first ];
        e->_open_count = 0;
        e->_object_ptr = 0;
        e->_requestor = 0;
        e->_first = 0;
    } else {
        first = e;
    }

    //LOG( "Sos_factory_agent::_obj_ack_msg: first->_open_count==" << first->_open_count << '\n' );

    if( --first->_open_count == 0 ) {
        //LOG( "Sos_factory_agent::_obj_ack_msg: first->_requestor=" << *first->_requestor << '\n' );
        if( first->_requestor ) {
            if( first->_xc ) {
                reply_error_msg( first->_requestor, first->_object_ptr, *first->_xc );
            } else {
                reply_ack_msg( first->_requestor, first->_object_ptr/*this?*/ );
            }
        }
        first->_requestor = 0;
        first->_object_ptr = 0;
        SOS_DELETE( first->_xc );
    }
}

//------------------------------------------------------------Sos_factory_agent::_obj_error_msg

void Sos_factory_agent::_obj_error_msg( Error_msg* m )
{
	int i;
    for( i = _creation_array.first_index(); i <= _creation_array.last_index(); i++ )
    {
        if( +_creation_array[ i ]._object_ptr == m->source_ptr() )  break;
    }

    if( i > _creation_array.last_index() )  {
        Base_class::_obj_msg( m );
        return;
    }

    Entry* e = &_creation_array[ i ];
    Entry* first = e->_first? ( e->_open_count = 0, &_creation_array[ e->_first ] ) : e;

    //SHOW_ERR( *m->source_ptr() << ": " << m->error() );    int SHOW_ERR_EXCEPTION;

    if( !first->_xc ) {
        //Sos_pointer x = obj_copy( m->error() );
        //first->_xc = (Xc*)+x;
        first->_xc = new Xc( m->error() );
    }

    if( --first->_open_count > 0 ) {
        // Die anderen Objekte stornieren
    }
    else
    if( first->_requestor )
    {
        reply_error_msg( first->_requestor, first->_object_ptr, *first->_xc );
        first->_requestor = 0;
        first->_object_ptr = 0;
    }
}

//-----------------------------------------------------------Sos_object_descr::Sos_object_descr

Sos_object_descr::Sos_object_descr()
{
   //Log_block q ( "Sos_object_descr::Sos_object_descr " << name() << '\n');
    ((Sos_factory*)sos_factory_ptr())->add( this );
}

} //namespace sos
