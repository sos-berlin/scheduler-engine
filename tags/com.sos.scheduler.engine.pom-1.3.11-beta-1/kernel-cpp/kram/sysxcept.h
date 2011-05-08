// sysxcept.h                           (c) SOS GmbH Berlin
// $Id$

// §1693

#ifndef __SYSXCEPT_H
#define __SYSXCEPT_H

#if !defined __SYSDEP_H
#    include "sysdep.h"
#endif

#if !defined __SOSSTRNG_H
#   include "sosstrng.h"
#endif

#if defined __BORLANDC__

#   if !defined __XCEPT_H
#       include <except.h>
#   endif

#   if !defined __TYPEINFO_H
#       define Type_info __Type_info__
#       include <typeinfo.h>
#       undef Type_info
#   endif

#   if !defined __BORSTRNG_H
#       include <borstrng.h>
#   endif

    inline const char* exception_name( const xmsg& x )
    {
        //return typeid( x ).name();
        return z::name_of_type( x );
    }

    inline const char* exception_text( const xmsg& x ) { return c_str( x.why() ); }

    typedef xmsg exception;

#elif defined SYSTEM_SOLARISxxx // 22.7.01

#   include <exception.h>

    namespace sos {

    inline const char* exception_name( const xmsg& x )        // der Name der aktuellen Exception wird geliefert!
    {
        //return exception_name();      // 3.0
        //return "UNKNOWN";               // 4.0
	    return x.why();			// 4.2	js 16/7/97
    }

    inline const char* exception_text( const xmsg& x ) { return "(exception)"; }

    // typedef xmsg exception; js 10.10.96: kollidiert mit <math.h>

    } //namespace sos

#elif defined SYSTEM_MICROSOFT

//# include <eh.h>         // terminate() und unexpected()
#   include <stdexcpt.h>
#   include <typeinfo.h>

    namespace sos 
    {
        string             exception_name( const exception& x ); //  { return typeid( x ).name(); }
        inline const char* exception_text( const exception& x )  { return x.what(); }

        typedef exception xmsg;
    } //namespace sos

#else

#   include <exception>       // GNU gcc 3
    using std::exception;

    namespace sos 
    {
        //struct exception {};

      //inline const char* exception_name( const exception& )  { return "(gnuexception)"; }
        string             exception_name( const exception& x );
        inline const char* exception_text( const exception& x )  { return x.what(); }
        //inline const char* exception_text( const exception& )  { return "(Exception)"; }

        //typedef xmsg exception;
    } //namespace sos

#endif

namespace sos
{
    // Die Namen sind obsolet:
    //inline const char* name( const exception& x ) { return exception_name( x ); }
    //inline const char* text( const exception& x ) { return exception_text(x); }
}

#endif
