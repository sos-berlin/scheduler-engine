// videowin.h                                           (c) SOS GmbH Berlin

#ifndef __VIDEOWIN_H
#define __VIDEOWIN_H

#include <video.h>

#if !defined _SV_HXX
#   include <sv.hxx>
#   undef min
#   undef max
#endif

#if defined __BORLANDC__
#   include <memory.h>
#endif

// --- struct ShowFont ----------------------------------------------
/*
struct ShowFont : public Control
{
  public:
                                ShowFont                ( Window* pParent, const ResId& rResId );

    virtual void                Paint                   ( const Rectangle& );
    Font                        ChangeFont              ( const Font& rFont );
};
*/
#define VW_NEXT_FONT   1
#define VW_PREV_FONT   2
#define VW_SELECT_FONT 3
#define VW_WEIGHT_FONT 4

#define MIN_DRAG_OFFSET 3 // Pixel

class Video_win_accelerator : public Accelerator
{
  public:
                                Video_win_accelerator();
};

typedef unsigned short ushort;

struct Video_window_font_dialog;

struct Video_window : WorkWindow, Video
{
    struct Selection
    {
                                Selection               ( Video_window* p ) :
                                    _video_win_ptr(p),
                                    _is_selected( FALSE ),
                                    _is_in_select(FALSE),
                                    _start_pixel(-1,-1),
                                    _start_point(Video_pos(0,0)),
                                    _end_point(Video_pos(0,0))
                                {}

         void                      select( const Video_pos& start, const Video_pos& ende ) {
            select_rect( start, ende );
            _start_point = start;
            _end_point = ende;
            _is_selected = TRUE;
         }

         void          start_select( const Video_pos& start ) {
                reset();
                _is_in_select = TRUE;
                _start_point = start;
          }
          void          stop_select() {
                if ( _is_in_select && _start_pixel != Point(-1,-1) )
                {
                    _video_win_ptr->_paint( _start_point, _end_point );
                } else
                {
                    // ganzer Bildschirm
                     _video_win_ptr->_paint( Video_pos(0,0), Video_pos(video_line_length-1,video_line_count-1) );
                }
                redraw();
                _is_in_select = FALSE;
          }

          void          redraw() {
            if ( _is_selected ) select_rect( _start_point, _end_point );
          };
          void          reset() {
            if ( _is_selected ) select_rect( _start_point, _end_point );
            _is_selected  = FALSE;
            _is_in_select = FALSE;
            _start_pixel  = Point(-1,-1);
    	  }

          void          start_pixel( const Point&     pixel,
                                     const Video_pos& vp );

          BOOL          is_drag_allowed( const Point& p ) {
             return BOOL( _start_pixel == Point(-1,-1) ||
                          abs( p.X() - _start_pixel.X() ) > MIN_DRAG_OFFSET ||
                          abs( p.Y() - _start_pixel.Y() ) > MIN_DRAG_OFFSET
                         );
          }

      void          drag_event( const Video_pos& );
      BOOL          selected() { return _is_selected; };
      BOOL          is_in_select() { return _is_in_select; };

      Video_pos     start() { return _start_point; };
      Video_pos     end() { return _end_point; };


    private:

      void          select_rect( const Video_pos&, const Video_pos& );
      BOOL          innerhalb( const Video_pos& );
      BOOL          ausserhalb( const Video_pos& vp ) {
        return BOOL( !innerhalb(vp) );
      };

      Video_pos     _start_point;
      Video_pos     _end_point;
      Point         _start_pixel;

      BOOL          _is_selected;
      BOOL          _is_in_select;
      Video_window* _video_win_ptr;
    };


/*  struct Buffer
    {
                        Buffer          ();
        Area            char_line       ( int line_no );
        Area            attr_line       ( int line_no );
        char&           chr             ( Video_pos );
        Video_attr&     attr            ( Video_pos );

    //private:
        char            _char_buffer [video_line_length * video_line_count + 1];  // Eins mehr fr 0-Bytes.
        Video_attr      _attr_buffer [video_line_length * video_line_count + 1];  // Eins mehr für Endemarke
    };
*/

                                Video_window            ( Bool e9750_mode = false );
                                Video_window            ( Window* pParent, WinBits nWinStyle,
                                                          BOOL bCursor = FALSE );
     virtual                   ~Video_window            ();

