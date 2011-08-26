// $Id: version.h 14774 2011-07-08 07:21:01Z ss $
// Für Windows-Dateiinfo

// #include "../target/cpp/mavenSettings.h"
#include "../generated/mavenSettings.h"

// Die Hostware-Versionsnummer wird nicht von Maven verwaltet, die ist fest (hoffentlich für alle Zeit)
#undef VER_PRODUCTVERSION
#undef VER_PRODUCTVERSION_STR
#define VER_PRODUCTVERSION       1,6,131 //,VER_REVISION
#define VER_PRODUCTVERSION_STR  "1.6.131." VER_PRODUCTVERSION_TAIL
