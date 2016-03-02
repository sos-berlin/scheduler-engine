package com.sos.scheduler.engine.jobapi.dotnet

/**
  * @author Joacim Zschimmer
  */
trait DotnetModuleInstanceFactory {

  def newJobInstance(taskContext: TaskContext, reference: DotnetModuleReference): sos.spooler.Job_impl with AutoCloseable

  def newMonitorInstance(taskContext: TaskContext, reference: DotnetModuleReference): sos.spooler.Monitor_impl with AutoCloseable
}
