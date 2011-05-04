//#define MODULE_NAME "frmpage"
//#define COPYRIGHT   "(c) SOS GmbH Berlin"
//#define AUTHOR      "Jörg Schwiemann"

/* Frame-Druckertreiber für MS-Windows
*/

// Implementation

#include "precomp.h"
#include "../kram/sysdep.h"

#if defined SYSTEM_WIN  &&  defined SYSTEM_STARVIEW

#define __MINMAX_DEFINED
#include <stdlib.h>


#if defined( SYSTEM_WIN )
#   include <svwin.h>
#   define _INC_WINDOWS
#   include <sysdep.hxx>
#endif

#include <sv.hxx>

#if defined SYSTEM_WIN || defined SYSTEM_DOS
#   define IOS_BINARY ios::binary
#else
#   define IOS_BINARY 0
#endif


#include <sosstrng.h>

#include <borstrng.h>
#include <sysxcept.h>
#include <ctype.h>
#include <stdio.h>
#include <mem.h>

#include "frmpage.h"

#include <log.h>
#include <xception.h>
#include <charset.h>

#include "file.hrc"

#define FILE_CLOSED

#if defined SYSTEM_SOLARIS
#include <unistd.h>       // getpid()
#endif


#define DEBUG( __text__ ) if ( debug() ) LOG( (__text__) )


