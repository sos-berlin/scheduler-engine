package com.sos.scheduler.engine.kernel.supervisor

import CppWebClient._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.SideEffect._
import com.sos.scheduler.engine.common.xml.XmlUtils.loadXml
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.async.{SchedulerThreadCallQueue, CppCall}
import com.sun.jersey.api.client.Client
import com.sun.jersey.api.client.filter.GZIPContentEncodingFilter
import java.io.InputStream
import javax.inject.Inject
import javax.ws.rs.core.MediaType._
import scala.util.{Failure, Success}

@ForCpp
final class CppWebClient @Inject private(schedulerCallQueue: SchedulerThreadCallQueue)
    extends AutoCloseable {

  private val webClient = Client.create() sideEffect { _.addFilter(new GZIPContentEncodingFilter(false)) }

  @ForCpp
  def close() {
    webClient.destroy()
  }

  @ForCpp
  def postXml(uri: String, xmlBytes: Array[Byte], resultCall: CppCall) {
    def process() {
      resultCall.value = try {
        val responseStream = webClient.resource(uri).`type`(TEXT_XML_TYPE).accept(TEXT_XML_TYPE).post(classOf[InputStream], xmlBytes)
        Success(loadXml(responseStream))
      } catch {
        case t: Throwable =>
          logger.debug(s"$uri: $t", t)
          Failure(t)
      }
      schedulerCallQueue add resultCall
    }

    val thread = new Thread("CppWebClient") {
      override def run() {
        process()
      }
    }
    thread.start()
  }
}

object CppWebClient {
  private val logger = Logger(getClass)
}
