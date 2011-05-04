// $Id$
#ifndef __ZSCHIMMER_STOP_WATCH_H_
#define __ZSCHIMMER_STOP_WATCH_H_

namespace zschimmer {

struct Stop_watch : Object
{
    Stop_watch() : _start_time( double_from_gmtime() ) {}
    double time() const { return double_from_gmtime() - _start_time; }
    string obj_name() const { return string_printf( "%.3fs", time() ); }

private:
    const double _start_time;
};

}

#endif
