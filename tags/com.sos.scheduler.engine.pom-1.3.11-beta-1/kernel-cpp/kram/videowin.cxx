//#if defined __BORLANDC__
//#  define CHECK_STACK_OVERFLOW
//#endif

#include "precomp.h"

//#define MODULE_NAME "videowin"
// videowin.cpp                                         (c) SOS GmbH Berlin
//                                                          Jörg Schwiemann

#include "../kram/sysdep.h"
#if !defined SYSTEM_SOLARIS

#if defined SYSTEM_STARVIEW         // Dieses Modul benötigt StarView

#include "../kram/optimize.h"
#define NDEBUG

#if defined SYSTEM_WIN
#   include <svwin.h>            // #include <windows.h> wg. OutputScreen-Berechnung etc.
#endif

#include <strstream.h>              // SHOW_MSG


#include "../kram/sos.h"
#include "../kram/log.h"

#include "../kram/sosstrng.h"
#include "../kram/svstring.h"
#include "../kram/sosprof.h"
#include "../kram/save.h"
#include "../kram/videowin.h"
#include "kram.hrc"

#define VLOG( x )
//#define VLOG( x )  LOG( x )

const int status_line_distance = 1;   // Abstand der Statuszeile zu den Textzeilen


inline ostream& operator << ( ostream& o, Size s )
{
    return o << '(' << s.Width() << ',' << s.Height() << ')';
}


    struct Video_window_font_dialog : ModelessDialog
    {
                                Video_window_font_dialog( Video_window* parent );

        void                    FontSelectHdl           ( ListBox* );
        void                    SizeSelectHdl           ( ListBox* );
        void                    TrueTypeHdl             ( CheckBox* );
      //const Font&             font                    ();
      //void                    font                    ( const Font& f );
      //void                    font                    ( Font f, Size size ) {};
      //Size                    size                    ();
      //void                    size                    ( Size size );
      //ushort                  new_height              ( ushort wished_height );
      //Size                    next_size               ();
      //Size                    prev_size               ();
      //void                    scalable                ( BOOL s ) { aScalableFlag.Check( s ); };
      //BOOL                    scalable                () { return aScalableFlag.IsChecked(); };
        void                    refresh                 ();
        void                    build_size_list         ();

     protected:
        Font         _old_font;
        FixedText    aFontText;
        ListBox      aFontList;
        FixedText    aSizeText;
        ListBox      aSizeList;
      //CheckBox     aTrueTypeFlag;
      //CheckBox     aScalableFlag;
      //ShowFont     aShowFont;
        CheckBox     _fett_checkbox;
        OKButton     aOkButton;
        CancelButton aCancelButton;
      //HelpButton   aHelpButton;

     private:
        void                   _button_event            ( Button* );
        BOOL                    IsTrueTypeFont          ( const Font& );
        void                   _init_fonts();

      //Font                    aFont;
        Video_window*          _video_window_ptr;
        int                    _refresh_recursion;
    };

//------------------------------------------------------------------------Video_win_accelerator

Video_win_accelerator::Video_win_accelerator()
{
    InsertItem( VW_NEXT_FONT,   KeyCode( KEY_ADD,      KEY_MOD1 ) );
    InsertItem( VW_PREV_FONT,   KeyCode( KEY_SUBTRACT, KEY_MOD1 ) );
    InsertItem( VW_SELECT_FONT, KeyCode( KEY_F,        KEY_MOD1 ) );
    InsertItem( VW_WEIGHT_FONT, KeyCode( KEY_W,        KEY_MOD1 ) );
};

//-----------------------------------------------------------------Video_window::Buffer::Buffer
/* in video.cxx
Video_window::Buffer::Buffer()
{
	 memset( _char_buffer, ' ', sizeof _char_buffer );
}

inline char& Video_window::Buffer::chr( Video_pos pos )
{
	 return _char_buffer[ pos.offset() ];
}

inline Video_attr& Video_window::Buffer::attr( Video_pos pos )
{
	 return _attr_buffer[ pos.offset() ];
}
*/
//------------------------------------------------------------------Video_window::Video_window

Video_window::Video_window( Bool e9750_mode )
:
    WorkWindow          ( NULL, WB_APP | WB_STDWORK ),
    _font_dialog_ptr    ( 0 ),
    _has_cursor         ( TRUE ),
    _resize_rekursiv    ( FALSE ),
    _dont_adjust_resize ( false ),
    _e9750_mode         ( e9750_mode ),
    _selection          ( this ),
    _old_pos            ( 0 ),
    _curr_attr          ( 0xC0 ),
    _paint_begin        ( 0 ),
    _paint_end          ( 0 ),
    _dont_paint         ( 0 ),
    _gray               ( false )
{
    _init();
}

//------------------------------------------------------------------Video_window::Video_window

