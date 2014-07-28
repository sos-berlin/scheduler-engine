package com.sos.scheduler.engine.client.command

import akka.actor.ActorSystem
import com.sos.scheduler.engine.common.xml.XmlUtils.loadXml
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.async.{CppCall, SchedulerThreadCallQueue}
import java.net.URI
import javax.inject.{Inject, Singleton}
import org.w3c.dom.Document
import scala.concurrent.ExecutionContext
import scala.util.Try

/**
 * @author Joacim Zschimmer
 */
@ForCpp
@Singleton
final class CppHttpSchedulerCommandClient @Inject private(
  callQueue: SchedulerThreadCallQueue,
  actorSystem: ActorSystem,
  newCommandClient: HttpSchedulerCommandClient.Factory)
  (implicit executionContext: ExecutionContext) {

  @ForCpp
  def postXml(uri: String, xmlBytes: Array[Byte], resultCall: CppCall) {
    newCommandClient(new URI(uri)).executeXml(xmlBytes).onComplete { o: Try[String] ⇒
      resultCall.value = o map loadXml: Try[Document]
      resultCall.call()
    }
  }
}
