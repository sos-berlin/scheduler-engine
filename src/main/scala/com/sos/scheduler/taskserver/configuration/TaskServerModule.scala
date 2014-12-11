package com.sos.scheduler.taskserver.configuration

import com.google.inject.{AbstractModule, TypeLiteral}
import com.sos.scheduler.taskserver.comrpc.IUnknown
import com.sos.scheduler.taskserver.comrpc.types.{CLSID, IID}
import com.sos.scheduler.taskserver.comtypes.ClsidToFactory
import com.sos.scheduler.taskserver.configuration.TaskServerModule._
import com.sos.scheduler.taskserver.job.RemoteModuleInstanceServer

/**
 * @author Joacim Zschimmer
 */
final class TaskServerModule extends AbstractModule {

  protected def configure() = {
    bind(new TypeLiteral[ClsidToFactory] {}) toInstance ClassMap
  }
}

private object TaskServerModule {
  private val RegisteredIUnknownFactories = List(RemoteModuleInstanceServer)
  private val ClassMap: Map[(CLSID, IID), () ⇒ IUnknown] =
    (RegisteredIUnknownFactories map { o ⇒ (o.clsid, o.iid) → o.apply _ }).toMap
}