Video_window::Video_window( Window* pParent, WinBits nWinStyle, BOOL bCursor )
:
    WorkWindow        ( pParent, nWinStyle ),
    _font_dialog_ptr  ( 0 ),
    _has_cursor       ( bCursor ),
    _resize_rekursiv  ( FALSE ),
    _e9750_mode       ( false ),
    _selection        ( this ),
    _old_pos          ( 0 )
{
   _init();
}

//-----------------------------------------------------------------Video_window::~Video_window

Video_window::~Video_window()
{
    SOS_DELETE( _font_dialog_ptr );
}

//---------------------------------------------------------------------------Video_window::font

inline const Font& Video_window::font()
{
    //Font font = GetFont();
    //font.ChangeSize( font_size() );
    return _font;
}

//--------------------------------------------------------------------Video_window::font_height

void Video_window::font_height( ushort height )
{
    if( height == 0 ) {
        SHOW_ERR( "Video_window::font_height( 0 )  -  Font-Größe 0 wird ignoriert" );
        return;
    }

    //Font f = font();
    _font.ChangeSize( Size( 0, height ));
    ChangeFont( _font );
    set_sizes();
}

//--------------------------------------------------------------------Video_window::change_font

void Video_window::change_font( const Font& font_ )
{
    _font = font_;

    _font.ChangeSize( Size( 0, _font.GetSize().Height() ));

    ChangeFont( _font );
    _font.ChangeSize( Size( 0, next_font_height( _font.GetSize().Height(), 0 )));
    ChangeFont( _font );
    set_sizes();
}

//---------------------------------------------------------------Video_window::next_font_height

ushort Video_window::next_font_height( ushort height, int offset /* -1, 0, +1 */ )
{
    if( !GetDevFontSize( _font, 0 ).Height() ) {
        return max( (ushort)1, min( (ushort)127, ushort( height + offset )));
    }

    ushort last_height = height;
    ushort h;

    for( int i = 0; i < GetDevFontSizeCount( _font ); i++ )
    {
        h = GetDevFontSize( _font, i ).Height();
        if( offset < 0  &&  h >= height )  return last_height;
        if( offset ==0  &&  h >= height )  return h;
        if( offset > 0  &&  h >  height )  return h;
        last_height = h;
    }

    return offset < 0? last_height : h;
}

//----------------------------------------------------------------------Video_window::set_sizes

void Video_window::set_sizes()
{
    static const String m_string ( "M" );
    _font_width  = GetTextSize( m_string ).Width();
    _font_height = _font.GetSize().Height();

    Size new_size = output_size_by_font();
    _dont_adjust_resize = true;
    ChangeOutputSizePixel( new_size );      // Fenstergröße anpassen
    _dont_adjust_resize = false;
  //Size real_size = GetOutputSizePixel();
  //if( real_size != new_size )  SHOW_MSG( "GetOutputSizePixel() != new_size" );
    InvalidateForeground();                              // Text neu schreiben
    GetCursor()->ChangeSize( font_size() );              // Cursor anpassen
    set_cursor( _cursor_pos );                           // Cursor neu positionieren

    if( _font_dialog_ptr ) {
        _font_dialog_ptr->refresh();
    }
}

//------------------------------------------------------------Video_window::output_size_by_font

Size Video_window::output_size_by_font()
{
    Size size = Size( font_width() * video_line_length,
                      font_height() * video_line_count  + _e9750_mode );
    return size;
}

//--------------------------------------------------------------------------Video_window::_init

void Video_window::_init()
{
    //Font font;

    ChangeCursor( &_cursor );

    Sos_string font_name; // = read_profile_string( "", "e9750 font", "name" );
    if ( font_name == "" ) font_name = "Fixedsys"; // Default Fixedsys

    _font.ChangeName( as_sv_string( font_name ) );  // Was ist, wenn der Fontname falsch ist? Evtl. prüfen
    uint font_size = 0; //read_profile_uint( "", "e9750", "fontsize" );

    if ( font_size == 0 )
    {
        if ( font_name == "Fixedsys" )
        {
            font_size = 15;     // Default 15 für Font Fixedsys
        } else font_size = 12;  // Default 12 sonst
    }

    _font.ChangeSize( Size( 0, font_size ) );
    change_font( _font );
    set_sizes();

  { // Versuch 6.7.95
    Size size = GetOutputSizePixel();
    size.Height() += font_height(); //read_profile_uint( "", "e9750", "add_height" );
    ChangeOutputSizePixel( size );
    Resize();
  }


    //_scalable = sos_init_parameters.check_flag( "scale", TRUE );

    _accel.PushSelectHdl( LINK( this, Video_window, Font_acc_hdl ) );
    pApp->InsertAccel( &_accel );
    //_font_dialog.scalable( _scalable );
    //_font_dialog.font( font );


    // Fenster geeignet auf dem Schirm positionieren
    Point my_position  = OutputToScreenPixel( Point( 0, 0 ) );
    Size  desktop_size = System::GetScreenSizePixel();
    
#   if defined SYSTEM_WIN
        if( my_position.X() < 0
         || my_position.X() + GetSizePixel().Width() > desktop_size.Width()
         || my_position.Y() < 0
         || my_position.Y() + GetSizePixel().Height() > desktop_size.Height() )
        {
            int border_multiplying_factor = 0;
            SystemParametersInfo( SPI_GETBORDER, 0, &border_multiplying_factor, 0 );
            ChangePosPixel( Point( -GetSystemMetrics( SM_CXBORDER ) * border_multiplying_factor,
                                   -GetSystemMetrics( SM_CYBORDER ) * border_multiplying_factor ));
        }
#   endif

    if( _e9750_mode ) {
    	 ChangePen( Pen( (Color) COL_BLACK ));
    }

    if ( _has_cursor ) {
       GetCursor()->Show();
       _cursor_visible = true;
    }
}

