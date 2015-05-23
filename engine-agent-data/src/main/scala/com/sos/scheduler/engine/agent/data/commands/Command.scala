package com.sos.scheduler.engine.agent.data.commands

import spray.json._

/**
 * @author Joacim Zschimmer
 */
trait Command {
  type Response <: com.sos.scheduler.engine.agent.data.commands.Response
}

object Command {

  implicit class WithFieldsJsObject(val delegate: JsObject) extends AnyVal {
    def withTypeField(typeName: String) = JsObject(delegate.fields + ("$TYPE" → JsString(typeName)))
  }

  def splitTypeAndJsObject(value: JsValue): (String, JsObject) =
    value.asJsObject.fields("$TYPE").asInstanceOf[JsString].value → JsObject(value.asJsObject.fields - "$TYPE")

  implicit object MyJsonFormat extends RootJsonFormat[Command] {
    def write(command: Command) =
      command match {
        case o: RequestFileOrderSourceContent ⇒ o.toJson.asJsObject withTypeField RequestFileOrderSourceContent.SerialTypeName
        case o ⇒ throw new UnsupportedOperationException(s"Class ${o.getClass.getName} is not serializable to JSON")
      }

    def read(value: JsValue) =
      splitTypeAndJsObject(value) match {
        case (RequestFileOrderSourceContent.SerialTypeName, o) ⇒ o.convertTo[RequestFileOrderSourceContent]
        case (typeName, _) ⇒ throw new IllegalArgumentException(s"Unknown JSON $$TYPE '$typeName'")
      }
  }
}
