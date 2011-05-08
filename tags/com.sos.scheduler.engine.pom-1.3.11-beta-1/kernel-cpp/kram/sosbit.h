// sosbit.h             Intel 386 Assembler!!

#ifndef __SOSBIT_H
#define __SOSBIT_H

//---------------------------------------------------------------------------------------bitxxx

extern "C" int _cdecl _far bitchk (
    Byte _far*  bitmap,
    int4        bitno,
    int4        bitcnt,
    int         bit
);

extern "C" int4 _far _cdecl bitscn10 (  // Sucht Einsen und setzt sie auf Null
					// Liefert -1, wenn nix gefunden
    Byte _far*  bitmap,
    int4        bitmap_bitzahl,         // Gr”áe der Bitmap in Bits
    int4        anzahl_einsen           // Anzahl der aufeinanderfolgenden Einsen
);

extern "C" int4 _far _cdecl bitscan1 (   // Sucht Einsen
					// Liefert -1, wenn nix gefunden
    Byte _far*  bitmap,
    int4        bitmap_bitzahl,         // Gr”áe der Bitmap in Bits
    int4        anzahl_einsen           // Anzahl der aufeinanderfolgenden Einsen
);

extern "C" void _cdecl _far bitset (
    Byte _far*  bitmap,
    int4        bitno,
    int4        anzahl,
    int2        bit                     // 0 oder 1
);

#endif
