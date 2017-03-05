package com.sos.scheduler.engine.tests.database

import com.sos.jobscheduler.common.scalautil.xmls.SafeXML
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.time.TimeoutWithSteps
import com.sos.jobscheduler.common.time.WaitForCondition.waitForCondition
import com.sos.scheduler.engine.data.filebased.{FileBasedActivated, FileBasedRemoved}
import com.sos.scheduler.engine.data.job.{JobPath, JobState, TaskClosed}
import com.sos.scheduler.engine.data.jobchain.{JobChainNodeAction, JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order.OrderId
import com.sos.scheduler.engine.kernel.folder.FolderSubsystemClient
import com.sos.scheduler.engine.kernel.job.JobSubsystemClient
import com.sos.scheduler.engine.kernel.order.OrderSubsystemClient
import com.sos.scheduler.engine.kernel.persistence.hibernate.RichEntityManager.toRichEntityManager
import com.sos.scheduler.engine.kernel.settings.{CppSettingName, CppSettings}
import com.sos.scheduler.engine.persistence.entities._
import com.sos.scheduler.engine.test.SchedulerTestUtils.jobOverview
import com.sos.scheduler.engine.test.TestEnvironment.TestSchedulerId
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.database.EntitiesIT._
import java.nio.file.Files.deleteIfExists
import java.time.Instant.now
import java.time.{Instant, LocalDateTime, ZoneId}
import javax.persistence.EntityManagerFactory
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class EntitiesIT extends FunSuite with ScalaSchedulerTest {

  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    cppSettings = CppSettings.TestMap + (CppSettingName.useJavaPersistence -> true.toString))

  private val testStartTime = Instant.ofEpochSecond(now().getEpochSecond)
  private lazy val taskHistoryEntities: Seq[TaskHistoryEntity] = entityManager.fetchSeq[TaskHistoryEntity]("select t from TaskHistoryEntity t order by t.id")
  private lazy val daylightSavingTimeInstant = LocalDateTime.parse(DaylightSavingTimeString).atZone(instance[ZoneId]).toInstant
  private lazy val standardTimeInstant = LocalDateTime.parse(StandardTimeInstantString).atZone(instance[ZoneId]).toInstant

  override def onSchedulerActivated(): Unit = {
    withEventPipe { eventPipe ⇒
      scheduler executeXml <order job_chain={jobChainPath.string} id={orderId.string}/>
      eventPipe.nextWhen[TaskClosed.type] { _.key.jobPath == orderJobPath }
      scheduler executeXml <start_job job={simpleJobPath.string} at="period"/>
      scheduler executeXml <start_job job={simpleJobPath.string} at={DaylightSavingTimeString}/>
      scheduler executeXml <start_job job={simpleJobPath.string} at={StandardTimeInstantString}><params><param name="myJobParameter" value="myValue"/></params></start_job>
      instance[JobSubsystemClient].forceFileReread(simpleJobPath)
      instance[FolderSubsystemClient].updateFolders()
      eventPipe.next[FileBasedActivated.type](simpleJobPath)
      instance[JobSubsystemClient].forceFileReread(simpleJobPath)
      instance[FolderSubsystemClient].updateFolders()
      eventPipe.next[FileBasedActivated.type](simpleJobPath)
    }
  }

  private def entityManager = instance[EntityManagerFactory].createEntityManager()   // Jedes Mal einen neuen EntityManager, um Cache-Effekt zu vermeiden

  test("TaskHistoryEntity") {
    taskHistoryEntities should have size 2
  }

  test("First TaskHistoryEntity is from Scheduler start") {
    taskHistoryEntities(0) match { case e =>
      e should have (
        'id (firstTaskHistoryEntityId),
        'schedulerId (TestSchedulerId.string),
        'clusterMemberId (null),
        'endTime (null),
        'jobPath ("(Spooler)"),
        'cause (null),
        'steps (null),
        'errorCode (null),
        'errorText (null)
      )
      assert(e.startTime.getTime >= testStartTime.toEpochMilli, s"TaskHistoryEntity.startTime=${e.startTime} should not be before testStartTime=$testStartTime")
      now() match { case n => assert(e.startTime.getTime <= n.toEpochMilli, s"TaskHistoryEntity.startTime=${e.startTime} should not be after now=$n") }
    }
  }

  test("Second TaskHistoryEntity is from "+orderJobPath) {
    taskHistoryEntities(1) match { case e =>
      e should have (
        'id (firstTaskHistoryEntityId + 1),
        'schedulerId (TestSchedulerId.string),
        'clusterMemberId (null),
        'jobPath (orderJobPath.withoutStartingSlash),
        'cause ("order"),
        'steps (3),
        'errorCode (null),
        'errorText (null)
      )
      assert(e.startTime.getTime >= testStartTime.toEpochMilli, s"TaskHistoryEntity.startTime=${e.startTime} should not be before testStartTime=$testStartTime")
      now() match { case n => assert(e.startTime.getTime <= n.toEpochMilli, s"TaskHistoryEntity.startTime=${e.startTime} should not be after now=$n") }
    }
  }

  test("TaskEntity is written as expected") {
    val e = fetchTaskEntities(simpleJobPath)
    e should have size 3

    e(0) should have (
      'taskId (firstTaskHistoryEntityId + 2),
      'schedulerId (TestSchedulerId.string),
      'clusterMemberId (null),
      'jobPath (simpleJobPath.withoutStartingSlash),
      'startTime (null),
      'parameterXml (null)
    )
    SafeXML.loadString(e(0).xml) shouldEqual <task force_start="no"/>
    assert(e(0).enqueueTime.getTime >= testStartTime.toEpochMilli, s"TaskEntity._enqueueTime=${e(0).enqueueTime} should not be before testStartTime=$testStartTime")
    assert(e(0).enqueueTime.getTime <= now().toEpochMilli, s"TaskEntity._enqueueTime=${e(0).enqueueTime} should not be after now")

    SafeXML.loadString(e(1).xml) shouldEqual <task force_start="yes"/>
    // Database UTC field is used for local time
    Instant.ofEpochMilli(e(1).startTime.getTime) shouldEqual daylightSavingTimeInstant

    SafeXML.loadString(e(2).xml) shouldEqual <task force_start="yes"/>
    SafeXML.loadString(e(2).parameterXml) shouldEqual <sos.spooler.variable_set count="1"><variable value="myValue" name="myJobParameter"/></sos.spooler.variable_set>
    Instant.ofEpochMilli(e(2).startTime.getTime) shouldEqual standardTimeInstant
  }

  test("TaskEntity is read as expected") {
    val queuedTasksElem = (scheduler executeXml <show_job job={simpleJobPath.string} what="task_queue"/>).elem \ "answer" \ "job" \ "queued_tasks"
    (queuedTasksElem \ "@length").text.toInt shouldEqual 3
    val queuedTaskElems = queuedTasksElem \ "queued_task"
    queuedTaskElems should have size 3
    for (q <- queuedTaskElems) {
      val enqueuedString = (q \ "@enqueued").text
      val t = Instant.parse(enqueuedString)
      assert(!(t isBefore testStartTime), s"<queued_task enqueued=$enqueuedString> should not be before testStartTime=$testStartTime")
      assert(!(t isAfter now()), s"<queued_task enqueued=$enqueuedString> should not be after now")
    }
    queuedTaskElems(0).attribute("start_at") shouldBe 'empty
    queuedTaskElems(1).attribute("start_at").head.text shouldEqual daylightSavingTimeInstant.toString.replace("Z", ".000Z")
    queuedTaskElems(2).attribute("start_at").head.text shouldEqual standardTimeInstant.toString.replace("Z", ".000Z")
    (queuedTaskElems(2) \ "params").head shouldEqual <params count="1"><param value="myValue" name="myJobParameter"/></params>
  }

  test("JobEntity is from "+orderJobPath) {
    tryFetchJobEntity(orderJobPath) shouldBe None

    stopJobAndWait(orderJobPath)
    tryFetchJobEntity(orderJobPath).get match { case e =>
      e should have (
        'schedulerId (TestSchedulerId.string),
        'clusterMemberId ("-"),
        'jobPath (orderJobPath.withoutStartingSlash),
        'stopped (true),
        'nextStartTime (null)
      )
    }
  }

  test("Job.tryFetchAverageStepDuration()") {
    val duration = job(simpleJobPath).tryFetchAverageStepDuration().get
    duration.toMillis should (be >= 0L and be <= 10*1000L)
  }

  test("JobChainEntity") {
    tryFetchJobChainEntity(jobChainPath) shouldBe 'empty

    scheduler executeXml <job_chain.modify job_chain={jobChainPath.string} state="stopped"/>
    tryFetchJobChainEntity(jobChainPath).get should have (
      'schedulerId (TestSchedulerId.string),
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
      for (n <- nodes) n should have ('schedulerId (TestSchedulerId.string), 'clusterMemberId ("-"), 'jobChainPath (jobChainPath.withoutStartingSlash))
      nodes(0) should have ('orderState ("200"), 'action ("next_state"))
      nodes(1) should have ('orderState ("300"), 'action ("stop"))
    }

    scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state="200" action="process"/>
    fetchJobChainNodeEntities(jobChainPath) match { case nodes =>
      nodes should have size 2
      for (n <- nodes) n should have ('schedulerId (TestSchedulerId.string), 'clusterMemberId ("-"), 'jobChainPath (jobChainPath.withoutStartingSlash))
      nodes(0) should have ('orderState ("200"), 'action (null))
      nodes(1) should have ('orderState ("300"), 'action ("stop"))
    }
  }

  ignore("After re-read of JobChain its state should be restored - IGNORED, DOES NOT WORK") {
    scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state="100" action="next_state"/>
    scheduler executeXml <job_chain.modify job_chain={jobChainPath.string} state="stopped"/>
    withEventPipe { eventPipe ⇒
      instance[OrderSubsystemClient].forceFileReread(jobChainPath)
      instance[FolderSubsystemClient].updateFolders()
      eventPipe.next[FileBasedActivated.type](jobChainPath)
    }
    val jobChain = instance[OrderSubsystemClient].jobChain(jobChainPath)
    pendingUntilFixed {   // FIXME Der Scheduler stellt den Zustand aus der Datenbank wird nicht wieder her
      jobChain shouldBe 'stopped
      jobChain.node(NodeId("100")).action shouldEqual JobChainNodeAction.next_state
      jobChain.node(NodeId("200")).action shouldEqual JobChainNodeAction.process
      jobChain.node(NodeId("300")).action shouldEqual JobChainNodeAction.stop
    }
  }

  test("After JobChain removal database should contain no record") {
    scheduler executeXml <job_chain.modify job_chain={jobChainPath.string} state="stopped"/>    // Macht einen Datenbanksatz
    scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state="100" action="next_state"/>
    withEventPipe { eventPipe ⇒
      deleteIfExists(instance[OrderSubsystemClient].jobChain(jobChainPath).file) || sys.error("JobChain configuration file could not be deleted")
      instance[FolderSubsystemClient].updateFolders()
      eventPipe.next[FileBasedRemoved.type](jobChainPath)
      tryFetchJobChainEntity(jobChainPath) shouldBe 'empty
      fetchJobChainNodeEntities(jobChainPath) shouldBe 'empty
    }
  }

  private def tryFetchJobEntity(jobPath: JobPath) =
    entityManager.findOption[JobEntity](JobEntityKey(TestSchedulerId.string, "-", jobPath.withoutStartingSlash))

  def tryFetchJobChainEntity(path: JobChainPath) =
    entityManager.fetchOption[JobChainEntity]("select j from JobChainEntity j where j.jobChainPath = :jobChainPath",
      Seq("jobChainPath" -> jobChainPath.withoutStartingSlash))

  private def fetchJobChainNodeEntities(path: JobChainPath) =
    entityManager.fetchSeq[JobChainNodeEntity]("select n from JobChainNodeEntity n where n.jobChainPath = :jobChainPath order by n.orderState",
      Seq("jobChainPath" -> jobChainPath.withoutStartingSlash))

  private def fetchTaskEntities(jobPath: JobPath): Seq[TaskEntity] =
    entityManager.fetchSeq[TaskEntity]("select t from TaskEntity t where t.jobPath = :jobPath order by t.taskId",
      Seq("jobPath" -> jobPath.withoutStartingSlash))

  private def stopJobAndWait(jobPath: JobPath): Unit = {
    scheduler executeXml <modify_job job={jobPath.string} cmd="stop"/>
    waitForCondition(TimeoutWithSteps(3.s, 10.ms)) { jobOverview(orderJobPath).state == JobState.stopped }
  }

  private def job(o: JobPath) =
    instance[JobSubsystemClient].job(o)
}


private object EntitiesIT {
  private val jobChainPath = JobChainPath("/test-job-chain")
  private val orderId = OrderId("ORDER-1")
  private val orderJobPath = JobPath("/test-order-job")
  private val simpleJobPath = JobPath("/test-simple-job")
  private val firstTaskHistoryEntityId = 2  // Scheduler zählt ID ab 2
  private val DaylightSavingTimeString = "2029-10-11T22:33:44"
  private val StandardTimeInstantString = "2029-11-11T11:11:11"
}
