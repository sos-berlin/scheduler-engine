#include <precomp.h>
#define MODULE_NAME "videohlp"

#if 0

/*                                                      (c) SOS GmbH Berlin

   KONTEXTSENSITIVE HILFE FÜR BS2000-MASKEN

   Mit der Maus oder einer Taste kann zu einer Position in einer Maske eine Hilfe abgerufen werden.

   Eine Maske wird durch konstante Texte an bestimmten Positionen identifiziert (die Maskenkennung).
   Jeder Maske werden eine Hilfedatei und ein Einsprung für den Maskenhintergrund zugeordnet.
   Rechtecken in der Masken lassen sich weitere Einsprünge in die selbe Hilfedatei zuordnen.
   Die Daten werden in einer Datei gehalten; Die Maskenkennung ist der Satzschlüssel.





*/

#include <cstring.h>

#include <sos.h>
#include <sosstore.h>
#include <soslist.h>
#include <video.h>
#include <videohlp.h>

//-----------------------------------------------------------Video_simple_field_descr::object_store

void Video_simple_field_descr::object_store( Sos_output_store& s )
{
    _pos.object_store( s );
    s.write_int2( _length );
}

//------------------------------------------------------------Video_simple_field_descr::object_load

Video_simple_field_descr& Video_simple_field_descr::object_load( Sos_input_store& s )
{
    _length = 0;      // Falls es schief geht

    _pos.object_load( s );
    _length = s.read_int2();

    return *this;
}

//------------------------------------------------------------------Video_simple_field::operator ==

Bool Video_simple_field::operator==( const Video& video ) const
{
    return video.equal( *this, text() );
}

//------------------------------------------------------------------------Video_rectangle::contents

Bool Video_rectangle::contents( const Video_pos& pos ) const
{
    return pos.column0() >= _pos.column0()  &&  pos.column0() <= _pos2.column0()
        && pos.line0()   >=  pos.line0()    &&  pos.line0()   <= _pos2.line0();
}

//--------------------------------------------------------------------Video_rectangle::object_store

void Video_rectangle::object_store( Sos_output_store& s )
{
    _pos.object_store( s );
    _pos2.object_store( s );
}

//---------------------------------------------------------------------Video_rectangle::object_load

Video_rectangle& Video_rectangle::object_load( Sos_input_store& s )
{
    _pos.object_load( s );
    _pos2.object_load( s );

    return *this;
}

//---------------------------------------------------------------------Video_rectangle::field_descr

Video_simple_field_descr Video_rectangle::field_descr( int i )
{
    return Video_simple_field_descr( Video_pos( _pos.column0(), _pos.line0() + i ),
                                     _pos2.column0() - _pos.column0() + 1 );
}

//-----------------------------------------------------------Video_mask_identification::operator ==

Bool Video_mask_identification::operator== ( const Video& video )
{
    const Video_field_list f = _video_field_list;

    while( !empty( f ) ) {
        if( f->head() != video )  return false;
        f = f->tail();
    }

    return true;
}

//----------------------------------------------------------Video_mask_identification::object_store

void Video_mask_identification::object_store( Sos_output_store& s )
{
    s.write_int2( field_descr_count() );

    Video_simple_field_descr_list p = _field_descr_list;

    while( !empty( p )) {
        p->head_ptr()->object_store( s );
        p = p->tail();
    }
}

//-----------------------------------------------------------Video_mask_identification::object_load

Video_mask_identification& Video_mask_identification::object_load( Sos_input_store& s )
{
    delete_list( &_field_descr_list );

    int count = s.read_int2();

    for( int i = 0; i < count; i++ ) {
        add( Video_simple_field_descr().object_load( s ) );
    }

    return *this;
}

//---------------------------------------------------------------Video_help_rectangle::object_store

void Video_help_rectangle::object_store( Sos_output_store& s )
{
    _rectangle.object_store( s );
    _help_entry.object_store( s );
}

//----------------------------------------------------------------Video_help_rectangle::object_load

Video_help_rectangle& Video_help_rectangle::object_load( Sos_input_store& s )
{
    _rectangle.object_load( s );
    _help_entry.object_load( s );

    return *this;
}

//--------------------------------------------------------------Video_mask_help_descr::object_store

void Video_mask_help_descr::object_store( Sos_output_store& s )
{
    s.write_int2( help_rectangle_count() );

    Video_help_rectangle_list list = _help_rectangle_list;

    while( !empty( list ) ) {
        list->head_ptr()->rectangle().object_store( s );
        list->head_ptr()->help_entry().object_store( s );
        list = list->tail();
    }
}

//---------------------------------------------------------------Video_mask_help_descr::object_load

Video_mask_help_descr& Video_mask_help_descr::object_load( Sos_input_store& s )
{
    int count = s.read_int2();

    while( count ) {
        add( Video_help_rectangle().object_load( s ) );
        count--;
    }

    return *this;
}
#endif

