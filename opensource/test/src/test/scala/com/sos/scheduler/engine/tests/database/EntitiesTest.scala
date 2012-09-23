package com.sos.scheduler.engine.tests.database

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskClosedEvent
import com.sos.scheduler.engine.persistence.TaskHistoryEntity
import com.sos.scheduler.engine.test.Environment.schedulerId
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import javax.persistence.EntityManager
import org.joda.time.DateTime.now
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import scala.collection.JavaConversions._

@RunWith(classOf[JUnitRunner])
final class EntitiesTest extends ScalaSchedulerTest {

  import EntitiesTest._

  private val testStartTime = now()

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
    e.getId should equal (firstTaskHistoryEntityId)
    e.getSchedulerId should be (schedulerId)
    e.getClusterMemberId should equal ("")
    //joda... assert(e.getStartTime. after testStartTime)
    e.getEndTime should be (null)
    //e.getJobPath should be (new JobPath("")))
    e.getCause should equal (null)
    e.getSteps should be (null)
    e.getErrorCode should be (null)
    e.getErrorText should be (null)
  }

  test("Second TaskHistoryEntity") {
    pending
  }
}

private object EntitiesTest {
  val jobPath = JobPath.of("/test-a")
  val firstTaskHistoryEntityId = 2  // Scheduler zählt ID ab 2
}
