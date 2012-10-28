package com.sos.scheduler.engine.tests.database

import com.sos.scheduler.engine.data.folder.{FileBasedActivatedEvent, JobChainPath, JobPath}
import com.sos.scheduler.engine.data.job.TaskClosedEvent
import com.sos.scheduler.engine.data.order.OrderId
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.{JobState, JobSubsystem}
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerTimeZone
import com.sos.scheduler.engine.kernel.settings.{SettingName, Settings}
import com.sos.scheduler.engine.persistence.entities.{TaskEntity, JobEntity, TaskHistoryEntity}
import com.sos.scheduler.engine.test.Environment.schedulerId
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.test.util.time.TimeoutWithSteps
import com.sos.scheduler.engine.test.util.time.WaitForCondition.waitForCondition
import javax.persistence.EntityManagerFactory
import org.joda.time.DateTime
import org.joda.time.DateTime.now
import org.joda.time.Duration.{millis, standardSeconds}
import org.joda.time.format.DateTimeFormat
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import scala.xml.XML

@RunWith(classOf[JUnitRunner])
final class EntitiesIT extends ScalaSchedulerTest {

  import EntitiesIT._

  private val testStartTime = now() withMillisOfSecond 0
  private lazy val jobSubsystem = controller.scheduler.instance[JobSubsystem]
  private lazy val orderJob = jobSubsystem.job(orderJobPath)
  private def simpleJob = scheduler.instance[JobSubsystem].job(simpleJobPath)

  override def checkedBeforeAll() {
    controller.useDatabase()
    controller.setLogCategories("java.stackTrace-")
    controller.setSettings(Settings.of(SettingName.useJavaPersistence, "true"))
    controller.activateScheduler()
    val eventPipe = controller.newEventPipe()
    scheduler executeXml <order job_chain={jobChainPath.asString} id={orderId.asString}/>
    eventPipe.nextWithCondition[TaskClosedEvent] { _.jobPath == orderJobPath }
    scheduler executeXml <start_job job={simpleJobPath.string} at="period"/>
    scheduler executeXml <start_job job={simpleJobPath.string} at="2029-10-11 22:33:44"/>
    scheduler executeXml <start_job job={simpleJobPath.string} at="2029-11-11 11:11:11"><params><param name="myJobParameter" value="myValue"/></params></start_job>
    simpleJob.forceFileReread()
    scheduler.instance[FolderSubsystem].updateFolders()
    //Warum geht das nicht: eventPipe.nextWithCondition[FileBasedRemovedEvent] { _.getTypedPath == simpleJobPath }
    eventPipe.nextWithCondition[FileBasedActivatedEvent] { _.getTypedPath == simpleJobPath }
  }

  lazy val entityManager = controller.scheduler.instance[EntityManagerFactory].createEntityManager()

  override def afterAll() {
    entityManager.close()
  }

  private lazy val taskHistoryEntities: Seq[TaskHistoryEntity] = entityManager.fetchSeq("select t from TaskHistoryEntity t order by t.id")

  test("TaskHistoryEntity") {
    taskHistoryEntities should have size (2)
  }

  test("First TaskHistoryEntity is from Scheduler start") {
    taskHistoryEntities(0) match { case e =>
      e should have (
        'id (firstTaskHistoryEntityId),
        'schedulerId (schedulerId.string),
        'clusterMemberId (null),
        'endTime (null),
        'jobPath ("(Spooler)"),
        'cause (null),
        'steps (null),
        'errorCode (null),
        'errorText (null)
      )
      assert(e.startTime.getTime >= testStartTime.getMillis, "TaskHistoryEntity.startTime="+e.startTime+" should not be before testStartTime="+testStartTime)
      now() match { case n => assert(e.startTime.getTime <= n.getMillis, "TaskHistoryEntity.startTime="+e.startTime+" should not be after now="+n) }
    }
  }

  test("Second TaskHistoryEntity is from "+orderJobPath) {
    taskHistoryEntities(1) match { case e =>
      e should have (
        'id (firstTaskHistoryEntityId + 1),
        'schedulerId (schedulerId.string),
        'clusterMemberId (null),
        'jobPath (orderJobPath.withoutStartingSlash),
        'cause ("order"),
        'steps (1),
        'errorCode (null),
        'errorText (null)
      )
      assert(e.startTime.getTime >= testStartTime.getMillis, "TaskHistoryEntity.startTime="+e.startTime+" should not be before testStartTime="+testStartTime)
      now() match { case n => assert(e.startTime.getTime <= n.getMillis, "TaskHistoryEntity.startTime="+e.startTime+" should not be after now="+n) }
    }
  }

