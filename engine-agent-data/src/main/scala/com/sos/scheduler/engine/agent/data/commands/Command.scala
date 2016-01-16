package com.sos.scheduler.engine.agent.data.commands

import com.sos.scheduler.engine.agent.data.sprayjson.TypedJson._
import spray.json._

/**
 * @author Joacim Zschimmer
 */
trait Command {
  type Response <: com.sos.scheduler.engine.agent.data.commandresponses.Response

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
        case o: CloseTask ⇒ o.toJson.asJsObject withTypeField CloseTask.SerialTypeName
        case o: DeleteFile ⇒ o.toJson.asJsObject withTypeField DeleteFile.SerialTypeName
        case o: MoveFile ⇒ o.toJson.asJsObject withTypeField MoveFile.SerialTypeName
        case o: RequestFileOrderSourceContent ⇒ o.toJson.asJsObject withTypeField RequestFileOrderSourceContent.SerialTypeName
        case o: SendProcessSignal ⇒ o.toJson.asJsObject withTypeField SendProcessSignal.SerialTypeName
        case o: StartApiTask ⇒ o.toJson.asJsObject withTypeField StartApiTask.SerialTypeName
        case o: StartNonApiTask ⇒ o.toJson.asJsObject withTypeField StartNonApiTask.SerialTypeName
        case o: Terminate ⇒ o.toJson.asJsObject withTypeField Terminate.SerialTypeName
        case AbortImmediately ⇒ JsObject() withTypeField AbortImmediately.SerialTypeName
        case o ⇒ throw new UnsupportedOperationException(s"Class ${o.getClass.getName} is not serializable to JSON")
      }

    def read(value: JsValue) =
      splitTypeAndJsObject(value) match {
        case (CloseTask.SerialTypeName, o) ⇒ o.convertTo[CloseTask]
        case (DeleteFile.SerialTypeName, o) ⇒ o.convertTo[DeleteFile]
        case (MoveFile.SerialTypeName, o) ⇒ o.convertTo[MoveFile]
        case (RequestFileOrderSourceContent.SerialTypeName, o) ⇒ o.convertTo[RequestFileOrderSourceContent]
        case (SendProcessSignal.SerialTypeName, o) ⇒ o.convertTo[SendProcessSignal]
        case (StartApiTask.SerialTypeName, o) ⇒ o.convertTo[StartApiTask]
        case (StartNonApiTask.SerialTypeName, o) ⇒ o.convertTo[StartNonApiTask]
        case (Terminate.SerialTypeName, o) ⇒ o.convertTo[Terminate]
        case (AbortImmediately.SerialTypeName, o) ⇒ o.convertTo[AbortImmediately.type]
        case (typeName, _) ⇒ throw new IllegalArgumentException(s"Unknown JSON $$TYPE '$typeName'")
      }
  }
}
