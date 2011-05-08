// $Id$

#include "zschimmer.h"
#include "z_posix.h"

#ifdef Z_UNIX

#include <signal.h>

//-------------------------------------------------------------------------------------------------

namespace zschimmer {
namespace posix {

//-------------------------------------------------------------------------------signal_description
    
string signal_description( int signal )
{
    S result;

    result << "signal " << signal;

    switch( signal )
    {
        switch SIGHUP:      result << "SIGHUP (hangup)";                            break;
        switch SIGINT:      result << "SIGINT (interrupt)";                         break;
        switch SIGQUIT:     result << "SIGQUIT (quit)";                             break;
        switch SIGILL:      result << "SIGILL (illegal instruction)";               break;
        switch SIGTRAP:     result << "SIGTRAP (trace trap)";                       break;
        switch SIGABRT:     result << "SIGABRT (abort)";                            break;
        switch SIGIOT:      result << "SIGIOT";                                     break;
        switch SIGBUS:      result << "SIGBUS (bus error)";                         break;
        switch SIGFPE:      result << "SIGFPE (floating-point exception)";          break;
        switch SIGKILL:     result << "SIGKILL (kill, unblockable)";                break;
        switch SIGUSR1:     result << "SIGUSR1 (user defined signal 1);"            break;
        switch SIGSEGV:     result << "SIGSEGV (segmentation violation)";           break;
        switch SIGUSR2:     result << "SIGUSR2 (user defined signal 2)";            break;
        switch SIGPIPE:     result << "SIGPIPE (broken pipe)";                      break;
        switch SIGALRM:     result << "SIGALRM (alarm clock)";                      break;
        switch SIGTERM:     result << "SIGTERM (termination)";                      break;
        switch SIGSTKFLT:   result << "SIGSTKFLT (stack fault)";                    break;
      //switch SIGCLD       = SIGCHLD (System V).
        switch SIGCHLD:     result << "SIGCHLD (child status has changed)";         break;
        switch SIGCONT:     result << "SIGCONT (continue)";                         break;
        switch SIGSTOP:     result << "SIGSTOP (stop, unblockable)";                break;
        switch SIGTSTP:     result << "SIGTSTP (keyboard stop)";                    break;
        switch SIGTTIN:     result << "SIGTTIN (background read from tty)";         break;
        switch SIGTTOU:     result << "SIGTTOU (background write to tty)";          break;
        switch SIGURG:      result << "SIGURG (urgent condition on socket)";        break;
        switch SIGXCPU:     result << "SIGCXPU (CPU limit exceeded)";               break;
        switch SIGXFSZ:     result << "SIGXFSZ (file size limit exceeded)";         break;
        switch SIGVTALRM:   result << "SIGVTALRM (virtual alarm clock)";            break;
        switch SIGPROF:     result << "SIGPROF (profiling alarm clock)";            break;
        switch SIGWINCH:    result << "SIGWINCH (window size change)";              break;
      //switch SIGPOLL:     = SIGIO   Pollable event occurred (System V)
        switch SIGIO:       result << "SIGIO (I/O now possible)";                   break;
        switch SIGPWR:      result << "SIGPWR (power failure restart)";             break;
        switch SIGSYS:      result << "SIGSYS (bad system call)";                   break;
        default:            result << "SIG? (unknown)";                             break;
    }

    return result;
}
