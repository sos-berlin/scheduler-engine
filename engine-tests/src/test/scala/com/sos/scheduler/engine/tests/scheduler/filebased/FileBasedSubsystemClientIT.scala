package com.sos.scheduler.engine.tests.scheduler.filebased

import com.sos.jobscheduler.common.guice.GuiceImplicits._
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.data.filebased.TypedPath
import com.sos.jobscheduler.data.filebased.TypedPath.ordering
import com.sos.jobscheduler.data.folder.FolderPath
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.job.{JobPath, JobState}
import com.sos.scheduler.engine.data.jobchain._
import com.sos.scheduler.engine.data.lock.LockPath
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.schedule.SchedulePath
import com.sos.scheduler.engine.kernel.filebased.{FileBased, FileBasedSubsystem}
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.{JobSubsystem, JobSubsystemClient}
import com.sos.scheduler.engine.kernel.lock.LockSubsystem
import com.sos.scheduler.engine.kernel.order.{Order, OrderSubsystem, StandingOrderSubsystem}
import com.sos.scheduler.engine.kernel.processclass.ProcessClassSubsystem
import com.sos.scheduler.engine.kernel.schedule.ScheduleSubsystem
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.scheduler.filebased.FileBasedSubsystemClientIT._
import java.time.Instant.now
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.immutable
import scala.util.Try

@RunWith(classOf[JUnitRunner])
final class FileBasedSubsystemClientIT extends FreeSpec with ScalaSchedulerTest {

  private lazy val jobSubsystemClient = injector.instance[JobSubsystemClient]
  private lazy val fileBasedSubsystemRegister = injector.instance[FileBasedSubsystem.Register]

  "FileBasedSubsystem.Register" in {
    fileBasedSubsystemRegister.companions.toSet shouldEqual (testSettings map { _.subsystemDescription }).toSet
  }

  for (setting <- testSettings) {
    import setting._

    lazy val subsystem = injector.getInstance(subsystemDescription.clientClass)
    subsystemDescription.subsystemClass.getSimpleName - {

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
          FileBasedState.not_initialized → (paths count pathIsNotInitialized),
          FileBasedState.active → (paths count { o ⇒ !pathIsNotInitialized(o) }))
        subsystem.fileBasedSubsystemOverview should have (
          'fileBasedType (subsystemDescription.fileBasedType),
          'count (paths.size),
          'fileBasedStateCounts (expectedFileBasedStates filter { _._2 != 0 } ))
      }

      for (path ← testPaths map { _.asInstanceOf[subsystem.ThisPath] }) {
        def expectedFileBasedState = if (pathIsNotInitialized(path)) FileBasedState.not_initialized else FileBasedState.active
        s"fileBased $path" - {
          lazy val fileBased: FileBased = subsystem.fileBased(path)
          lazy val fileBasedOverview = subsystem.fileBasedOverview(path)
          lazy val fileBasedDetailed = subsystem.fileBasedDetailed(path)

          "path" in {
            fileBasedOverview.path shouldEqual path
          }

          "configurationXmlBytes" in {
            if (pathDontHasXml(path))
              assert(fileBasedDetailed.sourceXml.isEmpty)
            else
              fileBasedDetailed.sourceXml.get should include ("<" + subsystem.companion.fileBasedType.cppName + " ")
          }

          "file" in {
            if (pathDontHasXml(path))
              assert(fileBasedDetailed.file.isEmpty)
            else
              assert(fileBasedDetailed.file == Some(testEnvironment.fileFromPath(path).toPath))
          }

          "fileBasedState" in {
            fileBasedOverview.fileBasedState shouldEqual expectedFileBasedState
          }

          "stringToPath" in {
            fileBased.stringToPath(path.string) shouldEqual path
            fileBased.stringToPath(path.string).getClass shouldEqual path.getClass
          }

          "isVisible" in {
            fileBased.isVisible shouldEqual (!(predefinedPaths contains path) || predefinedIsVisible)
          }

          "isFileBased" in {
            fileBased.isFileBased shouldEqual !pathDontHasXml(path)
//            path match {
//              case _: FolderPath ⇒ o.isFileBased shouldBe false
//              case _ ⇒ o.isFileBased shouldEqual !(predefinedPaths contains path)
//            }
          }

          "overview" in {
            fileBasedOverview should have (
              'path (path),
              'fileBasedState (expectedFileBasedState))
          }

          "detailed" in {
            assert(fileBasedDetailed.overview.fileBasedState == expectedFileBasedState)
            assert(fileBasedDetailed.path == path)
            assert(fileBasedDetailed.file == Try(fileBased.file).toOption)
            if (fileBased.isFileBased) {
              assert(fileBasedDetailed.sourceXml.get startsWith "<")
              fileBasedDetailed.fileModifiedAt.get should (be >= (now() - 30.s) and be <= now())
            } else
              fileBasedDetailed.fileModifiedAt shouldBe None
          }

          "toString" in {
            (fileBased, path) match {
              case (o: Order, orderKey: OrderKey) ⇒ o.toString should include (orderKey.id.string)
              case _ ⇒ fileBased.toString should include (path.string)
            }
          }

          subsystemDescription match {
            case ProcessClassSubsystem ⇒
              "stringToPath accepts empty string" in {
                fileBased.stringToPath("") shouldEqual ProcessClassPath("")  // There is the default process class named ""
              }
            case _ ⇒
              "stringToPath rejects empty string" in {
                intercept[RuntimeException] { fileBased.stringToPath("") }
              }
          }
        }
      }
    }
  }

