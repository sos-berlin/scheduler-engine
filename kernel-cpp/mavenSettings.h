// mavenSettings.h

/*  Die Variable ${...} werden von Maven ersetzt, siehe pom.xml. 
    Die ersetzte mavenSettings.h steht dann in target/cpp/.
    Wenn auch VER_PRODUCTVERSION und VER_DATE gesetzt werden soll, müssen wir wohl ein Skript (oder ein Maven-PlugIn) schreiben.
    Das Skript könnte make/make-scheduler-phase sein, das die Ersetzungen mit sed oder dergleichen vornimmt.
    Das Skript braucht bash, also unter Windows Cygwin. Man könnte es in Scala umschreiben.
*/

//#define VER_PRODUCTVERSION       2,01,010
#define VER_PRODUCTVERSION_STR  "${project.version}" //VER_PRODUCTVERSION_TAIL
//#define VER_DATE         "${maven.build.time}"
//#define VER_TIME         "16:33:08"

#ifdef _DEBUG
#   define VER_PRODUCTVERSION_DEBUG " (Debug)"
#else
#   define VER_PRODUCTVERSION_DEBUG ""
#endif

//#define VER_PRODUCTVERSION_TAIL  "  (" VER_DATE " " VER_TIME ") " VER_PRODUCTVERSION_DEBUG 
#define VER_PRODUCTVERSION_TAIL  VER_PRODUCTVERSION_DEBUG 
