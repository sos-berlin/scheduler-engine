package com.sos.scheduler.engine.tests.xmlcommand.job_why

import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.scheduler.engine.common.xml.CppXmlUtils.{elementXPath, loadXml, toXml}
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.xmlcommand.job_why.JobWhyIT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import org.w3c.dom.Element
import scala.collection.breakOut

@RunWith(classOf[JUnitRunner])
final class JobWhyIT extends FreeSpec with ScalaSchedulerTest {

  private lazy val results: Map[JobPath, Element] = JobPaths .map { o ⇒ o → executeJobWhy(o) } (breakOut)

  override protected def onSchedulerActivated(): Unit = {
    results
    super.onSchedulerActivated()
  }

  "Job stopped" in {
    checkXPathForJobs("obstacle[@state='stopped']", CJobPath)
  }

  "Job min_tasks" in {
    checkXPathForJobs("start_reason.why[@min_tasks='1']/obstacle[@in_period='false']", MinTasksJobPath)
  }

  "Job max_tasks" in {
    checkXPathForJobs("obstacle[@max_tasks='0']", MaxTasksJobPath)
  }

  "Job chain" in {
    for ((k, v) ← results) elementXPath(v, jobchainXPath(k))
  }

  "Jobchain stopped" in {
    checkXPathForAllJobs("job_chain_nodes.why/job_chain_node.why/job_chain.why/obstacle[@state='stopped']")
  }

  private def executeJobWhy(jobPath: JobPath): Element = {
    val xmlString = scheduler executeXml s"<job.why job='${jobPath.string}'/>"
    val result = elementXPath(loadXml(xmlString), "/spooler/answer/job.why")
    logger.debug(s"$jobPath: ${toXml(result)}")
    result
  }

  private def checkXPathForAllJobs(xPath: String) = checkXPathForJobs(xPath, JobPaths: _*)

  private def checkXPathForJobs(xPath: String, selectedJobPaths: JobPath*): Unit = {
    for (jobPath ← selectedJobPaths) elementXPath(results(jobPath), xPath)
  }
}

object JobWhyIT {
  private val logger = Logger(getClass)
  private val AJobPath = JobPath("/a")
  private val BJobPath = JobPath("/b")
  private val CJobPath = JobPath("/c")
  private val MinTasksJobPath = JobPath("/minTasks")
  private val MaxTasksJobPath = JobPath("/maxTasks")
  private val JobPaths = List(AJobPath, BJobPath, CJobPath, MinTasksJobPath, MaxTasksJobPath)
  private val JJobChainPath = JobChainPath("/j")

  private def jobchainXPath(jobName: JobPath) = jobchainNodeXPath(jobName) + s"/job_chain.why[@path='${JJobChainPath.string}']"

  private def jobchainNodeXPath(jobPath: JobPath) = {
    val orderState = "state-" + jobPath.withoutStartingSlash
    s"job_chain_nodes.why/job_chain_node.why[@order_state='$orderState']"
  }
}
