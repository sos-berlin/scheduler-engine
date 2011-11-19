// fsfile.h

#ifndef __FSFILE_H
#define __FSFILE_H

namespace sos {

//const long int fs_max_block_size = 256+2+32768+256;

//jz 19.10.96#if defined(__DLL__)
//jz 19.10.96    extern "C" int _exit_dll(); // hostapi.dll, Modul dllmain.cpp
//  extern "C" int sos_exit(); // hostapi.dll, Modul sosfile.cpp
//#endif

struct Fs_connection;

extern Fs_connection* fs_exists_connection( const Sos_string& server,
                                            const Sos_string& user = empty_string,
                                            const Sos_string& password = empty_string );


} //namespace sos
#endif

