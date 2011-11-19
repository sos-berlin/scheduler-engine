/**
spooler.h enthält alles was man brauch. Dieser Header ist vorkompiliert.
*/
#include "spooler.h"
//#include "../zschimmer/file.h"
#include "schema_reader.h"


using namespace zschimmer;

namespace sos
{
	namespace scheduler
	{
		namespace file 
		{
			/**
			* \file schema_reader.cxx
			* \brief Allgemeine Funktionen für Scheduler Files (XSD-Schema, ini, ...)
			* \detail
			*
			* \author RB/SS
			* \version 2.0.224 - 2010-02-16 16:06
			* <div class="sos_branding">
			*    <p>© 2010 SOS GmbH - Berlin (<a style="color:silver" href="http://www.sos-berlin.com">http://www.sos-berlin.com</a>)</p>
			* </div>
			*/

			/**
			* \brief Einlesen des XSD-Schemas in lokale Variable
			* \detail
			* Bisher wurde das XSD-Schema als Literal in ein C++Modul abgelegt und kompiliert.
			* Änderungen am XSD-Schema haben damit keinen Einfluß auf den Scheduler.
			* Diese Funktion liest das Schema dynamisch ein und stellt es als string zur Verfügung.
			*
			* \version 2.0.224 - 2010-02-16 16:06
			*
			* \param filename - Vollständiger Dateiname 
			* \return string
			*/
			string xsd_from_file( const string& filename )
			{ 
				return string_from_file( filename );

			}

		}
	}
}
