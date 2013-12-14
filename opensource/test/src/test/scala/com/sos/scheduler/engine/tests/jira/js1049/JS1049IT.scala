package com.sos.scheduler.engine.tests.jira.js1049

import JS1049IT._
import com.google.common.base.Charsets.UTF_8
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits.RichFile
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.kernel.job.{Job, JobSubsystem}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerEncoding
import com.sos.scheduler.engine.kernel.variable.VariableSet
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import java.nio.charset.Charset
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._

@RunWith(classOf[JUnitRunner])
final class JS1049IT extends FreeSpec with ScalaSchedulerTest {

  override def onBeforeSchedulerActivation() {
    for (o <- IncludeTypes)
      (controller.environment.liveDirectory / o.filename).write(o.content, o.encoding)
  }

  "XML configuration files" - {
    "scheduler.xml in UTF-8" in {
      instance[VariableSet].apply("Å") shouldEqual "å"
    }

    "job.xml in UTF-8" in {
      job(JobPath("/test-a")).getDescription shouldEqual "å"
    }

    for (t <- IncludeTypes)
      s"Include file ${t.filename} in ${t.encoding}" in {
        (controller.environment.liveDirectory / t.filename).contentString(t.encoding) shouldEqual t.content
        job(t.jobPath).getDescription shouldEqual t.content
      }
  }

  "Scheduler.executeXml" in {
    (scheduler executeXml <show_job job={JobPath("/test-a").string} what="description"/>).string should include ("<description>å</description>")
  }

  def job(o: JobPath): Job =
    instance[JobSubsystem].job(o)
}

private object JS1049IT {
  private case class IncludeType(jobPath: JobPath, encoding: Charset, filename: String, content: String)
  private val IncludeTypes = List(
    IncludeType(JobPath("/test-text-include"), schedulerEncoding, "test-description.txt", "ö"),
    IncludeType(JobPath("/test-xml-include"), UTF_8, "test-description.xhtml", "<p>ü</p>"))
}
