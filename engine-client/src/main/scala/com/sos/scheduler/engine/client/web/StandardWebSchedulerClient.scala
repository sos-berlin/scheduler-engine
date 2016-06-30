package com.sos.scheduler.engine.client.web

import akka.actor.ActorSystem

/**
  * @author Joacim Zschimmer
  */
final class StandardWebSchedulerClient(protected val schedulerUri: String)
extends WebSchedulerClient with AutoCloseable {

  private val actorSystem = ActorSystem("WebSchedulerClient")
  protected val actorRefFactory = actorSystem

  def close() = {
    actorSystem.shutdown()
    actorSystem.awaitTermination()
  }
}
