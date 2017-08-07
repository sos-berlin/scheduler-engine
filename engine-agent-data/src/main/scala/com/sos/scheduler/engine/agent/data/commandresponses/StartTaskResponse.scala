package com.sos.scheduler.engine.agent.data.commandresponses

import com.sos.scheduler.engine.agent.data.AgentTaskId
import com.sos.scheduler.engine.agent.data.sprayjson.TypedJson.WithFieldsJsObject
import com.sos.scheduler.engine.base.exceptions.StandardPublicException
import com.sos.scheduler.engine.tunnel.data.TunnelToken
import spray.json.DefaultJsonProtocol._
import spray.json._

/**
 * @author Joacim Zschimmer
 */
sealed trait StartTaskResponse extends Response {
  /** StartTaskFailed throws an exception, because it was unknown before v1.10.8 and v1.11.4. */
  def toLegacy: StartTaskSucceeded
}



final case class StartTaskSucceeded(
  agentTaskId: AgentTaskId,
  tunnelToken: TunnelToken)
extends StartTaskResponse {
  def toLegacy = this
}

object StartTaskSucceeded {
  implicit val MyJsonFormat = jsonFormat2(apply)
}



final case class StartTaskFailed(message: String) extends StartTaskResponse {
  def toLegacy: Nothing = throw new StandardPublicException(message)
}

object StartTaskFailed {
  implicit val MyJsonFormat = jsonFormat1(apply)
}



object StartTaskResponse {
  implicit val jsonFormat = new RootJsonFormat[StartTaskResponse] {

    def write(response: StartTaskResponse) = response match {
      case o: StartTaskSucceeded ⇒ o.toJson.asJsObject
      case o: StartTaskFailed ⇒ o.toJson.asJsObject withTypeField "StartTaskFailed"
    }

    def read(jsValue: JsValue) = {
      val jsObject = jsValue.asJsObject
      jsObject.fields.get("$TYPE") match {
        case Some(JsString("StartTaskSucceeded")) | None ⇒ jsObject.convertTo[StartTaskSucceeded]
        case Some(JsString("StartTaskFailed")) ⇒ jsObject.convertTo[StartTaskFailed]
        case typ ⇒ throw new IllegalArgumentException(s"Unknown JSON $$TYPE '$typ'")
      }
    }
  }
}
