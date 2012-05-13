#include "system.h"

#if defined Z_USE_JAVAXML
#   include "xml_java.h"
#   include "xslt_java.h"
#else
#   include "xml_libxml2.h"
#   include "xslt_libxslt.h"
#endif
