// $Id$

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
