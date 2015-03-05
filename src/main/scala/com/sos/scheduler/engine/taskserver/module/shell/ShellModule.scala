package com.sos.scheduler.engine.taskserver.module.shell

import com.sos.scheduler.engine.taskserver.module.{Module, Script, ShellModuleLanguage}

/**
 * @author Joacim Zschimmer
 */
final class ShellModule(val script: Script) extends Module {
  def moduleLanguage = ShellModuleLanguage
}
