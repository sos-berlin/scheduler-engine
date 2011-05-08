#ifndef __VIDEOWIN_H
#define __VIDEOWIN_H

#include <sv.hxx>
#undef min
#undef max

#include <video.h>



const unsigned short _LineOffset = 2;

class VideoWin : public WorkWindow,
                 public Video
{
public:
    VideoWin() :
       WorkWindow( NULL, WB_APP | WB_STDWORK ) {
        _init(); 
    };

    VideoWin( Window* pParent, WinBits nWinStyle ) :
       WorkWindow( pParent, nWinStyle ) {
       _init();
    };

    virtual void Paint( const Rectangle& );

    virtual void write( Video_pos pos, char c, Video_attr );
    virtual void write( Video_pos pos, char *text, int len, Video_attr );
    

    virtual void move(
      Subrange< int, 0, video_line_count >,
      Subrange< int, 0, video_line_count >,
      Subrange< int, 0, video_line_count > );

    void ScrollUpLine( Subrange< int, 0, video_line_count > = 1 );


    char aVideoBuffer[video_line_length * video_line_count + 1];
 
    Size FontSize() { return aFontSize; };

    void ChangeCharSet( FontCharSet fcs ) {
       Font f = GetFont();
       f.ChangeCharSet( fcs );
       ChangeFont( f );
    };
    void sync();

private:
    void _init();
    Size aFontSize;
    Size aScreenSize;
};

inline void Video_window::sync()
{
    Show();
    Invalidate(); //Paint( Rectangle(0,0,0,0) );
}

#endif
