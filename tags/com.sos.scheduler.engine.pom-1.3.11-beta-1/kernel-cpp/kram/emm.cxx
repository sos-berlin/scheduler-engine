#define MODULE_NAME "emm"
// emm.cpp                                               (c) SOS GmbH Berlin

#include <sysdep.h>
#if defined SYSTEM_DOS  &&  !defined SYSTEM_WIN


#include <stdlib.h>
#include <dos.h>

#include <emm.h>


int emm_exist()
{
    const int emm_interrupt_no = 0x67;
    REGS regs;

    if( *((uint4*) (emm_interrupt_no * 4) ) == 0 ) {
        return 0;
    } else {
        regs.h.ah = 0x46;
        int86( emm_interrupt_no, &regs, &regs );
        return regs.h.ah;
    }
}

#endif
