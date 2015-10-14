package com.sos.scheduler.engine.tests.jira.js1191

import com.sos.scheduler.engine.tests.jira.js1191.TestJob._
import org.scalatest.Matchers._
import scala.xml.XML

/**
 * @author Joacim Zschimmer
 */
final class TestJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    for (previousState ← order.job_chain.states sliding 2 collectFirst { case Array(previous, state) if state == order.state ⇒ previous })
      settingFor(previousState).check()
    for (orderShow ← Some(spooler execute_xml <show_order job_chain={order.job_chain.path} order={order.id}/>.toString))
      assert((XML.loadString(orderShow) \ "answer" \ "order" \ "@last_error").text == order.last_error)
    settingFor(order.state).spooler_process()
  }

  private def settingFor(orderState: String) = orderState match {
    case o if o startsWith "NO-ERROR-" ⇒ Setting(
      () ⇒ true,
      () ⇒ assert(order.last_error.isEmpty))
    case "SHELL" ⇒ Setting(
      () ⇒ throw new NotImplementedError,
      () ⇒ assert(order.last_error == "SCHEDULER-280  Process terminated with exit code 7 (0x7)"))
    case "LOG-ERROR-TRUE" ⇒ Setting(
      () ⇒ { spooler_log.error("ERROR LOG LINE, RETURN TRUE"); true },
      () ⇒ assert(order.last_error.isEmpty))
    case "LOG-ERROR-FALSE" ⇒ Setting(
      () ⇒ { spooler_log.error("ERROR LOG LINE AND RETURNING FALSE"); false },
      () ⇒ assert(order.last_error == s"SCHEDULER-140  Task logged error: ERROR LOG LINE AND RETURNING FALSE"))
    case "EXCEPTION" ⇒ Setting(
      () ⇒ throw new RuntimeException("EXCEPTION THROWN"),
      () ⇒ assert(order.last_error contains "EXCEPTION THROWN"))
    case "LAST" ⇒ Setting(
      () ⇒ true,
      () ⇒ ())
  }

  def order = spooler_task.order
}

private object TestJob {
  private case class Setting(spooler_process: () ⇒ Boolean, check: () ⇒ Unit)
}
