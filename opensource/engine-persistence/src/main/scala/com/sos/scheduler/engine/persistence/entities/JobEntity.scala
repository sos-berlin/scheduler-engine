package com.sos.scheduler.engine.persistence.entities

import com.google.common.base.Strings.emptyToNull
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.persistence.SchedulerDatabases._
import java.util.{Date => JavaDate}
import javax.annotation.Nullable
import javax.persistence.TemporalType.TIMESTAMP
import javax.persistence._
import org.joda.time.DateTime
import org.joda.time.DateTimeZone.UTC

@Entity
@Table(name = "SCHEDULER_JOBS")
@IdClass(classOf[JobEntityKey])
class JobEntity {
  @Column(name = "spooler_id"       , nullable = false) @Id         private var _schedulerId    : String = _
  @Column(name = "cluster_member_id", nullable = false) @Id         private var _clusterMemberId: String = _
  @Column(name = "path"             , nullable = false) @Id         private var _jobPath        : String = _
  @Column(name = "stopped"          , nullable = false)                     var isStopped       : Boolean = _
  @Column(name = "next_start_time") @Temporal(TIMESTAMP) @Nullable  private var _nextStartTime  : JavaDate = _

  def this(key: JobEntityKey) {
    this()
    _schedulerId = key._schedulerId
    _clusterMemberId = key._clusterMemberId
    _jobPath = key._jobPath
  }

  def schedulerId = schedulerIdFromDatabase(_schedulerId)

  def clusterMemberId = _clusterMemberId match {
    case "-" => ClusterMemberId.empty
    case s => new ClusterMemberId(s)
  }

  final def jobPath = Option(emptyToNull(_jobPath)) map { o => JobPath.of("/"+ o) }

  final def nextStartTime = Option(_nextStartTime) map { o => new DateTime(o.getTime, UTC) }

  final def nextStartTime_=(o: Option[DateTime]) {
    o match {
      case Some(dt) => _nextStartTime = new JavaDate(dt.getMillis)
      case None => _nextStartTime = null
    }
  }
}
