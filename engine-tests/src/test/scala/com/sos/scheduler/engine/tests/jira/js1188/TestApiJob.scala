package com.sos.scheduler.engine.tests.jira.js1188

import java.io.{PrintWriter, StringWriter}
import org.scalatest.Matchers._

/**
 * @author Joacim Zschimmer
 */
final class TestApiJob extends sos.spooler.Job_impl {

  private val aTcpAddress = "127.0.0.1:1"
  private val bTcpAddress = "127.0.0.1:222"
  private val url = "http://127.0.0.1:1"

  override def spooler_process() =
    try {
      spooler.process_classes().process_class("/test-api-tcp") match { case processClass ⇒
        processClass.remote_scheduler shouldEqual aTcpAddress
        processClass.set_remote_scheduler(bTcpAddress)
        processClass.remote_scheduler shouldEqual bTcpAddress
        intercept[Exception] { processClass.set_remote_scheduler(url) }.getMessage should include ("COM-80070057")
      }
      spooler.process_classes().process_class("/test-api-http") match { case processClass ⇒
        assert(processClass.remote_scheduler startsWith "http://")
        intercept[Exception] { processClass.set_remote_scheduler(bTcpAddress) }.getMessage should include ("COM-80070005")
        intercept[Exception] { processClass.set_remote_scheduler(url) }
      }
      spooler.process_classes().process_class("/test-replace") match { case processClass ⇒
        intercept[Exception] { processClass.set_remote_scheduler(bTcpAddress) }.getMessage should include ("COM-80070005")
        intercept[Exception] { processClass.set_remote_scheduler(url) }
      }
      false
    } catch {
      case e: Exception ⇒
        val s = new StringWriter
        e.printStackTrace(new PrintWriter(s))
        spooler_log.error(s.toString)
        throw e
    }
}