  test("TaskEntity is written as expected") {
    val e = fetchTaskEntities(simpleJobPath)
    e should have size (3)

    e(0) should have (
      'taskId (firstTaskHistoryEntityId + 2),
      'schedulerId (schedulerId.asString),
      'clusterMemberId (null),
      'jobPath (simpleJobPath.withoutStartingSlash),
      'startTime (null),
      'parameterXml (null)
    )
    XML.loadString(e(0).xml) should equal (<task force_start="no"/>)
    assert(e(0).enqueueTime.getTime >= testStartTime.getMillis, "TaskEntity._enqueueTime="+ e(0).enqueueTime +" should not be before testStartTime="+testStartTime)
    assert(e(0).enqueueTime.getTime <= now().getMillis, "TaskEntity._enqueueTime="+ e(0).enqueueTime +" should not be after now")

    XML.loadString(e(1).xml) should equal (<task force_start="yes"/>)
    new DateTime(e(1).startTime) should equal (new DateTime(2029, 10, 11, 22, 33, 44))

    XML.loadString(e(2).xml) should equal (<task force_start="yes"/>)
    XML.loadString(e(2).parameterXml) should equal (<sos.spooler.variable_set count="1"><variable value="myValue" name="myJobParameter"/></sos.spooler.variable_set>)
    new DateTime(e(2).startTime) should equal (new DateTime(2029, 11, 11, 11, 11, 11))
  }

  test("TaskEntity is read as expected") {
    val queuedTasksElem = (scheduler executeXml <show_job job={simpleJob.getPath.string} what="task_queue"/>) \ "answer" \ "job" \ "queued_tasks"
    (queuedTasksElem \ "@length").text.toInt should equal (3)
    val queuedTaskElems = queuedTasksElem \ "queued_task"
    queuedTaskElems should have size (3)
    for (q <- queuedTaskElems) {
      val enqueuedString = (q \ "@enqueued").text
      val t = xmlDateTimeFormatter.parseDateTime(enqueuedString)
      assert(!(t isBefore testStartTime), "<queued_task enqueued="+enqueuedString+"> should not be before testStartTime="+testStartTime)
      assert(!(t isAfter now()), "<queued_task enqueued="+enqueuedString+"> should not be after now")
    }
    queuedTaskElems(0).attribute("start_at") should be ('empty)
    queuedTaskElems(1).attribute("start_at").head.text should equal ("2029-10-11T20:33:44.000Z")
    queuedTaskElems(2).attribute("start_at").head.text should equal ("2029-11-11T10:11:11.000Z")
    (queuedTaskElems(2) \ "params").head should equal (<params count="1"><param value="myValue" name="myJobParameter"/></params>)
  }

  test("JobEntity is from "+orderJobPath) {
    fetchJobEntityOption(orderJobPath) should be (None)

    stopJobAndWait(orderJobPath)
    fetchJobEntityOption(orderJobPath) match { case Some(e) =>
      e should have (
        'schedulerId (schedulerId.asString),
        'clusterMemberId ("-"),
        'jobPath (orderJobPath.withoutStartingSlash),
        'stopped (true),
        'nextStartTime (null)
      )
    }
  }

  test("Job.tryFetchAverageStepDuration()") {
    val duration = simpleJob.tryFetchAverageStepDuration().get
    duration.getMillis should be >= (0L)
    duration.getMillis should be <= (10*1000L)
  }

  private def fetchJobEntityOption(jobPath: JobPath) =
    entityManager.findOption[JobEntity](JobEntity.PrimaryKey(schedulerId.asString, "-", jobPath.withoutStartingSlash))

  private def fetchTaskEntities(jobPath: JobPath): Seq[TaskEntity] =
    entityManager.fetchSeq("select t from TaskEntity t where t.jobPath=:jobPath order by t.taskId",
      Seq("jobPath" -> jobPath.withoutStartingSlash()))

  private def stopJobAndWait(jobPath: JobPath) {
    scheduler executeXml <modify_job job={jobPath.asString} cmd="stop"/>
    waitForCondition(TimeoutWithSteps(standardSeconds(3), millis(10))) { orderJob.state == JobState.stopped }
  }
}

private object EntitiesIT {
  val jobChainPath = JobChainPath.of("/test-job-chain")
  val orderId = new OrderId("ORDER-1")
  val orderJobPath = JobPath.of("/test-order-job")
  val simpleJobPath = JobPath.of("/test-simple-job")
  val firstTaskHistoryEntityId = 2  // Scheduler zählt ID ab 2
  val xmlDateTimeFormatter = DateTimeFormat.forPattern("yyyy-MM-dd'T'HH:mm:ss.SSSZ") withZone schedulerTimeZone
}
