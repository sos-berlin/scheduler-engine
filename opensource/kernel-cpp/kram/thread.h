// $Id: thread.h 11394 2005-04-03 08:30:29Z jz $

#ifndef __SOS_THREAD_H
#define __SOS_THREAD_H

//-------------------------------------------------------------------------------------------------

namespace sos {

struct Hostware_exception_thread_data;

//----------------------------------------------------------------------------------Thread_container

struct Thread_container : Sos_self_deleting
{
    struct Data : Sos_self_deleting
    {
        virtual                ~Data                    () {}
    };


                                Thread_container        () : _zero_(this+1) {}

    Fill_zero                  _zero_;
    Sos_ptr<Data>              _com_server_last_exception;  // kram/com_server.cxx
    Hostware_exception_thread_data* _hostole_exception;     // hostole/exception.cxx
};

//-------------------------------------------------------------------------------------------------

Thread_container*               thread_container        ();

//-------------------------------------------------------------------------------------------------

} //namespace sos

#endif