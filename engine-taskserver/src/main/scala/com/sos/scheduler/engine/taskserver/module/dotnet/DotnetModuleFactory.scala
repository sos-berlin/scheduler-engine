package com.sos.scheduler.engine.taskserver.module.dotnet

import com.sos.scheduler.engine.jobapi.dotnet.api.DotnetModuleInstanceFactory
import com.sos.scheduler.engine.taskserver.module.ModuleArguments.DotnetModuleArguments
import javax.inject.{Inject, Singleton}
import org.jetbrains.annotations.TestOnly

/**
  * @author Joacim Zschimmer
  */
@Singleton
private[module] final class DotnetModuleFactory @Inject private(dotnetModuleInstanceFactory: DotnetModuleInstanceFactory) {

  def apply(arguments: DotnetModuleArguments) = new DotnetModule(arguments, dotnetModuleInstanceFactory)
}

private[module] object DotnetModuleFactory {

  @TestOnly
  val Unsupported = new DotnetModuleFactory(DotnetModuleInstanceFactory.Unsupported)
}