//------------------------------------------------------Video_window::write

void Video_window::write( Video_pos pos, char c, Video_attr attr )
{
    if ( selection()->selected() ) selection()->reset();
    write( pos, &c, 1, attr );
  /*
    VLOG( "Video_window::write( " << pos << ", \'" << c << "\' )\n" );

    if( attr.dark() )  c = ' ';        // Dunkel

    if( _buffer.chr( pos ) != c  ||  _buffer.attr( pos ) != attr )
    {
        _buffer.chr( pos ) = c;
        _buffer.attr( pos ) = attr;
        _paint( pos, pos );
    }
  */
}

//------------------------------------------------------Video_window::write

void Video_window::write( Video_pos pos, char* text, int len, Video_attr attr )
{
  //if( attr.dark() )  LOG( "Video_window::write("<<pos.offset()<<"): dark()\n" );
    if ( selection()->selected() ) selection()->reset();
    if( pos.offset()==0 ) VLOG( "Video_window::write( " << pos << ", \"" << Const_area( text, len ) << "\" )\n" );
    Bool change = false;

    if( !attr.dark() ) {
        if( memcmp( _buffer._char_buffer + pos.offset(), text, len ) != 0 ) {
            memcpy( _buffer._char_buffer + pos.offset(), text, len );
            change = true;
        }
    } else {
        memset( _buffer._char_buffer + pos.offset(), ' ', len );
        change = true;
    }

    for( int i = pos.offset(); i < pos.offset() + len; i++ ) {
        if( _buffer._attr_buffer[ i ] != attr )  break;
    }
    if( i < pos.offset() + len ) {
        for( ; i < pos.offset() + len; i++ )  {
				_buffer._attr_buffer[ i ] = attr;
        }
        change = true;
    }

    if( change )            // _paint_begin und _paint_end puffern den noch zu schreibenen Bereich
    {
        if( _paint_begin.offset() > pos.offset() + len
         || _paint_end.offset()   < pos.offset()       )
        {
            sync();
        }

        if( _paint_begin.offset() == _paint_end.offset() ) {
            _paint_begin = pos;
            _paint_end   = pos.offset() + len;
        } else {
            if( _paint_end.offset() >= pos.offset()
             && _paint_end.offset() <  pos.offset() + len )
            {
                _paint_end = pos.offset() + len;
                VLOG( "new _paint_end=="<<_paint_end<<'\n');
            }
            if( _paint_begin.offset() <= pos.offset() + len
             && _paint_begin.offset() >  pos.offset()       )
            {
                _paint_begin = pos;
                VLOG( "new _paint_begin=="<<_paint_begin<<'\n');
            }
        }
        //_paint( pos, Video_pos( pos.column0() + len - 1, pos.line0() ));
    }
    //VLOG( "_paint_begin=="<<_paint_begin<<"  _paint_end=="<<_paint_end<<'\n');
}

//-------------------------------------------------------Video_window::sync

void Video_window::sync()
{
    if( _paint_begin.offset() == _paint_end.offset() )  return;

    VLOG( "Video_window::sync(): _paint_begin = " << _paint_begin << ", _paint_end = " << _paint_end << "\n" );

    GetCursor()->Hide();

    if( _paint_begin.line0() < _paint_end.line0()  &&  _paint_begin.column0() > 0  )
    {
        Video_pos pos = Video_pos( 0, _paint_begin.line0() + 1 );
        _paint( _paint_begin, pos.offset() - 1 );
        _paint_begin = pos;
    }

    Video_pos pos = Video_pos( 0, _paint_end.offset() / video_line_length  );

    if( _paint_begin.offset() < pos.offset() ) {
        _paint( _paint_begin, pos.offset() - 1 );
        _paint_begin = pos;
    }

    assert( _paint_begin.line0() == _paint_end.line0() );

    if( _paint_begin.offset() < _paint_end.offset() ) {
        _paint( _paint_begin, _paint_end.offset() - 1 );
        _paint_begin = _paint_end;
    }

    GetCursor()->Show();

    if( _has_cursor && !_cursor_visible ) {
        GetCursor()->Show();
        _cursor_visible = true;
    }

    //Update();
}

