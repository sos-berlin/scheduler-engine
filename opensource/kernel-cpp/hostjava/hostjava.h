// $Id: hostjava.h 13912 2010-06-30 15:41:14Z ss $

#ifndef __HOSTJAVA_H
#define __HOSTJAVA_H

namespace sos {
namespace hostjava {

//-------------------------------------------------------------------------------------------------

void                            init_hostjava               ( zschimmer::javabridge::Vm* );               
void                            init_hostjava_process_with_pid( zschimmer::javabridge::Vm* );
void                            register_natives            ( zschimmer::javabridge::Vm*, const string& class_path, const JNINativeMethod*, int native_methods_count );

//-------------------------------------------------------------------------------------------------

} //namespace hostjava
} //namespace sos

#endif
