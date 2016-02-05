package com.sos.scheduler.engine.persistence.entities

import javax.annotation.Nullable
import javax.persistence.TemporalType._
import javax.persistence._

/** JPA-Entity für gelaufende Tasks.
  *
  * CREATE TABLE SCHEDULER_HISTORY (
  *   ID integer PRIMARY KEY NOT NULL,
  *   SPOOLER_ID varchar(100) NOT NULL,
  *   CLUSTER_MEMBER_ID varchar(100),
  *   JOB_NAME varchar(255) NOT NULL,
  *   START_TIME timestamp NOT NULL,
  *   END_TIME timestamp,
  *   CAUSE varchar(50),
  *   STEPS integer,
  *   EXIT_CODE integer,
  *   ERROR boolean,
  *   ERROR_CODE varchar(50),
  *   ERROR_TEXT varchar(250),
  *   PARAMETERS clob,
  *   LOG blob,
  *   PID integer );
  * CREATE INDEX SCHEDULER_HISTORY_JOB_NAME ON SCHEDULER_HISTORY ( JOB_NAME );
  * CREATE INDEX SCHEDULER_HISTORY_SPOOLER_ID ON SCHEDULER_HISTORY ( SPOOLER_ID );
  * CREATE INDEX SCHEDULER_HISTORY_START_TIME ON SCHEDULER_HISTORY ( START_TIME );
  * CREATE INDEX SCHEDULER_H_CLUSTER_MEMBER ON SCHEDULER_HISTORY ( CLUSTER_MEMBER_ID )
  * @author Joacim Zschimmer */
@Entity
@Table(name = "SCHEDULER_HISTORY") class TaskHistoryEntity {

  @Column(name=""""ID"""", nullable=false) @Id
  var id: Int = _

  @Column(name=""""SPOOLER_ID"""", nullable=false)
  var schedulerId: String = _

  @Column(name=""""CLUSTER_MEMBER_ID"""") @Nullable
  var clusterMemberId: String = _

  @Column(name=""""JOB_NAME"""", nullable=false)
  var jobPath: String = _

  @Column(name=""""START_TIME"""", nullable=false) @Temporal(TIMESTAMP)
  var startTime: java.util.Date = _

  @Column(name=""""END_TIME"""") @Temporal(TIMESTAMP) @Nullable
  var endTime: java.util.Date = _

  @Column(name=""""CAUSE"""") @Nullable
  var cause: String = null

  @Column(name=""""STEPS"""") @Nullable
  var steps: java.lang.Integer = _

  @Column(name=""""ERROR_CODE"""") @Nullable
  var errorCode: String = _

  @Column(name=""""ERROR_TEXT"""") @Nullable
  var errorText: String = _

  @Column(name=""""PARAMETERS"""") @Nullable
  var parameterXml: String = _

  @Column(name=""""PID"""") @Nullable
  var processId: java.lang.Integer = _

  @Column(name=""""AGENT_URL"""") @Nullable
  var agentUrl: String = _

//  @Column(name=""""LOG"""") @Nullable
//  var compressedLog: Array[Byte]
}

object TaskHistoryEntity {
  final val schedulerDummyJobPath = "(Spooler)"
}
