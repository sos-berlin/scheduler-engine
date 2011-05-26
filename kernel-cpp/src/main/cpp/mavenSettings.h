// mavenSettings.h

/*  Die Variable ${...} werden von Maven ersetzt, siehe pom.xml. 
    Die ersetzte mavenSettings.h steht dann in target/cpp/.
    Wenn auch VER_PRODUCTVERSION und VER_DATE gesetzt werden soll, m�ssen wir wohl ein Skript (oder ein Maven-PlugIn) schreiben.
    Das Skript k�nnte make/make-scheduler-phase sein, das die Ersetzungen mit sed oder dergleichen vornimmt.
    Das Skript braucht bash, also unter Windows Cygwin. Man k�nnte es in Scala umschreiben.
*/

// Maven-Plugin build-helper-maven-plugin wird benötigt für die Maven-Variablen parsedVersion.xxx, siehe pom.xml.

// Die numerische Dateiversion einer Windows-Datei (bei uns VER_PRODUCTVERSION) hat vier Zahlen

#define VER_PRODUCTVERSION       ${parsedVersion.majorVersion},${parsedVersion.minorVersion},${parsedVersion.incrementalVersion}
#define VER_PRODUCTVERSION_STR  "${project.version}" VER_PRODUCTVERSION_TAIL

#ifdef _DEBUG
#   define VER_PRODUCTVERSION_DEBUG " (Debug)"
#else
#   define VER_PRODUCTVERSION_DEBUG ""
#endif

#define VER_PRODUCTVERSION_TAIL  VER_PRODUCTVERSION_DEBUG 
