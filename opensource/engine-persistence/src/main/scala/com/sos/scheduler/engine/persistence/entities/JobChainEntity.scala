package com.sos.scheduler.engine.persistence.entities

import java.io.Serializable
import javax.persistence._
import scala.reflect.BeanProperty

/** JPA-Entity für einen Jobkettenzustand.
  * <pre>
  * CREATE TABLE SCHEDULER_JOB_CHAINS (
  *   SPOOLER_ID varchar(100) NOT NULL,
  *   CLUSTER_MEMBER_ID varchar(100) NOT NULL,
  *   PATH varchar(255) NOT NULL,
  *   STOPPED boolean NOT NULL,
  *   PRIMARY KEY (spooler_id, cluster_member_id, path));
  * CREATE UNIQUE INDEX PRIMARY_KEY_1 ON SCHEDULER_JOB_CHAINS ( SPOOLER_ID, CLUSTER_MEMBER_ID, PATH )
  * </pre>*/
@Entity
@Table(name="SCHEDULER_JOB_CHAINS")
@IdClass(classOf[JobChainEntity.Key])
class JobChainEntity {
  private var _schedulerId: String = _
  private var _clusterMemberId: String = _
  private var _jobChainPath: String = _
  private var _isStopped: Boolean = _
  private var _nodes: java.util.List[JobChainNodeEntity] = _

  def this(k: JobChainEntity.Key) {
    this()
    _schedulerId = k.schedulerId
    _clusterMemberId = k.clusterMemberId
    _jobChainPath = k.jobChainPath
  }

  @Column(name="`SPOOLER_ID`", nullable=false) @Id
  private[entities] def getSchedulerId = _schedulerId

  private[entities] def setSchedulerId(o: String) {
    _schedulerId = o
  }

  @Column(name="`CLUSTER_MEMBER_ID`", nullable=false) @Id
  private[entities] def getClusterMemberId = _clusterMemberId

  private[entities] def setClusterMemberId(o: String) {
    _clusterMemberId = o
  }

  @Column(name="`PATH`", nullable=false) @Id
  private[entities] def getJobChainPath = _jobChainPath

  private[entities] def setJobChainPath(o: String) {
    _jobChainPath = o
  }

  @Column(name="`STOPPED`" , nullable=false)
  private[entities] def isStopped = _isStopped

  private[entities] def setStopped(o: Boolean) {
    _isStopped = o
  }

  override def toString = "JobChainEntity"+ Seq(_schedulerId, _clusterMemberId, _jobChainPath, _isStopped).mkString("(", ",", ")")
}

object JobChainEntity {
  case class Key(
      @BeanProperty var schedulerId: String,
      @BeanProperty var clusterMemberId: String,
      @BeanProperty var jobChainPath: String)
  extends Serializable {
    def this() = this(null: String, null: String, null: String)
  }
}
