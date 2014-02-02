package com.sos.scheduler.engine.tests.database

import EntitiesIT._
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.data.folder.{FileBasedRemovedEvent, FileBasedActivatedEvent, JobChainPath, JobPath}
import com.sos.scheduler.engine.data.job.TaskClosedEvent
import com.sos.scheduler.engine.data.order.jobchain.JobChainNodeAction
import com.sos.scheduler.engine.data.order.{OrderState, OrderId}
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.{JobState, JobSubsystem}
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.kernel.persistence.hibernate.RichEntityManager.toRichEntityManager
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerTimeZone
import com.sos.scheduler.engine.kernel.settings.CppSettingName
import com.sos.scheduler.engine.persistence.entities._
import com.sos.scheduler.engine.test.TestEnvironment.schedulerId
import com.sos.scheduler.engine.test.configuration.{DefaultDatabaseConfiguration, TestConfiguration}
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
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.xml.XML

@RunWith(classOf[JUnitRunner])
final class EntitiesIT extends FunSuite with ScalaSchedulerTest {

  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    database = Some(DefaultDatabaseConfiguration()),
    logCategories = "java.stackTrace-",  // Exceptions wegen fehlender Datenbanktabellen wollen wir nicht sehen.
    cppSettings = Map(CppSettingName.useJavaPersistence -> "true"))

  private val testStartTime = now() withMillisOfSecond 0

  override def onSchedulerActivated() {
    autoClosing(controller.newEventPipe()) { eventPipe =>
      scheduler executeXml <order job_chain={jobChainPath.string} id={orderId.string}/>
      eventPipe.nextWithCondition[TaskClosedEvent] { _.jobPath == orderJobPath }
      scheduler executeXml <start_job job={simpleJobPath.string} at="period"/>
      scheduler executeXml <start_job job={simpleJobPath.string} at="2029-10-11 22:33:44"/>
      scheduler executeXml <start_job job={simpleJobPath.string} at="2029-11-11 11:11:11"><params><param name="myJobParameter" value="myValue"/></params></start_job>
      job(simpleJobPath).forceFileReread()
      instance[FolderSubsystem].updateFolders()
      eventPipe.nextKeyed[FileBasedActivatedEvent](simpleJobPath)
    }
  }

  def entityManager =
    instance[EntityManagerFactory].createEntityManager()   // Jedes Mal einen neuen EntityManager, um Cache-Effekt zu vermeiden

  private lazy val taskHistoryEntities: Seq[TaskHistoryEntity] =
    entityManager.fetchSeq[TaskHistoryEntity]("select t from TaskHistoryEntity t order by t.id")

  test("TaskHistoryEntity") {
    taskHistoryEntities should have size 2
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
      assert(e.startTime.getTime >= testStartTime.getMillis, s"TaskHistoryEntity.startTime=${e.startTime} should not be before testStartTime=$testStartTime")
      now() match { case n => assert(e.startTime.getTime <= n.getMillis, s"TaskHistoryEntity.startTime=${e.startTime} should not be after now=$n") }
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
        'steps (3),
        'errorCode (null),
        'errorText (null)
      )
      assert(e.startTime.getTime >= testStartTime.getMillis, s"TaskHistoryEntity.startTime=${e.startTime} should not be before testStartTime=$testStartTime")
      now() match { case n => assert(e.startTime.getTime <= n.getMillis, s"TaskHistoryEntity.startTime=${e.startTime} should not be after now=$n") }
    }
  }

  test("TaskEntity is written as expected") {
    val e = fetchTaskEntities(simpleJobPath)
    e should have size 3

    e(0) should have (
      'taskId (firstTaskHistoryEntityId + 2),
      'schedulerId (schedulerId.string),
      'clusterMemberId (null),
      'jobPath (simpleJobPath.withoutStartingSlash),
      'startTime (null),
      'parameterXml (null)
    )
    XML.loadString(e(0).xml) shouldEqual <task force_start="no"/>
    assert(e(0).enqueueTime.getTime >= testStartTime.getMillis, s"TaskEntity._enqueueTime=${e(0).enqueueTime} should not be before testStartTime=$testStartTime")
    assert(e(0).enqueueTime.getTime <= now().getMillis, s"TaskEntity._enqueueTime=${e(0).enqueueTime} should not be after now")

    XML.loadString(e(1).xml) shouldEqual <task force_start="yes"/>
    new DateTime(e(1).startTime) shouldEqual new DateTime(2029, 10, 11, 22, 33, 44)

    XML.loadString(e(2).xml) shouldEqual <task force_start="yes"/>
    XML.loadString(e(2).parameterXml) shouldEqual <sos.spooler.variable_set count="1"><variable value="myValue" name="myJobParameter"/></sos.spooler.variable_set>
    new DateTime(e(2).startTime) shouldEqual new DateTime(2029, 11, 11, 11, 11, 11)
  }

  test("TaskEntity is read as expected") {
    val queuedTasksElem = (scheduler executeXml <show_job job={simpleJobPath.string} what="task_queue"/>).elem \ "answer" \ "job" \ "queued_tasks"
    (queuedTasksElem \ "@length").text.toInt shouldEqual 3
    val queuedTaskElems = queuedTasksElem \ "queued_task"
    queuedTaskElems should have size 3
    for (q <- queuedTaskElems) {
      val enqueuedString = (q \ "@enqueued").text
      val t = xmlDateTimeFormatter.parseDateTime(enqueuedString)
      assert(!(t isBefore testStartTime), "<queued_task enqueued="+enqueuedString+"> should not be before testStartTime="+testStartTime)
      assert(!(t isAfter now()), "<queued_task enqueued="+enqueuedString+"> should not be after now")
    }
    queuedTaskElems(0).attribute("start_at") shouldBe 'empty
    queuedTaskElems(1).attribute("start_at").head.text shouldEqual "2029-10-11T20:33:44.000Z"
    queuedTaskElems(2).attribute("start_at").head.text shouldEqual "2029-11-11T10:11:11.000Z"
    (queuedTaskElems(2) \ "params").head shouldEqual <params count="1"><param value="myValue" name="myJobParameter"/></params>
  }

  test("JobEntity is from "+orderJobPath) {
    tryFetchJobEntity(orderJobPath) shouldBe None

    stopJobAndWait(orderJobPath)
    tryFetchJobEntity(orderJobPath).get match { case e =>
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
    val duration = job(simpleJobPath).tryFetchAverageStepDuration().get
    duration.getMillis should (be >= 0L and be <= 10*1000L)
  }

  test("JobChainEntity") {
    tryFetchJobChainEntity(jobChainPath) shouldBe 'empty

    scheduler executeXml <job_chain.modify job_chain={jobChainPath.string} state="stopped"/>
    tryFetchJobChainEntity(jobChainPath).get should have (
      'schedulerId (schedulerId.string),
      'clusterMemberId ("-"),
      'jobChainPath (jobChainPath.withoutStartingSlash),
      'stopped (true)
    )

    scheduler executeXml <job_chain.modify job_chain={jobChainPath.string} state="running"/>
    tryFetchJobChainEntity(jobChainPath) shouldBe 'empty
  }

  test("JobChainNodeEntity") {
    fetchJobChainNodeEntities(jobChainPath) shouldBe 'empty

    scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state="200" action="next_state"/>
    scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state="300" action="stop"/>
    fetchJobChainNodeEntities(jobChainPath) match { case nodes =>
      nodes should have size 2
      for (n <- nodes) n should have ('schedulerId (schedulerId.string), 'clusterMemberId ("-"), 'jobChainPath (jobChainPath.withoutStartingSlash))
      nodes(0) should have ('orderState ("200"), 'action ("next_state"))
      nodes(1) should have ('orderState ("300"), 'action ("stop"))
    }

    scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state="200" action="process"/>
    fetchJobChainNodeEntities(jobChainPath) match { case nodes =>
      nodes should have size 2
      for (n <- nodes) n should have ('schedulerId (schedulerId.string), 'clusterMemberId ("-"), 'jobChainPath (jobChainPath.withoutStartingSlash))
      nodes(0) should have ('orderState ("200"), 'action (null))
      nodes(1) should have ('orderState ("300"), 'action ("stop"))
    }
  }

  ignore("After re-read of JobChain its state should be restored - IGNORED, DOES NOT WORK") {
    scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state="100" action="next_state"/>
    scheduler executeXml <job_chain.modify job_chain={jobChainPath.string} state="stopped"/>
    autoClosing(controller.newEventPipe()) { eventPipe =>
      instance[OrderSubsystem].jobChain(jobChainPath).forceFileReread()
      instance[FolderSubsystem].updateFolders()
      eventPipe.nextKeyed[FileBasedActivatedEvent](jobChainPath)
    }
    val jobChain = instance[OrderSubsystem].jobChain(jobChainPath)
    pendingUntilFixed {   // FIXME Der Scheduler stellt den Zustand aus der Datenbank wird nicht wieder her
      jobChain shouldBe 'stopped
      jobChain.node(OrderState("100")).action shouldEqual JobChainNodeAction.nextState
      jobChain.node(OrderState("200")).action shouldEqual JobChainNodeAction.process
      jobChain.node(OrderState("300")).action shouldEqual JobChainNodeAction.stop
    }
  }

  test("After JobChain removal database should contain no record") {
    scheduler executeXml <job_chain.modify job_chain={jobChainPath.string} state="stopped"/>    // Macht einen Datenbanksatz
    scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state="100" action="next_state"/>
    autoClosing(controller.newEventPipe()) { eventPipe =>
      instance[OrderSubsystem].jobChain(jobChainPath).file.delete() || sys.error("JobChain configuration file could not be deleted")
      instance[FolderSubsystem].updateFolders()
      eventPipe.nextKeyed[FileBasedRemovedEvent](jobChainPath)
      tryFetchJobChainEntity(jobChainPath) shouldBe 'empty
      fetchJobChainNodeEntities(jobChainPath) shouldBe 'empty
    }
  }

  private def tryFetchJobEntity(jobPath: JobPath) =
    entityManager.findOption[JobEntity](JobEntityKey(schedulerId.string, "-", jobPath.withoutStartingSlash))

  def tryFetchJobChainEntity(path: JobChainPath) =
    entityManager.fetchOption[JobChainEntity]("select j from JobChainEntity j where j.jobChainPath = :jobChainPath",
      Seq("jobChainPath" -> jobChainPath.withoutStartingSlash))

  private def fetchJobChainNodeEntities(path: JobChainPath) =
    entityManager.fetchSeq[JobChainNodeEntity]("select n from JobChainNodeEntity n where n.jobChainPath = :jobChainPath order by n.orderState",
      Seq("jobChainPath" -> jobChainPath.withoutStartingSlash))

  private def fetchTaskEntities(jobPath: JobPath): Seq[TaskEntity] =
    entityManager.fetchSeq[TaskEntity]("select t from TaskEntity t where t.jobPath = :jobPath order by t.taskId",
      Seq("jobPath" -> jobPath.withoutStartingSlash))

  private def stopJobAndWait(jobPath: JobPath) {
    scheduler executeXml <modify_job job={jobPath.string} cmd="stop"/>
    waitForCondition(TimeoutWithSteps(standardSeconds(3), millis(10))) { job(orderJobPath).state == JobState.stopped }
  }

  private def job(o: JobPath) =
    instance[JobSubsystem].job(o)
}


private object EntitiesIT {
  private val jobChainPath = JobChainPath("/test-job-chain")
  private val orderId = OrderId("ORDER-1")
  private val orderJobPath = JobPath("/test-order-job")
  private val simpleJobPath = JobPath("/test-simple-job")
  private val firstTaskHistoryEntityId = 2  // Scheduler zählt ID ab 2
  private val xmlDateTimeFormatter = DateTimeFormat.forPattern("yyyy-MM-dd'T'HH:mm:ss.SSSZ") withZone schedulerTimeZone
}
