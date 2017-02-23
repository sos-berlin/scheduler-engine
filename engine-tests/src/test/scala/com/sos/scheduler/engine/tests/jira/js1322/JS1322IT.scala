package com.sos.scheduler.engine.tests.jira.js1322

import com.sos.jobscheduler.common.scalautil.xmls.ScalaXmls.implicits.RichXmlFile
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.kernel.folder.FolderSubsystemClient
import com.sos.scheduler.engine.test.SchedulerTestUtils.{job, jobOverview}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1322 XmlConfigurationChangingPlugin.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1322IT extends FreeSpec with ScalaSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(getClass, terminateOnError = false)

  "A job defined in scheduler.xml is not processed" in {
    assert(job(JobPath("/scheduler.xml")).title == "scheduler.xml")
  }

  private val testJobPath = JobPath("/test")

  "Initial reading of job configuration is processed by TestPlugin" in {
    assert(jobOverview(testJobPath).fileBasedState == FileBasedState.active)
    assert(job(testJobPath).title == s"TÄST-TITLE - TEST-SUFFIX")
  }

  "Reading of changed job configuration is processed by TestPlugin" in {
    val otherTitle = "OTHER TITLE"
    testEnvironment.fileFromPath(testJobPath).xml = <job title={otherTitle}><script language="shell">#</script></job>
    instance[FolderSubsystemClient].updateFolders()
    assert(jobOverview(testJobPath).fileBasedState == FileBasedState.active)
    assert(job(testJobPath).title == s"$otherTitle - TEST-SUFFIX")
  }

  "After exception in Plugin the job's state is 'undefined'" in {
    assert(jobOverview(JobPath("/error")).fileBasedState == FileBasedState.undefined)
  }
}
