/* filedde.h                                (c) SOS GmbH Berlin
                                            Joacim Zschimmer
*/

#ifndef __FILEDDE_H
#define __FILEDDE_H

namespace sos {


struct File_dde_server_impl;

struct File_dde_server          // Agent für File_dde_server_impl
{
                                File_dde_server         ( const char* name );
                               ~File_dde_server         ();

    virtual void                log_stream_ptr          ( ostream* );
    void                        log                     ( Bool );   // Quatsch
                               
  private:
    File_dde_server_impl*      _file_dde_server_impl_ptr;
};


} //namespace sos
#endif