package com.sos.scheduler.engine.tests.jira.js1729

import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1729IT extends FreeSpec with ScalaSchedulerTest {

  "<show_calendar> returns all spaces in order ID" in {
    val result = scheduler.executeXml(<show_calendar before="2031-01-01T00:00:00" what="orders"/>)
    val orderId = (result.answer \ "calendar" \ "period" \ "@order").text
    assert(orderId == "two  spaces")
  }
}
