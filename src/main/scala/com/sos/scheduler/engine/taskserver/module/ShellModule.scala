package com.sos.scheduler.engine.taskserver.module

/**
 * @author Joacim Zschimmer
 */
final class ShellModule(val script: Script) extends Module {
  def scriptLanguage = ShellScriptLanguage
}
