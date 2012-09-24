package com.sos.scheduler.engine.tests.database

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskClosedEvent
import com.sos.scheduler.engine.persistence.TaskHistoryEntity
import com.sos.scheduler.engine.test.Environment.schedulerId
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import javax.persistence.EntityManager
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import scala.collection.JavaConversions._

@RunWith(classOf[JUnitRunner])
final class EntitiesTest extends ScalaSchedulerTest {

  import EntitiesTest._

  private val testStartTime = new java.util.Date()

  override def checkedBeforeAll() {
    controller.useDatabase()
    controller.activateScheduler()
    val eventPipe = controller.newEventPipe()
    scheduler executeXml <start_job job={jobPath.asString()}/>
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
    assert(e.getStartTime.getTime / 1000 >= testStartTime.getTime / 1000, "TaskHistoryEntity.startTime="+e.getStartTime+" should not be before testStartTime="+testStartTime)
    new java.util.Date() match { case now => assert(e.getStartTime.getTime / 1000 <= now.getTime / 1000, "TaskHistoryEntity.startTime="+e.getStartTime+" should not be after now="+now) }
  }

  test("Second TaskHistoryEntity") {
    pending
  }
}

private object EntitiesTest {
  val jobPath = JobPath.of("/test-a")
  val firstTaskHistoryEntityId = 2  // Scheduler zählt ID ab 2
}