//------------------------------------------------------Video_window::Paint

void Video_window::Paint( const Rectangle& r )
{
    char buf [ video_line_length + 1 ];
    buf[ video_line_length ] = 0;

    int height = font_height();
    int width  = font_width();

    _paint( Video_pos( min( max( r.Left() / width, 0 ), video_line_length - 1 ),
                       min( max( r.Top() / height, 0 ), video_line_count - 1 )),
            Video_pos( max( min( (r.Right() /*+ width - 1*/) / width, video_line_length - 1 ), 0 ),
                       max( min( (r.Bottom() /*+ height - 1*/) / height, video_line_count - 1 ), 0 )));

    if( _e9750_mode ) {
        int l = (video_line_count-1) * height;
        Point points[ 2 ];

        if( r.Top() <= l  &&  l <= r.Bottom() ) {
            points[ 0 ] = Point( 0, l );
            points[ 1 ] = Point( video_line_length * width, l );
            DrawPolyLine( Polygon(2, points) );
        }
    }

	 if( _has_cursor && !_cursor_visible ) {
		  GetCursor()->Show();
		  _cursor_visible = true;
	 }
}

//-----------------------------------------------------Video_window::_paint

void Video_window::_paint( Video_pos oben_links, Video_pos unten_rechts )
{
    VLOG( "Video_window::_paint( " << oben_links << ", " << unten_rechts << " )\n" );

    if( _dont_paint )  return;    // Scroll()

 //char       buf [ video_line_length + 1 ];
   const int  height                           = font_height();
   const int  width                            = font_width();
   int        zeilennr                         = oben_links.line0();
   const int  letzte_zeilennummer              = unten_rechts.line0();
   const int  erste_spaltennummer              = oben_links.column0();
   const int  letzte_spaltennummer             = unten_rechts.column0();
   Bool       font_changed                     = true;

 //if( _curr_attr.dark() )  LOG( "_paint: dark()!\n" );
   //_font.ChangeFillColor( _curr_attr.half_light()? COL_LIGHTGRAY : COL_WHITE );
   _font.ChangeColor( _gray? _curr_attr.blinking()? COL_LIGHTRED : COL_GRAY
                           : _curr_attr.blinking()? COL_RED      : COL_BLACK );
   //_font.ChangeUnderline( _curr_attr.underlined() ? UNDERLINE_SINGLE : UNDERLINE_NONE );
   //_font.ChangeColor( _gray? COL_GRAY : COL_BLACK );        // Gehört nach gray()
   //_font.ChangeUnderline( UNDERLINE_NONE );

   while( zeilennr <= letzte_zeilennummer )    // Zeilen
   {
       int         spaltennr    = erste_spaltennummer;
       Video_attr* at_ptr       = _buffer._attr_buffer + zeilennr * video_line_length
                                  + erste_spaltennummer;
       Video_attr* at_end_ptr   = at_ptr + video_line_length - erste_spaltennummer;

       while( spaltennr <= letzte_spaltennummer )           // Spalten
       {
           Video_attr* at_begin_ptr = at_ptr;
           Video_attr current_attr = *at_ptr;
/*
   _font.ChangeFillColor( current_attr.half_light()? COL_LIGHTGRAY : COL_WHITE );
   _font.ChangeColor( _gray? current_attr.blinking()? COL_LIGHTRED : COL_GRAY
                           : current_attr.blinking()? COL_RED      : COL_BLACK );
   _font.ChangeUnderline( current_attr.underlined() ? UNDERLINE_SINGLE : UNDERLINE_NONE );
*/

           while( ++at_ptr < at_end_ptr  &&  *at_ptr == current_attr );

           int len = at_ptr - at_begin_ptr;

           //if( zeilennr == 0 && spaltennr == 0 )  VLOG( "_paint: (0,0) current_attr==" <<hex<< (int)current_attr << dec << "\n" );

           if( _curr_attr.half_light() != current_attr.half_light()  ) {
               _font.ChangeFillColor( current_attr.half_light()? COL_LIGHTGRAY :  COL_WHITE );
               font_changed = true;
           }
           if( current_attr.blinking() != _curr_attr.blinking() ) {
               _font.ChangeColor( _gray? current_attr.blinking()? COL_LIGHTRED : COL_GRAY
                                       : current_attr.blinking()? COL_RED      : COL_BLACK );
               font_changed = true;
           }
           if( current_attr.underlined() != _curr_attr.underlined() ) {
               _font.ChangeUnderline( current_attr.underlined() ? UNDERLINE_SINGLE : UNDERLINE_NONE );
               font_changed = true;
           }
           if( font_changed ) {
               WorkWindow::ChangeFont( _font );
               font_changed = false;
           }

           _curr_attr = current_attr;

           char* p       = _buffer._char_buffer + zeilennr * video_line_length + spaltennr;
           {
               Save<char> save_char ( p + len );

               *(p + len) = '\0';
               DrawText( Point( spaltennr * width,
                                zeilennr * height
                                  + ((_e9750_mode &&     // 9750-Statuszeile?
                                      zeilennr == video_line_count - 1)? status_line_distance : 0)),
                         p );
           }
           spaltennr += len;
       }
       zeilennr++;
   }
}


