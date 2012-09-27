package com.sos.scheduler.engine.persistence.entities

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.persistence.SchedulerDatabases._
import java.io.Serializable
import javax.persistence.Column

case class JobEntityKey(
  @Column(name = "spooler_id"       , nullable = false) var _schedulerId    : String,
  @Column(name = "cluster_member_id", nullable = false) var _clusterMemberId: String,
  @Column(name = "path"             , nullable = false) var _jobPath        : String
) extends Serializable {

  def this() {  // Für JPA
    this(null: String, null: String, null: String)
  }

  def this(schedulerId: SchedulerId, clusterMemberId: ClusterMemberId, jobPath: JobPath) {
    this(
      idForDatabase(schedulerId),
      if (clusterMemberId.isEmpty) "-" else clusterMemberId.asString,
      jobPath.withoutStartingSlash)
  }
}
