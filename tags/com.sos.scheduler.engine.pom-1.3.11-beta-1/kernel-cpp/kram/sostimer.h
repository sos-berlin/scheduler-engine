// sostimer.h                            ©1996 SOS GmbH Berlin

#ifndef __SOSTIMER_H
#define __SOSTIMER_H

//#include <time.h>
#ifndef __SOSLIST_H
#   include "soslist.h"
#endif

namespace sos
{

//------------------------------------------------------------------------------clock_as_double

double clock_as_double();

//---------------------------------------------------------------------------Sos_timer_callback

typedef int __cdecl Sos_timer_callback( void* );

//------------------------------------------------------------------------------------Sos_timer

struct Sos_timer
{
                                Sos_timer               ();
                                Sos_timer               ( Sos_timer_callback*, void* _callback_data, double time );
                               ~Sos_timer               ();

    Fill_zero                  _zero_;
    Sos_timer_callback*        _callback;
    void*                      _callback_data;
    double                     _alarm_time;
  //double                     _repeat_time;
};

//-------------------------------------------------------------------------------Sos_timer_node

struct Sos_timer_node : Sos_self_deleting
{
                                Sos_timer_node          () {}
                                Sos_timer_node          ( const Sos_timer& timer, Sos_timer_node* tail ) : _timer(timer), _tail(tail) {}

    void                        remove                  ();

    Sos_timer                  _timer;
    Sos_ptr<Sos_timer_node>    _tail;

  private:
                                Sos_timer_node          ( const Sos_timer_node& );   // nicht implementiert
    Sos_timer_node              operator =              ( const Sos_timer_node& );   // nicht implementiert
};

typedef Sos_timer_node* Sos_timer_handle;

//----------------------------------------------------------------------------Sos_timer_manager

struct Sos_timer_manager : Sos_self_deleting
{
                              //Sos_timer_manager       () : _list ( NULL ) {}


    Sos_timer_handle            add_timer               ( Sos_timer_callback*, void* callback_data,
                                                          double time );
    void                        remove_timer            ( Sos_timer_handle );

    double                      wait_time               ();
    void                        check                   ();

    Sos_ptr<Sos_timer_node>    _list;
};


} //namespace sos

#endif

