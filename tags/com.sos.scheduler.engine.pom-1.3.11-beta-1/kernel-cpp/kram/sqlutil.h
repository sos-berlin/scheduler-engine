// sqlutil.h                            ©1996 SOS GmbH Berlin

#ifndef __SQLUTIL_H
#define __SQLUTIL_H

#ifndef __DYNOBJ_H
#   include <dynobj.h>
#endif

namespace sos
{

Dyn_obj get_single_row( const Sos_string& stmt, const Dyn_obj& param = null_dyn_obj );

} //namespace sos

#endif
