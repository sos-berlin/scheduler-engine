package com.sos.scheduler.engine.tests.scheduler.filebased

import FileBasedSubsystemIT._
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.common.xml.XmlUtils.xmlBytesToString
import com.sos.scheduler.engine.data.filebased.TypedPath
import com.sos.scheduler.engine.data.filebased.TypedPath.ordering
import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.lock.LockPath
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.schedule.SchedulePath
import com.sos.scheduler.engine.kernel.filebased.{FileBasedState, FileBasedSubsystem}
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.{JobState, JobSubsystem}
import com.sos.scheduler.engine.kernel.lock.LockSubsystem
import com.sos.scheduler.engine.kernel.order.{StandingOrderSubsystem, OrderSubsystem, Order}
import com.sos.scheduler.engine.kernel.processclass.ProcessClassSubsystem
import com.sos.scheduler.engine.kernel.schedule.ScheduleSubsystem
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.immutable

@RunWith(classOf[JUnitRunner])
final class FileBasedSubsystemIT extends FreeSpec with ScalaSchedulerTest {

  "FileBasedSubsystem.Register" in {
    injector.apply[FileBasedSubsystem.Register].companions.toSet shouldEqual (testSettings map { _.subsystemCompanion }).toSet
  }

  for (setting <- testSettings) {
    import setting._

    lazy val subsystem = injector.getInstance(subsystemCompanion.subsystemClass)
    subsystemCompanion.subsystemClass.getSimpleName - {

      "count" in {
        subsystem.count shouldEqual size
      }

      "paths" in {
        subsystem.paths.sorted shouldEqual (predefinedPaths ++ testPaths).sorted
      }

      "visiblePaths" in {
        val expected = testPaths ++ (if (predefinedIsVisible) predefinedPaths else Nil)
        subsystem.visiblePaths.sorted shouldEqual expected.sorted
      }

      "overview" in {
        subsystem.overview should have (
          'fileBasedType (subsystemCompanion.fileBasedType),
          'count (size),
          'fileBasedStateCounts (Map(FileBasedState.active -> (predefinedPaths.size + testPaths.size)))
        )
      }

      for (path <- testPaths map { _.asInstanceOf[subsystem.Path] }) {
        s"fileBased $path" - {
          lazy val o = subsystem.fileBased(path)

          "path" in {
            o.path shouldEqual path
          }

          "name" in {
            o.name shouldEqual path.name
          }

          "configurationXmlBytes" in {
            if ((subsystemCompanion eq FolderSubsystem) || (subsystemCompanion eq ProcessClassSubsystem) && (predefinedPaths contains path))
              o.configurationXmlBytes.toSeq shouldBe 'empty
            else
              xmlBytesToString(o.configurationXmlBytes) should include ("<" + subsystem.companion.fileBasedType.cppName + " ")
          }

          "file" in {
            if ((subsystemCompanion eq FolderSubsystem) || (subsystemCompanion eq ProcessClassSubsystem) && (predefinedPaths contains path))
              intercept[RuntimeException] { o.file }
            else
              o.file shouldEqual testEnvironment.fileFromPath(path)
          }

          "fileBasedState" in {
            o.fileBasedState shouldEqual FileBasedState.active
          }

          "stringToPath" in {
            o.stringToPath(path.string) shouldEqual path
            o.stringToPath(path.string).getClass shouldEqual path.getClass
          }

          "isVisible" in {
            o.isVisible shouldEqual (!(predefinedPaths contains path) || predefinedIsVisible)
          }

          "hasBaseFile" in {
            path match {
              case _: FolderPath ⇒ o.hasBaseFile shouldBe false
              case _ ⇒ o.hasBaseFile shouldEqual !(predefinedPaths contains path)
            }
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
    injector.getInstance(classOf[JobSubsystem]).overview should have (
      'jobStateCounts (Map(JobState.pending -> jobSubsystemSetting.size))
    )
  }
}


private object FileBasedSubsystemIT {
  private case class TestSubsystemSetting(
    subsystemCompanion: FileBasedSubsystem.AnyCompanion,
    predefinedPaths: immutable.Seq[TypedPath],
    testPaths: immutable.Seq[TypedPath],
    predefinedIsVisible: Boolean = false) {

    def size = predefinedPaths.size + testPaths.size
  }

  private val jobSubsystemSetting = TestSubsystemSetting(
      JobSubsystem,
      List(JobPath("/scheduler_file_order_sink"), JobPath("/scheduler_service_forwarder")),
      List(JobPath("/test-job-a"), JobPath("/test-job-b")))

  private val testSettings = List(
    TestSubsystemSetting(
      FolderSubsystem,
      Nil,
      List(FolderPath("/"))),
    jobSubsystemSetting,
    TestSubsystemSetting(
      LockSubsystem,
      Nil,
      List(LockPath("/test-lock"))),
    TestSubsystemSetting(
      OrderSubsystem,
      List(JobChainPath("/scheduler_service_forwarding")),
      List(JobChainPath("/test-jobChain"))),
    TestSubsystemSetting(
      ProcessClassSubsystem,
      List(ProcessClassPath(""), ProcessClassPath("/(temporaries)")),
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
