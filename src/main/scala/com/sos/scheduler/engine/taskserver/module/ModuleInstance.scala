package com.sos.scheduler.engine.taskserver.module

/**
 * @author Joacim Zschimmer
 */
trait ModuleInstance {
  type MyModule <: Module
  def module: MyModule
  lazy val spoolerTask = namedInvocables.spoolerTask
  lazy val spoolerLog = namedInvocables.spoolerLog
  def namedInvocables: NamedInvocables
}
