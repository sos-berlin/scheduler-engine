package com.sos.scheduler.engine.kernel.command

import com.sos.scheduler.engine.common.scalautil.Closers.withCloser
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.event.KeyedEvent
import com.sos.scheduler.engine.data.events.custom.{CustomEvent, VariablesCustomEvent}
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.collection.JavaConversions._
import scala.concurrent.Promise

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class PublishEventTest extends FreeSpec {

  "<publish_event>" in {
    val eventBus = new SchedulerEventBus
    val commandDispatcher = new CommandDispatcher(PublishEvent.commandHandlers(eventBus))

    withCloser { implicit closer ⇒
      val promise = Promise[KeyedEvent[VariablesCustomEvent]]()
      eventBus.onHot[VariablesCustomEvent] {
        case o ⇒ promise.success(o)
      }
      val resultXml = commandDispatcher.executeXml(
        <publish_event>
          {{
            "TYPE": "VariablesCustomEvent",
            "key": "TEST-KEY",
            "variables": {{
              "KEY": "VALUE",
              "a": "123"
            }}
          }}
        </publish_event>.toString())
      assert(SafeXML.loadString(resultXml).label == "ok")

      val keyedEvent = promise.future await 99.s
      assert(keyedEvent ==
        KeyedEvent(
          VariablesCustomEvent(Map(
            "KEY" → "VALUE",
            "a" → "123"
        )))(CustomEvent.Key("TEST-KEY")))
    }
  }
}
