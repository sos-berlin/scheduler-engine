package com.sos.scheduler.engine.eventbus

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.data.event.KeyedEvent.NoKey
import com.sos.scheduler.engine.data.event.{Event, NoKeyEvent}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class CachedSuperEventClassesTest extends FreeSpec {

  "test" in {
    //???
  }
}

object CachedSuperEventClassesTest {
  private val logger = Logger(getClass)

  private trait A
  private trait BEvent extends Event
  private trait CEvent extends Event
  private trait D

  private case object XObject extends A with BEvent with CEvent with D with NoKeyEvent

  private case object YObject extends BEvent {
    type Key = NoKey
  }
}
