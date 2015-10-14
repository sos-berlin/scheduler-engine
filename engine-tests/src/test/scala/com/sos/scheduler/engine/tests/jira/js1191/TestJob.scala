package com.sos.scheduler.engine.tests.jira.js1191

import com.sos.scheduler.engine.tests.jira.js1191.TestJob._
import org.scalatest.Matchers._
import scala.xml.XML

/**
 * @author Joacim Zschimmer
 */
final class TestJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    val order = spooler_task.order
    order.params.value(LastState) match {
      case "" ⇒
      case s if s startsWith "NO-ERROR" ⇒
      case "ERROR" ⇒ assert(order.last_error contains "ERROR LOG LINE")
      case "EXCEPTION" ⇒
        assert(order.last_error contains "EXCEPTION THROWN")
    }

    val orderShow = spooler execute_xml <show_order job_chain={order.job_chain.path} order={order.id}/>.toString
    assert((XML.loadString(orderShow) \ "answer" \ "order" \ "@last_error").text == order.last_error)

    order.params.set_value(LastState, order.state)
    order.state match {
      case s if s startsWith "NO-ERROR" ⇒
        true
      case "ERROR" ⇒
        spooler_log.error("ERROR LOG LINE")
        false
      case "EXCEPTION" ⇒
        throw new RuntimeException("EXCEPTION THROWN")
      case "LAST" ⇒
        true
    }
  }
}

object TestJob {
  private val LastState = "LAST-STATE"
}
