package com.sos.scheduler.engine.tests.jira.js1739

import com.sos.scheduler.engine.data.events.custom.VariablesCustomEvent
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1739IT extends FreeSpec with ScalaSchedulerTest {

  "<publish_event>" in {
    eventBus.awaiting[VariablesCustomEvent]("TEST-KEY") {
      scheduler.executeXml(
        <publish_event>
          {{
            "TYPE": "VariablesCustomEvent",
            "key": "TEST-KEY",
            "variables": {{
              "KEY": "VALUE",
              "a": "123"
            }}
          }}
        </publish_event>)
    } shouldEqual VariablesCustomEvent(Map(
      "KEY" → "VALUE",
      "a" → "123"
    ))
  }
}
