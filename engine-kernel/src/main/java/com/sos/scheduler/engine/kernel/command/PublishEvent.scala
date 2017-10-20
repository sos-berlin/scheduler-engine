package com.sos.scheduler.engine.kernel.command

import com.sos.scheduler.engine.data.event.KeyedTypedEventJsonFormat.KeyedSubtype
import com.sos.scheduler.engine.data.event.{Event, KeyedEvent, KeyedTypedEventJsonFormat}
import com.sos.scheduler.engine.data.events.custom.CustomEvent
import com.sos.scheduler.engine.eventbus.EventBus
import org.w3c.dom.Element
import spray.json._

/**
  * @author Joacim Zschimmer
  */
object PublishEvent {

  private val KeyedCustomEventJsonFormat: KeyedTypedEventJsonFormat[Event] =
    KeyedEvent.typedJsonFormat[Event](
      KeyedSubtype[CustomEvent])

  def commandHandlers(eventBus: EventBus): List[CommandHandler] =
    Parser :: new Executor(eventBus) :: Result.Xmlizer :: Nil

  private object Parser extends GenericCommandXmlParser[Cmd]("publish_event") {
    def doParse(element: Element) = {
      val text = element.getTextContent
      KeyedCustomEventJsonFormat.read(text.parseJson) match {
        case KeyedEvent(key: CustomEvent.Key, event: CustomEvent) ⇒
          Cmd(KeyedEvent(event)(key))
        case x ⇒
          println(x)
          throw new IllegalArgumentException("Only a CustomEvent is accepted")
      }
    }
  }

  private case class Cmd(keyedEvent: KeyedEvent[CustomEvent]) extends Command

  private class Executor(eventBus: EventBus) extends CommandExecutor {
    def getCommandClass = classOf[Cmd]

    def execute(command: Command): Result = {
      val cmd = command.asInstanceOf[Cmd]
      eventBus.publish(cmd.keyedEvent)
      Result.OK
    }
  }
}
