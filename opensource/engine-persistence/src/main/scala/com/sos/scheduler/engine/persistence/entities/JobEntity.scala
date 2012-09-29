package com.sos.scheduler.engine.persistence.entities

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.JobPersistentState
import com.sos.scheduler.engine.data.scheduler.{SchedulerId, ClusterMemberId}
import com.sos.scheduler.engine.persistence.SchedulerDatabases._
import com.sos.scheduler.engine.persistence.SchedulerDatabases.databaseToDateTime
import java.io.Serializable
import java.util.{Date => JavaDate}
import javax.annotation.Nullable
import javax.persistence.TemporalType.TIMESTAMP
import javax.persistence._

/** JPA-Entity für einen Jobzustand.
  * <pre>
  * CREATE TABLE SCHEDULER_JOBS (
  *   SPOOLER_ID varchar(100) NOT NULL,
  *   CLUSTER_MEMBER_ID varchar(100) NOT NULL,
  *   PATH varchar(255) NOT NULL,
  *   STOPPED boolean NOT NULL,
  *   NEXT_START_TIME varchar(24),
  *   PRIMARY KEY (spooler_id, cluster_member_id, path));
  * CREATE UNIQUE INDEX PRIMARY_KEY_B ON SCHEDULER_JOBS (SPOOLER_ID, CLUSTER_MEMBER_ID, PATH);
  * </pre>*/
@Entity
@Table(name = "SCHEDULER_JOBS")
@IdClass(classOf[JobEntity.PrimaryKey])
class JobEntity {

  @Column(name="\"SPOOLER_ID\"", nullable=false) @Id
  var _schedulerId: String = _

  @Column(name="\"CLUSTER_MEMBER_ID\"", nullable=false) @Id
  var _clusterMemberId: String = _

  @Column(name="\"PATH\"", nullable=false) @Id
  var _jobPath: String = _

  @Column(name="\"STOPPED\"" , nullable=false)
  var isStopped: Boolean = _

  @Column(name="\"NEXT_START_TIME\"") @Temporal(TIMESTAMP) @Nullable
  var _nextStartTime: JavaDate = _


  def this(key: JobEntity.PrimaryKey) {
    this()
    _schedulerId = key._schedulerId
    _clusterMemberId = key._clusterMemberId
    _jobPath = key._jobPath
  }

  final def schedulerId = schedulerIdFromDatabase(_schedulerId)

  final def clusterMemberId = if (_clusterMemberId == "-") ClusterMemberId.empty else new ClusterMemberId(_clusterMemberId)

  final def toObject = JobPersistentState(
    jobPath = JobPath.of("/"+ _jobPath),
    isPermanentlyStopped = isStopped,
    nextStartTimeOption = Option(_nextStartTime) map databaseToDateTime)
}

object JobEntity {
  def apply(schedulerId: SchedulerId, clusterMemberId: ClusterMemberId, o: JobPersistentState) = {
    val e = new JobEntity(PrimaryKey(schedulerId, clusterMemberId, o.jobPath))
    e.isStopped = o.isPermanentlyStopped
    e._nextStartTime = (o.nextStartTimeOption map dateTimeToDatabase).orNull
    e
  }

  case class PrimaryKey(var _schedulerId: String, var _clusterMemberId: String, var _jobPath: String) extends Serializable {
    def this() = this(null: String, null: String, null: String)
  }

  object PrimaryKey {
    def apply(s: SchedulerId, clusterMemberId: ClusterMemberId, jobPath: JobPath) = new PrimaryKey(
      idForDatabase(s),
      if (clusterMemberId.isEmpty) "-" else clusterMemberId.asString,
      jobPath.withoutStartingSlash)
  }
}