  "JobSubsystemOverview" in {
    jobSubsystemClient.fileBasedSubsystemOverview should have (
      'jobStateCounts (Map(JobState.pending → jobSubsystemSetting.paths.size))
    )
  }

  "JobChainDetailed" - {
    "normal job chain" in {
      val details: JobChainDetailed = jobChainDetailed(testJobChainPath)
      details.nodes(0).asInstanceOf[SimpleJobNodeOverview] should have (
        'nodeKey (NodeKey(testJobChainPath, NodeId("A"))),
        'nextNodeId (NodeId("SINK")),
        'errorNodeId (NodeId("ERROR")),
        'jobPath (JobPath("/test-job-a")))
      details.nodes(1).asInstanceOf[SinkNodeOverview] should have (
        'nodeKey (NodeKey(testJobChainPath, NodeId("SINK"))),
        'nextNodeId (NodeId("")),
        'errorNodeId (NodeId("")),
        'jobPath (schedulerFileOrderSinkJobPath))
      details.nodes(2).asInstanceOf[EndNodeOverview] should have (
        'nodeKey (NodeKey(testJobChainPath, NodeId("ERROR"))))
    }

    "nested job chain" in {
      val details: JobChainDetailed = jobChainDetailed(nestedJobChainPath)
      details.nodes(0).asInstanceOf[NestedJobChainNodeOverview] should have (
        'nodeKey (NodeKey(nestedJobChainPath, NodeId("NESTED"))),
        'nextNodeId (NodeId("END")),
        'errorNodeId (NodeId("")),   // Warum nicht automatisch gleich nextNodeId?
        'nestedJobChainPath (JobChainPath("/NESTED")))
      details.nodes(1).asInstanceOf[EndNodeOverview] should have (
        'nodeKey (NodeKey(nestedJobChainPath, NodeId("END"))))
    }
  }
}


private object FileBasedSubsystemClientIT {
  private case class TestSubsystemSetting(
    subsystemDescription: FileBasedSubsystem.Companion,
    predefinedPaths: immutable.Seq[TypedPath],
    testPaths: immutable.Seq[TypedPath],
    predefinedIsVisible: Boolean = false) {

    val paths = predefinedPaths ++ testPaths
    val visiblePaths = paths filterNot pathIsInvisible
  }

  private val testJobChainPath = JobChainPath("/test-jobChain")
  private val nestedJobChainPath = JobChainPath("/test-jobChain-nested")
  private val schedulerFileOrderSinkJobPath = JobPath("/scheduler_file_order_sink")
  private val schedulerServiceForwardingJobChainPath = JobChainPath("/scheduler_service_forwarding")
  private val schedulerServiceForwarderJobPath = JobPath("/scheduler_service_forwarder")
  private val emptyProcessClassPath = ProcessClassPath("")
  //private val testNestedJobChainPath =

  private val pathDontHasXml = Set[TypedPath](
    schedulerFileOrderSinkJobPath,
    schedulerServiceForwarderJobPath,
    emptyProcessClassPath,
    FolderPath.Root)

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
      List(FolderPath.Root)),
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
      List(emptyProcessClassPath),
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
