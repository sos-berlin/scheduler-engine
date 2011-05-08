// $Id$

#include "precomp.h"
//#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#if defined _DEBUG  &&  defined WIN32
#   undef STRICT
#   define STRICT
#   include <windows.h>     // DebugBreak()
#endif

#include <stdlib.h>         // abort()
#include <string.h>
#include <stdio.h>          // sprintf

#include "../kram/sysdep.h"

//#if defined SYSTEM_RTTI
//#   include <typeinfo.h>
//#endif

#include "../kram/sos.h"
#include "../kram/log.h"
#include "../kram/sosalloc.h"
#include "../kram/sosprof.h"        // wg. obj_print_by_delete Flag && Referenz-Meldung
#include "../kram/sosobjba.h"

//#pragma implementation

using namespace std;
namespace sos {


void check_pointer( const void*, uint length );  // sysdep.cxx
//extern Bool sosalloc_check;                         // sosalloc.cxx

//----------------------------------------------------------SOS_CAST--Sos_object_base::obj_cast

Sos_object_base* Sos_object_base::obj_cast( Sos_type_code type_code, const char* type_name, const char* text, int lineno )
{
    if( !this  ||  !obj_is_type( type_code ) ) {
        char line_text [ 17 ];
        sprintf( line_text, ", Zeile %.0d", lineno );
        LOG( "SOS_CAST(" << type_name << ',' << *this << ") in " << text << ", Zeile " << lineno << " nicht möglich. " );
        //throw_xc( "SOS-1130", Msg_insertions( this, text, line_text ) );
        throw_xc( "SOS-1130", this, type_name );
    }

    return this;
}

//-------------------------------------------------------------------Sos_object_base::_obj_name

string Sos_object_base::_obj_name() const
{
#   if defined SYSTEM_RTTI
        return z::name_of_type( *this );
#    else
        return "Sos_object_base";
#   endif
}

//------------------------------------------------------------------Sos_object_base::_obj_print

void Sos_object_base::_obj_print( ostream* s ) const
{
    *s << _obj_name();
}

//---------------------------------------------------------Sos_self_deleting::Sos_self_deleting

Sos_self_deleting::Sos_self_deleting()
:
    _obj_ref_count ( 1 ),
    _obj_const_name( 0 )
{
#   if !defined SYSTEM_WIN16DLL             // das sparen wir uns
        //Nicht threadsicher Sos_static* s = sos_static_ptr();
        //Nicht threadsicher if( s )  s->_object_count++;
#   endif
}

//---------------------------------------------------------Sos_self_deleting::Sos_self_deleting

Sos_self_deleting::Sos_self_deleting( const Sos_self_deleting& )
:
    _obj_ref_count ( 1 ),
    _obj_const_name( 0 )
{
#   if !defined SYSTEM_WIN16DLL             // das sparen wir uns
        //Nicht threadsicher Sos_static* s = sos_static_ptr();
        //Nicht threadsicher if( s )  s->_object_count++;
#   endif
}

//--------------------------------------------------------Sos_self_deleting::~Sos_self_deleting

Sos_self_deleting::~Sos_self_deleting()
{
    //if( _obj_ref_count <= 0 )  return;      // Besser in Ruhe lassen

    //--_obj_ref_count;

    if( _obj_ref_count == 1 ) {
        _obj_ref_count = 0;
    }
    else
    {
        const char* name = _obj_const_name? _obj_const_name : "Sos_self_deleting";
        LOG_ERR( "\n******* ~" << name << " mit " << _obj_ref_count << " Referenzen *******\n\n" );
/*
        static Bool ich_war_hier = false;
#       if defined SYSTEM_WIN
        if( !ich_war_hier )
#       endif
        {
            ich_war_hier = true;
            if ( read_profile_bool( "", "debug", "check-new", false ) )
*/
        //if( sosalloc_check )
        //    {
        //        SHOW_ERR( '~' << name << " mit " << (long)_obj_ref_count << " Referenzen" );
        //    }
        //}

        if( _obj_ref_count > 1 ) {
            _obj_ref_count = 0;     // Damit weiteres ~Sos_self_deleting oder ~Sos_pointer keinen Unsinn macht
        }
    }

#   if !defined SYSTEM_WIN16DLL             // das sparen wir uns
        //Nicht threadsicher Sos_static* s = sos_static_ptr();
        //Nicht threadsicher if( s )  s->_object_count--;
#   endif
}

//--------------------------------------------------------------Sos_self_deleting::operator new

void* Sos_self_deleting::operator new( size_t size, const char* info )
{
	return sos_alloc( size, info );
}

//-----------------------------------------------------------Sos_self_deleting::operator delete

void Sos_self_deleting::operator delete( void* ptr )
{
    sos_free( ptr );
}

//-----------------------------------------------------------Sos_self_deleting::operator delete
#ifdef SYSTEM_DELETE_WITH_PARAMS

void Sos_self_deleting::operator delete( void* ptr, const enum New_type, const char* )
{
    sos_free( ptr );
}

#endif
//------------------------------------------------------------Sos_self_deleting::obj_remove_ref
/*
void Sos_self_deleting::obj_remove_ref() const
{
    --((Sos_self_deleting*)this)->_obj_ref_count;

    if( _obj_ref_count == 0 ) {
        ((Sos_self_deleting*)this)->obj_del();
    }
}
*/
//-------------------------------------------------------------------Sos_self_deleting::obj_del

//static Bool obj_print_at_delete         = false;
//static Bool obj_print_at_delete_was_set = false;

void Sos_self_deleting::obj_del()
{
    {
        assert( _obj_ref_count == 0 );
        _obj_ref_count = 1;                 // Damit ~Sos_self_deleting keinen Fehler meldet
        delete this;
    }
}

//------------------------------------------------------------------AB HIER KEINE STACK-PRÜFUNG

#if defined __BORLANDC__
#   pragma option -N-
#endif

//-------------------------------------------------------------------------------throw_sos_1126

void throw_sos_1126()
{
    Xc x ( "SOS-1126" );       // Zeiger auf bereits zerstörtes Objekt verwendet
    SHOW_ERR( x );

#   if defined _DEBUG  &&  defined SYSTEM_WIN32
        DebugBreak();
#   endif

    throw x;
}

//---------------------------------------------------------------------Sos_pointer::Sos_pointer

Sos_pointer::Sos_pointer( const Sos_self_deleting* ptr )
{
    //checked_pointer( (Sos_self_deleting*)ptr );

    if( ptr ) {
        if( !ptr->obj_ref_count() )  throw_sos_1126();
        ((Sos_self_deleting*)ptr)->obj_add_ref();
    }
    _ptr = (Sos_self_deleting*)ptr;
}

//---------------------------------------------------------------------Sos_pointer::Sos_pointer

Sos_pointer::Sos_pointer( const Sos_pointer& src )
{
    //checked_pointer( src._ptr );

    _ptr = copy( src._ptr );
}

//------------------------------------------------------------------------Sos_pointer::__assign
// ohne Stackprüfung

void Sos_pointer::__assign( Sos_self_deleting* src )
{
    //checked_pointer( src );

    Sos_self_deleting* p = copy( src );     // falls Sos_pointer = Sos_self_deleting*, damit ref_count nicht vorübergehend 0 wird
    //if( _ptr ) _ptr->obj_remove_ref();
    inline_del();
    _ptr = p;
}

//----------------------------------------------------------------------------Sos_pointer::copy
/*
Sos_self_deleting* Sos_pointer::copy( Sos_self_deleting* p )
{
    if( p ) p->obj_add_ref();
    return p;
}
*/
//-------------------------------------------------------------------------Sos_pointer::pointer

void Sos_pointer::del()
{
    inline_del();
}

//--------------------------------------------------------------------operator<< ( Sos_object )

ostream& operator<< ( ostream& s, const Sos_object_base& object )
{
    if( &object )
    {
        object._obj_print( &s );
    }
    else
    {
        s << "NULL-Sos_object";
    }

    return s;
}

//------------------------------------------------------------------------------------_obj_copy
/*
Sos_pointer _obj_copy( const Sos_self_deleting& o )
{
    //return sos_new_ptr( o._obj_copy() );
    return Sos_pointer( o._obj_copy(), Sos_self_deleting::New_type(0) );
}
*/

} //namespace sos
