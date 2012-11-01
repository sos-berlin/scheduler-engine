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
  @Column(name=""""SPOOLER_ID"""", nullable=false) @Id
  private[entities] var schedulerId: String = _

  @Column(name=""""CLUSTER_MEMBER_ID"""", nullable=false) @Id
  private[entities] var clusterMemberId: String = _

  @Column(name=""""PATH"""", nullable=false) @Id
  private[entities] var jobChainPath: String = _

  @Column(name=""""STOPPED"""" , nullable=false)
  private[entities] var isStopped: Boolean = _

  def this(k: JobChainEntity.Key) {
    this()
    schedulerId = k.schedulerId
    clusterMemberId = k.clusterMemberId
    jobChainPath = k.jobChainPath
  }

  override def toString = "JobChainEntity"+ Seq(schedulerId, clusterMemberId, jobChainPath, isStopped).mkString("(", ",", ")")
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
