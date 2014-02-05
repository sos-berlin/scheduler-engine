package com.sos.scheduler.engine.persistence.entities

import javax.annotation.Nullable
import javax.persistence.TemporalType.TIMESTAMP
import javax.persistence._

/** JPA-Entity für einen Auftrag.
  * <pre>
  * CREATE TABLE SCHEDULER_ORDERS (
  *    JOB_CHAIN varchar(250) NOT NULL,
  *    ID varchar(250) NOT NULL,
  *    SPOOLER_ID varchar(100) NOT NULL,
  *    DISTRIBUTED_NEXT_TIME timestamp,
  *    OCCUPYING_CLUSTER_MEMBER_ID varchar(100),
  *    PRIORITY integer NOT NULL,
  *    STATE varchar(100),
  *    STATE_TEXT varchar(100),
  *    TITLE varchar(200),
  *    CREATED_TIME timestamp NOT NULL,
  *    MOD_TIME timestamp,
  *    ORDERING integer NOT NULL,
  *    PAYLOAD clob,
  *    INITIAL_STATE varchar(100),
  *    RUN_TIME clob,
  *    ORDER_XML clob,
  *    PRIMARY KEY (spooler_id, job_chain, id));
  * CREATE UNIQUE INDEX PRIMARY_KEY_E ON SCHEDULER_ORDERS ( SPOOLER_ID, JOB_CHAIN, ID )
  * </pre> */
@Entity
@Table(name="SCHEDULER_ORDERS")
@IdClass(classOf[OrderEntityKey])
class OrderEntity {
  @Column(name=""""SPOOLER_ID"""", nullable=false) @Id
  private[entities] var schedulerId: String = _

  @Column(name=""""JOB_CHAIN"""", nullable=false) @Id
  private[entities] var jobChainPath: String = _

  @Column(name=""""ID"""", nullable=false) @Id
  private[entities] var orderId: String = _

  @Column(name=""""DISTRIBUTED_NEXT_TIME"""") @Nullable @Temporal(TIMESTAMP)
  private[entities] var distributedNextTime: java.util.Date = _

  @Column(name=""""OCCUPYING_CLUSTER_MEMBER_ID"""") @Nullable
  private[entities] var occupyingClusterMemberId: String = _

  @Column(name=""""PRIORITY"""")
  private[entities] var priority: Int = _

  @Column(name=""""ORDERING"""")
  private[entities] var ordering: Int = _

  @Column(name=""""STATE"""") @Nullable
  private[entities] var state: String = _

  @Column(name=""""INITIAL_STATE"""") @Nullable
  private[entities] var initialState: String = _

  @Column(name=""""STATE_TEXT"""") @Nullable
  private[entities] var stateText: String = _

  @Column(name=""""TITLE"""") @Nullable
  private[entities] var title: String = _

  @Column(name=""""CREATED_TIME"""") @Temporal(TIMESTAMP)
  private[entities] var creationTimestamp: java.util.Date = _

  @Column(name=""""MOD_TIME"""") @Nullable @Temporal(TIMESTAMP)
  private[entities] var modificationTimestamp: java.util.Date = _

  @Column(name=""""PAYLOAD"""") @Nullable
  private[entities] var payload: String = _

  @Column(name=""""RUN_TIME"""") @Nullable
  private[entities] var runtime: String = _

  @Column(name=""""ORDER_XML"""") @Nullable
  private[entities] var xml: String = _

  private[entities] def this(k: OrderEntityKey) {
    this()
    schedulerId = k.schedulerId
    jobChainPath = k.jobChainPath
    orderId = k.orderId
  }
}

