package com.sos.scheduler.engine.tests.jira.js1048

import JS1048IT._
import com.google.common.io.Files
import com.sos.scheduler.engine.common.scalautil.AutoClosing._
import com.sos.scheduler.engine.common.scalautil.ScalaXmls.implicits._
import com.sos.scheduler.engine.data.folder.{FileBasedRemovedEvent, JobChainPath, FileBasedActivatedEvent}
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import java.io.File
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1048IT extends FreeSpec with ScalaSchedulerTest {

  import controller.environment.liveDirectory

  private lazy val orderSubsystem = instance[OrderSubsystem]

  "After reestablishing the job chain (this should be equivalent with a JobScheduler restart)" - {
    "... with an unchanged .order.xml, the previously modified title should remain" in {
      val myOrderKey = testJobChainPath orderKey "A"
      autoClosing(controller.newEventPipe()) { eventPipe =>
        orderSubsystem.order(myOrderKey).getTitle shouldEqual originalTitle
        orderSubsystem.order(myOrderKey).setTitle(commandModifiedTitle)
        removeAndReestablishJobChain(testJobChainPath) {}
        orderSubsystem.order(myOrderKey).getTitle shouldEqual commandModifiedTitle
      }
    }

//    "... with an unchanged .order.xml, the previous suspension should remain" in {
//      val myOrderKey = testJobChainPath orderKey "A"
//      autoClosing(controller.newEventPipe()) { eventPipe =>
//        orderSubsystem.order(myOrderKey).setSuspended(true)
//        orderSubsystem.order(myOrderKey).isSuspended shouldBe true
//        removeAndReestablishJobChain(testJobChainPath) {}
//        orderSubsystem.order(myOrderKey).isSuspended shouldBe true
//      }
//    }

    "... with a changed .order.xml, the previous modification of the title should be lost" in {
      val myOrderKey = testJobChainPath orderKey "B"
      autoClosing(controller.newEventPipe()) { eventPipe =>
        orderSubsystem.order(myOrderKey).getTitle shouldEqual originalTitle
        orderSubsystem.order(myOrderKey).setTitle(commandModifiedTitle)
        removeAndReestablishJobChain(testJobChainPath) {
          myOrderKey.file(liveDirectory).xml = <order title={fileChangedTitle}><run_time/></order>
        }
        orderSubsystem.order(myOrderKey).getTitle shouldEqual fileChangedTitle
      }
    }
  }

  private def removeAndReestablishJobChain(jobChainPath: JobChainPath)(f: => Unit) {
    autoClosing(controller.newEventPipe()) { eventPipe =>
      val file = jobChainPath.file(liveDirectory)
      val renamedFile = new File(s"$file~")
      Files.move(file, renamedFile)
      eventPipe.nextKeyed[FileBasedRemovedEvent](jobChainPath)
      f
      Files.move(renamedFile, file)
      eventPipe.nextKeyed[FileBasedActivatedEvent](jobChainPath)
    }
  }
}


private object JS1048IT {
  private val testJobChainPath = JobChainPath("/test")
  private val originalTitle = "TITLE"
  private val commandModifiedTitle = "COMMAND-MODIFIED"
  private val fileChangedTitle = "FILE-CHANGED"
}
