// FRMPAGE.H
#ifndef __FRMPAGE_H
#define __FRMPAGE_H

#if defined( SYSTEM_WIN )
#include <svwin.h>
#define _INC_WINDOWS
#endif

#pragma option -x-
#include <sv.hxx>
#pragma option -x

#include "../kram/sos.h"     
#include "../kram/area.h"
#include "../kram/xception.h"
#include "../kram/log.h"



// DECLARE_ACCESS_METHODS deklariert und definiert Methoden für den Zugriff auf eine Variable

#define DECLARE_REF_ACCESS_METHODS( Type, name )                           \
    void            name( const Type& value )  { _##name = value; }    \
    Type& const     name()                     { return _##name;  }    \
  /*Type _##name;  wird außerhalb des Makros deklariert */

// DECLARE_PUBLIC_MEMBER deklariert ein privates Element und die öffentlichen Methoden für den Zugriff

#define DECLARE_PUBREF_MEMBER( Type, name )   \
  public:                                     \
    DECLARE_REF_ACCESS_METHODS( Type, name )      \
  private:                                    \
    Type _##name;                             \
  public:                 // Endet in public!!


#include <sosarray.h>

typedef Sos_simple_array<long> Long_list;

struct Page_list {
                        Page_list               ();
    void                insert                  ( long offset );
    long                bytes_count             ( uint page_no );
    long                start_offset            ( uint page_no );
    long                ende_offset             ( uint page_no );
    int                 count                   ();
private:
    Long_list _list;
};


struct Frame_code {
    enum                Frame_code_enum { fc_ascii, fc_german };

                        Frame_code      ( Frame_code_enum c = fc_ascii ) { code(c); };

    DECLARE_PUBLIC_MEMBER( Frame_code_enum, code )

    void                modify_table( char* table );
private:
    int                 _index( const char* );
};

#define MAX_STACK_CURSOR    20

enum Scripting { scripting_normal, scripting_super, scripting_sub };

struct Format_status {
                            Format_status();

    DECLARE_PUBREF_MEMBER( int,         left_margin     )  // = 0;
    DECLARE_PUBREF_MEMBER( int,         diff_left_margin)  // = 0;
    DECLARE_PUBREF_MEMBER( int,         line_distance   )  // = 240;
    DECLARE_PUBREF_MEMBER( Orientation, orientation     )  // = ORIENTATION_PORTRAIT;
    DECLARE_PUBREF_MEMBER( Frame_code,  code            )  // = one-of { ASCII, GERMAN }
    DECLARE_PUBREF_MEMBER( Point,       position        )  // = Point(0,0);
    DECLARE_PUBREF_MEMBER( Font,        font            )  // = CourierNew, transparent, etc.
    DECLARE_PUBREF_MEMBER( int,         size            )  // = in Points: 10 pt
    DECLARE_PUBREF_MEMBER( Scripting,   scripting       )  // = Sub/Superscripting
    DECLARE_PUBREF_MEMBER( Sos_bool,    sv_stretch      )  // = in Points: 10 pt

    Bool                    bold()      { return( font().GetWeight() == WEIGHT_BOLD ); }
    Bool                    underline() { return( font().GetUnderline() == UNDERLINE_SINGLE ); }
    Bool                    italic()    { return( font().IsItalic() ); }

    void                    restore_cursor();
    void                    save_cursor();
    void                    reset_stack() { _stack_count = 0; }

    void                    parse_font_name( const char* );

    int                     _fontwidth_array[256];
    static void             window_char_sizes( int*, const Font& );
private:
    Point                   _pos_stack[MAX_STACK_CURSOR];
    uint                    _stack_count;
};


struct Frame_page_list;

struct Frame_esc_parser {
                            Frame_esc_parser    ( OutputDevice*, Format_status& );
             virtual        ~Frame_esc_parser() {}

    DECLARE_PUBREF_MEMBER   ( Format_status, status )
    void                    output_device( OutputDevice* out );

    void                    parse               ( istream*, long* );

    virtual void            open                ()                                      = 0;
    virtual void            put                 ( const Const_area& )                   = 0;
    virtual void            close               ()                                      = 0;

protected:
    // Page-Handling
    virtual void            end_page            ();
    virtual void            draw_text           ( char*, int );

    // Format veräändernd
    virtual void            newline             ();
    virtual void            gutter              ( int );
    virtual void            char_indent         ( int );
    virtual void            line_indent         ( int );
    virtual void            horizontal_move     ( uint );
    virtual void            tabulate            ( uint );
    virtual void            font_attr           ( char, Bool );
    virtual void            scripting           ( char, Bool );
    virtual void            brace_sequence      ( const char*, int );

    OutputDevice*           _output_device;

    DECLARE_PUBLIC_MEMBER( Bool, debug )
    DECLARE_PUBLIC_MEMBER( Bool, page_error )

private:
    virtual void            _init_device();
    friend                  Frame_page_list; // nur der soll _init_device() überschreiben können!
    char                    _translate_table[256];
};


inline void Frame_esc_parser::newline()
{
    status().position() = Point( /*status().left_margin()*/ 0,
                                 status().position().Y() + status().line_distance() );
};


inline void Frame_esc_parser::end_page()
{
    status().position() = Point( /*status().left_margin()*/ 0, 0 );
};


inline void Frame_esc_parser::char_indent ( int l )
{
    // ??? aus CIxxx folgt Auswahl der Punktschrift (CI120==10pt,CI100==12pt)
    int size    = 1200 / l;

    if ( status().size() != size ) {
      status().size( size );
      status().font().ChangeSize( Size( 0, status().size()*20 ) );
      _output_device->ChangeFont( status().font() );
      _init_device();
    };
}

inline void Frame_esc_parser::line_indent ( int l )
{
    status().line_distance( 14400 / l );
}

inline void Frame_esc_parser::font_attr( char attr, Bool on )
{
     switch( attr ) {
        case 'M': status().font().ChangeWeight(    on ? WEIGHT_BOLD      : WEIGHT_NORMAL  ); break;
        case 'U': status().font().ChangeUnderline( on ? UNDERLINE_SINGLE : UNDERLINE_NONE ); break;
        case 'I': status().font().ChangeItalic(    on                                     ); break;
        default:  break;
    };
     _init_device();
}

inline void Frame_esc_parser::scripting( char attr, Bool on )
{
    if ( on && attr == 'E' )
    {
        status().scripting(scripting_super);
    } else if ( on && attr == 'S' )
    {
        status().scripting(scripting_sub);
    } else
    {
        status().scripting(scripting_normal);
    }
}


// Klasse fuer Direktdruck

struct Frame_esc_print : Frame_esc_parser {
                        Frame_esc_print         ( Printer*, Format_status& );
    virtual             ~Frame_esc_print        () {}

    virtual void       open                     ();
    virtual void       put                      ( const Const_area& );
    virtual void       close                    ();
    virtual void       end_page                 ();
    virtual void       draw_text                ( char*, int );

private:
    Bool                _new_page;
};



struct Frame_page_list : Frame_esc_parser {
                        Frame_page_list         ();
    virtual             ~Frame_page_list        ();

    uint                page_count              () { return _page_list.count(); };
    long                bytes_count             ( uint page_no ) { return _page_list.bytes_count(page_no); };
    long                offset                  ( uint page_no ) { return _page_list.start_offset(page_no); };

    void                print_page              ( Printer*, uint page_no );
    void                get_page                ( uint page_no, Dynamic_area& );

    virtual void        open                    ();
    virtual void        put                     ( const Const_area& );
    virtual void        close                   ();


protected:
                        Frame_page_list         ( const char* filename );

    virtual void        end_page                ();
    virtual void        draw_text               ( char*, int );
    virtual void        gutter                  ( int );
//    virtual void        brace_sequence          ( const char*, int );

private:
    virtual void        _init_device();

    long                write_status            ();
    void                read_status             () {};

    Bool                _opened;
    Bool                _new_page;
    Bool                _pass1;
    long                _offset;
    long                _parse_len;
    long                _extra_bytes;
    String              _filename;
    fstream             _file;
    Page_list           _page_list;
};




struct Frame_preview_box :  Control {
                        Frame_preview_box                       ( Window*, const ResId&,
                                                                  Frame_page_list*,
                                                                  const ResId&, const ResId&        );
                        ~Frame_preview_box                      (                                   );



    Link                change_request_page_hdl                 ( const Link& rLink                 );


    // Button-Handler
    void                zoom_in_hdl                             ( Button* = 0                       );
    void                zoom_out_hdl                            ( Button* = 0                       );
    void                normal_page_hdl                         ( Button* = 0                       );
    void                full_page_hdl                           ( Button* = 0                       );
    void                double_page_hdl                         ( Button* = 0                       );        
    void                prev_page_hdl                           ( Button* = 0                       );
    void                next_page_hdl                           ( Button*                           );    

    void                do_print_direct                         ( Button* = 0                       );
    void                do_print                                ( Button* = 0                       );
    void                do_setup                                ( Button* = 0                       );
    
    void                print_page                              ( Preview*                          );
    void                print_page_hdl                          ( Preview*                          );

    void                handle_exception                        ();
    
private:
    ScrollBar           aVScroll;
    ScrollBar           aHScroll;
    //void                vertical_scrollbar                      ( ScrollBar*                        );
    //void                horizontal_scrollbar                    ( ScrollBar*                        );
    // Scroll-Handler
    void                vscroll_end_hdl                         ( ScrollBar* pScrollBar             );
    void                hscroll_end_hdl                         ( ScrollBar* pScrollBar             );
    
    // Preview Berechnungsfunktionen
    Size                calc_preview_size                       (                                   );
    void                check_scroll_range                      (                                   );

    Bool                _first_page;
    USHORT              _page;
    Fraction            _zoom;
    Point               _offset;
    Size                _max_size;
    Link                _request_page_hdl;
    Preview*            _preview_ptr;
    Preview*            _second_preview_ptr;
    JobSetup*           _job_setup_ptr;
    Printer*            _printer_ptr;

    ScrollBar*          _vertical_scrollbar_ptr;
    ScrollBar*          _horizontal_scrollbar_ptr;

    Frame_page_list*    _page_list_ptr;

};


struct Frame_modeless_preview : ModelessDialog {

private:
    Frame_preview_box   aPVBox;
    PushButton          aZoomIn;
    PushButton          aZoomOut;
    PushButton          aNormalPage;
    PushButton          aFullPage;
    PushButton          aDoublePage;
    PushButton          aPrevPage;
    PushButton          aNextPage;
    PushButton          aPrintButton;
    PushButton          aSetupButton;
    CancelButton        aCancel;

public:
                        Frame_modeless_preview( Window* pParent, Frame_page_list* );
                        ~Frame_modeless_preview();

    virtual BOOL        Close();
    void                CancelHdl( Button* );
};


struct Frame_modal_preview : ModalDialog {

private:
    Frame_preview_box   aPVBox;
    PushButton          aZoomIn;
    PushButton          aZoomOut;
    PushButton          aNormalPage;
    PushButton          aFullPage;
    PushButton          aDoublePage;
    PushButton          aPrevPage;
    PushButton          aNextPage;
    PushButton          aPrintButton;
    PushButton          aSetupButton;
    CancelButton        aCancel;

public:
                        Frame_modal_preview( Window* pParent, Frame_page_list*, const ResId& );
                        ~Frame_modal_preview();

    virtual BOOL        Close();
    void                CancelHdl( Button* );

};



#endif