//------------------------------------------------Video_window::_invalidate

void Video_window::_invalidate( Video_pos oben_links, Video_pos unten_rechts )
{
   const int height   = font_height();
   const int width    = font_width();

   InvalidateForeground
             ( Rectangle( Point(   oben_links.column0()         * width,
                                   oben_links.line0()           * height ),
                          Point( ( unten_rechts.column0() + 1 ) * width  - 1,
                                 ( unten_rechts.line0()   + 1 ) * height - 1 )
                        )
             );
}

//-------------------------------------------------------Video_window::move

void Video_window::move( int ziel_zeilennr, int quell_zeilennr, int zeilenanz )
{
    VLOG( "Video_window::move( " << ziel_zeilennr << ", " << quell_zeilennr << ", " << zeilenanz << " )\n" )

    sync();

    memmove( _buffer._char_buffer + ziel_zeilennr  * video_line_length,
             _buffer._char_buffer + quell_zeilennr * video_line_length,
             zeilenanz * video_line_length );
    memmove( _buffer._attr_buffer + ziel_zeilennr  * video_line_length * sizeof(Video_attr),
             _buffer._attr_buffer + quell_zeilennr * video_line_length * sizeof(Video_attr),
             zeilenanz * video_line_length * sizeof(Video_attr) );
#if 1
    Update();
    _dont_paint++;
    Scroll( 0,
            (ziel_zeilennr - quell_zeilennr) * font_height(),
            Rectangle( Point( 0,
                              quell_zeilennr * font_height() ),
                       Point( video_line_length * font_width() - 1,
                              ( quell_zeilennr + zeilenanz ) * font_height() - 1
          )          )      );
    _dont_paint--;
    Update();
    ///*_invalidate*/_paint_begin = Video_pos( 0, video_line_count - 2 );
    //             _paint_end = Video_pos( video_line_length-1,video_line_count-2);
#else
    _invalidate( Video_pos( 0, 0 ),
                 Video_pos( video_line_length-1,video_line_count-1) );
#endif
}


void Video_window::ScrollUpLine( int anz )
{
   Scroll( 0, - anz * font_height() );
   scroll_up( anz, video_line_count-1, anz );
   // Loeschen der restliche Zeilen;
   for ( int i = video_line_count - (int) anz; i < video_line_count; i++ ) {
       _buffer._char_buffer[i*video_line_length] = 0;
   };
};

//-------------------------------------------------Video_window::set_cursor

void Video_window::set_cursor( Video_pos pos )
{
    _cursor_pos = pos;

    if ( !_has_cursor ) return;
    GetCursor()->ChangePos( Point( pos.column0() * font_width(),
                                   pos.line0()   * font_height() ));
    if( _has_cursor ) {
        GetCursor()->Show();
        _cursor_visible = true;
    }
}

//-----------------------------------------------------Video_window::Resize

void Video_window::Resize()
{
     if( _resize_rekursiv || _dont_adjust_resize )  return;

	 struct X
     {
		 X( Video_window* w ) : _w ( w )
         {
             /*sos_application_ptr->busy(true);*/  pApp->Wait(TRUE);
             _w->_resize_rekursiv++;
         }

		 ~X()
         {
             /*sos_application_ptr->busy(false);*/ pApp->Wait(FALSE);
             _w->_resize_rekursiv--;
         }

       private:
         Video_window* _w;
	 } x ( this );


	 if ( !IsMaximized() ) {
//		_restored_size = GetOutputSizePixel();
//		_fs = _font_size;
//		_font = GetFont();
	 }

	 if( IsMinimized() )  {
		if ( _maximized ) _maximized = FALSE;
		_minimized = TRUE;
		return;
	 };

	 if( IsMaximized() )  {
		if ( _minimized ) _minimized = FALSE;
		_maximized = TRUE;
	 } else if ( _minimized ) {
		_minimized = FALSE;
		return;
	 } else if ( _maximized ) {
		_maximized = FALSE;
		// _restored_size wiederherstellen!
		Resize(); // nur einmal !
		return;
	 };

	 //LOG( "GetOutputSizePixel() == " << GetOutputSizePixel() << endl );

  //const USHORT menubar_height  = System::GetMenuBarHeightPixel();
    const Size   size            = GetOutputSizePixel();
    const ushort wished_height
        = /*Size( max( 2, size.Width() / video_line_length ),*/
	            max( 2, ( size.Height()  - ( _e9750_mode ? status_line_distance
                                                         : 0                    )) / video_line_count );
    //LOG( "Video_window::Resize(): wished_height="<<wished_height<<'\n' );

	if( wished_height != font_height() ) {
        // new_font_size <= wished_size
        //ushort new_font_height = _font_dialog.new_height( wished_height );
	    //LOG( "Video_window::Resize(): new_font_height="<<new_font_height<<'\n' );
        //Font new_font = GetFont();
	    _font.ChangeSize( Size( 0, next_font_height( wished_height, 0 )));
        //ChangeOutputSizePixel( output_size_by_font() );
	    change_font( _font );
    }/* else {
        if ( _max_size != size ) {
            ChangeOutputSizePixel( _max_size );
        }
    }*/

    //Size new_size = output_size_by_font();
    //if( size != new_size ) {
        set_sizes();
    //}
}

