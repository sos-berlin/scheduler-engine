// emm.h                                                 (c) SOS GmbH Berlin


#ifndef __EMM_H
#define __EMM_H


#ifndef _Windows
    int emm_exist();
#else
    inline int emm_exist()  { return 0; }
#endif


#endif
