// javatest.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "spooler.h"
#include "spooler_module_script_interface.h"
#include <iostream>

using namespace std;
using namespace zschimmer;
using namespace zschimmer::java;


namespace sos {
namespace scheduler {
namespace scheduler_java {

// const class_factory<ScriptConnector_class> ScriptConnector_class::class_factory ( "sos.service.scheduler.ModuleScriptConnector" );
const class_factory<ScriptInterface_class> ScriptInterface_class::class_factory ( "sos.modules.javascript.ModuleScriptJavaScript" );

//------------------------------------------------------------------------ ScriptInterface::add_obj

/*
void ScriptInterface::add_obj( jobject* object, const string& name )
{
    Z_LOG2("scheduler","Script_module_instance::add_obj name=" << name << "\n");
}
*/

//-------------------------------------------------------------------------------------------------

} //namespace java
} //namespace scheduler
} //namespace sos
