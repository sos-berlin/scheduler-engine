package com.sos.scheduler.engine.data.job

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.scheduler.{SchedulerId, ClusterMemberId}
import org.joda.time.DateTime

trait TaskHistoryEntry {
  val id: Int
  val schedulerId: SchedulerId
  val clusterMemberId: ClusterMemberId
  val jobPath: JobPath
  val startTime: DateTime
  val endTimeOption: Option[DateTime]
  val cause: String
  val stepsOption: Option[Int]
  val errorCode: String
  val errorText: String
  val parameterXml: String
  val processIdOption: Option[Int]
}
