package com.sos.scheduler.engine.tests.scheduler.job.login

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskEndedEvent
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import java.util.regex.Pattern
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import scala.collection.JavaConversions._
import scala.util.matching.Regex

@RunWith(classOf[JUnitRunner])
class JobLoginTest extends ScalaSchedulerTest {
  import JobLoginTest._

  private val noLoginJobPath = JobPath.of("/testNoLogin")
  private val loginJobPath = JobPath.of("/testLogin")

  test("Job without <login>") {
    val expectedPropertyMap = (propertyNames map { name => name -> System.getProperty(name) }).toMap
    runJob(noLoginJobPath) should equal (expectedPropertyMap)
  }

  ignore("Job with <login>") {
    val properties = runJob(loginJobPath)
    properties("user.name") should equal ("-user name-")
    properties("user.dir") should equal ("-working directory-")
    properties("user.home") should equal ("-home directory-")
  }

  private def runJob(jobPath: JobPath) = {
    val eventPipe = controller.newEventPipe
    startJob(jobPath)
    eventPipe.nextWithCondition[TaskEndedEvent] { _.getJobPath == jobPath }
    jobPropertyMap(jobPath)
  }

  private def startJob(jobPath: JobPath) {
    scheduler executeXml <start_job job={jobPath.asString}/>
  }

  private def jobPropertyMap(jobPath: JobPath) = {
    val namePrefix = jobPath.getName +"."
    val NamePattern = new Regex(Pattern.quote(namePrefix) +"(.*)")
    scheduler.getVariables.toMap collect { case (NamePattern(propertyName), v) => propertyName -> v }
  }
}

object JobLoginTest {
  val propertyNames = List("user.name", "user.home", "user.dir")
}
