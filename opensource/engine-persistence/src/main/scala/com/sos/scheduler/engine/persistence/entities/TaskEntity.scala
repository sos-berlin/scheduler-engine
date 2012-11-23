package com.sos.scheduler.engine.persistence.entities

import com.google.common.base.Strings.{emptyToNull, nullToEmpty}
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.{TaskId, TaskPersistent}
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.persistence.SchedulerDatabases._
import java.util.{Date => JavaDate}
import javax.annotation.Nullable
import javax.persistence.TemporalType._
import javax.persistence._

/** JPA-Entity für auf ihren Start wartende Tasks.
  * <pre>
  * CREATE TABLE SCHEDULER_TASKS (
  *   TASK_ID integer PRIMARY KEY NOT NULL,
  *   SPOOLER_ID varchar(100) NOT NULL,
  *   CLUSTER_MEMBER_ID varchar(100),
  *   JOB_NAME varchar(255) NOT NULL,
  *   ENQUEUE_TIME timestamp,
  *   START_AT_TIME timestamp,
  *   PARAMETERS clob,
  *   TASK_XML clob);
  * </pre> */
@Entity
@Table(name = "SCHEDULER_TASKS")
class TaskEntity {

  @Column(name=""""TASK_ID"""") @Id
  var taskId: Int = _

  @Column(name=""""SPOOLER_ID"""", nullable=false)
  var schedulerId: String = _

  @Column(name=""""CLUSTER_MEMBER_ID"""") @Nullable
  var clusterMemberId: String = _

  @Column(name=""""JOB_NAME"""", length=255, nullable=false)
  var jobPath: String = _

  @Column(name=""""ENQUEUE_TIME"""") @Temporal(TIMESTAMP) @Nullable   // Entgegen der Tabellendefinition ist das Feld immer gefüllt.
  var enqueueTime: JavaDate = _

  @Column(name=""""START_AT_TIME"""", nullable=false) @Temporal(TIMESTAMP)
  var startTime: JavaDate = _

  @Column(name=""""PARAMETERS"""") @Lob @Nullable
  var parameterXml: String = _

  @Column(name=""""TASK_XML"""")  @Lob @Nullable
  var xml: String = _


  def this(taskIdInt: Int) = {
    this()
    taskId = taskIdInt
  }

  override final def toString =
    "TaskEntity" + Seq(taskId, schedulerId, clusterMemberId, jobPath, enqueueTime, startTime).mkString("(", ",", ")")
}
