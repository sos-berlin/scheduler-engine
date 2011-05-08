// $Id$

#ifndef __SOS_SOSSOCK1_H
#define __SOS_SOSSOCK1_H


#if defined SYSTEM_WIN

#   include <winsock.h>

    typedef unsigned int SOCKET;
    typedef int          socklen_t;

#else

    extern "C" 
    {
#		if !defined SYSTEM_GNU
#       	include <sys/types.h>
#		endif

#       include <errno.h>

#       include <sys/socket.h>
#       include <sys/ioctl.h>
#       include <netdb.h>
#       include <netinet/in.h>   // gethostbyname()
#       include <arpa/inet.h>    // inet_addr()
#       include <unistd.h>       // close()
    }

    typedef int SOCKET;

    inline int closesocket( SOCKET s )
    {
        return close( s );
    }

    inline int ioctlsocket( SOCKET s, long a, u_long* b )
    {
        return ioctl( s, a, b );
    }

#   if defined SYSTEM_SOLARIS

#       include <sys/filio.h>       // ioctl( , FIONBIO )
#       include <sys/utsname.h>     // uname()

#   elif defined SYSTEM_LINUX

        extern "C" 
        {
#           include </usr/include/linux/errno.h>
#           include <sys/ioctl.h>       // ioctl
#           include <asm/ioctls.h>      // FIONBIO
#           include <sys/utsname.h>     // struct utsname, uname()
        }

#endif

#endif


namespace sos
{
    void throw_sos_socket_error( int errno_, const char* insertion );
    void throw_sos_socket_error( const char* insertion );
    void throw_sos_socket_error( const char* insertion, const char* ins2 );
}

#endif
