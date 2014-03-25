package com.sos.scheduler.engine.tests.scheduler.filebased

import FileBasedSubsystemIT._
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.ScalaCollections.emptyToNone
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.xml.XmlUtils.xmlBytesToString
import com.sos.scheduler.engine.data.filebased.TypedPath
import com.sos.scheduler.engine.data.filebased.TypedPath.ordering
import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.lock.LockPath
import com.sos.scheduler.engine.data.order.{OrderKey, OrderState}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.schedule.SchedulePath
import com.sos.scheduler.engine.kernel.filebased.{FileBasedState, FileBasedSubsystem}
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.{JobState, JobSubsystem}
import com.sos.scheduler.engine.kernel.lock.LockSubsystem
import com.sos.scheduler.engine.kernel.order.jobchain.{EndNodeOverview, JobChainDetails, JobNodeOverview, NestedJobChainNodeOverview}
import com.sos.scheduler.engine.kernel.order.{Order, OrderSubsystem, StandingOrderSubsystem}
import com.sos.scheduler.engine.kernel.processclass.ProcessClassSubsystem
import com.sos.scheduler.engine.kernel.schedule.ScheduleSubsystem
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import org.joda.time.Instant.now
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.immutable
import scala.util.Try

@RunWith(classOf[JUnitRunner])
final class FileBasedSubsystemIT extends FreeSpec with ScalaSchedulerTest {

  private lazy val jobSubsystem = injector.apply[JobSubsystem]
  private lazy val fileBasedSubsystemRegister = injector.apply[FileBasedSubsystem.Register]

  "FileBasedSubsystem.Register" in {
    fileBasedSubsystemRegister.companions.toSet shouldEqual (testSettings map { _.subsystemCompanion }).toSet
  }

