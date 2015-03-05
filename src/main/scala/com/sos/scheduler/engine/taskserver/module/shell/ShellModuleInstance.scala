package com.sos.scheduler.engine.taskserver.module.shell

import com.sos.scheduler.engine.taskserver.module.{ModuleInstance, NamedObjects}

/**
 * @author Joacim Zschimmer
 */
final class ShellModuleInstance(val module: ShellModule, val namedObjects: NamedObjects) extends ModuleInstance {
  type MyModule = ShellModule
}
