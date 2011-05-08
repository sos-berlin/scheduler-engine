// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __ZSCHIMMER_BASE64_H
#define __ZSCHIMMER_BASE64_H

namespace zschimmer {

//-------------------------------------------------------------------------------------------------

string                          base64_decoded              ( const io::Char_sequence& base64_encoded );
string                          base64_encoded              ( const io::Byte_sequence& );

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