//-------------------------------------------------------------------Video_window::Font_acc_hdl

long Video_window::Font_acc_hdl( Accelerator* pAcc )
{
    //LOG( "Video_window::Font_acc_hdl: itemId=" << pAcc->GetCurItemId() << endl );
    //Font f = GetFont();
    switch ( pAcc->GetCurItemId() )
    {
    case VW_NEXT_FONT  : font_height( next_font_height( font_height(), +1 ));  // Control-+
                         break;
    case VW_PREV_FONT  : font_height( next_font_height( font_height(), -1 ));  // Control--
                         break;
    case VW_SELECT_FONT: show_font_dialog(); break;                            // Control-F
    case VW_WEIGHT_FONT: _font.ChangeWeight( WEIGHT_BOLD );                    // Control-W
                         change_font( _font );
                         break;
    default: return FALSE;
    }

    return TRUE;
}


//---------------------------------------------------------------Video_window::show_font_dialog

void Video_window::show_font_dialog()
{
    if( !_font_dialog_ptr ) {
        _font_dialog_ptr = new Video_window_font_dialog( this );
    }

    _font_dialog_ptr->Show();
    _font_dialog_ptr->GrabFocus();
}

// ---------------------------------------Video_window::Selection::select_rect
void Video_window::Selection::select_rect( const Video_pos& vp1, const Video_pos& vp2 )
{
  Video_pos lo( min( vp1.column0(), vp2.column0() ),
                min( vp1.line0(),   vp2.line0()   ) );
  Video_pos ru( max( vp1.column0(), vp2.column0() ),
                max( vp1.line0(),   vp2.line0()   ) );

  Point _start = _video_win_ptr->video_pos_to_pixel( lo, TRUE  );
  Point _end   = _video_win_ptr->video_pos_to_pixel( ru, FALSE );

  _video_win_ptr->HighlightRect( Rectangle( _start, _end ) );
};

// -----------------------------------------Video_window::Selection::innerhalb
BOOL Video_window::Selection::innerhalb( const Video_pos& vp ) {
//    if ( !_is_selected ) return FALSE;
//    int max_line0   = max( _start_point.line0(), _end_point.line0() );
//    int min_line0   = min( _start_point.line0(), _end_point.line0() );
//    int max_column0 = max( _start_point.column0(), _end_point.column0() );
//    int min_column0 = min( _start_point.column0(), _end_point.column0() );
//    return BOOL( vp.line0()   >= min_line0   &&
//                 vp.line0()   <= max_line0   &&
//                 vp.column0() >= min_column0 &&
//                 vp.column0() <= max_column0    );
    return TRUE;
}


// ----------------------------------------Video_window::Selection:start_pixel

void Video_window::Selection::start_pixel( const Point&     pixel,
                                           const Video_pos& vp ) {
   _start_pixel = pixel;
   start_select( vp );
};

// ----------------------------------------Video_window::Selection::drag_event

void Video_window::Selection::drag_event( const Video_pos& vp ) {

   if ( !_is_selected ) {
     // alten Cursor wiederherstellen, wir sind im Selektionsmodus!
     _video_win_ptr->reset_cursor();
     _is_selected = TRUE;
     _is_in_select = TRUE;
     _end_point = vp;
     if ( _start_pixel == Point(-1,-1) ) {
       _start_pixel = _video_win_ptr->video_pos_to_pixel( vp );
     }
     select_rect( _start_point, _end_point ); // Initiale Markierung
     return;
   } else if ( !_is_in_select ) return;              // Kein weitere Selektion

   if ( vp.offset() == _end_point.offset() ) return; // nix geändert

   redraw(); // ??? Invertiert wieder
   _end_point = vp;
   select_rect( _start_point, _end_point );
}

// ------------------------------------------------Video_window::reset_cursor

void Video_window::reset_cursor()
{
     application_cursor( _old_pos );
}

// ------------------------------------Video_window::pixel_to_video_pos

