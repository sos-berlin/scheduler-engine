package com.sos.scheduler.engine.data.job

import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import org.joda.time.DateTime

trait SchedulerHistoryEntry {
  val id: Int
  val schedulerId: SchedulerId
  val clusterMemberId: ClusterMemberId
  val startTime: DateTime
  val endTimeOption: Option[DateTime]
  val errorCode: String
  val errorText: String
  val parameterXml: String
  val processId: Int
}
