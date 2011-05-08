// sosappl.h                                  ©1996 SOS GmbH Berlin

#ifndef __SOSAPPL_H
#define __SOSAPPL_H

#ifndef __SOSSTAT_H
#   include "sosstat.h"
#endif

namespace sos
{

struct Sos_appl
{
                                Sos_appl            ( Bool init = true );  // bei false: init() selbst aufrufen!
                               ~Sos_appl            ();

    void                        init                ();
    void                        exit                ();

    Bool                       _valid; 
};


inline void Sos_appl::init()
{
    //jz 5.8.97 sos_static_ptr()->init();
    sos_static_ptr()->add_ref();
    _valid = true;
}


inline void Sos_appl::exit()
{
    if( _valid ) {
        sos_static_ptr()->remove_ref();
        _valid = false;
    }
}


inline Sos_appl::Sos_appl( Bool ini )
:
    _valid ( false )
{
    if( ini )  init();
}


inline Sos_appl::~Sos_appl()
{
    exit();
}


} //namespace sos


#endif

