package com.sos.scheduler.engine.tests.database

import com.sos.scheduler.engine.data.folder.{FileBasedRemovedEvent, FileBasedActivatedEvent, JobChainPath, JobPath}
import com.sos.scheduler.engine.data.job.{JobPersistentState, TaskClosedEvent}
import com.sos.scheduler.engine.data.order.OrderId
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.kernel.job.ScalaJPA._
import com.sos.scheduler.engine.kernel.job.{JobState, JobSubsystem}
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
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import scala.xml.XML
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem

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
    scheduler executeXml <start_job job={simpleJobPath.string} at="period"><params><param name="myJobParameter" value="myValue"/></params></start_job>
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
        'getId (firstTaskHistoryEntityId),
        'schedulerId (schedulerId),
        'getClusterMemberId (ClusterMemberId.empty),
        'getEndTime (null),
        'getJobPath (""),
        'getCause (null),
        'getSteps (null),
        'getErrorCode (""),
        'getErrorText ("")
      )
      assert(e.getStartTime.getTime >= testStartTime.getMillis, "TaskHistoryEntity.startTime="+e.getStartTime+" should not be before testStartTime="+testStartTime)
      now() match { case n => assert(e.getStartTime.getTime <= n.getMillis, "TaskHistoryEntity.startTime="+e.getStartTime+" should not be after now="+n) }
    }
  }

  test("Second TaskHistoryEntity is from "+orderJobPath) {
    taskHistoryEntities(1) match { case e =>
      e should have (
        'getId (firstTaskHistoryEntityId + 1),
        'schedulerId (schedulerId),
        'getClusterMemberId (ClusterMemberId.empty),
        'getJobPath (orderJobPath.asString),
        'getCause ("order"),
        'getSteps (1),
        'getErrorCode (""),
        'getErrorText ("")
      )
      assert(e.getStartTime.getTime >= testStartTime.getMillis, "TaskHistoryEntity.startTime="+e.getStartTime+" should not be before testStartTime="+testStartTime)
      now() match { case n => assert(e.getStartTime.getTime <= n.getMillis, "TaskHistoryEntity.startTime="+e.getStartTime+" should not be after now="+n) }
    }
  }

  test("TaskEntity is written as expected") {
    val e = fetchTaskEntities(simpleJobPath)
    e should have size (3)

    e(0) should have (
      '_taskId (firstTaskHistoryEntityId + 2),
      '_schedulerId (schedulerId.asString),
      '_clusterMemberId (null),
      '_jobPath (simpleJobPath.withoutStartingSlash),
      '_startTime (null),
      '_parameterXml (null)
    )
    XML.loadString(e(0)._xml) should equal (<task force_start="no"/>)
    assert(e(0)._enqueueTime.getTime >= testStartTime.getMillis, "TaskEntity._enqueueTime="+e(0)._enqueueTime+" should not be before testStartTime="+testStartTime)
    assert(e(0)._enqueueTime.getTime <= now().getMillis, "TaskEntity._enqueueTime="+e(0)._enqueueTime+" should not be after now")

    XML.loadString(e(1)._xml) should equal (<task force_start="yes"/>)
    new DateTime(e(1)._startTime) should equal (new DateTime(2029, 10, 11, 22, 33, 44))

    XML.loadString(e(2)._xml) should equal (<task force_start="no"/>)
    XML.loadString(e(2)._parameterXml) should equal (<sos.spooler.variable_set count="1"><variable value="myValue" name="myJobParameter"/></sos.spooler.variable_set>)
  }

  test("TaskEntity is read as expected") {
    val answerDoc = scheduler executeXml <show_job job={simpleJob.getPath.string}/>
    val queuedTasksElem =  answerDoc \ "answer" \ "job" \ "queued_tasks"
    (queuedTasksElem \ "@length").text.toInt should equal (3)
    val t = queuedTasksElem \ "queued_task"
    // (t(0) \ "@start_time") should equal (xx)
  }

  test("JobEntity is from "+orderJobPath) {
    fetchJobEntityOption(orderJobPath) should be (None)

    stopJobAndWait(orderJobPath)
    fetchJobEntityOption(orderJobPath) match { case Some(e) =>
      e should have (
        '_schedulerId (schedulerId.asString),
        '_clusterMemberId ("-"),
        '_jobPath (orderJobPath.withoutStartingSlash),
        'stopped (true),
        '_nextStartTime (null),
        'schedulerId (schedulerId),
        'clusterMemberId (ClusterMemberId.empty),
        'toObject (JobPersistentState(orderJobPath, nextStartTimeOption=None, isPermanentlyStopped=true))
      )
    }
  }

  private def fetchJobEntityOption(jobPath: JobPath) =
    entityManager.findOption[JobEntity](JobEntity.PrimaryKey(schedulerId, ClusterMemberId.empty, jobPath))

  private def fetchTaskEntities(jobPath: JobPath): Seq[TaskEntity] =
    entityManager.fetchSeq("select t from TaskEntity t where t._jobPath=:jobPath order by t._taskId",
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
}
