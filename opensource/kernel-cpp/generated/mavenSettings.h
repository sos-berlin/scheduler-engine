// mavenSettings.h

/*  Die Variable (Dollar-Zeichen und geschweifte Klammern) werden von Maven ersetzt, siehe pom.xml. 
    Die ersetzte mavenSettings.h steht dann in target/cpp/.
    Wenn auch VER_PRODUCTVERSION und VER_DATE gesetzt werden soll, muessen wir wohl ein Skript (oder ein Maven-PlugIn) schreiben.
    Das Skript koennte make/make-scheduler-phase sein, das die Ersetzungen mit sed oder dergleichen vornimmt.
    Das Skript braucht bash, also unter Windows Cygwin. Man koennte es in Scala umschreiben.
*/

// Maven-Plugin build-helper-maven-plugin wird benötigt für die Maven-Variablen parsedVersion.xxx, siehe pom.xml.

// Die numerische Dateiversion einer Windows-Datei (bei uns VER_PRODUCTVERSION) hat vier Zahlen

#define VER_PRODUCTVERSION       1,3,12
#define VER_PRODUCTVERSION_STR  "1.3.12.2094-SNAPSHOT" VER_PRODUCTVERSION_TAIL

#ifdef _DEBUG
#   define VER_PRODUCTVERSION_DEBUG " (Debug)"
#else
#   define VER_PRODUCTVERSION_DEBUG ""
#endif

#define VER_PRODUCTVERSION_TAIL  VER_PRODUCTVERSION_DEBUG 
