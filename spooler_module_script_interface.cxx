// javatest.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "spooler.h"
#include "spooler_module_script_interface.h"
#include <iostream>

using namespace std;
using namespace zschimmer;
using namespace zschimmer::javabridge;

namespace sos {
namespace scheduler {
namespace scheduler_java {

const class_factory<ScriptInterface_class> ScriptInterface_class::class_factory ( "sos.service.scripting.Module" );

//-------------------------------------------------------------------------------------------------

} //namespace java
} //namespace scheduler
} //namespace sos
