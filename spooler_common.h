// $Id$

#ifndef __SPOOLER_COMMON_H
#define __SPOOLER_COMMON_H


#include "../zschimmer/embedded_files.h"


namespace sos {
namespace spooler {

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

//---------------------------------------------------------------------------Modified_event_handler

struct Modified_event_handler
{
    virtual void                before_modify_run_time_event()                                      {}
    virtual void                run_time_modified_event     () = 0;
};

//---------------------------------------------------------------------------------Scheduler_object

struct Scheduler_object
{
    enum Type_code
    {
        type_none,
        type_scheduler,
        type_job,
        type_task,
        type_order,
        type_job_chain,
        type_database,
        type_web_service,
        type_web_service_operation,
        type_web_service_request,
        type_web_service_response,
        type_process,
      //type_subprocess_register
    };


    static string               name_of_type_code           ( Type_code );


                                Scheduler_object            ( Spooler* sp, IUnknown* me, Type_code code )         : _spooler(sp), _my_iunknown(me), _scheduler_object_type_code(code) {}
    virtual                    ~Scheduler_object            ()                                      {}    // Für gcc


    Type_code                   scheduler_type_code         () const                                { return _scheduler_object_type_code; }
    void                    set_mail_xslt_stylesheet_path   ( const string& path )                  { _mail_xslt_stylesheet.release();  _mail_xslt_stylesheet_path = path; }
    virtual ptr<Xslt_stylesheet> mail_xslt_stylesheet       ();
    virtual Prefix_log*         log                         ()                                      = 0;


    Spooler*                   _spooler;
    IUnknown*                  _my_iunknown;
    Type_code                  _scheduler_object_type_code;
    ptr<Xslt_stylesheet>       _mail_xslt_stylesheet;
    string                     _mail_xslt_stylesheet_path;
};

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

} //namespace spooler
} //namespace sos

#endif
