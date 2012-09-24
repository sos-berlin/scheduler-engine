package com.sos.scheduler.engine.tests.database

import com.sos.scheduler.engine.data.folder.{JobChainPath, JobPath}
import com.sos.scheduler.engine.data.job.TaskClosedEvent
import com.sos.scheduler.engine.data.order.OrderId
import com.sos.scheduler.engine.test.Environment.schedulerId
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import javax.persistence.EntityManager
import org.joda.time.DateTime.now
import org.joda.time.DateTimeZone.UTC
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import scala.collection.JavaConversions._
import com.sos.scheduler.engine.persistence.entities.TaskHistoryEntity

@RunWith(classOf[JUnitRunner])
final class EntitiesIT extends ScalaSchedulerTest {

  import EntitiesIT._

  private val testStartTime = now(UTC) withMillisOfSecond 0

  override def checkedBeforeAll() {
    controller.useDatabase()
    controller.setLogCategories("java.stackTrace-")
    controller.activateScheduler()
    val eventPipe = controller.newEventPipe()
    scheduler executeXml <order job_chain={jobChainPath.asString} id={orderId.asString}/>
    eventPipe.nextWithCondition[TaskClosedEvent] { e => e.getJobPath == jobPath}
  }

  lazy val entityManager = controller.scheduler.instance[EntityManager]

  private lazy val taskHistoryEntities: Seq[TaskHistoryEntity] = {
    val ql = "select t from TaskHistoryEntity t order by t.id, t.clusterMemberId, t.startTime, t.jobPath"
    entityManager.createQuery(ql, classOf[TaskHistoryEntity]).getResultList
  }

  test("TaskHistoryEntity") {
    taskHistoryEntities should have size(2)
  }

  test("First TaskHistoryEntity is for Scheduler start") {
    val e = taskHistoryEntities(0)
    e should have (
      'getId (firstTaskHistoryEntityId),
      'getSchedulerId (schedulerId),
      'getClusterMemberId (""),
      'getEndTime (null),
      'getJobPath (""),
      'getCause (null),
      'getSteps (null),
      'getErrorCode (""),
      'getErrorText ("")
    )
    assert(e.getStartTime.getTime >= testStartTime.getMillis, "TaskHistoryEntity.startTime="+e.getStartTime+" should not be before testStartTime="+testStartTime)
    now(UTC) match { case n => assert(e.getStartTime.getTime <= n.getMillis, "TaskHistoryEntity.startTime="+e.getStartTime+" should not be after now="+n) }
  }

  test("Second TaskHistoryEntity is from an order job") {
    val e = taskHistoryEntities(1)
    e should have (
      'getId (firstTaskHistoryEntityId + 1),
      'getSchedulerId (schedulerId),
      'getClusterMemberId (""),
      'getJobPath (jobPath.asString),
      'getCause ("order"),
      'getSteps (1),
      'getErrorCode (""),
      'getErrorText ("")
    )
    assert(e.getStartTime.getTime >= testStartTime.getMillis, "TaskHistoryEntity.startTime="+e.getStartTime+" should not be before testStartTime="+testStartTime)
    now(UTC) match { case n => assert(e.getStartTime.getTime <= n.getMillis, "TaskHistoryEntity.startTime="+e.getStartTime+" should not be after now="+n) }
  }
}

private object EntitiesIT {
  val jobChainPath = JobChainPath.of("/test-job-chain")
  val orderId = new OrderId("ORDER-1")
  val jobPath = JobPath.of("/test-order-job")
  val firstTaskHistoryEntityId = 2  // Scheduler zählt ID ab 2
}
