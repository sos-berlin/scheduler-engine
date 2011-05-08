// argv.h                                      ©2005 Joacim Zschimmer
// $Id$

#ifndef __ZSCHIMMER_ARGV_H
#define __ZSCHIMMER_ARGV_H

#include "zschimmer.h"

namespace zschimmer {

//-------------------------------------------------------------------------------------------------

bool                        get_argv_option             ( const char* argv_i, const string& option );
bool                        get_argv_option             ( const char* argv_i, const string& prefix, string* );
bool                        get_argv_option             ( const char* argv_i, const string& prefix, int* );
bool                        get_argv_option             ( const char* argv_i, const string& prefix, bool* );

//-------------------------------------------------------------------------------------------------

} //namsepace zschimmer

#endif
