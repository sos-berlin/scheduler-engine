package com.sos.scheduler.engine.persistence.entities

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.JobPersistentState
import com.sos.scheduler.engine.data.scheduler.{SchedulerId, ClusterMemberId}
import com.sos.scheduler.engine.persistence.SchedulerDatabases._
import java.io.Serializable
import java.util.{Date => JavaDate}
import javax.annotation.Nullable
import javax.persistence.TemporalType.TIMESTAMP
import javax.persistence._
import org.joda.time.DateTime
import org.joda.time.DateTimeZone.UTC

@Entity
@Table(name = "SCHEDULER_JOBS")
@IdClass(classOf[JobEntity.Key])
class JobEntity {
  @Column(name = "spooler_id"       , nullable = false) @Id         private var _schedulerId    : String = _
  @Column(name = "cluster_member_id", nullable = false) @Id         private var _clusterMemberId: String = _
  @Column(name = "path"             , nullable = false) @Id         private var _jobPath        : String = _
  @Column(name = "stopped"          , nullable = false)                     var isStopped       : Boolean = _
  @Column(name = "next_start_time") @Temporal(TIMESTAMP) @Nullable  private var _nextStartTime  : JavaDate = _

  def this(key: JobEntity.Key) {
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

  final def toObject = JobPersistentState(jobPath, nextStartTimeOption, isStopped)

  final def jobPath = JobPath.of("/"+ _jobPath)

  final def nextStartTimeOption = Option(_nextStartTime) map { o => new DateTime(o.getTime, UTC) }

  final def nextStartTimeOption_=(o: Option[DateTime]) {
    o match {
      case Some(dt) => _nextStartTime = new JavaDate(dt.getMillis)
      case None => _nextStartTime = null
    }
  }
}

object JobEntity {
  def apply(schedulerId: SchedulerId, clusterMemberId: ClusterMemberId, o: JobPersistentState) = {
    val e = new JobEntity(Key(schedulerId, clusterMemberId, o.jobPath))
    e.nextStartTimeOption = o.nextStartTimeOption
    e.isStopped = o.isPermanentlyStopped
    e
  }

  case class Key(var _schedulerId: String, var _clusterMemberId: String, var _jobPath: String) extends Serializable {
    def this() = this(null: String, null: String, null: String)  // Für JPA
  }

  object Key {
    def apply(s: SchedulerId, clusterMemberId: ClusterMemberId, jobPath: JobPath) = new Key(
        idForDatabase(s),
        if (clusterMemberId.isEmpty) "-" else clusterMemberId.asString,
        jobPath.withoutStartingSlash)
  }
}
