//#if defined __BORLANDC__
//#  define CHECK_STACK_OVERFLOW
//#endif

#include "precomp.h"

//#define MODULE_NAME "xlat"
// xlat.cpp                                     (c) SOS GmbH Berlin
//                                              Joacim Zschimmer

#include "sos.h"


#if defined SYSTEM_WIN16

void xlat( void* d, const void* s, unsigned int l, const Byte* t )
{
    if( l == 0 )  return;

    if( d != s )  memcpy( d, s, l );

    __asm push ds
    __asm sub bx, bx
    __asm mov cx, [l]
    __asm lds si, [d]
    __asm les di, [t]
          loop:
    __asm     mov bl, [si]
    __asm     inc si
    __asm     mov al, es:[di+bx]
    __asm     dec cx
    __asm     mov [si-1], al
    __asm     jnz loop
    __asm pop ds
}

#endif
