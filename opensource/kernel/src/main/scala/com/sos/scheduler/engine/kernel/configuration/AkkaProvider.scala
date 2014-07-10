package com.sos.scheduler.engine.kernel.configuration

import akka.actor.ActorSystem
import com.google.common.io.Closer
import com.sos.scheduler.engine.common.scalautil.HasCloser.implicits._
import com.typesafe.config.ConfigFactory
import scala.concurrent.duration._

/**
 * @author Joacim Zschimmer
 */
private[configuration] object AkkaProvider {
  private val ConfigurationResourcePath = "com/sos/scheduler/engine/kernel/configuration/akka.conf"

  private[configuration] def newActorSystem(closer: Closer): ActorSystem = {
    val actorSystem = ActorSystem("JobScheduler", ConfigFactory.load(ConfigurationResourcePath))
    closer {
      //implicit val timeout = Timeout(15.seconds)
      //IO(Http) ? Http.CloseAll
      actorSystem.shutdown()
      actorSystem.awaitTermination(30.seconds)
    }

//    val deadLetterActorRef = actorSystem.actorOf(name = "DeadLetters", props =
//      Props {
//        new Actor with ActorLogging {
//          def receive = {
//            case o: DeadLetter ⇒ if (log.isDebugEnabled) log.debug(s"Dead letter: $o")
//          }
//        }
//      })
    //actorSystem.eventStream.subscribe(deadLetterActorRef, classOf[DeadLetter])
    //actorSystem.eventStream.setLogLevel(Logging.DebugLevel);
    actorSystem
  }
}