Video_pos Video_window::pixel_to_video_pos ( const Point& pt )
{
  return Video_pos( min( max(0,pt.X()) / font_width(), (ushort)video_line_length-1 ),
                    min( max(0,pt.Y()) / font_height(), (ushort)video_line_count-1 ) );
}

// ------------------------------------------Video_window::video_pos_to_pixel

Point Video_window::video_pos_to_pixel( const Video_pos& vp,
                                              BOOL upper_left ) {
  int off = upper_left ? 0 : 1;
  // Vorsicht: Variable off einmal font-, einmal pixelorientiert benutzt!
  return Point( font_width()  * ( vp.column0() + off ) - off,
                font_height() * ( vp.line0()   + off ) - off );
};

// -----------------------------------------Video_window::copy_text_selection

void Video_window::copy_text_selection ( const Video_pos& lo ,
                                         const Video_pos& ru ) {
   String out;
   int line_max_len=ru.column0() - lo.column0() + 1;
   for ( int i=lo.line0(); i <= ru.line0(); i++ ) {
      int line_len = 0;
      Video_pos vp( lo.column0(), i );
      // nach Blanks am Ende suchen (falls vorhanden) in jeder Zeile!
      for ( int j=line_max_len; j >= 0; j-- ) {
         if ( *(char_buffer()+vp.offset()+j) != ' ' ) {
           line_len = j; break;
         };
      }; 
      out+= String( (const char*)(char_buffer()+vp.offset()), line_len ) +
            String( "\r\n" );
   };
   Clipboard::Clear(); 
   Clipboard::CopyString( out );
};


// -----------------------------------------Video_window::hardcopy

void Video_window::hardcopy () {
//   if ( selection->selected() ) selection()->redraw();
//   Video_pos lo(0);
//   Video_pos ru
//   Bitmap b=GetBitmap( video_pos_to_pixel(lo),
//                       Size( line_max_len*font_width(),
//                             (ru.line0()-lo.line0()+1)*font_height() ) );
//   Clipboard::CopyBitmap( b );
//   if ( selection->selected() ) selection()->redraw();
};

// ----------------------------------- Video_window_font_dialog::Video_window_font_dialog

Video_window_font_dialog::Video_window_font_dialog( Video_window* parent )
:
   ModelessDialog( parent, ResId( FONT_DIALOG          ) ),
   aFontText     ( this,   ResId( FONT_DIALOG_FONT     ) ),
   aFontList     ( this,   ResId( FONT_DIALOG_FONT     ) ),
   aSizeText     ( this,   ResId( FONT_DIALOG_SIZE     ) ),
   aSizeList     ( this,   ResId( FONT_DIALOG_SIZE     ) ),
 //aTrueTypeFlag ( this,   ResId( FONT_DIALOG_TRUETYPE ) ),
 //aScalableFlag ( this,   ResId( FONT_DIALOG_SCALABLE ) ),
 //aShowFont     ( this,   ResId( FONT_DIALOG_SHOWFONT ) ),
   aOkButton     ( this,   ResId( FONT_DIALOG_OK       ) ),
   aCancelButton ( this,   ResId( FONT_DIALOG_CANCEL   ) ),
   _fett_checkbox( this,   ResId( FONT_DIALOG_FETT     ) ),
 //aHelpButton   ( this,   ResId( FONT_DIALOG_HELP     ) ),
   _video_window_ptr( parent ),
   _refresh_recursion ( 0 )
{
   FreeResource();

   aFontList     .ChangeSelectHdl( LINK( this, Video_window_font_dialog, FontSelectHdl ) );
   aSizeList     .ChangeSelectHdl( LINK( this, Video_window_font_dialog, SizeSelectHdl ) );
 //aTrueTypeFlag .ChangeClickHdl ( LINK( this, Video_window_font_dialog, TrueTypeHdl ) );
   aOkButton     .ChangeClickHdl ( LINK( this, Video_window_font_dialog, _button_event ) );
   aCancelButton .ChangeClickHdl ( LINK( this, Video_window_font_dialog, _button_event ) );
   _fett_checkbox.ChangeClickHdl ( LINK( this, Video_window_font_dialog, _button_event ) );

   _old_font = _video_window_ptr->font();  // Wird bei CANCEL wiederhergestellt
   _init_fonts();

   refresh();
}

// -------------------------------------------------------------- Video_window_font_dialog::refresh

void Video_window_font_dialog::refresh()
{
   if( _refresh_recursion )  return;
   _refresh_recursion++;

   _fett_checkbox.Check( _video_window_ptr->font().GetWeight() == WEIGHT_BOLD );
   aFontList.SelectEntry( _video_window_ptr->font().GetName() );
   build_size_list();

   _refresh_recursion--;
}

// --------------------------------- Video_window_font_dialog::_init_fonts

