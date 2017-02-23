package com.sos.scheduler.engine.persistence.entities

import com.sos.jobscheduler.data.scheduler.SchedulerId
import com.sos.scheduler.engine.data.job.{JobPath, JobPersistentState}
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import java.time.Instant
import java.util.{Date ⇒ JavaDate}
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JobEntityConverterTest extends FunSuite {

  private val clusterMemberIds = Seq(
    new ClusterMemberId("M") -> "M",
    new ClusterMemberId("") -> "-")

  for ((testClusterMemberId, entityClusterMemberId) <- clusterMemberIds;
       testStopped <- Seq(false, true)) {

    val converter = new JobEntityConverter {
      val schedulerId = new SchedulerId("SCHEDULER-ID")
      val clusterMemberId = testClusterMemberId
    }

    val testKey = "clusterMemberId="+ testClusterMemberId + ", stopped=" + testStopped +" "

    test(testKey +"toEntityKey") {
      converter.toEntityKey(JobPath("/path/name")) should equal (JobEntityKey("SCHEDULER-ID", entityClusterMemberId, "path/name"))
    }

    test(testKey +"toEntity ") {
      val timestamp = Instant.parse("2012-10-02T22:33:44Z")
      val e =  converter.toEntity(JobPersistentState(JobPath("/path/name"), testStopped, Some(timestamp)))
      e.schedulerId should equal("SCHEDULER-ID")
      e.clusterMemberId should equal (entityClusterMemberId)
      e.jobPath should equal ("path/name")
      e.isStopped should equal (testStopped)
      e.nextStartTime should equal (new JavaDate(timestamp.toEpochMilli))
    }

    test(testKey +"toObject") {
      val timestamp = Instant.parse("2012-10-02T22:33:44Z")
      val e =  converter.toEntity(JobPersistentState(JobPath("/path/name"), true, Some(timestamp)))
      e.schedulerId = "SCHEDULER-ID"
      e.clusterMemberId = null
      e.jobPath = "path/name"
      e.isStopped = testStopped
      e.nextStartTime = new JavaDate(timestamp.toEpochMilli)
      converter.toObject(e) should equal (JobPersistentState(JobPath("/path/name"), testStopped, Some(timestamp)))
    }
  }
}
