package com.sos.scheduler.engine.taskserver.module.shell

import com.sos.scheduler.engine.taskserver.module.{ModuleInstance, NamedInvocables}

/**
 * @author Joacim Zschimmer
 */
final class ShellModuleInstance(val module: ShellModule, val namedInvocables: NamedInvocables) extends ModuleInstance {
  type MyModule = ShellModule
}
