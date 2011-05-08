#define MODULE_NAME "videobio"
//                                                      (c) Joacim Zschimmer

#include <sysdep.h>
#if defined SYSTEM_DOS               // Dieses Modul gilt nur für DOS-BIOS


#include <bios.h>

#include "videobios.h"

//------------------------------------------------------------Video_bios::write
#ifdef __BORLANDC__

void Video_bios::write(
    Video_pos   pos, 
    char*       text, 
    int         len, 
    Video_attr  attr 
)
{
    gotoxy( pos.column0() + 1, pos.line0() + 1 );
    textattr (attr);
    while (len--)  putch (*text++);
}

#endif
//-------------------------------------------------------------Video_bios::fill
#ifdef __BORLANDC__

void Video_bios::fill(
    Video_pos   pos, 
    char        c,
    int         len, 
    Video_attr  attr 
)
{
    gotoxy( pos.column0() + 1, pos.line0() + 1 );
    textattr (attr);
    while (len--)  putch (c);
}

#endif

#endif                          // SYSTEM_DOS

