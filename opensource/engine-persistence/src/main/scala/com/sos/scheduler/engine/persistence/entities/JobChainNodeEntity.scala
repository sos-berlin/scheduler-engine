package com.sos.scheduler.engine.persistence.entities

import java.io.Serializable
import java.util.{Date => JavaDate}
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
@IdClass(classOf[JobChainNodeEntity.Key])
class JobChainNodeEntity {
  @Column(name=""""SPOOLER_ID"""", nullable=false) @Id
  private[entities] var schedulerId: String = _

  @Column(name=""""CLUSTER_MEMBER_ID"""", nullable=false) @Id
  private[entities] var clusterMemberId: String = _

  @Column(name=""""JOB_CHAIN"""", nullable=false) @Id
  private[entities] var jobChainPath: String = _

  @Column(name=""""ORDER_STATE"""", nullable=false) @Id
  private[entities] var orderState: String = _

  @Column(name=""""ACTION"""") @Nullable
  private[entities] var action: String= _

  def this(k: JobChainNodeEntity.Key) {
    this()
    schedulerId = k.schedulerId
    clusterMemberId = k.clusterMemberId
    jobChainPath = k.jobChainPath
    orderState = k.orderState
  }

  override def toString = "JobChainEntity"+ Seq(schedulerId, clusterMemberId, jobChainPath, orderState, action).mkString("(", ",", ")")
}

object JobChainNodeEntity {
  case class Key(var schedulerId: String, var clusterMemberId: String, var jobChainPath: String, orderState: String) extends Serializable {
    def this() = this(null: String, null: String, null: String, null: String)
  }
}
