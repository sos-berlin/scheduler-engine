// $Id: z_signals.h 13199 2007-12-06 14:15:42Z jz $

#ifndef __Z_SIGNALS_H__
#define __Z_SIGNALS_H__

namespace zschimmer {

//-------------------------------------------------------------------------------------------------

int                             signal_code_from_name       ( const string&, bool* unknown_in_this_os_only = NULL );
int                             signal_code_from_name_or_0  ( const string&, bool* unknown_in_this_os_only = NULL );
string                          signal_name_from_code       ( int );
string                          signal_title_from_code      ( int );
//string                          signal_name_from_code_or_default( int, const string& name );

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
