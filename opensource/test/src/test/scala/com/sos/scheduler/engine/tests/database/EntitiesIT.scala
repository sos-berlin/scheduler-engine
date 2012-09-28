package com.sos.scheduler.engine.tests.database

import com.sos.scheduler.engine.data.folder.{JobChainPath, JobPath}
import com.sos.scheduler.engine.data.job.{JobPersistentState, TaskClosedEvent}
import com.sos.scheduler.engine.data.order.OrderId
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.kernel.job.{JobState, JobSubsystem}
import com.sos.scheduler.engine.kernel.settings.{SettingName, Settings}
import com.sos.scheduler.engine.persistence.entities.{JobEntity, TaskHistoryEntity}
import com.sos.scheduler.engine.test.Environment.schedulerId
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.test.util.time.TimeoutWithSteps
import com.sos.scheduler.engine.test.util.time.WaitForCondition.waitForCondition
import javax.persistence.EntityManagerFactory
import org.joda.time.DateTime.now
import org.joda.time.Duration.{millis, standardSeconds}
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import scala.collection.JavaConversions._

@RunWith(classOf[JUnitRunner])
final class EntitiesIT extends ScalaSchedulerTest {

  import EntitiesIT._

  private val testStartTime = now() withMillisOfSecond 0
  private lazy val jobSubsystem = controller.scheduler.instance[JobSubsystem]
  private lazy val orderJob = jobSubsystem.job(orderJobPath)

  override def checkedBeforeAll() {
    controller.useDatabase()
    controller.setLogCategories("java.stackTrace-")
    controller.setSettings(Settings.of(SettingName.useJavaPersistence, "true"))
    controller.activateScheduler()
    val eventPipe = controller.newEventPipe()
    scheduler executeXml <order job_chain={jobChainPath.asString} id={orderId.asString}/>
    eventPipe.nextWithCondition[TaskClosedEvent] { e => e.getJobPath == orderJobPath}
  }

  lazy val entityManager = controller.scheduler.instance[EntityManagerFactory].createEntityManager()

  override def afterAll() {
    entityManager.close()
  }

  private lazy val taskHistoryEntities = fetch[TaskHistoryEntity]("select t from TaskHistoryEntity t order by t.id")

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

  test("JobEntity is from "+orderJobPath) {
    fetchJobEntities() should be ('empty)

    stopJobAndWait(orderJobPath)
    fetchJobEntities() match { case entities =>
      entities should have size (1)
      entities(0) should have (
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

  private def fetchJobEntities() = fetch[JobEntity]("select j from JobEntity j order by j._schedulerId, j._clusterMemberId, j._jobPath")

  private def fetch[A](ql: String)(implicit m: Manifest[A]) =
    entityManager.createQuery(ql, m.erasure).getResultList.asInstanceOf[java.util.List[A]].toSeq

  private def stopJobAndWait(jobPath: JobPath) {
    scheduler executeXml <modify_job job={jobPath.asString} cmd="stop"/>
    waitForCondition(TimeoutWithSteps(standardSeconds(3), millis(10))) { orderJob.state == JobState.stopped }
  }
}

private object EntitiesIT {
  val jobChainPath = JobChainPath.of("/test-job-chain")
  val orderId = new OrderId("ORDER-1")
  val orderJobPath = JobPath.of("/test-order-job")
  val firstTaskHistoryEntityId = 2  // Scheduler zählt ID ab 2
}
