package com.sos.scheduler.engine.persistence.entities

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

  @Column(name=""""SPOOLER_ID"""", nullable=false) @Id
  private [entities] var schedulerId: String = _

  @Column(name=""""CLUSTER_MEMBER_ID"""", nullable=false) @Id
  private [entities] var clusterMemberId: String = _

  @Column(name=""""PATH"""", nullable=false) @Id
  private [entities] var jobPath: String = _

  @Column(name=""""STOPPED"""" , nullable=false)
  private [entities] var isStopped: Boolean = _

  @Column(name=""""NEXT_START_TIME"""") @Temporal(TIMESTAMP) @Nullable
  private [entities] var nextStartTime: JavaDate = _

  override def toString = "JobEntity"+ Seq(schedulerId, clusterMemberId, jobPath, isStopped, nextStartTime).mkString("(", ",", ")")
}

object JobEntity {
  case class PrimaryKey(var schedulerId: String, var clusterMemberId: String, var jobPath: String) extends Serializable {
    def this() = this(null: String, null: String, null: String)
  }
}
