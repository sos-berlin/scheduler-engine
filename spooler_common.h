// $Id$

#ifndef __SPOOLER_COMMON_H
#define __SPOOLER_COMMON_H

namespace sos {
namespace spooler {

#ifdef SYSTEM_WIN
#   define DIR_SEP "\\"
# else
#   define DIR_SEP "/"
#endif


typedef zschimmer::Thread::Id   Thread_id;                  // _beginthreadex()
typedef DWORD                   Process_id;

//-----------------------------------------------------------------------------------------FOR_EACH

#define        FOR_EACH(             TYPE, CONTAINER, ITERATOR )  for( TYPE::iterator       ITERATOR = (CONTAINER).begin(); (ITERATOR) != (CONTAINER).end(); (ITERATOR)++ )
#define        FOR_EACH_CONST(       TYPE, CONTAINER, ITERATOR )  for( TYPE::const_iterator ITERATOR = (CONTAINER).begin(); (ITERATOR) != (CONTAINER).end(); (ITERATOR)++ )

//---------------------------------------------------------------------------Modified_event_handler

struct Modified_event_handler
{
    virtual void                 before_modify_event        ()                                      {}
    virtual void                 modified_event             () = 0;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
