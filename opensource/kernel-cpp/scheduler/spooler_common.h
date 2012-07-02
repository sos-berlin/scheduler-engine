// $Id: spooler_common.h 14676 2011-06-25 20:58:20Z jz $

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
//typedef DWORD                   Process_id;

namespace time
{
    struct Time;
};

//-------------------------------------------------------------------------------------------------

extern const Embedded_files            embedded_files;             // spooler_embedded_files.cxx
extern const string reason_start_element_name;
extern const string obstacle_element_name;

//-----------------------------------------------------------------------------------------FOR_EACH

#define        FOR_EACH(             TYPE, CONTAINER, ITERATOR )  for( TYPE::iterator       ITERATOR = (CONTAINER).begin(); (ITERATOR) != (CONTAINER).end(); (ITERATOR)++ )
#define        FOR_EACH_CONST(       TYPE, CONTAINER, ITERATOR )  for( TYPE::const_iterator ITERATOR = (CONTAINER).begin(); (ITERATOR) != (CONTAINER).end(); (ITERATOR)++ )

//-------------------------------------------------------------------------------------------Do_log

enum Do_log
{
    dont_log = false,
    do_log   = true
};

//-----------------------------------------------------------------------------------First_and_last

enum First_and_last
{
    fl_first_only,
    fl_all,
    fl_last_only,
    fl_first_and_last_only
};

//---------------------------------------------------------------------------------------Visibility

enum Visibility
{
    visible_never = 1,          // Wird nicht automatisch auf visible_yes gesetzt
    visible_no,                 // Kann automatisch auf visible_yes gesetzt werden
    visible_yes
};

//-------------------------------------------------------------------------------------------------

typedef int64                   Process_id;   // Für Process

//-------------------------------------------------------------------------------------------------

ptr<Com_variable_set>           variable_set_from_environment();
bool                            is_allowed_operation_while_waiting( Async_operation* op );
xml::Element_ptr append_obstacle_element(const xml::Element_ptr& element, const string& attribute_name, const string& value);
xml::Element_ptr append_obstacle_element(const xml::Element_ptr& element, const xml::Element_ptr& obstacle_child);
inline string                   as_bool_string(bool b) { return b? "true" : "false"; }
void require_not_attribute(const xml::Element_ptr&, const string& name);

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
