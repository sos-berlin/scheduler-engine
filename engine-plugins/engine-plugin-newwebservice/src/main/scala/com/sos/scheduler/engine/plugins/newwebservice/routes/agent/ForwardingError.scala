package com.sos.scheduler.engine.plugins.newwebservice.routes.agent

import com.sos.scheduler.engine.data.common.WebError
import spray.json.DefaultJsonProtocol._

/**
  * @author Joacim Zschimmer
  */
final case class ForwardingError(status: Int, message: String) extends WebError

object ForwardingError {
  implicit val jsonFormat = jsonFormat2(apply)
}
