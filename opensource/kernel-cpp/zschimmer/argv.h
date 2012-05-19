// argv.h                                      ©2005 Joacim Zschimmer
// $Id: argv.h 13199 2007-12-06 14:15:42Z jz $

#ifndef __ZSCHIMMER_ARGV_H
#define __ZSCHIMMER_ARGV_H

namespace zschimmer {

//-------------------------------------------------------------------------------------------------

bool                        get_argv_option             ( const char* argv_i, const string& option );
bool                        get_argv_option             ( const char* argv_i, const string& prefix, string* );
bool                        get_argv_option             ( const char* argv_i, const string& prefix, int* );
bool                        get_argv_option             ( const char* argv_i, const string& prefix, bool* );

//-------------------------------------------------------------------------------------------------

} //namsepace zschimmer

#endif
