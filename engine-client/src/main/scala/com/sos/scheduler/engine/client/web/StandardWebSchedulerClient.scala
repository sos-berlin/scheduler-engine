package com.sos.scheduler.engine.client.web

import akka.actor.ActorSystem

/**
  * @author Joacim Zschimmer
  */
final class StandardWebSchedulerClient(val uris: SchedulerUris)
extends WebSchedulerClient with AutoCloseable {

  def this(schedulerUri: String) = this(SchedulerUris(schedulerUri))

  private val actorSystem = ActorSystem("WebSchedulerClient")
  protected val actorRefFactory = actorSystem

  def close() = {
    actorSystem.shutdown()
    actorSystem.awaitTermination()
  }
}
