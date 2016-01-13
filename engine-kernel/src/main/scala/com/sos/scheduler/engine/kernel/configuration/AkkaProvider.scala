package com.sos.scheduler.engine.kernel.configuration

import akka.actor.ActorSystem
import com.google.common.io.Closer
import com.sos.scheduler.engine.common.ClassLoaders.currentClassLoader
import com.sos.scheduler.engine.common.akkautils.DeadLetterActor
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.typesafe.config.ConfigFactory
import scala.concurrent.duration._

/**
 * @author Joacim Zschimmer
 */
private[configuration] object AkkaProvider {
  private val ConfigurationResourcePath = "com/sos/scheduler/engine/kernel/configuration/akka.conf"

  private[configuration] def newActorSystem(closer: Closer): ActorSystem = {
    val actorSystem = ActorSystem("JobScheduler-Engine", ConfigFactory.load(currentClassLoader, ConfigurationResourcePath))
    closer.onClose {
      actorSystem.shutdown()
      actorSystem.awaitTermination(30.seconds)
    }
    DeadLetterActor.subscribe(actorSystem)
    actorSystem
  }
}
