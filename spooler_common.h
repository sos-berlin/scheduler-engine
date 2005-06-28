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
    virtual void                before_modify_event         ()                                      {}
    virtual void                modified_event              () = 0;
};

//---------------------------------------------------------------------------------Scheduler_object

struct Scheduler_object : Sos_self_deleting
{
    enum Type_code
    {
        type_none,
        type_scheduler,
        type_job,
        type_task,
        type_order,
        type_job_chain
    };


    static string               name_of_type_code           ( Type_code );


                                Scheduler_object            ( Spooler* sp, Type_code code )         : _spooler(sp), _scheduler_object_type_code(code) {}


    Type_code                   scheduler_type_code         () const                                { return _scheduler_object_type_code; }
    virtual Xslt_stylesheet     mail_xslt_stylesheet        ();
    virtual Prefix_log*         log                         ()                                      = 0;


    Spooler*                   _spooler;
    Type_code                  _scheduler_object_type_code;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
