package com.sos.scheduler.engine.tests.jira.js644.v3

import com.google.common.base.Charsets.UTF_8
import com.google.common.io.Files
import com.sos.scheduler.engine.common.sync.Gate
import com.sos.jobscheduler.data.event.KeyedEvent
import com.sos.jobscheduler.data.filebased.TypedPath
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderFinished
import com.sos.scheduler.engine.data.scheduler.SchedulerTerminatedEvent
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js644.v3.JS644v3IT._
import java.io.File
import java.lang.Thread.sleep
import java.time.Duration
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import org.slf4j.{Logger, LoggerFactory}

@RunWith(classOf[JUnitRunner])
final class JS644v3IT extends FreeSpec with ScalaSchedulerTest {

  private val lowerCaseGate = new Gate[Boolean](JS644v3IT.LowerCaseJobChainPath.toString)
  private val upperCaseGate = new Gate[Boolean](JS644v3IT.UpperCaseJobChainPath.toString)

  "test" in {
    try prepare()
    catch {
      case t: Throwable ⇒ throw new Error(s"VOM TEST UNBEABSICHTIGTER FEHLER: $t", t)
    }
    runOrders()
  }

  private def prepare() {
    runOrders() // zwei Mal nur zum Beleg, dass runOrders() funktioniert
    runOrders()
    modify(LowerCaseJobChainPath)
    modify(UpperCaseJobChainPath)
    sleep(100)
    modify(TestJobPath)
    sleep(4000)
  }

  private def modify(p: TypedPath) {
    val file: File = controller.environment.fileFromPath(p)
    val text: String = Files.toString(file, UTF_8)
    Files.write(text + " ", file, UTF_8)
  }

  private def runOrders() {
    scheduler.executeXml("<modify_order job_chain='" + LowerCaseJobChainPath.string + "' order='1' at='now'/>")
    scheduler.executeXml("<modify_order job_chain='" + UpperCaseJobChainPath.string + "' order='1' at='now'/>")
    waitForFinishedOrder(lowerCaseGate)
    waitForFinishedOrder(upperCaseGate)
  }

  eventBus.on[OrderFinished] {
    case KeyedEvent(orderKey, e: OrderFinished) ⇒
      val gate = if (orderKey.jobChainPath.equals(LowerCaseJobChainPath)) lowerCaseGate else upperCaseGate
      gate.put(true)
  }

  eventBus.on[SchedulerTerminatedEvent] { case _ ⇒
    lowerCaseGate.put(false)
  }
}

private object JS644v3IT {
  private val logger: Logger = LoggerFactory.getLogger(classOf[JS644v3IT])
  private val LowerCaseJobChainPath: JobChainPath = new JobChainPath("/lowerCase")
  private val UpperCaseJobChainPath: JobChainPath = new JobChainPath("/upperCase")
  private val TestJobPath: JobPath = new JobPath("/a")
  private val orderTimeout: Duration = Duration.ofSeconds(20)

  private def waitForFinishedOrder(gate: Gate[Boolean]): Boolean = {
    Option(gate.poll(orderTimeout)) match {
      case None ⇒ sys.error("An order has not been finished in time")
      case Some(ok) ⇒
        if (ok) logger.debug("An order has been finished")
        ok
    }
  }
}
