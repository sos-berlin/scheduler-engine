package com.sos.scheduler.engine.taskserver.module

/**
 * @author Joacim Zschimmer
 */
final class ShellModuleInstance(val module: ShellModule, val namedObjects: NamedObjects) extends ModuleInstance {
  type MyModule = ShellModule
}
