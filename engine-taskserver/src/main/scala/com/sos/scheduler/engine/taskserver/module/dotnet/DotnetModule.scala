package com.sos.scheduler.engine.taskserver.module.dotnet

import com.sos.scheduler.engine.jobapi.dotnet.api.{DotnetModuleInstanceFactory, TaskContext}
import com.sos.scheduler.engine.taskserver.module.ModuleArguments.DotnetModuleArguments
import com.sos.scheduler.engine.taskserver.module._
import com.sos.scheduler.engine.taskserver.module.dotnet.DotnetModule._
import com.sos.scheduler.engine.taskserver.module.javamodule.{ApiModule, JavaModule}

/**
  * @author Joacim Zschimmer
  */
final class DotnetModule private[dotnet](val arguments: DotnetModuleArguments, factory: DotnetModuleInstanceFactory)
extends ApiModule {

  import arguments.dotnetModuleReference

  def newJobInstance(namedInvocables: NamedInvocables) =
    factory.newInstance(classOf[sos.spooler.Job_impl], namedInvocablesToTaskContext(namedInvocables), dotnetModuleReference)

  def newMonitorInstance(namedInvocables: NamedInvocables) =
    factory.newInstance(classOf[sos.spooler.Monitor_impl], namedInvocablesToTaskContext(namedInvocables), dotnetModuleReference)
}

private object DotnetModule {

  private def namedInvocablesToTaskContext(namedInvocables: NamedInvocables) = TaskContext(
    JavaModule.spooler_log(namedInvocables),
    JavaModule.spooler_task(namedInvocables),
    JavaModule.spooler_job(namedInvocables),
    JavaModule.spooler(namedInvocables))
}
