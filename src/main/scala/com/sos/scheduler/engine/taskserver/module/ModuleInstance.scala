package com.sos.scheduler.engine.taskserver.module

/**
 * @author Joacim Zschimmer
 */
trait ModuleInstance {
  type MyModule <: Module
  def module: MyModule
  lazy val spoolerTask = namedObjects.spoolerTask
  lazy val spoolerLog = namedObjects.spoolerLog
  def namedObjects: NamedObjects
}
