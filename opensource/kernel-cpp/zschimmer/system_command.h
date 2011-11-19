// $Id: system_command.h 11394 2005-04-03 08:30:29Z jz $
// ©2000 Joacim Zschimmer

#ifndef __SYSTEM_COMMAND_H
#define __SYSTEM_COMMAND_H

namespace zschimmer {

//-----------------------------------------------------------------------------------System_command

struct System_command
{
                            System_command              ()                                          : _zero_(this+1), _exit_code(-1), _throw_xc(true) {}
                            System_command              ( const string& cmd )                       : _zero_(this+1), _exit_code(-1), _throw_xc(true) { execute( cmd ); }
    
    void                    operator()                  ( const string& cmd )                       { execute( cmd ); }

    void                    set_throw                   ( bool throw_xc )                           { _throw_xc = throw_xc; }
    Xc*                     xc                          ()                                          { return _error? &_xc : 0; }
    int                     exit_code                   () const                                    { return _exit_code; }
    void                    execute                     ( const string& cmd );
  //int                     execute_with_exit_code      ( const string& cmd );

    const string&           stdout_text                 ()                                          { return _stdout_text; }
    const string&           stderr_text                 ()                                          { return _stderr_text; }
    

    Fill_zero              _zero_;
    bool                   _throw_xc;
    bool                   _error;
    Xc                     _xc;
    int                    _exit_code;
    string                 _stdout_text;
    string                 _stderr_text;
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
