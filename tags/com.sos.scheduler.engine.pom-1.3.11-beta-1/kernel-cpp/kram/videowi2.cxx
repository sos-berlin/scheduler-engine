#include <precomp.h>
#define MODULE_NAME "videowi2"
//                                                    (c) SOS GmbH Berlin
//                                                        Jörg Schwiemann

#if 0

#define SOURCE "videowi2.cpp"
#include <videowin.h>
#include <memory.h>

void VideoWin::_init() {
      Font aFont( "Fixedsys", Size( 0, 15 ) );
      aFont.ChangeCharSet( CHARSET_ANSI );
      aFont.ChangeFamily( FAMILY_MODERN );
      aFont.ChangePitch( PITCH_FIXED );
      aFont.ChangeWeight ( WEIGHT_NORMAL );
      aFont.ChangeAlign( ALIGN_TOP );

      //Font aFont( ResId( FONT_COURIER ) );
      aFont.ChangeSize( Size( 0, 15 ) );
      String aStr; 
      aStr.Fill( video_line_length ); // zur Berechnung der Font-Breite
      ChangeFont( aFont );
      aFontSize = Size( GetTextSize( aStr ).Width() / video_line_length,
                        aFont.GetSize().Height() );
      aScreenSize = Size( aFontSize.Width() * video_line_length,               
                          video_line_count * aFontSize.Height() + _LineOffset );
      ChangeOutputSizePixel( aScreenSize );
      //Maximize();

};

void VideoWin::write( Video_pos pos, char c, Video_attr ) {
       aVideoBuffer[pos.offset()] = c;
       DrawText( Point( pos.column0() * FontSize().Width(),
                        pos.line0() * FontSize().Height() + _LineOffset ),
                 String( c ) );
};

void VideoWin::write( Video_pos pos, char *text, int len, Video_attr ) {
       memcpy( aVideoBuffer+pos.offset(), text, len );
       char buf[video_line_length+1];
       memcpy( buf, text, len );
       buf[len] = 0;
       DrawText( Point( pos.column0() * FontSize().Width(),
                        pos.line0() * FontSize().Height() + _LineOffset ),
                 String( buf ) );
};


void VideoWin::Paint( const Rectangle& ) {
// Rectangle => Berechnung der betroffenen Zeilen und Spalten!
       char buf[video_line_length+1];
       buf[video_line_length] = 0;
       int aHeight = FontSize().Height();

       for ( int i = 0; i < video_line_count; i++ ) {
         memcpy( buf, aVideoBuffer + ( i * video_line_length ),
                 video_line_length );
         DrawText( Point( 0, i * aHeight + _LineOffset), buf );
       };

 };

void VideoWin::move (
        Subrange< int, 0, video_line_count >  ziel_zeilennr,
        Subrange< int, 0, video_line_count >  quell_zeilennr,
        Subrange< int, 0, video_line_count >  zeilenanz )  
{
    memmove( aVideoBuffer + ziel_zeilennr  * video_line_length,
             aVideoBuffer + quell_zeilennr * video_line_length,
             zeilenanz * video_line_length );
    //InvalidateForeground( Rectangle& );
};


void VideoWin::ScrollUpLine( Subrange< int, 0, video_line_count > anz ) {
   Scroll( 0, - anz * FontSize().Height() );
   scroll_up( anz, video_line_count-1, anz );
   // Loeschen der restliche Zeilen;
   for ( int i = video_line_count - anz; i < video_line_count; i++ ) {
     aVideoBuffer[i*video_line_length] = 0;
   };
};

#endif

