package com.sos.scheduler.engine.persistence.entities

import javax.annotation.Nullable
import javax.persistence._

/** JPA-Entity für einen Jobkettenzustand.
  * <pre>
  * CREATE TABLE SCHEDULER_JOB_CHAIN_NODES (
  *    SPOOLER_ID varchar(100) NOT NULL,
  *    CLUSTER_MEMBER_ID varchar(100) NOT NULL,
  *    JOB_CHAIN varchar(250) NOT NULL,
  *    ORDER_STATE varchar(100) NOT NULL,
  *    ACTION varchar(100),
  *    PRIMARY KEY (spooler_id, cluster_member_id, job_chain, order_state)
  *  CREATE UNIQUE INDEX PRIMARY_KEY_8 ON SCHEDULER_JOB_CHAIN_NODES ( SPOOLER_ID, CLUSTER_MEMBER_ID, JOB_CHAIN, ORDER_STATE )
  * </pre> */
@Entity
@Table(name="SCHEDULER_JOB_CHAIN_NODES")
@IdClass(classOf[JobChainNodeEntityKey])
class JobChainNodeEntity {
  @Column(name=""""SPOOLER_ID"""", nullable=false) @Id
  var schedulerId: String = _

  @Column(name=""""CLUSTER_MEMBER_ID"""", nullable=false) @Id
  var clusterMemberId: String = _

  @Column(name=""""JOB_CHAIN"""", nullable=false) @Id
  var jobChainPath: String = _

  @Column(name=""""ORDER_STATE"""", nullable=false) @Id
  var orderState: String = _

  @Column(name=""""ACTION"""") @Nullable
  var action: String= _

  def this(k: JobChainNodeEntityKey) {
    this()
    schedulerId = k.schedulerId
    clusterMemberId = k.clusterMemberId
    jobChainPath = k.jobChainPath
    orderState = k.orderState
  }

  override def toString = "JobChainEntity"+ Seq(schedulerId, clusterMemberId, jobChainPath, orderState, action).mkString("(", ",", ")")
}
