package com.sos.scheduler.engine.agent.data.commandresponses

import com.sos.scheduler.engine.agent.data.AgentTaskId
import com.sos.scheduler.engine.agent.data.sprayjson.TypedJson.{WithFieldsJsObject, splitTypeAndJsObject}
import com.sos.scheduler.engine.tunnel.data.TunnelToken
import spray.json.DefaultJsonProtocol._
import spray.json._

/**
 * @author Joacim Zschimmer
 */
sealed trait StartTaskResponse extends Response

final case class StartTaskSucceeded(
  agentTaskId: AgentTaskId,
  tunnelToken: TunnelToken)
extends StartTaskResponse

object StartTaskSucceeded {
  implicit val MyJsonFormat = jsonFormat2(apply)
}

final case class StartTaskFailed(message: String) extends StartTaskResponse

object StartTaskFailed {
  implicit val MyJsonFormat = jsonFormat1(apply)
}

object StartTaskResponse {
  implicit val jsonFormat = new RootJsonFormat[StartTaskResponse] {

    def write(response: StartTaskResponse) = response match {
      case o: StartTaskSucceeded ⇒ o.toJson.asJsObject withTypeField "StartTaskSucceeded"
      case o: StartTaskFailed ⇒ o.toJson.asJsObject withTypeField "StartTaskFailed"
    }

    def read(jsValue: JsValue) =
      splitTypeAndJsObject(jsValue) match {
        case ("StartTaskSucceeded", v) ⇒ v.convertTo[StartTaskSucceeded]
        case ("StartTaskFailed", v) ⇒ v.convertTo[StartTaskFailed]
        case (typeName, _) ⇒ throw new IllegalArgumentException(s"Unknown JSON $$TYPE '$typeName'")
      }
  }
}