void Video_window_font_dialog::_init_fonts()
{
    int n = GetDevFontCount();
    aFontList.Clear();
    for ( int i = 0; i < n; i++ ) {
        if ( GetDevFont( i ).GetPitch() == PITCH_FIXED ) {
            /*if ( aTrueTypeFlag.IsChecked() ) {
                IsTrueTypeFont( GetDevFont( i ) );
            };*/
            aFontList.InsertEntry( GetDevFont( i ).GetName() );
        }
    }
}

//------------------------------------------------------Video_window_font_dialog::button_event

void Video_window_font_dialog::_button_event( Button* button_ptr )
{
    if( button_ptr == &_fett_checkbox ) {
        Font f = _video_window_ptr->font();
        f.ChangeWeight( _fett_checkbox.IsChecked()? WEIGHT_BOLD : WEIGHT_NORMAL );
        _video_window_ptr->change_font( f );
        return;
    }

    Hide();

    if( button_ptr == &aCancelButton ) {
        _video_window_ptr->change_font( _old_font );    // Alte Schrift wiederherstellen
    }

    SOS_DELETE( _video_window_ptr->_font_dialog_ptr );
}

// --------------------------------- Video_window_font_dialog::IsTrueTypeFont
/*
BOOL Video_window_font_dialog::IsTrueTypeFont( Font f ) {
   static Window* pWindow = NULL;

   if (!pWindow) pWindow = new Window( this );
   pWindow->ChangeFont( f );
   FontMetric fm = pWindow->GetFontMetric();
   USHORT nHeight = pWindow->GetDevFontSize( f, 0 ).Height();
   return BOOL( fm.GetType() == TYPE_SCALABLE || !nHeight );
};
*/
// --------------------------------- Video_window_font_dialog::FontSelectHdl

void Video_window_font_dialog::FontSelectHdl( ListBox* )
{
    Font font = _video_window_ptr->font();
    font.ChangeName( aFontList.GetSelectEntry() );
    _video_window_ptr->change_font( font );

    build_size_list();
    //SizeSelectHdl( &aSizeList );
}

//--------------------------------------------- Video_window_font_dialog::build_size_list

void Video_window_font_dialog::build_size_list()
{
    const Font& font = _video_window_ptr->font();

    aSizeList.ChangeUpdateMode( FALSE );
    aSizeList.Clear();

    if ( !GetDevFontSize( font, 0 ).Height() )         // True-Type?
    {
        for ( int i = 3; i <= 50; i++ ) {
            aSizeList.InsertEntry( i );
        }
    }
    else
    {
        for ( int i = 0; i < GetDevFontSizeCount( font ); i++ ) {
   	        aSizeList.InsertEntry( GetDevFontSize( font, i ).Height() );
        }
    }

    aSizeList.ChangeUpdateMode( TRUE );
    aSizeList.Invalidate();
    aSizeList.SelectEntry( _video_window_ptr->next_font_height( _video_window_ptr->font_height(), 0 ) );
}

// --------------------------------- Video_window_font_dialog::SizeSelectHdl

void Video_window_font_dialog::SizeSelectHdl( ListBox* )
{
   //aFont.ChangeSize( Size( 0, aSizeList.GetSelectEntry() ) );
   //aShowFont.ChangeFont( aFont );

   _video_window_ptr->font_height( as_int( aSizeList.GetSelectEntry() ) );
}

// --------------------------------- Video_window_font_dialog::TrueTypeHdl
/*
void Video_window_font_dialog::TrueTypeHdl( CheckBox* )
{
   String _current = aFontList.GetSelectEntry();
   _init_fonts();
   if ( _current != "" &&
   	  aFontList.GetEntryPos(_current) != LISTBOX_ENTRY_NOTFOUND ) {
     aFontList.SelectEntry(_current);
   } else {
     aFontList.SelectEntryPos(0);
   };
   FontSelectHdl( &aFontList );
};
*/
// --- ShowFont::ShowFont() ----------------------------------------
/*
ShowFont::ShowFont( Window* pParent, const ResId& rResId ) :
    Control( pParent, rResId )
{
   ChangeBackgroundBrush( Brush( Color( COL_WHITE ) ) );
}

// --- ShowFont::Paint() -------------------------------------------

void ShowFont::Paint( const Rectangle& )
{
    USHORT x,y;
	 Size   aTextSize( GetTextSize( GetText() ) );
    Size   aWindowSize( GetOutputSize() );

    x = aWindowSize.Width()/2 - aTextSize.Width()/2;
    y = aWindowSize.Height()/2 - aTextSize.Height()/2;
    DrawText( Point( x, y ), GetText() );
}

// --- ShowFont::ChangeFont() --------------------------------------

Font ShowFont::ChangeFont( const Font& rFont )
{
    Invalidate();
    return Control::ChangeFont( rFont );
}
*/


#endif                  // SYSTEM_STARVIEW

#endif                  // SYSTEM_SOLARIS
