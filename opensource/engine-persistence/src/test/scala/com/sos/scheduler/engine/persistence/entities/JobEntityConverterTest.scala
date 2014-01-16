package com.sos.scheduler.engine.persistence.entities

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.JobPersistentState
import com.sos.scheduler.engine.data.scheduler.{SchedulerId, ClusterMemberId}
import com.sos.scheduler.engine.persistence.SchedulerDatabases.databaseTimeZone
import java.util.{Date => JavaDate}
import org.joda.time.DateTime
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
      val timestamp = new DateTime(2012, 10, 2, 22, 33, 44)
      val e =  converter.toEntity(JobPersistentState(JobPath("/path/name"), testStopped, Some(timestamp)))
      e.schedulerId should equal("SCHEDULER-ID")
      e.clusterMemberId should equal (entityClusterMemberId)
      e.jobPath should equal ("path/name")
      e.isStopped should equal (testStopped)
      e.nextStartTime should equal (new JavaDate(timestamp.getMillis))
    }

    test(testKey +"toObject") {
      val timestamp = new DateTime(2012, 10, 2, 22, 33, 44, databaseTimeZone)   // DateTime.equals() vergleicht auch die Zeitzone
      val e =  converter.toEntity(JobPersistentState(JobPath("/path/name"), true, Some(timestamp)))
      e.schedulerId = "SCHEDULER-ID"
      e.clusterMemberId = null
      e.jobPath = "path/name"
      e.isStopped = testStopped
      e.nextStartTime = new JavaDate(timestamp.getMillis)
      converter.toObject(e) should equal (JobPersistentState(JobPath("/path/name"), testStopped, Some(timestamp)))
    }
  }
}
