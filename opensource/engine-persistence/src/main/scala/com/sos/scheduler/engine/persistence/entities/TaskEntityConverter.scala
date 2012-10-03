package com.sos.scheduler.engine.persistence.entities

import com.google.common.base.Strings._
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.job.TaskPersistent
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.persistence.SchedulerDatabases._
import com.sos.scheduler.engine.persistence.entity.ObjectEntityConverter

trait TaskEntityConverter extends ObjectEntityConverter[TaskPersistent, TaskId, TaskEntity] {
  import TaskEntityConverter._

  protected val schedulerId: SchedulerId
  protected val clusterMemberId: ClusterMemberId
  protected lazy val schedulerIdString = idForDatabase(schedulerId)
  protected lazy val clusterMemberIdString = emptyToNull(clusterMemberId.string)

  protected def toEntityKey(taskId: TaskId) = java.lang.Integer.valueOf(taskId.value)

  protected def toEntity(o: TaskPersistent) =  {
    val e = new TaskEntity(o.taskId.value)
    e.schedulerId = schedulerIdString
    e.clusterMemberId = clusterMemberIdString
    e.jobPath = toFieldValue(o.jobPath)
    e.enqueueTime = dateTimeToDatabase(o.enqueueTime)
    e.startTime = (o.startTimeOption map dateTimeToDatabase).orNull
    e.parameterXml = emptyToNull(o.parametersXml)
    e.xml = emptyToNull(o.xml)
    e
  }

  def toObject(e: TaskEntity) =
    TaskPersistent(
      taskId = TaskId(e.taskId),
      jobPath = JobPath.of("/"+ e.jobPath),
      enqueueTime = databaseToDateTime(e.enqueueTime),
      startTimeOption = Option(e.startTime) map databaseToDateTime,
      parametersXml = nullToEmpty(e.parameterXml),
      xml = nullToEmpty(e.xml))
}

object TaskEntityConverter {
  def toFieldValue(o: JobPath) = o.withoutStartingSlash
}
