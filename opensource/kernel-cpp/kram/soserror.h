// $Id: soserror.h 11394 2005-04-03 08:30:29Z jz $

#ifndef __SOSERROR_H

namespace sos 
{
    struct Soserror_text
    {
        const char*            _code;
        const char*            _text;
    };


    extern Soserror_text soserror_texts[];

} //namespace sos

#endif
