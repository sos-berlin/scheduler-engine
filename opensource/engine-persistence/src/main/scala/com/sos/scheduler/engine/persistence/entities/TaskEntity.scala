package com.sos.scheduler.engine.persistence.entities

import com.google.common.base.Strings.{emptyToNull, nullToEmpty}
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.{TaskId, TaskObject}
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

  @Column(name="\"TASK_ID\"") @Id
  var _taskId: Int = _

  @Column(name="\"SPOOLER_ID\"", nullable=false)
  var _schedulerId: String = _

  @Column(name="\"CLUSTER_MEMBER_ID\"") @Nullable
  var _clusterMemberId: String = _

  @Column(name="\"JOB_NAME\"", length=255, nullable=false)
  var _jobPath: String = _

  @Column(name="\"ENQUEUE_TIME\"") @Temporal(TIMESTAMP) @Nullable   // Entgegen der Tabellendefinition ist das Feld immer gefüllt.
  var _enqueueTime: JavaDate = _

  @Column(name="\"START_AT_TIME\"", nullable=false) @Temporal(TIMESTAMP)
  var _startTime: JavaDate = _

  @Column(name="\"PARAMETERS\"") @Lob @Nullable
  var _parameterXml: String = _

  @Column(name="\"TASK_XML\"")  @Lob @Nullable
  var _xml: String = _


  def this(taskId: Int) = {
    this()
    _taskId = taskId
  }

  final def schedulerId = schedulerIdFromDatabase(_schedulerId)

  final def clusterMemberId = Option(_clusterMemberId) map { o => new ClusterMemberId(o) }

  final def toObject = TaskObject(
    taskId = TaskId(_taskId),
    jobPath = JobPath.of("/"+ _jobPath),
    enqueueTime = databaseToDateTime(_enqueueTime),
    startTimeOption = Option(_startTime) map databaseToDateTime,
    parametersXml = nullToEmpty(_parameterXml),
    xml = nullToEmpty(_xml))

  override final def toString = "TaskEntity(" + (Seq(_taskId, _schedulerId, _clusterMemberId, _jobPath, _enqueueTime, _startTime) mkString ",") + ")"
}

object TaskEntity {
  def apply(schedulerId: SchedulerId, clusterMemberId: ClusterMemberId, o: TaskObject) = {
    val e = new TaskEntity(o.taskId.value)
    e._schedulerId = toFieldValue(schedulerId)
    e._clusterMemberId = toFieldValue(clusterMemberId)
    e._jobPath = toFieldValue(o.jobPath)
    e._enqueueTime = dateTimeToDatabase(o.enqueueTime)
    e._startTime = (o.startTimeOption map dateTimeToDatabase).orNull
    e._parameterXml = emptyToNull(o.parametersXml)
    e._xml = emptyToNull(o.xml)
    e
  }

  def toFieldValue(o: SchedulerId) = idForDatabase(o)

  def toFieldValue(o: ClusterMemberId) = emptyToNull(o.string)

  def toFieldValue(o: JobPath) = o.withoutStartingSlash
}