            void                show_font_dialog        ();
            void                change_font             ( const Font& rNewFont );
            void                set_sizes               ();
          //Size                ChangeOutputSizePixel   ( const Size& rNewSize );
    virtual void                Paint                   ( const Rectangle& );
    virtual void                Resize                  ();
    virtual void                write                   ( Video_pos, char, Video_attr );
    virtual void                write                   ( Video_pos, char*, int len, Video_attr );
    void                        sync                    ();
    virtual void                move                    ( int ziel, int quelle, int zeilenanz );
    virtual void                paste_data              () {} // so gut wie abstrakt!
    virtual void                copy_selection()
    {
      if ( selection()->selected() ) {
        Video_pos lo( min( selection()->start().column0(), selection()->end().column0() ),
                      min( selection()->start().line0(),   selection()->end().line0()   ) );
        Video_pos ru( max( selection()->start().column0(), selection()->end().column0() ),
                      max( selection()->start().line0(),   selection()->end().line0()   ) );

       copy_text_selection( lo, ru );
       copy_bitmap_selection( lo, ru );
      };
    };

    virtual void                copy_text_selection     ( const Video_pos&, const Video_pos& );
    virtual void                copy_bitmap_selection   ( const Video_pos& lo, const Video_pos& ru )
    {
      selection()->redraw();
      int line_max_len=ru.column0() - lo.column0() + 1;
      Bitmap b=GetBitmap( video_pos_to_pixel(lo),
                          Size( line_max_len*font_size().Width(),
                                (ru.line0()-lo.line0()+1)*font_size().Height() ) );
      Clipboard::CopyBitmap( b );
      selection()->redraw();
    };

    virtual void                hardcopy                ();
    Selection*                  selection               () { return &_selection; };
    void                        reset_cursor            ();
    virtual void                application_cursor      ( Video_pos pos )
    {
      set_cursor( pos ); // Default Implementierung
    }

    void                        old_pos                 ( const Video_pos& old )  // Alte Cursor-Position merken
    {
      _old_pos = Video_pos( old.offset() ); // Copy
    }

    virtual void                set_cursor              ( Video_pos pos );
    const Font&                 font                    ();
    Size                        font_size               () { return Size( font_width(), font_height() ); }
    ushort                      font_height             ();
    void                        font_height             ( ushort );
    ushort                      next_font_height        ( ushort height, int offset = 1 /* oder -1 */ );
    ushort                      font_width              ();
    const char*                 char_buffer             () { return _buffer._char_buffer; };
    void                        ChangeCharSet           ( CharSet fcs )
    {
       Font f = GetFont();
       f.ChangeCharSet( fcs );
       ChangeFont( f );
    }

    void                        ScrollUpLine            ( int count = 1 );
    void                        gray                    ( Bool g )      { if( _gray != g ) { _gray = g; InvalidateForeground(); } }

    // Konvertierungsroutinen
    virtual Video_pos           pixel_to_video_pos      ( const Point& );
            Point               video_pos_to_pixel      ( const Video_pos&, BOOL upper_left = TRUE );

    int                        _resize_rekursiv;        // Nur für Resize!!


  protected:
    virtual void               _init                    ();

  private:
    friend class                Video_window_font_dialog;
    friend class                Video_window::Selection;
    void                       _paint                   ( Video_pos ol, Video_pos ur );
    void                       _invalidate              ( Video_pos ol, Video_pos ur );
    Size                        output_size_by_font     ();
    long                        Font_acc_hdl            ( Accelerator* );

    Bool                       _dont_adjust_resize;
    Selection                  _selection;         // Markierung
  //Buffer                     _buffer;            // Der ganze Text
    Cursor                     _cursor;
    BOOL                       _has_cursor;
    Bool                       _cursor_visible;
    Video_pos                  _cursor_pos;
    Video_pos                  _old_pos;
    const Bool                 _e9750_mode;
    BOOL                       _minimized;
    BOOL                       _maximized;
    Font                       _font;
    unsigned short             _font_width;
    unsigned short             _font_height;
    Video_attr                 _curr_attr;
    Video_pos                  _paint_begin;
    Video_pos                  _paint_end;
    BOOL                       _scalable;
    Video_window_font_dialog*  _font_dialog_ptr;
    Video_win_accelerator      _accel;
    int1                       _dont_paint;
    Bool                       _gray;
};


//------------------------------------------------------------------------Video_window::font_height

inline ushort Video_window::font_height()
{
    //return _font.GetSize().Height();
    return _font_height;
    //return GetTextSize( String("M") ).Height();
}

//-------------------------------------------------------------------------Video_window::font_width

inline ushort Video_window::font_width()
{
    //return GetTextSize( String("M") ).Width();
    return _font_width;
}



#endif

