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
import com.sos.scheduler.engine.test.SchedulerTestHelpers

@RunWith(classOf[JUnitRunner])
final class JS1049IT extends FreeSpec with ScalaSchedulerTest with SchedulerTestHelpers {

  override def onBeforeSchedulerActivation() {
    for (i <- JobIncludeSettings flatMap { _.includes })
      (controller.environment.liveDirectory / i.filename).write(i.content, i.encoding)
  }

  "XML configuration files" - {
    "scheduler.xml in UTF-8" in {
      instance[VariableSet].apply("Å") shouldEqual "å"
    }

    "job.xml in UTF-8" in {
      job(JobPath("/test-a")).getDescription shouldEqual "å"
    }

    for (j <- JobIncludeSettings; i = j.descriptionInclude)
      s"Include $i" in {
        (controller.environment.liveDirectory / i.filename).contentString(i.encoding) shouldEqual i.content
        job(j.jobPath).getDescription shouldEqual i.content
      }
  }

  "Job description by executeXml" in {
    (scheduler executeXml <show_job job={JobPath("/test-a").string} what="description"/>).string should include ("<description>å</description>")
  }

  "Job script with include" in {
    runJobAndWaitForEnd(textIncludeJobPath)
  }

  def job(o: JobPath): Job =
    instance[JobSubsystem].job(o)
}


private object JS1049IT {
  private case class Include(filename: String, encoding: Charset, content: String) {
    override def toString = s"$filename $encoding"
  }

  private case class JobIncludeSetting(
      jobPath: JobPath,
      descriptionInclude: Include,
      scriptInclude: Include) {
    def includes = List(descriptionInclude, scriptInclude)
  }

  private val textIncludeJobPath = JobPath("/test-text-include")
  private val xmlIncludeJobPath = JobPath("/test-xml-include")
  private val JobIncludeSettings = List(
    JobIncludeSetting(
      textIncludeJobPath,
      descriptionInclude = Include("test-description.txt", schedulerEncoding, "ö"),
      scriptInclude = Include("test-script.txt", schedulerEncoding, "exit 0")),
    JobIncludeSetting(
      xmlIncludeJobPath,
      descriptionInclude = Include("test-description.xhtml", UTF_8, "<p>ü</p>"),
      scriptInclude = Include("test-script.xml", UTF_8, "<p>ß</p>")))
}