namespace sos {

const int max_text_size = 200;

#if defined( SYSTEM_SOLARIS )
void unlink( const char* filename )
{
    char buf[_MAX_PATH+11+1];
    strcpy( buf, "/bin/rm -f " );
    strcat( buf, filename );
    system( buf );
}

void GetTempFileName( const int, const char* ext, const int, char* dest )
{
    strcpy( dest, "/tmp/" );
    strcat( dest, itoa( getpid() ) );
    strcat( dest, "frmpage.tmp." );
    strcat( dest, ext );
}
#endif

// Statischer printer never deleted
static Printer*  _static_printer    = NULL;
static JobSetup* _static_job_setup  = NULL;

// Implementation Frame_esc_parser

int Frame_code::_index ( const char* name )
{
   for ( int i=0; i < char_set_table_count(); i++ )
   {
     if ( strcmp( name, char_set_table[i].frame_name_ptr ) == 0 ) return i;
   }
   return 0;
}

void Frame_code::modify_table ( char* table )
{
    const char* p;
    int i = _index( (code()==fc_ascii) ? "ascii" : "german" );
    p = char_set_table[i].translate_string_ptr;

    while (*p) {
       table[ *p ] = *(p+1);
       p += 2;
    }
}

// ------------------------------------------------------------------- Format_status::Format_status

Format_status::Format_status() :
    _left_margin            (0),
    _diff_left_margin       (0),
    _line_distance          (240),
    _orientation            (ORIENTATION_PORTRAIT),
    _code                   (Frame_code::fc_ascii),
    _position               (0,0),
    _stack_count            (0),
    _size                   (10),
    _scripting              (scripting_normal),
    _sv_stretch             (true)   // Gib Frame keine Chance
{
    _font.ChangeName        ( String("Courier New")     );
    _font.ChangeCharSet     ( CHARSET_ANSI              );
    _font.ChangeWeight      ( WEIGHT_NORMAL             );
    _font.ChangeAlign       ( ALIGN_TOP                 );
    _font.ChangeTransparent ( TRUE                      );
    _font.ChangeSize        ( Size( 0, _size*20 )   ); // MapMode MAP_TWIP
}

// ------------------------------------------------------------------- Format_status::save_cursor

void Format_status::save_cursor()
{
    if ( _stack_count == MAX_STACK_CURSOR ) throw_xc( "CURSOFLOW" );
    _pos_stack[_stack_count++] = position();

 exceptions
};

// ------------------------------------------------------------------- Format_status::restore_cursor

void Format_status::restore_cursor()
{
    if ( _stack_count <= 0 )  throw_xc( "CURSUFLOW" );
    position() = _pos_stack[--_stack_count];

 exceptions
};


// ------------------------------------------------------------------- Format_status::parse_font_name

void Format_status::parse_font_name( const char* fontname )
{
    // Syntax: Font_name/[b|i|u|zzz]
    // bB Bold, iI Italic, uU Underline, <nnn> Größe
    // Parse font_attrs ...

    // !!! Size wird draussen geseztz wg. MapMode

    Bool  _bold       = false; // bold();
    Bool  _italic     = false; // italic();
    Bool  _underline  = false; // underline();
    uint  size        = 10;
    char  _fontname[64];

    strcpy( _fontname, fontname );

    char* p = strchr( _fontname, '/' );
    if ( !p ) strchr( _fontname, ';' ); // alternatives Trennsymbol
    if ( p )
    {
        *p = 0; p++;
        while (*p != 0)
        {
           if ( (*p) == 'b' || (*p) == 'B' )
           {
              _bold = true; p++;
           }
           else if ( (*p) == 'i' || (*p) == 'I' )
           {
              _italic = true; p++;
           }
           else if ( (*p) == 'u' || (*p) == 'U' )
           {
              _underline = true; p++;
           }
           else if ( isdigit(*p) )
           {
              size = atoi(p);
              while ( *p != 0 && isdigit(*p) ) p++;
           } else
           {
              p++; // ignorieren
           }
        }
    }

    // Underlines ersetzen
    for ( p = strchr( _fontname, '_' ); p; p = strchr( p+1, '_' ) )
    {
        *p = ' ';
    }

    // Font Attributes setzen

    _size = size;  // ??? Größe abchecken ( 2 <= _size <= 96 )
    _font.ChangeSize(       Size(0, size*20) ); // MapMode == MAP_TWIP
    _font.ChangeName(       _fontname );
    _font.ChangeWeight (    _bold ? WEIGHT_BOLD : WEIGHT_NORMAL         );
    _font.ChangeItalic (    _italic                                     );
    _font.ChangeUnderline(  _underline?UNDERLINE_SINGLE:UNDERLINE_NONE  );

    LOG( "parse_font_name: font=" << _fontname << ", size=" << _size << ", bold=" << (_bold?"1":"0")
        << ", italic=" << (_italic?"1":"0") << ", underline=" << (_underline?"1":"0") << endl );

}


// ------------------------------------------------------------------- Frame_esc_parser::Frame_esc_parser

Frame_esc_parser::Frame_esc_parser( OutputDevice* output_device, Format_status& status ) :
    _output_device  (output_device),
    _status         (status),
    _debug          (false),
    _page_error     (false)
{
    if ( _output_device == NULL ) {
        _output_device = new WorkWindow( NULL, WB_HIDE );
            if ( !_output_device ) throw_no_memory_error();
    }
    _init_device();

    // Tabele initialisieren
    for ( int i=0; i < 256; i++ ) _translate_table[i] = i;
    _status.code().modify_table( _translate_table );
}

void Frame_esc_parser::_init_device()
{
    MapMode aMapMode( MAP_TWIP );
    _output_device->ChangeMapMode( aMapMode );
    _output_device->ChangeFont( _status.font() );  // Font ist immer im MAP_TWIP
    Font f = _status.font();
    f.ChangeSize( Size(0,_status.size() ) );
    _status.window_char_sizes( _status._fontwidth_array, f  );
    _output_device->ChangePen( Pen( Color( COL_BLACK ) ) );
}




void Frame_esc_parser::output_device( OutputDevice* out )
{
    _output_device = out;
    _init_device();
}

// Wiederholungssequenz ESC(RPc[x]*) ESC(RP.47)

void Frame_esc_parser::gutter( int g )
{
    if ( g < 0 ) return; // raise(???)
    //uint _char_distance = status()._fontwidth_array[ebc2iso['m']];
    //uint _char_distance = status().char_distance();


    //status().left_margin()  = g * _char_distance + status().diff_left_margin();
    //status().position().X() = status().left_margin();
    int gut = g;
    char buffer[100+1];
    memset( buffer, ' ', 100 );
    buffer[sizeof buffer] = 0;

    while ( gut > 0 )
    {
        draw_text( buffer, min( (uint) gut, sizeof buffer ) );
        gut -= sizeof buffer;
    }

    status().left_margin() = status().position().X();

    //DEBUG( "Gutter gesetzt: gu=" << g << " (" << status().left_margin() << " twips)" << endl );
    if ( debug() ) {
        Pen aTBPen( Color( COL_LIGHTGRAY ), 1, PEN_DOT );
        Pen oldPen = _output_device->ChangePen( aTBPen );

        _output_device->DrawLine( Point( status().diff_left_margin(), status().position().Y() ),
                                  status().position() );
        _output_device->ChangePen( oldPen );

    }
}

void Frame_esc_parser::horizontal_move( uint hm )
{
  Point old_pos( status().position() );
  status().position().X() += hm; // relative Position
  if ( debug() ) {
    Pen aHMPen( Color( COL_LIGHTBLUE ), 1, PEN_DOT );
    Pen oldPen = _output_device->ChangePen( aHMPen );

    _output_device->DrawLine( old_pos, status().position() );

    _output_device->ChangePen( oldPen );

  }
}

void Frame_esc_parser::tabulate( uint tb)
{
  Point old_pos( status().position() );
  status().position().X()  =  tb + status().diff_left_margin() /* + _left_margin */;  // absolute Position
  if ( debug() ) {
    Pen aTBPen( Color( COL_LIGHTGREEN ), 1, PEN_DOT );
    Pen oldPen = _output_device->ChangePen( aTBPen );

    _output_device->DrawLine( old_pos, status().position() );

    _output_device->ChangePen( oldPen );

  }
}

#define FRAME2TWIP( f ) (((long)f*1440L)/300L)


// ------------------------------------------------------------ Frame_esc_parser::brace_sequence

/*

    Unterstützte Escape-Sequenzen           Stand: 10.2.95
    ======================================================

    0. !R!* | PSTMS*
        werden kommentarlos ignoriert (Kyocera-Müll)

    1. CHGERMAN | CHASCII
        Frame-Zeichensatzumschaltung (§ vs. @)

    2. SC | RC
        Save/Restore Cursor
        Bemerkung: Cursor-Stack (max. 20) wird bei neuer Seite zurückgesetzt.

    3. TB | HM
        Cursor-X absolut/relativ setzen (in 1/300 Zoll)

    4. FT<font>[/[b|i|u|<size>]*]
        Fontauswahl: b=fett, i=kursiv, u=unterstrichen
                     size in Punkt (10pt default)
                     font muß ein Windowsfont sein (falsche Fonts werden noch
                     nicht abgefangen)

        Beispiel: FTArial/bi018 (Arial 18pt,fett und kursiv)

    5. BOX<x>,<y>
        Rechteck zeichnen ( links oben=Cursorposition,
                            x,y = Position Ecke links unten (in 1/300 Zoll) )

    6. FO<bitmap> | FX<bitmap>
        Draw Bitmap: absolut (=linke obere Ecke), relativ (=Cursorposition)
                     bitmap == DOS-Dateiname (BMP-Format)

        Beispiel: FOs:/windows/auto.bmp

    7. POSX[+|-]<x> | POSY[+|-]<y>
        Cursor X/Y-Position setzen (absolut/relativ) (in 1/300 Zoll)

        Beispiel: POSX600  (= Cursor absolut setzen)
                  POSX+200 (= Cursor relativ setzen)

    8. LANDSCAPE | PORTRAIT
        Papierorientierung ändern (wird zur Zeit noch nicht unterstützt)

    9. SVSTRETCH
        Schaltet StarView-Modus an/aus (Zeichenzwischenraumverteilungsautomatik)

    10. Diverse: CI, LI, GU ...
*/

void Frame_esc_parser::brace_sequence( const char* sequence, int len )
{
   if ( len == 0 ) return;

   if ( memcmp( sequence, "!R!", 3 ) == 0 ||
        memcmp( sequence, "PSTMS", 5 ) == 0 ) {
     return; // Kyocera-Sequenzen ignorieren!
   } else if ( memcmp( sequence, "HM", 2 ) == 0 ) {
     if( len < 3 )  throw_xc( "UNEXPTEOF", "HM" );
     int m = atoi( &sequence[2] );
     horizontal_move( FRAME2TWIP( m ) );
   } else if ( memcmp( sequence, "TB", 2 ) == 0 ) {
     if( len < 3 )  {
        throw_xc( "UNEXPTEOF", "TB" );
     }
     int m = atoi( &sequence[2] );
     tabulate( FRAME2TWIP( m ) );
   } else if ( memcmp( sequence, "LANDSCAPE", 9 ) == 0 )
   {
     status().orientation( ORIENTATION_LANDSCAPE );
   } else if ( memcmp( sequence, "PORTRAIT", 8 ) == 0 ) {
     status().orientation( ORIENTATION_PORTRAIT );
   } else if ( memcmp( sequence, "SC", 2 ) == 0 ) {
     status().save_cursor(); xc;
   } else if ( memcmp( sequence, "RC", 2 ) == 0 ) {
     status().restore_cursor(); xc;
   }
   else if ( memcmp( sequence, "FO", 2 ) == 0 ) {
#  if !defined _RTLDLL     // Modul sv.bmbsav liefert Bindefehler. jz 19.2.95
     // mit/ohne Bitmap ; Formularverzeichnis
     if( len < 3 )  throw_xc( "UNEXPTEOF", "FO" );
     Bitmap b;
     ifstream f( &sequence[2] );
     f >> b;
     // (0,0) ???
     _output_device->DrawBitmap( Point(0,0), b );

     LOG( "FO-Kommando" << endl );
#  endif
   } else if ( memcmp( sequence, "FX", 2 ) == 0 ) {
#  if !defined _RTLDLL     // Modul sv.bmbsav liefert Bindefehler. jz 19.2.95
     if( len < 3 )  throw_xc( "UNEXPTEOF", "FX" );
     Bitmap b;
     ifstream f( &sequence[2] );
     f >> b;
     // (0,0) ???
     _output_device->DrawBitmap( status().position(), b );

     LOG( "FX-Kommando" << endl );
#  endif
   } else if ( memcmp( sequence, "CHGERMAN", 8 ) == 0 ) {
     status().code( Frame_code::fc_german );
     status().code().modify_table( _translate_table );
   } else if ( memcmp( sequence, "CHASCII", 7 ) == 0 ) {
     status().code( Frame_code::fc_ascii );
     status().code().modify_table( _translate_table );
   } else if ( memcmp( sequence, "FT", 2 ) == 0 ) {
     if( len < 3 )  throw_xc( "UNEXPTEOF", "FT" );
     status().font().ChangeSize( Size(0, 10) ); // default: 10pt, oder letzte Größe ???
     status().parse_font_name( &sequence[2] ); // Bold, Italic, Underline && size kann ebenfalls gesetzt werden
     _init_device(); // neuen Font propagieren ...
   } else if ( memcmp( sequence, "POSX", 4 ) == 0 ) { // horizontal
     if( len < 5 ) throw_xc( "UNEXPTEOF", "POSX" );
     const char* start = &sequence[4];
     if ( *start == '+' || *start == '-' )
     { // relativ
        int sign = ( *start=='-' ? -1 : 1 );
        start++;
        int m = atoi( start );
        status().position().X() += sign * FRAME2TWIP( m ); // relative Position
     } else { // absolut
        int m = atoi( start );
        status().position().X() = FRAME2TWIP( m ); // absolut
     }
   } else if ( memcmp( sequence, "POSY", 4 ) == 0 ) { // vertikal Move
     if( len < 5 ) throw_xc( "UNEXPTEOF", "POSY" );
     const char* start = &sequence[4];
     if ( *start == '+' || *start == '-' )
     { // relativ
        int sign = ( *start=='-' ? -1 : 1 );
        start++;
        int m = atoi( start );
        status().position().Y() += sign * FRAME2TWIP( m ); // relative Position
     } else { // absolut
        int m = atoi( start );
        status().position().Y() = FRAME2TWIP( m ); // absolut
     }
   } else if ( memcmp( sequence, "BOX", 3 ) == 0 ) {
     if( len < 4 ) throw_xc( "UNEXPTEOF", "BOX" );
     // (BOX x1, y1, x2, y2)
     //int xy_pos[4];
     const char* startx = &sequence[3];
     const char* starty;
     const char* p;
     p = strchr( startx, ',' );
     if ( p == 0  ) throw_xc( "UNEXPTEOF", "BOX2" );
     starty = p+1;
     int x = FRAME2TWIP( atoi(startx) );
     int y = FRAME2TWIP( atoi(starty) );                                    // runde Ecken?
     _output_device->DrawRect( Rectangle( status().position(), Size(x,y) ) /*, 250, 250 */);
   } else if ( memcmp( sequence, "LINE", 4 ) == 0 ) {  // 0,01 cm
     // (LINE x1,y1,x2,y2) // Strichstärke ???
   } else if ( memcmp( sequence, "POINT", 5 ) == 0 ) { // 0,01 cm
     // (POINT x,y ) // Punktstärke ???
   } else if ( memcmp( sequence, "SVSTRETCH", 9 ) == 0 ) {
     status().sv_stretch(!status().sv_stretch()); // Flag togglen
   } else if ( memcmp( sequence, "ENDESEITE", 9 ) == 0 ) {
     // ignorieren
   } else {
     // ignorieren
     LOG( "Frame_esc_parser:: unknown brace sequence: " << sequence << endl );
   }

exceptions
}

void Format_status::window_char_sizes( int* size_buffer, const Font& font )
{
    MapMode map_frame( MAP_1000TH_INCH, Point(0,0), Fraction(10,3), Fraction(10,3) );
    Window window( pApp->GetAppWindow(), WB_HIDE );

    for( int j = 0; j < 255; j++ )  size_buffer[ j ] = 0;
    window.ChangeFont( font );

    int i;
    WORD wFirstChar, wLastChar;
    HWND hwnd = Sysdepen::GethWnd( window );
    HDC hdc = Sysdepen::GethDC( window );

    try
    {
        wFirstChar = (WORD) 0;
        wLastChar  = (WORD) 255;

        if (GetCharWidth(hdc, wFirstChar, wLastChar, (int FAR*) size_buffer))
        {
        } else  throw_xc( "WFONT" );

        ReleaseDC(hwnd, hdc);

        {  // Frame MapMode: 1/300 Zoll
            window.ChangeMapMode( map_frame );

            int prozent = 30; // aus Profile lesen?
            if ( font.IsItalic() ) prozent = 20; // Zur Probe

            // Abstand zwischen zwei Zeichen: 30% von 'n'
            int distance = window.PixelToLogic( Size( size_buffer[ 'n' ], 0 ) ).Width() * prozent/100;

            // Char Widths ins FrameMapMode umrechnen + Einbeziehung Zeichenabstand
            for ( i=0; i < 256; i++ )
            {
                size_buffer[i] = distance + window.PixelToLogic( Size(size_buffer[i],0) ).Width();
            }

        }
        // ??? window.Close();
    }
    catch(...) {
        ReleaseDC(hwnd, hdc);
        // ??? window.Close();
        throw;
    }
}


#define MAX_BRACE_SEQ_LEN 132


void Frame_esc_parser::parse( istream* s, long* len_ptr )
{
    // len_ptr muss nich gleich Null sein
    // Druckt eine Seite
    char c = s->get();
    if( c == EOF || s->bad() )  return;
    *len_ptr = *len_ptr + 1;

    while( c!=EOF )  {
        if( c == '\x1B' ) {
            char seq[3];
            s->get( seq, 3 );
            if( s->bad() || s->gcount() != 2 )  {
                LOG( "s->bad()=" << s->bad() << ", s->gcount()=" << s->gcount() << " => throw_xc( \"UNEXPTEOF\", \"<ESC>xx\" )\n" );
                throw_xc( "UNEXPTEOF", "<ESC>xx" );
            }
            *len_ptr = *len_ptr + 2;
            if( memcmp( seq, "FF", 2 ) == 0 )
            {
                end_page(); return;
            } else if( memcmp( seq, "GU", 2 ) == 0 ) {    // Left Margin
                char g[ 4 ];
                s->get( g, 4 );
                if( s->bad() || s->gcount() != 3 ) throw_xc( "UNEXPTEOF", "GU" );
                *len_ptr = *len_ptr + 3;
                g[3] = 0;
                int m = atoi( g ); // die naechsten drei Zeichen
                gutter( m );
            } else if( memcmp( seq, "NL", 2 ) == 0 )
            {
                newline();
            } else if( seq[0] == 'U' ||
                       seq[0] == 'M' ||
                       seq[0] == 'I'    )
            { // Font Attribute
              font_attr( seq[0], seq[1] == '1' ? true : false );
            } else if( seq[0] == '(' )
            {
                char brace_seq_buf[MAX_BRACE_SEQ_LEN+1];
                memset( brace_seq_buf, 0, MAX_BRACE_SEQ_LEN+1 );
                s->get( brace_seq_buf+1, MAX_BRACE_SEQ_LEN+1, ')' );
                if ( s->bad() || s->gcount() < 0 ) throw_xc( "UNEXPTEOF", "<BRACE>" );
                brace_seq_buf[0] = seq[1];
                int brace_len = s->gcount() + 1;
                *len_ptr = *len_ptr + s->gcount();
                s->get(); *len_ptr = *len_ptr + 1; // ')' wegschmeissen
                brace_sequence( brace_seq_buf, brace_len ); xc; // ignore_exception() ?
            } else if( memcmp( seq, "CI", 2 ) == 0 )
            {    // Zeichenabstand
                char ci[ 4 ];
                s->get( ci, 4 );
                if( s->bad() || s->gcount() != 3 ) throw_xc( "UNEXPTEOF", "CI" );
                *len_ptr = *len_ptr + 3;
                ci[3] = 0;
                int l = atoi( ci ); // die naechsten drei Zeichen
                char_indent( l );
            } else if( memcmp( seq, "LI", 2 ) == 0 )
            {    // Zeilenabstand
                char li[ 4 ];
                s->get( li, 4 );
                if( s->bad() || s->gcount() != 3 ) throw_xc( "UNEXPTEOF", "LI" );
                *len_ptr = *len_ptr + 3;
                li[3] = 0;
                int l = atoi( li ); // die naechsten drei Zeichen
                line_indent( l );
            } else if( seq[0] == 'E' || seq[0] == 'S' )
            {
              // E1/E0/S1/S0-Sequenz !!! Flag setzen oder gleich bearbeiten ???
              scripting( seq[0], seq[1] == '1' ? true : false );
            } else {
              LOG( "unknown ESC-Sequence: " << seq << endl );
            };
            // naechstes Zeichen holen
            c = s->get();
            if( c == EOF || s->bad() )  return;
            *len_ptr = *len_ptr + 1;

        } else { // c != '\x1B'
            char buffer[ max_text_size ];
            int buflen = 0;

            while ( c != '\x1B' ) {
               if ( buflen == sizeof buffer - 1 ) {
                 // erssma Buffer leeren
                 draw_text( buffer, buflen );
                 buflen = 0;
               }
               if ( c != '\n' && c != '\r' && c != '\t' ) // ignorieren
               {
                 buffer[buflen++] = c;
               }
               c = s->get();
               if( c == EOF || s->bad() ) break;
               *len_ptr = *len_ptr + 1;
            }

            if ( buflen > 0 ) {
              draw_text( buffer, buflen );
            }
            if ( c == EOF || s->bad() ) return; // Ende der Vorstellung
        }
    }
}

void Frame_esc_parser::draw_text( char* buffer, int buflen )
{
    String str;
    short kern_array[ max_text_size ];
    short frame_len = 0;

    int  font_size  = status().size();
    int  y_diff     = 0;

    xlat( (void*) buffer, buflen, (const Byte*) _translate_table );

    if ( status().scripting() != scripting_normal )
    {
        if ( buflen == 1 && buffer[0] == '2' && status().scripting() == scripting_super )
        {
          buffer[0] = '²';
        }
        else if ( buflen == 1 && buffer[0] == '3' && status().scripting() == scripting_super )
        {
          buffer[0] = '³';
        } else
        {
           Font new_font = status().font();
           int new_size = (2 * font_size) / 3;    // übern Daumen gepeilt
           if ( status().scripting() == scripting_super )
           {
                y_diff = -(20 * font_size) / 5 ;  // übern Daumen gepeilt
           } else                    // scripting_sub
           {
                y_diff =  (20 * font_size) / 2;   // übern Daumen gepeilt
           }
           status().position().Y() += y_diff;
           status().size( new_size );
           status().font().ChangeSize( Size( 0, new_size*20 ) );
           _init_device();
        }
    }

    for( int i = 0; i < buflen; i++ ) {
        kern_array[ i ] = frame_len;
        frame_len += 1440L*(long)(status()._fontwidth_array[ (uchar)buffer[ i ] ])/300L;
    }

    str = as_string( buffer, buflen );

    if ( status().sv_stretch() )
    {
#if 0   // Fehler SV - Doku
        short _short_widths[256];
        int factor = status().font().IsItalic() ? 5 : 4;
        short _distance = status()._fontwidth_array['n']/factor; // == 30% bzw. 20% bei Italic
        for( int j = 0; j < 256; j++ ) { // Zeichenabstände wieder abziehen fuer SV
        {
            _short_widths[ j ] = (short)( 1440 * ( status()._fontwidth_array[ j ] - _distance ) / 300 );
        }
#endif
        _output_device->DrawStretchText( status().position(), frame_len, str, 0, buflen);
    } else {
        _output_device->DrawKernText( status().position(), str, 0, buflen, kern_array+1 );
    }

    status().position().X() += frame_len;
    if ( debug() )
    {
        int s =_output_device->GetTextSize( str ).Width();
        LOG( "Frame_esc_parser::draw_text: Size(SV)=" << s << ", Size(Frame)=" << frame_len << endl );
    }

    if ( status().scripting() != scripting_normal && y_diff != 0 )
    {
        status().size( font_size );                         // alte Font-Size wiederherstellen ...
        status().font().ChangeSize( Size( 0, font_size*20 ) );
        status().position().Y() -= y_diff;
        _init_device();
    }
}


// ----------------------------------------------------------------------------Page_list::Page_list()

Page_list::Page_list()
{
    _list.obj_const_name( "Page_list::_list" );
    _list.first_index(1);
}

void Page_list::insert( long offs )
{
    LOG( "Page_list::insert : offset=" << offs << endl );
    if ( offs == 0 ) return; // 1. Seite ignorieren
    _list.add( offs );
}

int Page_list::count()
{
    return (_list.last_index() - _list.first_index() + 1);
}

long Page_list::start_offset( uint page_no )
{
    if ( page_no > count() ) throw_xc( "OUTOFRANGE" );
    if ( page_no == 1 ) return 0; // Seite 1 hat immer Offset 0!

    //DEBUG( "offset(" << page_no << ")=" << _list[page_no-1] << endl );
    return _list[page_no-1];
}

long Page_list::ende_offset( uint page_no )
{
    if ( page_no > count() ) throw_xc( "OUTOFRANGE" );
    //DEBUG( "offset(" << page_no << ")=" << _list[page_no] << endl );
    return _list[page_no];
}


long Page_list::bytes_count( uint page_no )
{
    long erg = ende_offset( page_no ) - start_offset( page_no );
    //DEBUG( "bytes_count(" << page_no << ")=" << erg << endl );
    return erg;
};


//----------------------------------------------------Frame_esc_print::Frame_esc_print

Frame_esc_print::Frame_esc_print( Printer* outdev, Format_status& stat ) :
    Frame_esc_parser( outdev, stat ),
    _new_page(true)
{
};

//----------------------------------------------------Frame_esc_printer::open

void Frame_esc_print::open()
{
    _new_page = true;
    ((Printer*)_output_device)->SetPageQueueSize(0); // sofort auf Systemdrucker!
    if ( !((Printer*)_output_device)->StartJob( "Frame Druck" ) ) throw_xc( "STARTJOB" );
    if ( pApp->GetAppWindow() ) pApp->GetAppWindow()->Disable();
    pApp->Wait( TRUE );

exceptions
};



//----------------------------------------------------Frame_esc_print::put

void Frame_esc_print::put( const Const_area& area )
{
try {
    long len = (long) area.length();

    istrstream s( (char*) area.char_ptr(), len );
    long tmp;
    long printed_len = 0;
    while ( printed_len < len )
    {
        tmp = 0;
        parse( &s, &tmp ); xc;
        printed_len += tmp;
    }
}
catch(...) {
    if ( pApp->GetAppWindow() ) pApp->GetAppWindow()->Enable();
    pApp->Wait( FALSE );
    throw;
}
}

//----------------------------------------------------Frame_esc_printer::close

void Frame_esc_print::close()
{
    if ( !_new_page ) {
        ((Printer*)_output_device)->EndPage();
    }
    ((Printer*)_output_device)->EndJob();

    if ( pApp->GetAppWindow() ) pApp->GetAppWindow()->Enable();
    if ( pApp->GetAppWindow() ) pApp->GetAppWindow()->ToTop();
    pApp->Wait( FALSE );
};


//----------------------------------------------------Frame_esc_print::end_page

void Frame_esc_print::end_page()
{
    ((Printer*)_output_device)->EndPage();
    Frame_esc_parser::end_page();
}


//----------------------------------------------------Frame_esc_print::draw_text

void Frame_esc_print::draw_text( char* text, int len )
{
    if ( len > 0 && _new_page ) {
       ((Printer*)_output_device)->StartPage();
       _new_page = false;
    };
    Frame_esc_parser::draw_text( text, len );
}



//----------------------------------------------------Frame_page_list::Frame_page_list

Frame_page_list::Frame_page_list( const char* filename ) :
    Frame_esc_parser( NULL, Format_status() ),
    _filename(filename),
    _file(filename,ios::in|IOS_BINARY),
    _new_page(false),
    _pass1(true),
    _opened(false),
    _parse_len(0)
{

    if ( _file.bad() )  throw_xc( "FILEBAD" );
  {
    String str;

    // nach ESC-F[F|1|2|...] suchen !!!

/*
#if defined( STRING_TOOLS_HPP )
    RWCRegexp to_find("\x1B" "FF");
#else
    Sos_string to_find("\x1B" "FF");
#endif
*/
    String to_find("\x1B" "FF");

    size_t offs = 0;
    Dynamic_area area;

    area.allocate( 32000 );
    while (1) {
        _file.read( area.char_ptr(), area.size() );
        if( _file.gcount() == 0 || _file.bad() )  break;
        area.length( _file.gcount() );
        //str = as_string( area );
        str = String( area.char_ptr(), 0, area.length() );

        size_t pos = 0;
/*
#if defined( STRING_TOOLS_HPP )
        while( ( pos = str.index( to_find, NULL,  pos ) ) != RW_NPOS )
#else
        while ( ( pos = str.find( to_find, pos ) ) != NPOS )
#endif
*/
        while ( ( pos = str.Search( to_find, pos ) ) != STRING_NOTFOUND )
        {
          _page_list.insert( offs+pos );
          pos += 3; // Position erhoehen
        }
        offs += area.length(); //!!! Achtung, falls to_find getrennt wurde!
        str = "";
    };
    _page_list.insert( offs ); // Der letzte Pseudo-Offset == FileGroesse
#if defined(FILE_CLOSED)
    _file.close();
#else
    _file.seekg(0); // an den Anfang positionieren
#endif
   }

}


Frame_page_list::Frame_page_list() :
    Frame_esc_parser( NULL, Format_status() ),
    _filename(""),
    _new_page(true),
    _opened(false),
    _pass1(true),
    _parse_len(0)
{

}
//----------------------------------------------------Frame_page_list::~Frame_page_list

Frame_page_list::~Frame_page_list()
{
    unlink( _filename );
}

//----------------------------------------------------Frame_page_list::open

void Frame_page_list::open()
{
    char buf[144];
    GetTempFileName( 0, "frm", 0, buf );
    LOG( "Frame_page_list::open: temp_filename=" << buf << "\n" );
    _filename = buf;
    _file.open(_filename,ios::out|IOS_BINARY);
    if ( _file.bad() )   throw_xc( "FILEBAD" );
    _offset = 0;
    _new_page = true;
    _opened = true;
}

//----------------------------------------------------Frame_page_list::put

void Frame_page_list::put( const Const_area& area )
{
    //if ( !_opened ) raise( "NOTOPENED", "D???" );
//static long sum_length = 0;
  {
    unsigned int len = area.length();
    istrstream s( (char*) area.char_ptr(), len );
    long read_len = 0;

//DEBUG( "put(" << _offset << "," << area.length() << "): " );
//if ( debug() && log_ptr ) log_ptr->write( area.char_ptr(), area.length() );
//DEBUG( endl );

// js 28/01/97   while ( _offset < old + len ) // len=area.length(), kann
    LOG( "Frame_page_list::put: vorher s.tellg()=" << s.tellg() << "\n" );
    while ( read_len < len )
    {
        long sum_len = read_len;
        _parse_len = 0;
        _extra_bytes = 0;

        parse( &s, &_parse_len );
        read_len += _parse_len;
        _offset += _extra_bytes; // Falls die draw_text-Methode OHNE voriges <ESC>-FF aufgerufen wurde

        //_offset += parse_len;
// js 28/01/97     if (  parse_len + temp < old + len // nicht alles gelesen
        if (  read_len < len // nicht alles gelesen
        /*||
             (_offset == old + len && len >= 3 &&
              strncmpi( "\x1B" "FF", area.char_ptr()+len-3, 3 ) == 0 )*/
           )
        { // neue Seite wurde angefangen
    //DEBUG( "put(" << _offset << "," << old << "," << area.length() << ")" << endl );
            if ( _parse_len-3 > 0 )
            {
                //_file.write( area.char_ptr()+temp-old, _offset-temp-3 );
                _file.write( area.char_ptr()+sum_len, _parse_len-3 );  _offset += _parse_len - 3;
            } else { /* keine Ahnung */ }
                // end_page()
                //_page_list.insert( _offset-3 ); // <ESC>-FF wurde gelesen
            _file.write( "\x1B" "FF", 3 );  _offset += 3;   // js 28/01/97 ??? // <ESC>-FF schreiben
            _offset += write_status();                 // Seitenheader schreiben
            LOG( "Frame_page_list::put/1: _offset=" << _offset << ", _file.tellp()=" << _file.tellp() << "\n" );

        } else
        {
            _file.write( area.char_ptr()+sum_len, _parse_len ); _offset += _parse_len;
            LOG( "Frame_page_list::put/2: _offset=" << _offset << ", _file.tellp()=" << _file.tellp() << "\n" );
            if ( len >= 3 && memcmp( "\x1B" "FF", area.char_ptr()+len-3, 3 ) == 0 )
            {
                _offset += write_status();              // Seitenheader schreiben
            }
        }
    }
    // direkt rausschreiben
    //_file.write( area.char_ptr(), area.length() );
#if defined SYSTEM_WIN
    _file << "\r\n"; _offset += 2;        // Nur damit mans besser lesen kann
#else
    _file << "\n";   _offset += 1;
#endif
//    sum_length += area.length() + 2;
//DEBUG( "Frame_page_list::put : _offset=" << _offset << ",  sum_length=" << sum_length <<
//     ", diff=" << _offset - sum_length << endl );

   }
}

//----------------------------------------------------Frame_page_list::close

void Frame_page_list::close()
{
   if ( !_opened ) return;

   if ( !_new_page ) {
      LOG( "Frame_page_list::close: _page_list.insert(" << _offset << " vs. _file.tellp()=" << _file.tellp() << "\n" ); 
      _page_list.insert( _offset );
   }
   _file.close();
   _opened = false;
   _pass1 = false;
}


//----------------------------------------------------Frame_page_list::end_page

void Frame_page_list::end_page()
{
    if ( _opened && _pass1 ) {
        _new_page = false;
        LOG( "_page_list.insert(" << _offset << "+" << _parse_len << "+" << _extra_bytes <<"-3=" << (_offset+_parse_len+_extra_bytes-3) << "), _file.tellp()=" << _file.tellp() << "\n" );
        _page_list.insert( _offset+_parse_len+_extra_bytes-3 ); // <ESC>-FF wurde gelesen

    }
    Frame_esc_parser::end_page(); // Position == (0,0)
}


//----------------------------------------------------Frame_page_list::_init_device

void Frame_page_list::_init_device( )
{
    if ( !_pass1 ) Frame_esc_parser::_init_device();
}

//----------------------------------------------------Frame_page_list::gutter

void Frame_page_list::gutter( int g )
{
    if ( !_pass1 ) Frame_esc_parser::gutter( g );
}


//----------------------------------------------------Frame_page_list::draw_text

void Frame_page_list::draw_text( char* text, int len )
{
    if ( _opened && len > 0 && _new_page == true ) {
        // Status-Informationen der letzten (?) Seite schreiben
        _new_page = false;
        _file.write( "\x1B" "FF", 3 );  // <ESC>-FF schreiben
        _page_list.insert( 0 );  // neue (erste) Seite begonnen ohne <ESC>-FF

        _extra_bytes += 3; // js 28/01/98
        _extra_bytes += write_status();
    } else if ( !_pass1 )
    {
        Frame_esc_parser::draw_text( text, len );
    }
}


//----------------------------------------------------Frame_page_list::write_status

long Frame_page_list::write_status()
{
    if ( !_opened && !_pass1 ) throw_xc( "Frame_page_list::write_status" );
    // Format: <ESC>(FT<font>)<ESC>Mb<ESC>Ub<ESC>Ib<ESC>CIxxx<ESC>LIxxx<ESC><code>
    //char buf[256];
    Dynamic_area area( 512 );
    Font& f = status().font();

    sprintf( area.char_ptr(), "\r\n" "\x1B" "(FT%s/%03d)" "\x1B" "M%c" "\x1B" "U%c" "\x1B" "I%c"
                  "\x1B" "LI%03d" "\x1B" "(%s)",
             (const char*) f.GetName(), status().size(),
             (( f.GetWeight()      == WEIGHT_BOLD      ) ? '1' : '0' ),
             (( f.GetUnderline()   == UNDERLINE_SINGLE ) ? '1' : '0' ),
             (  f.IsItalic()                             ? '1' : '0' ),
             14400 / status().line_distance(),
             (( status().code().code() == Frame_code::fc_ascii ) ? "CHASCII" : "CHGERMAN")
           );

    status().reset_stack();

    uint len = strlen(area.char_ptr());
    area.length( len );
    LOG( "Frame_page_list::write_status(len=" << len << "): " << area << "\n" );
    _file.write( area.char_ptr(), area.length() );
        // !!! Fehler abfragen !!!
    return len;
}


//----------------------------------------------------Frame_page_list::print_page

void Frame_page_list::print_page( Printer* pOutDev, uint page_no ) {

    if ( !debug() ) {
        status().diff_left_margin( - pOutDev->GetPageOffset().X() ); // HACK!!!
    } else {
        status().diff_left_margin(0);
    }
    //DEBUG( "_diff_left_margin=" << status().diff_left_margin() << endl );

    pOutDev->SetOrientation( status().orientation() );
    pOutDev->SetQuality    ( QUALITY_PROOF );            // Beste Qualität

    //DEBUG( "print_page: page_no=" << page_no << endl );

    if ( page_no > 0 && page_no <= page_count() )
    {

#define OLD_STYLE
#if defined(OLD_STYLE)
       Dynamic_area area;
       get_page( page_no, area ); //!!! Fehlerabfrage
       istrstream s( area.char_ptr(), area.length() );
       output_device( pOutDev );
       long len = 0;

if ( debug() ) {
     // Grid Zeichnen alle 144 twips
     Pen aGridPen( Color( COL_RED ), 1, PEN_DOT );
     Pen oldPen = pOutDev->ChangePen( aGridPen );
     Size aSize = pOutDev->PixelToLogic( pOutDev->Printer::GetPaperSizePixel() );

     //DEBUG( "Printer::OutputSize width=" << aSize.Width() << ", height=" << aSize.Height() << endl );

     if ( aSize.Width() < 0 ) aSize.Width() *= -1; // GetOutputSize liefert negative Werte

/*
     for ( int i=144; i < aSize.Width(); i+=144 )
       pOutDev->Printer::DrawLine( Point( i, 0 ), Point( i, aSize.Height() ) );

     for (     i=status().line_distance(); i < aSize.Height(); i+=status().line_distance() )
       pOutDev->Printer::DrawLine( Point( 0, i ), Point( aSize.Width(), i ) );
*/
        pOutDev->DrawRect( Rectangle( Point( 0+status().diff_left_margin(), 0 ),
                                   Point( aSize.Width()+status().diff_left_margin()-1, aSize.Height()-1 ) ) );
        pOutDev->ChangePen( oldPen );

}

       while ( len < area.length() ) {
          try {
             parse( &s, &len ); //ignore_exception();
          } catch( const Xc& x ) {
            SHOW_ERR( "Fehler beim Lesen der Seite " << page_no << ": " << x );
            LOG( "Frame_page_list::print_page (FEHLER): len=" << len << ", area='" << area << "'\n" ); 
            //_page_error = true;
            //status().font().ChangeColor( Color( COL_RED ) );
            //pOutDev->ChangeFont( status().font() );
            break; // Verarbeitung wird eingestellt
          }
       }
//DEBUG( "Frame_page_list::print_page(" << page_no << ") : len_ptr=" << len <<
//     ", area.length()=" << area.length() << endl );
#else
#endif
    }
    //status().font().ChangeColor( Color( COL_BLACK ) ); // Fehler-Farbe wieder zurücksetzen
    //pOutDev->ChangeFont( status().font() );

}

//----------------------------------------------------Frame_page_list::get_page

void Frame_page_list::get_page( uint page_no, Dynamic_area& area )
{
    uint len;
    long offs;

#if defined(FILE_CLOSED)
    _file.open(_filename,ios::in|IOS_BINARY);
    if ( _file.bad() )  throw_xc( "NOTOPENED" );  //raise( "NOTOPENED", "NOTOPENED" );
#endif
    if ( page_no > page_count() || page_no < 1 )  throw_xc( "PAGECOUNT" ); //raise( "PAGECOUNT", "PAGECOUNT" );
    offs = offset(page_no);

    LOG( "Frame_page_list::get_page : offs=" << offs );

    _file.seekg( offs );
    if ( _file.bad() )   throw_xc( "FILEBAD" ); //raise( "FILEBAD", "FILEBAD" );
    len = bytes_count(page_no);
    LOG( ", len=" << len );
    area.allocate_min( len );
    _file.read( area.char_ptr(), len );
    if ( _file.bad() )  throw_xc( "FILEBAD" ); //raise( "FILEBAD", "FILEBAD" );
    if ( _file.gcount() != len )   throw_too_long_error( "D320" ); //raise( "TRUNCATE", "D320" );
    LOG( ", data=" << Const_area( area.char_ptr(), min(len,(uint)30) ) << endl );
    area.length( len );
#if defined(FILE_CLOSED)
    _file.close();
#endif

}



// Frame_preview_box IMPLEMENTATION


// --- Frame_preview_box::Frame_preview_box() -----------------------------

Frame_preview_box::Frame_preview_box( Window* parent, const ResId& rResId,
                                      Frame_page_list* page_list_ptr,
                                      const ResId& hscroll,
                                      const ResId& vscroll ) :
    Control                     ( parent, rResId    ),
    aHScroll                    ( parent, hscroll   ),
    aVScroll                    ( parent, vscroll   ),
    _page_list_ptr              ( page_list_ptr     ),
    _zoom                       ( 1, 1              ), // NormalPage ist Default !
    _preview_ptr                ( NULL              ),
    _second_preview_ptr         ( NULL              ),
    _job_setup_ptr              ( NULL              ),
    _printer_ptr                ( NULL              ),
    _vertical_scrollbar_ptr     ( &aVScroll         ),
    _horizontal_scrollbar_ptr   ( &aHScroll         ),
    _first_page                 ( true              )
{
    _page               = 1;
    _preview_ptr        = new Preview( this, 0 );
            if ( !_preview_ptr )  throw_no_memory_error();
    _max_size           = GetSizePixel();
#if 0
    _printer_ptr        = new Printer;
            if ( !_printer_ptr )  throw_no_memory_error();
#else
    if ( !_static_printer ) {
        _static_printer = new Printer;
            if ( !_static_printer ) throw_no_memory_error();
    }
#endif
    if ( _static_printer->GetName() == "" )   throw_xc("NOPRINTER" ); //raise( "NOPRINTER", "NOPRINTER" );
#if 0
    _job_setup_ptr      = (JobSetup*)new char[_printer_ptr->GetJobSetupSize()];
            if ( !_job_setup_ptr ) throw_no_memory_error();  //raise( "NOMEMORY", "R101" );
#else
    if ( !_static_job_setup ) {
        _static_job_setup  = (JobSetup*)new char[_static_printer->GetJobSetupSize()];
            if ( !_static_job_setup ) throw_no_memory_error();  //raise( "NOMEMORY", "R101" );
    }
#endif
    if ( _static_printer->GetJobSetup( _static_job_setup ) )
    {

    } else throw_xc( "NOSETUP" ); //raise( "NOSETUP", "NOSETUP" );

    _preview_ptr->SetJobSetup( _static_job_setup );

    ((Window*)_preview_ptr)->ChangeBackgroundBrush( Brush( Color( COL_WHITE ) ) );

    _preview_ptr->SetPageQueueSize( 10 ); // dürfte wohl reichen
    change_request_page_hdl( LINK( this, Frame_preview_box, print_page_hdl ) );
    _preview_ptr->ChangeRequestPageHdl(             LINK( this, Frame_preview_box, print_page ) );
    _horizontal_scrollbar_ptr->ChangeEndScrollHdl(  LINK( this, Frame_preview_box, hscroll_end_hdl ) );
    _vertical_scrollbar_ptr->ChangeEndScrollHdl  (  LINK( this, Frame_preview_box, vscroll_end_hdl ) );

    _preview_ptr->ChangeZoomFactor( _zoom );

    _preview_ptr->Show();
    _preview_ptr->ChangeCurPage( 1 );

}

// --- Frame_preview_box::~Frame_preview_box() -----------------------------

Frame_preview_box::~Frame_preview_box()
{
#ifndef RS6000
    //int kein_delete_auf_preview_ptr_und_second_preview_ptr;

#   if 0  // jz 26.11.97 Absturz in der nächsten Zeile:
    if ( _printer_ptr != NULL )  SOS_DELETE( _printer_ptr );
    if ( _preview_ptr != NULL )  SOS_DELETE( _preview_ptr );
    if ( _second_preview_ptr != NULL )  SOS_DELETE( _second_preview_ptr );
    if ( _job_setup_ptr != NULL ) delete [] _job_setup_ptr;
    _job_setup_ptr = NULL;
#   else
    int KEIN_DELETE_PRINTER_AND_JOBSETUP;
    LOG( "***** ~Frame_preview_box: KEIN DELETE Printer und JobSetup (statisch)! *****\n" );
    //if ( _preview_ptr        != NULL ) SOS_DELETE( _preview_ptr );
    //if ( _second_preview_ptr != NULL ) SOS_DELETE( _second_preview_ptr );
#   endif
#endif
}

// --- Frame_preview_box::vertical_scrollbar() ----------------------------
#if 0
void Frame_preview_box::vertical_scrollbar( ScrollBar* scrollbar )
{
   _vertical_scrollbar_ptr = scrollbar;
   _vertical_scrollbar_ptr->ChangeEndScrollHdl( LINK( this, Frame_preview_box, vscroll_end_hdl ) );
   check_scroll_range();
};

// --- Frame_preview_box::horizontal_scrollbar() ----------------------------

void Frame_preview_box::horizontal_scrollbar( ScrollBar* scrollbar )
{
   _horizontal_scrollbar_ptr = scrollbar;
   _horizontal_scrollbar_ptr->ChangeEndScrollHdl( LINK( this, Frame_preview_box, hscroll_end_hdl ) );
   check_scroll_range();
};
#endif

// --- Frame_preview_box::calc_preview_size() ----------------------------

Size Frame_preview_box::calc_preview_size()
{
    Size    aPreviewSize;
    Point   aPreviewPos;
// ??? js ???    Point   aPageOffset;

    aPreviewSize = _preview_ptr->CalcWindowSizePixel( _max_size );

    if ( aPreviewSize.Width() < _max_size.Width() )
        aPreviewPos.X() = (_max_size.Width()>>1) -
                          (aPreviewSize.Width()>>1);
    if ( aPreviewSize.Height() < _max_size.Height() )
        aPreviewPos.Y() = (_max_size.Height()>>1) -
                          (aPreviewSize.Height()>>1);

    _preview_ptr->ChangePosPixel( aPreviewPos );
    _preview_ptr->ChangeSizePixel( aPreviewSize );

    return aPreviewSize;
}

// --- Frame_preview_box::check_scroll_range() ---------------------------

void Frame_preview_box::check_scroll_range()
{
    if ( _horizontal_scrollbar_ptr == NULL || _vertical_scrollbar_ptr == NULL ) {
       return;
    };

    if ( _zoom.GetNumerator() == 0 )
    {
        _horizontal_scrollbar_ptr->Hide();
        _vertical_scrollbar_ptr->Hide();
        _horizontal_scrollbar_ptr->ChangeThumbPos( 0 );
        _vertical_scrollbar_ptr->ChangeThumbPos( 0 );
        _preview_ptr->ChangePageOffset( Point( 0, 0 ) );
        return;
    }

    Size aPaperSize   = _preview_ptr->GetPaperSize();
    Size aViewSize    = _preview_ptr->GetVisibleSize();
    Size aPreviewSize = _preview_ptr->GetSizePixel();

    if ( aPreviewSize.Width() == _max_size.Width() )
    {
        short nThumbPos = _horizontal_scrollbar_ptr->GetThumbPos();

        _horizontal_scrollbar_ptr->ChangeRange( Range( 0, aPaperSize.Width() -
                                        aViewSize.Width() ) );
        _horizontal_scrollbar_ptr->ChangeLineSize( 1440 / 10 );                 // default _char_distance
        _horizontal_scrollbar_ptr->ChangePageSize( aViewSize.Width() / 2  );

        _horizontal_scrollbar_ptr->ChangeThumbPos( nThumbPos );
        _horizontal_scrollbar_ptr->Show();
    }
    else
    {
        _horizontal_scrollbar_ptr->Hide();
        _horizontal_scrollbar_ptr->ChangeThumbPos( 0 );
        Point aOfs = _preview_ptr->GetPageOffset();
        aOfs.X() = 0;
        _preview_ptr->ChangePageOffset( aOfs );
    }

    if ( aPreviewSize.Height() == _max_size.Height() )
    {
        short nThumbPos = _vertical_scrollbar_ptr->GetThumbPos();

        _vertical_scrollbar_ptr->ChangeRange( Range( 0, aPaperSize.Height() -
                                        aViewSize.Height() ) );
        _vertical_scrollbar_ptr->ChangeLineSize( 240 );                             // default _line_distance
        _vertical_scrollbar_ptr->ChangePageSize( aViewSize.Height() - 2 * 240  );   // 1 Seite - 2 Zeilen

        _vertical_scrollbar_ptr->ChangeThumbPos( nThumbPos );
        _vertical_scrollbar_ptr->Show();
    }
    else
    {
        _vertical_scrollbar_ptr->Hide();
        _vertical_scrollbar_ptr->ChangeThumbPos( 0 );
        Point aOfs = _preview_ptr->GetPageOffset();
        aOfs.Y() = 0;
        _preview_ptr->ChangePageOffset( aOfs );
    }

    _preview_ptr->ChangePageOffset( Point( _horizontal_scrollbar_ptr->GetThumbPos(),
                                           _vertical_scrollbar_ptr->GetThumbPos() ) );
}

// --- Frame_preview_box::change_request_page_hdl() -----------------------

Link Frame_preview_box::change_request_page_hdl( const Link& rLink )
{
    Link aOldLink = _request_page_hdl;
    _request_page_hdl = rLink;
    return aOldLink;
}

// --- Frame_preview_box::vscroll_end_hdl() ------------------------------

void Frame_preview_box::vscroll_end_hdl( ScrollBar* )
{
    _offset = Point( _horizontal_scrollbar_ptr->GetThumbPos(),
                     _vertical_scrollbar_ptr->GetThumbPos() );
    _preview_ptr->ChangePageOffset( _offset );
    _preview_ptr->Update();
}

// --- Frame_preview_box::hscroll_end_hdl() ------------------------------

void Frame_preview_box::hscroll_end_hdl( ScrollBar* )
{
    _offset = Point( _horizontal_scrollbar_ptr->GetThumbPos(),
                     _vertical_scrollbar_ptr->GetThumbPos() );
    _preview_ptr->ChangePageOffset( _offset );
    _preview_ptr->Update();
}

// --- Frame_preview_box::zoom_in_hdl() ----------------------------------

void Frame_preview_box::zoom_in_hdl( Button* )
{
    if ( _second_preview_ptr != NULL )
        _second_preview_ptr->Hide();

    if ( _zoom.GetNumerator() == 0 )
        _zoom = Fraction( 1, 4 );
    else
        _zoom *= Fraction( 3, 2 );

    if ( _zoom > Fraction( 16, 1 ) )
        _zoom = Fraction( 16, 1 );

    _preview_ptr->ChangeZoomFactor( _zoom );
    calc_preview_size();
    check_scroll_range();
}

// --- Frame_preview_box::zoom_out_hdl() ---------------------------------

void Frame_preview_box::zoom_out_hdl( Button* )
{
    if ( _second_preview_ptr != NULL )
        _second_preview_ptr->Hide();

    if ( _zoom.GetNumerator() == 0 )
        _zoom = Fraction ( 1, 4 );
    else
        _zoom *= Fraction( 2, 3 );

    if( _zoom < Fraction( 1, 16 ) )
        _zoom = Fraction( 1, 16 );

    _preview_ptr->ChangeZoomFactor( _zoom );
    calc_preview_size();
    check_scroll_range();
}

// --- Frame_preview_box::normal_page_hdl() ------------------------------

void Frame_preview_box::normal_page_hdl( Button* )
{
    if ( _second_preview_ptr != NULL )
        _second_preview_ptr->Hide();

    _zoom = Fraction( 1, 1 );
    _preview_ptr->ChangeZoomFactor( _zoom );
    calc_preview_size();
    check_scroll_range();
}

// --- Frame_preview_box::full_page_hdl() --------------------------------

void Frame_preview_box::full_page_hdl( Button* )
{
    _vertical_scrollbar_ptr->Hide();
    _horizontal_scrollbar_ptr->Hide();

    if ( _second_preview_ptr != NULL )
        _second_preview_ptr->Hide();

    _zoom = Fraction( 0, 1 );
    _preview_ptr->ChangeZoomFactor( _zoom );
    calc_preview_size();
    check_scroll_range();
}

// --- Frame_preview_box::double_page_hdl() ------------------------------

void Frame_preview_box::double_page_hdl( Button* )
{
    short X, Y;
    Size  aHalfSize;

    _zoom = Fraction( 0, 1 );
    _preview_ptr->ChangeZoomFactor( _zoom );
    check_scroll_range();

    if ( _second_preview_ptr == NULL )
    {
        _second_preview_ptr = new Preview( this, 0 );
        _second_preview_ptr->SetJobSetup( _static_job_setup );
        ((Window*)_second_preview_ptr)->ChangeBackgroundBrush( Brush( Color( COL_WHITE ) ) );
        _second_preview_ptr->ChangeRequestPageHdl( LINK( this, Frame_preview_box, print_page ) );
        _second_preview_ptr->ChangeCurPage( _page + 1 );
    }
    _second_preview_ptr->ChangeZoomFactor( _zoom );
    _second_preview_ptr->ChangePageOffset( Point( 0, 0 ) );
    _second_preview_ptr->Invalidate();
    _second_preview_ptr->Show();

    aHalfSize = Size( (_max_size.Width() / 2) - 7,
                      _max_size.Height() - 10 );
    aHalfSize = _preview_ptr->CalcWindowSizePixel( aHalfSize );
    aHalfSize = _second_preview_ptr->CalcWindowSizePixel( aHalfSize );

    Y = (_max_size.Height() - aHalfSize.Height()) / 2;
    X = _max_size.Width() / 2 - aHalfSize.Width() - 2;
    _preview_ptr->ChangePosPixel( Point( X, Y ) );
    X = _max_size.Width() / 2 + 2;
    _second_preview_ptr->ChangePosPixel( Point( X, Y ) );

    _preview_ptr->ChangeSizePixel( aHalfSize );
    _second_preview_ptr->ChangeSizePixel( aHalfSize );
}

// --- Frame_preview_box::prev_page_dl() --------------------------------

void Frame_preview_box::prev_page_hdl( Button* )
{
    if ( _page > 1 )
    {
        _preview_ptr->ChangeCurPage( --_page );

        if ( GetParent() ) {
            GetParent()->SetText(   String( "Frame - Preview   (Seite ") +
                                    String( _page ) +
                                    String( " von " ) +
                                    String( _page_list_ptr->page_count() ) +
                                    String( ")" )
                                );
        }
        if ( _second_preview_ptr != NULL )
        {
            _preview_ptr->Update();
            _second_preview_ptr->ChangeCurPage( _page + 1 );
            if ( _second_preview_ptr->IsVisible() )
                _second_preview_ptr->Update();
        }
        else
            _preview_ptr->Update();
    } else {
      Sound::Beep();
    };
}

// --- Frame_preview_box::next_page_hdl() --------------------------------

void Frame_preview_box::next_page_hdl( Button* )
{
    if ( _page < _page_list_ptr->page_count() )
    {
        _preview_ptr->ChangeCurPage( ++_page );

        if ( GetParent() ) {
            GetParent()->SetText(   String( "Frame - Preview   ( Seite ") +
                                    String( _page ) +
                                    String( " von " ) +
                                    String( _page_list_ptr->page_count() ) +
                                    String( " )" )
                                );
        }
        if ( _second_preview_ptr != NULL ) {
            _preview_ptr->Update();
            _second_preview_ptr->ChangeCurPage( _page + 1 );
            if ( _second_preview_ptr->IsVisible() )
                _second_preview_ptr->Update();
        }
        else
            _preview_ptr->Update();
    } else {
      Sound::Beep();
    }
}

// --- Frame_preview_box::handle_exception() --------------------------------------
/*
void Frame_preview_box::handle_exception()
{
    SHOW_EXCEPTION( "Fehler aufgetreten: " );
    ignore_exception();
}
*/
// --- Frame_preview_box::print_page_hdl() --------------------------------------

void Frame_preview_box::print_page_hdl( Preview* pPreview )
{
try {
    LOG( "Frame_preview_box::print_page_hdl: page=" << pPreview->GetCurPage() << "\n" );
    _page_list_ptr->print_page( (Printer*)pPreview, pPreview->GetCurPage() );

    // Vertikaler Scrollbereich stimmt erst bei folgender Kombination full_page/normal_page (sonst fehlt ein Bereich unten)
    if ( _first_page )
    {   // muß nur beim ersten Mal gemacht werden
        _first_page = false;
        normal_page_hdl( 0 );
    }

}
catch( const Xc& x )
{
    SHOW_MSG( x );
}
catch( const xmsg& x )
{
    SHOW_MSG( c_str( x.why() ) );
}
    //handle_exception();
}

// --- Frame_preview_box::print_page() ----------------------------------

void Frame_preview_box::print_page( Preview* pPrev )
{
   _request_page_hdl.Call( pPrev );
}

// --- Frame_preview_box::do_print_direct() ----------------------------------

void Frame_preview_box::do_print_direct( Button* )
{
        pApp->Wait( TRUE );
        if ( !_static_printer->StartJob( "Frame - Drucken" ) )
        {
            ErrorBox( NULL, WB_OK, "StartJob Error" ).Execute();
            return;
        }

        GetParent()->Disable();

        _static_printer->SetCopyCount( 1, TRUE );

        for ( USHORT i = 1; i <= _page_list_ptr->page_count(); i++ )
        {
            _static_printer->StartPage();
            _page_list_ptr->print_page( _static_printer, i );
            _static_printer->EndPage();
        }
        _static_printer->EndJob();

        pApp->Wait( FALSE );

        GetParent()->Enable();

}

// --- Frame_preview_box::do_print() ----------------------------------

void Frame_preview_box::do_print( Button* )
{
    PrintDialog aPrintDlg( NULL, WB_SVLOOK );

    aPrintDlg.ChangePrinter( _static_printer );
    aPrintDlg.ChangeCopyCount( 1 );
    aPrintDlg.ChangeFirstPage( 1 );
    aPrintDlg.ChangeLastPage( _page_list_ptr->page_count() );
    aPrintDlg.EnablePageFields( TRUE );
    aPrintDlg.EnableSelection( FALSE );
    aPrintDlg.EnableCollate( TRUE );
    aPrintDlg.CheckCollate( FALSE );

    if ( aPrintDlg.Execute() == RET_OK )
    {
        pApp->Wait( TRUE );
        if ( !_static_printer->StartJob( "Frame - Drucken" ) )
        {
            ErrorBox( NULL, WB_OK, "StartJob Error" ).Execute();
            return;
        }

        GetParent()->Disable();

        _static_printer->SetCopyCount( aPrintDlg.GetCopyCount(),
                                   aPrintDlg.IsCollateChecked() );
        USHORT nFirstPage = aPrintDlg.GetFirstPage();
        USHORT nLastPage = aPrintDlg.GetLastPage();

        for ( USHORT i = nFirstPage; i <= nLastPage; i++ )
        {
            _static_printer->StartPage();
            _page_list_ptr->print_page( _static_printer, i );
            _static_printer->EndPage();
        }
        _static_printer->EndJob();

        pApp->Wait( FALSE );

        GetParent()->Enable();
     }

}


// --- Frame_preview_box::do_setup() ----------------------------------

void Frame_preview_box::do_setup( Button* )
{
    PrinterSetupDialog aSetupDlg( NULL, WB_SVLOOK );
    aSetupDlg.ChangePrinter( _static_printer );
    aSetupDlg.Execute();
    // JobSetup im Preview anpassen ???
}




// --- Frame_modeless_preview::Frame_modeless_preview() ------------------------------

Frame_modeless_preview::Frame_modeless_preview( Window* pParent, Frame_page_list* page_list_ptr ) :
    ModelessDialog( pParent, ResId( DLG_PREVIEW ) ),
    // WICHTIG: aHScroll UND aVScroll müssen schon erzeugt sein (s. Reihenfolge im Header!)
    aPVBox(         this, ResId( DP_BOX ), page_list_ptr, ResId( DP_HSCROLL ), ResId( DP_VSCROLL ) ),
    aZoomIn(        this, ResId( DP_ZOOMIN ) ),
    aZoomOut(       this, ResId( DP_ZOOMOUT ) ),
    aNormalPage(    this, ResId( DP_NORMALPAGE ) ),
    aFullPage(      this, ResId( DP_FULLPAGE ) ),
    aDoublePage(    this, ResId( DP_DOUBLEPAGE ) ),
    aPrevPage(      this, ResId( DP_PREVPAGE ) ),
    aNextPage(      this, ResId( DP_NEXTPAGE ) ),
    aPrintButton(   this, ResId( DP_PRINT_PB ) ),
    aSetupButton(   this, ResId( DP_SETUP_PB ) ),
    aCancel(        this, ResId( DP_CANCEL ) )
{
    xc;
    FreeResource();

    // Der Preview Box die ScrollBars bekanntmachen: die Box kümmert sich dann darum
    //aPVBox.vertical_scrollbar(  &aVScroll );
    //aPVBox.horizontal_scrollbar( &aHScroll );

    aZoomIn.ChangeClickHdl(         LINK( &aPVBox, Frame_preview_box,      zoom_in_hdl     ) );
    aZoomOut.ChangeClickHdl(        LINK( &aPVBox, Frame_preview_box,      zoom_out_hdl    ) );
    aNormalPage.ChangeClickHdl(     LINK( &aPVBox, Frame_preview_box,      normal_page_hdl ) );
    aFullPage.ChangeClickHdl(       LINK( &aPVBox, Frame_preview_box,      full_page_hdl   ) );
    aDoublePage.ChangeClickHdl(     LINK( &aPVBox, Frame_preview_box,      double_page_hdl ) );
    aPrevPage.ChangeClickHdl(       LINK( &aPVBox, Frame_preview_box,      prev_page_hdl   ) );
    aNextPage.ChangeClickHdl(       LINK( &aPVBox, Frame_preview_box,      next_page_hdl   ) );
    aPrintButton.ChangeClickHdl(    LINK( &aPVBox, Frame_preview_box,      do_print        ) );
    aSetupButton.ChangeClickHdl(    LINK( &aPVBox, Frame_preview_box,      do_setup        ) );

    aCancel.ChangeClickHdl(         LINK( this,    Frame_modeless_preview, CancelHdl       ) );

exceptions
}

// --- Frame_modeless_preview::~Frame_modeless_preview() -----------------------------

Frame_modeless_preview::~Frame_modeless_preview()
{
}


// --- Frame_modeless_preview::CancelHdl() --------------------------------

BOOL Frame_modeless_preview::Close() {
    this->Hide();
    pApp->Quit();
    return TRUE;
}

// --- Frame_modeless_preview::CancelHdl() --------------------------------

void Frame_modeless_preview::CancelHdl( Button* ) {
    this->Hide();
    pApp->Quit();
}


// --- Frame_modal_preview::Frame_modal_preview() ------------------------------

Frame_modal_preview::Frame_modal_preview( Window* pParent, Frame_page_list* page_list_ptr, const ResId& resid ) :
    ModalDialog( pParent, resid ),
    aPVBox(         this, ResId( DP_BOX ), page_list_ptr, ResId( DP_HSCROLL ), ResId( DP_VSCROLL ) ),
    aZoomIn(        this, ResId( DP_ZOOMIN ) ),
    aZoomOut(       this, ResId( DP_ZOOMOUT ) ),
    aNormalPage(    this, ResId( DP_NORMALPAGE ) ),
    aFullPage(      this, ResId( DP_FULLPAGE ) ),
    aDoublePage(    this, ResId( DP_DOUBLEPAGE ) ),
    aPrevPage(      this, ResId( DP_PREVPAGE ) ),
    aNextPage(      this, ResId( DP_NEXTPAGE ) ),
    aPrintButton(   this, ResId( DP_PRINT_PB ) ),
    aSetupButton(   this, ResId( DP_SETUP_PB ) ),
    aCancel(        this, ResId( DP_CANCEL ) )
{
    FreeResource();

    aZoomIn.ChangeClickHdl(         LINK( &aPVBox, Frame_preview_box,      zoom_in_hdl     ) );
    aZoomOut.ChangeClickHdl(        LINK( &aPVBox, Frame_preview_box,      zoom_out_hdl    ) );
    aNormalPage.ChangeClickHdl(     LINK( &aPVBox, Frame_preview_box,      normal_page_hdl ) );
    aFullPage.ChangeClickHdl(       LINK( &aPVBox, Frame_preview_box,      full_page_hdl   ) );
    aDoublePage.ChangeClickHdl(     LINK( &aPVBox, Frame_preview_box,      double_page_hdl ) );
    aPrevPage.ChangeClickHdl(       LINK( &aPVBox, Frame_preview_box,      prev_page_hdl   ) );
    aNextPage.ChangeClickHdl(       LINK( &aPVBox, Frame_preview_box,      next_page_hdl   ) );
    aPrintButton.ChangeClickHdl(    LINK( &aPVBox, Frame_preview_box,      do_print        ) );
    aSetupButton.ChangeClickHdl(    LINK( &aPVBox, Frame_preview_box,      do_setup        ) );

    aCancel.ChangeClickHdl(         LINK( this,    Frame_modal_preview,    CancelHdl       ) );
    
 exceptions
}

// --- Frame_modal_preview::~Frame_modal_preview() -----------------------------

Frame_modal_preview::~Frame_modal_preview()
{
}

      
// --- Frame_modal_preview::CancelHdl() --------------------------------

BOOL Frame_modal_preview::Close() {
    EndDialog(FALSE);
    return TRUE;
}

// --- Frame_modal_preview::CancelHdl() --------------------------------

void Frame_modal_preview::CancelHdl( Button* ) {
    EndDialog(FALSE);
}


} //namespace sos

#endif

