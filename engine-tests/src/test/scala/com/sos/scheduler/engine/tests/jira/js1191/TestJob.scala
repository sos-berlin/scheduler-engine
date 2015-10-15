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
    for (previousState ← order.job_chain.states sliding 2 collectFirst { case Array(previous, state) if state == order.state ⇒ previous }) {
      settingFor(previousState).check(order.last_error)
    }
    for (orderShow ← Some(spooler execute_xml <show_order job_chain={order.job_chain.path} order={order.id}/>.toString))
      assert((XML.loadString(orderShow) \ "answer" \ "order" \ "@last_error").text == order.last_error)
    settingFor(order.state).spooler_process(this)
  }
}

private[js1191] object TestJob {
  private[js1191] case class Setting(spooler_process: sos.spooler.Job_impl ⇒ Boolean, check: String ⇒ Unit)

  private[js1191] def settingFor(orderState: String) = orderState match {
    case o if o startsWith "NO-ERROR-" ⇒ Setting(
      job ⇒ true,
      lastError ⇒ assert(lastError.isEmpty))
    case o if o startsWith "NO-ERROR-FALSE" ⇒ Setting(
      job ⇒ false,
      lastError ⇒ assert(lastError.isEmpty))
    case "SHELL" ⇒ Setting(
      job ⇒ throw new NotImplementedError,
      lastError ⇒ assert(lastError == "SCHEDULER-280  Process terminated with exit code 7 (0x7)"))
    case "LOG-ERROR-TRUE" ⇒ Setting(
      job ⇒ { job.spooler_log.error("ERROR LOG LINE, RETURN TRUE"); true },
      lastError ⇒ assert(lastError.isEmpty))
    case "LOG-ERROR-FALSE" ⇒ Setting(
      job ⇒ { job.spooler_log.error("ERROR LOG LINE AND RETURNING FALSE"); false },
      lastError ⇒ assert(lastError == s"SCHEDULER-140  Task logged error: ERROR LOG LINE AND RETURNING FALSE"))
    case "EXCEPTION" ⇒ Setting(
      job ⇒ throw new RuntimeException("EXCEPTION THROWN"),
      lastError ⇒ assert(lastError contains "EXCEPTION THROWN"))
    case "LAST" ⇒ Setting(
      job ⇒ true,
      lastError ⇒ assert(lastError.isEmpty))
  }
}
