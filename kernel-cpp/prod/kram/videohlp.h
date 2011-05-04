/* e9750hlp.h                                       (c) SOS GmbH Berlin
                                                        Joacim Zschimmer
*/


#ifndef __E9750HLP_H
#define __E9750HLP_H

struct Sos_output_store;
struct Sos_input_store;

struct Video_simple_field_descr
{
    // Beschreibt ein Feld innerhalb einer Video-Zeile durch Position und Länge

                                Video_simple_field_descr();
                                Video_simple_field_descr( const Video_pos&, int length );

    Video_pos                   pos                     () const;
    int                         length                  () const;

    void                        object_store            ( Sos_output_store& );
    Video_simple_field_descr&   object_load             ( Sos_input_store& );

  private:
    Video_pos                  _pos;
    int                        _length;
};


struct Video_simple_field : Video_simple_field_descr
{
    /* Feld mit Inhalt */
                                Video_simple_field      ();
                                Video_simple_field      ( const Video_simple_field_descr&, const string& );

    string                      text                    ();
    Bool                        operator ==             ( const Video& ) const;

    void                        object_store            ( Sos_output_store& );
    Video_simple_field&         object_load             ( Sos_input_store& );

  private:
    string                     _text;
};


struct Video_rectangle
{
    /* Beschreibt ein Rechteck in einem Video durch linke obere und rechte untere Position.
       Das Rechteck kann als Folge von Feldern (Video_simple_field_descr) interpretiert werden.
    */
                                Video_rectangle         ();
                                Video_rectangle         ( const Video_pos& links_oben, const Video_pos& rechts_unten );

    Bool                        contents                ( const Video_pos& ) const;
    int                         field_descr_count       () const;
    Video_simple_field_descr    field_descr             ( int );

    void                        object_store            ( Sos_output_store& );
    Video_rectangle&            object_load             ( Sos_input_store& );

  private:
    Video_pos                  _pos;
    Video_pos                  _pos2;
};


typedef Sos_simple_list_node< Video_simple_field_descr >*  Video_simple_field_descr_list;

struct Video_mask_identification
{
    /* Eine Maske wird durch eine Folge von Feldern (mit Position) bestimmt, die bestimmten
       Text enthalten müssen.
       Die maximale Zahl der Felder ist video_max_fields_per_mask_identification.
       Felder und Text werden hier gehalten.
    */
                                Video_mask_identification();

    void                        add                     ( const Video_simple_field& );

    int                         field_count             () const;
    const Video_simple_field&   field                   ( int );
    Bool                        operator ==             ( const Video& ) const;

    void                        object_store            ( Sos_output_store& );
    Video_mask_identification&  object_load             ( Sos_input_store& );

  private:
    Video_simple_field_list     _field_list;
};

Bool operator== ( const Video&, const Video_mask_identification& ); 

struct Sos_help_entry
{
                                Sos_help_entry          ();
                                Sos_help_entry          ( const char* filename, int help_id );
                               ~Sos_help_entry          ();

    void                        show_help               () const;    // Fenster wird vom Benutzer geschlossen

    void                        object_store            ( Sos_output_store& );
    Sos_help_entry&             object_load             ( Sos_input_store& );

  private:
    string                     _filename;
    int                        _help_id;
};

const extern Sos_help_entry null_help_entry;  // Ersetzen durch Hintergrundhilfe, Hilfe für die gesamte Maske!


struct Video_help_rectangle                     // Ein Video_rectangle und ein Sos_help_entry
{
                                Video_help_rectangle    ();
                                Video_help_rectangle    ( const Video_rectangle&, const Sos_help_entry& );

    Video_rectangle             rectangle               () const;
    Sos_help_entry              help_entry              () const;

    void                        object_store            ( Sos_output_store& );
    Video_help_rectangle&       object_load             ( Sos_input_store& );

  private:
    Video_rectangle            _rectangle;
    Sos_help_entry             _help_entry;
};

typedef Sos_simple_list_node< Video_help_rectangle >*  Video_help_rectangle_list;

struct Video_mask_help_descr                    // Eine Maske und deren Hilfeverweise
{
                                Video_mask_help_descr   ();
                               ~Video_mask_help_descr   ();

    void                        mask_identification     ( const Video_mask_identification& ); // muß
    void                        default_help_entry      ( const Sos_help_entry& );            // muß
    void                        add                     ( const Video_help_rectangle& );

    const Video_mask_identification& mask_identification() const;
    int                         help_rectangle_count    () const;
    const Video_help_rectangle& help_rectangle          ( int );
    int                         help_rectangle_index    ( const Video_pos& );   // -1: not found (?)
    const Sos_help_entry&       help_entry              ( const Video_pos& );

    void                        object_store            ( Sos_output_store& );
    Video_mask_help_descr&      object_load             ( Sos_input_store& );

  private:
  //Video_mask_identification  _mask_identification     () const;

    Video_mask_identification  _mask_identification;
    int                        _help_rectangle_count;
    Video_help_rectangle_list  _help_rectangle_list;
};


struct Video_help
{
                                Video_help              ( const string& video_database_filename );

    void                        show_help               ( const Video&, const Video_pos& );

  private:
  //Any_indexed_file           _file;
    Video_mask_help_descr      _mask_help_descr [ 3 ];
};

#include <videohlp.inl>
#endif
