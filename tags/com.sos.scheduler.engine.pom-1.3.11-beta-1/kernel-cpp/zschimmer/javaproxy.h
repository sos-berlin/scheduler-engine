// $Id$

#ifndef __ZSCHIMMER_JAVAPROXY_H__
#define __ZSCHIMMER_JAVAPROXY_H__

#include "Has_proxy.h"

namespace javaproxy { namespace java { namespace lang { struct String; }}}

namespace zschimmer { 
namespace javabridge {

// Ermöglicht die Übergabe von const char* und string an ein proxy_jobject<java::lang::String>.
// Damit können C++-Strings an Java-Proxies übergeben werden.

template<> struct proxy_jobject< ::javaproxy::java::lang::String > : Proxy_jobject
{
    proxy_jobject() {}
    proxy_jobject(const char*   s) : _local_jstring(s) { set_jobject(_local_jstring); }
    proxy_jobject(const string& s) : _local_jstring(s) { set_jobject(_local_jstring); }

    Local_jstring _local_jstring;
};
}}

#endif