  for (setting <- testSettings) {
    import setting._

    lazy val subsystem = injector.getInstance(subsystemCompanion.subsystemClass)
    subsystemCompanion.subsystemClass.getSimpleName - {

      "count" in {
        subsystem.count shouldEqual paths.size
      }

      "paths" in {
        subsystem.paths.sorted shouldEqual paths.sorted
      }

      "visiblePaths" in {
        subsystem.visiblePaths.sorted shouldEqual visiblePaths.sorted
      }

      "overview" in {
        val expectedFileBasedStates = Map(
          FileBasedState.notInitialized -> (paths count pathIsNotInitialized),
          FileBasedState.active -> (paths count { o ⇒ !pathIsNotInitialized(o) }))
        subsystem.overview should have (
          'fileBasedType (subsystemCompanion.fileBasedType),
          'count (paths.size),
          'fileBasedStateCounts (expectedFileBasedStates filter { _._2 != 0 } ))
      }

      for (path <- testPaths map { _.asInstanceOf[subsystem.Path] }) {
        def expectedFileBasedState = if (pathIsNotInitialized(path)) FileBasedState.notInitialized else FileBasedState.active
        s"fileBased $path" - {
          lazy val o = subsystem.fileBased(path)

          "path" in {
            o.path shouldEqual path
          }

          "name" in {
            o.name shouldEqual path.name
          }

          "configurationXmlBytes" in {
            if (pathDontHasXml(path))
              o.configurationXmlBytes.toSeq shouldBe 'empty
            else
              xmlBytesToString(o.configurationXmlBytes) should include ("<" + subsystem.companion.fileBasedType.cppName + " ")
          }

          "file" in {
            if (pathDontHasXml(path))
              intercept[RuntimeException] { o.file }
            else
              o.file shouldEqual testEnvironment.fileFromPath(path)
          }

          "fileBasedState" in {
            o.fileBasedState shouldEqual expectedFileBasedState
          }

          "stringToPath" in {
            o.stringToPath(path.string) shouldEqual path
            o.stringToPath(path.string).getClass shouldEqual path.getClass
          }

          "isVisible" in {
            o.isVisible shouldEqual (!(predefinedPaths contains path) || predefinedIsVisible)
          }

          "hasBaseFile" in {
            o.hasBaseFile shouldEqual !pathDontHasXml(path)
//            path match {
//              case _: FolderPath ⇒ o.hasBaseFile shouldBe false
//              case _ ⇒ o.hasBaseFile shouldEqual !(predefinedPaths contains path)
//            }
          }

          "overview" in {
            o.overview should have (
              'path (path),
              'fileBasedState (expectedFileBasedState))
          }

          "details" in {
            o.details should have (
              'path (path),
              'fileBasedState (expectedFileBasedState),
              'file (Try(o.file).toOption),
              'sourceXml (emptyToNone(o.sourceXmlBytes) map xmlBytesToString))
            if (o.hasBaseFile)
              o.details.fileModificationInstant.get should (be >= (now() - 30.s) and be <= now())
            else
              o.details.fileModificationInstant shouldBe None
          }

          "toString" in {
            (o, path) match {
              case (o: Order, path: OrderKey) ⇒ o.toString should (include (path.jobChainPath.string) and include (path.id.string))
              case _ ⇒ o.toString should include (path.string)
            }
          }

          subsystemCompanion match {
            case ProcessClassSubsystem ⇒
              "stringToPath accepts empty string" in {
                  o.stringToPath("") shouldEqual ProcessClassPath("")  // There is the default process class named ""
              }
            case _ ⇒
              "stringToPath rejects empty string" in {
                intercept[RuntimeException] { o.stringToPath("") }
              }
          }
        }
      }
    }
  }

  "JobSubsystemOverview" in {
    jobSubsystem.overview should have (
      'jobStateCounts (Map(JobState.pending -> jobSubsystemSetting.paths.size))
    )
  }

  "JobChainDetails" - {
    "normal job chain" in {
      val details: JobChainDetails = jobChain(JobChainPath("/test-jobChain")).details
      details.nodes(0).asInstanceOf[JobNodeOverview] should have (
        'orderState (OrderState("A")),
        'nextState (OrderState("SINK")),
        'errorState (OrderState("ERROR")),
        'jobPath (JobPath("/test-job-a")))
      details.nodes(1).asInstanceOf[JobNodeOverview] should have (
        'orderState (OrderState("SINK")),
        'nextState (OrderState("")),
        'errorState (OrderState("")),
        'jobPath (schedulerFileOrderSinkJobPath))
      details.nodes(2).asInstanceOf[EndNodeOverview] should have (
        'orderState (OrderState("ERROR")))
    }

    "nested job chain" in {
      val details: JobChainDetails = jobChain(JobChainPath("/test-jobChain-nested")).details
      details.nodes(0).asInstanceOf[NestedJobChainNodeOverview] should have (
        'orderState (OrderState("NESTED")),
        'nextState (OrderState("END")),
        'errorState (OrderState("")),   // Warum nicht automatisch gleich nextState?
        'nestedJobChainPath (JobChainPath("/NESTED")))
      details.nodes(1).asInstanceOf[EndNodeOverview] should have (
        'orderState (OrderState("END")))
    }
  }
}


private object FileBasedSubsystemIT {
  private case class TestSubsystemSetting(
    subsystemCompanion: FileBasedSubsystem.AnyCompanion,
    predefinedPaths: immutable.Seq[TypedPath],
    testPaths: immutable.Seq[TypedPath],
    predefinedIsVisible: Boolean = false) {

    val paths = predefinedPaths ++ testPaths
    val visiblePaths = paths filterNot pathIsInvisible
  }

  private val schedulerFileOrderSinkJobPath = JobPath("/scheduler_file_order_sink")
  private val schedulerServiceForwardingJobChainPath = JobChainPath("/scheduler_service_forwarding")
  private val schedulerServiceForwarderJobPath = JobPath("/scheduler_service_forwarder")
  private val emptyProcessClassPath = ProcessClassPath("")
  private val temporaryProcessClassPath = ProcessClassPath("/(temporaries)")
  private val rootFolderPath = FolderPath("/")
  //private val testNestedJobChainPath =

  private val pathDontHasXml = Set[TypedPath](
    schedulerFileOrderSinkJobPath,
    schedulerServiceForwarderJobPath,
    emptyProcessClassPath,
    temporaryProcessClassPath,
    rootFolderPath)
  
  private val pathIsInvisible = Set[TypedPath](
    schedulerServiceForwardingJobChainPath,
    schedulerServiceForwarderJobPath)

  private val pathIsNotInitialized = Set[TypedPath](JobChainPath("/test-jobChain-nested"))   // Die referenzierte Jobkette fehlt

  private val jobSubsystemSetting = TestSubsystemSetting(
      JobSubsystem,
      List(schedulerServiceForwarderJobPath),
      List(schedulerFileOrderSinkJobPath, JobPath("/test-job-a"), JobPath("/test-job-b")))

  private val testSettings = List(
    TestSubsystemSetting(
      FolderSubsystem,
      Nil,
      List(rootFolderPath)),
    jobSubsystemSetting,
    TestSubsystemSetting(
      LockSubsystem,
      Nil,
      List(LockPath("/test-lock"))),
    TestSubsystemSetting(
      OrderSubsystem,
      List(JobChainPath("/scheduler_service_forwarding")),
      List(JobChainPath("/test-jobChain"), JobChainPath("/test-jobChain-nested"))),
    TestSubsystemSetting(
      ProcessClassSubsystem,
      List(emptyProcessClassPath, temporaryProcessClassPath),
      List(ProcessClassPath("/test-processClass")),
      predefinedIsVisible = true),
    TestSubsystemSetting(
      ScheduleSubsystem,
      Nil,
      List(SchedulePath("/test-schedule"))),
    TestSubsystemSetting(
      StandingOrderSubsystem,
      Nil,
      List(OrderKey("/test-jobChain", "1"))))
}
