// $Id$
// 14. 3.92                                             (c) Joacim Zschimmer

#ifndef __STDFILE_H
#define __STDFILE_H

namespace sos {

Bool                    is_absolute_filename        ( const char* filename );
Bool                    is_filename                 ( const char* filename );
void                    make_path                   ( const Sos_string& path );
bool                    file_exists                 ( const Sos_string& filename );

} //namespace sos

#endif
