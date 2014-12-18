package com.sos.scheduler.engine.persistence.entities

import javax.persistence._

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
@IdClass(classOf[JobChainEntityKey])
class JobChainEntity {
  def this(k: JobChainEntityKey) {
    this()
    schedulerId = k.schedulerId
    clusterMemberId = k.clusterMemberId
    jobChainPath = k.jobChainPath
  }

  @Column(name=""""SPOOLER_ID"""", nullable=false) @Id
  var schedulerId: String = _

  @Column(name=""""CLUSTER_MEMBER_ID"""", nullable=false) @Id
  var clusterMemberId: String = _

  @Column(name=""""PATH"""", nullable=false) @Id
  var jobChainPath: String = _

  @Column(name=""""STOPPED"""" , nullable=false)
  var isStopped: Boolean = _

  override def toString = "JobChainEntity"+ Seq(schedulerId, clusterMemberId, jobChainPath, isStopped).mkString("(", ",", ")")
}
