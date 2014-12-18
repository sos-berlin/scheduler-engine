package com.sos.scheduler.engine.taskserver.configuration.inject

import com.google.inject.AbstractModule
import com.sos.scheduler.engine.minicom.inject.MinicomModule
import com.sos.scheduler.engine.taskserver.configuration.inject.TaskServerModule._
import com.sos.scheduler.engine.taskserver.job.RemoteModuleInstanceServer

/**
 * @author Joacim Zschimmer
 */
final class TaskServerModule extends AbstractModule {

  protected def configure() = {
    install(new MinicomModule(RegisteredIUnknownFactories))
  }
}

private object TaskServerModule {
  private val RegisteredIUnknownFactories = List(RemoteModuleInstanceServer)
}
