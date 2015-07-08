package com.sos.scheduler.engine.agent.data.commands

import spray.json._
import com.sos.scheduler.engine.agent.data.sprayjson.TypedJson._

/**
 * @author Joacim Zschimmer
 */
trait Command {
  type Response <: com.sos.scheduler.engine.agent.data.responses.Response

  def toShortString = toString

  /**
   * true if toString returns a longer string than toShortString.
   */
  def toStringIsLonger = false
}

object Command {

  implicit object MyJsonFormat extends RootJsonFormat[Command] {
    def write(command: Command) =
      command match {
        case o: RequestFileOrderSourceContent ⇒ o.toJson.asJsObject withTypeField RequestFileOrderSourceContent.SerialTypeName
        case o: StartSeparateProcess ⇒ o.toJson.asJsObject withTypeField StartSeparateProcess.SerialTypeName
        case o: StartThread ⇒ o.toJson.asJsObject withTypeField StartThread.SerialTypeName
        case o: Terminate ⇒ o.toJson.asJsObject withTypeField Terminate.SerialTypeName
        case AbortImmediately ⇒ JsObject() withTypeField AbortImmediately.SerialTypeName
        case o ⇒ throw new UnsupportedOperationException(s"Class ${o.getClass.getName} is not serializable to JSON")
      }

    def read(value: JsValue) =
      splitTypeAndJsObject(value) match {
        case (RequestFileOrderSourceContent.SerialTypeName, o) ⇒ o.convertTo[RequestFileOrderSourceContent]
        case (StartSeparateProcess.SerialTypeName, o) ⇒ o.convertTo[StartSeparateProcess]
        case (StartThread.SerialTypeName, o) ⇒ o.convertTo[StartThread]
        case (Terminate.SerialTypeName, o) ⇒ o.convertTo[Terminate]
        case (AbortImmediately.SerialTypeName, o) ⇒ o.convertTo[AbortImmediately.type]
        case (typeName, _) ⇒ throw new IllegalArgumentException(s"Unknown JSON $$TYPE '$typeName'")
      }
  }
}
