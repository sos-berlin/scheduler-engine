package com.sos.scheduler.engine.taskserver.module.dotnet

import com.sos.scheduler.engine.jobapi.dotnet.{DotnetModuleInstanceFactory, DotnetModuleReference, TaskContext}
import com.sos.scheduler.engine.taskserver.module._
import com.sos.scheduler.engine.taskserver.module.dotnet.DotnetModule._
import com.sos.scheduler.engine.taskserver.module.javamodule.{ApiModule, JavaModule}

/**
  * @author Joacim Zschimmer
  */
final case class DotnetModule(val moduleLanguage: ModuleLanguage, dotnetModuleReference: DotnetModuleReference)
extends ApiModule {

  def newJobInstance(namedInvocables: NamedInvocables) =
    factory.newJobInstance(namedInvocablesToTaskContext(namedInvocables), dotnetModuleReference)

  def newMonitorInstance(namedInvocables: NamedInvocables) =
    factory.newMonitorInstance(namedInvocablesToTaskContext(namedInvocables), dotnetModuleReference)
}

private object DotnetModule {
  private val factory: DotnetModuleInstanceFactory = ???

  private def namedInvocablesToTaskContext(namedInvocables: NamedInvocables) = TaskContext(
    JavaModule.spooler_log(namedInvocables),
    JavaModule.spooler_task(namedInvocables),
    JavaModule.spooler_job(namedInvocables),
    JavaModule.spooler(namedInvocables))
}
