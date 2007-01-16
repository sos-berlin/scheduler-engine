// $Id$

#ifndef __SPOOLER_COMMON_H
#define __SPOOLER_COMMON_H


#include "../zschimmer/embedded_files.h"


namespace sos {
namespace scheduler {

#ifdef SYSTEM_WIN
#   define DIR_SEP "\\"
# else
#   define DIR_SEP "/"
#endif


typedef zschimmer::Thread::Id   Thread_id;                  // _beginthreadex()
typedef DWORD                   Process_id;

namespace time
{
    struct Time;
};

//-------------------------------------------------------------------------------------------------

extern const Embedded_files     embedded_files;             // spooler_embedded_files.cxx

//-----------------------------------------------------------------------------------------FOR_EACH

#define        FOR_EACH(             TYPE, CONTAINER, ITERATOR )  for( TYPE::iterator       ITERATOR = (CONTAINER).begin(); (ITERATOR) != (CONTAINER).end(); (ITERATOR)++ )
#define        FOR_EACH_CONST(       TYPE, CONTAINER, ITERATOR )  for( TYPE::const_iterator ITERATOR = (CONTAINER).begin(); (ITERATOR) != (CONTAINER).end(); (ITERATOR)++ )

//--------------------------------------------------------------------------------------------Do_log

enum Do_log
{
    dont_log = false,
    do_log   = true
};

//---------------------------------------------------------------------------Modified_event_handler

struct Modified_event_handler
{
    virtual void                before_modify_run_time_event()                                      {}
    virtual void                run_time_modified_event     () = 0;
};

//-------------------------------------------------------------------------------------------------

ptr<Com_variable_set>           variable_set_from_environment();

//-------------------------------------------------------------------------------scheduler_object<>
/*
template< class BASE_CLASS >
struct scheduler_object : BASE_CLASS, Scheduler_object_base
{
                                scheduler_object            ( Spooler* sp, Type_code code )         : Scheduler_object_base( sp, code ) {}
};

//---------------------------------------------------------------------------------Scheduler_object

typedef scheduler_object< Object >  Scheduler_object;       // Standard-Fall
*/
//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
