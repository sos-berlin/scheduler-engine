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
  protected lazy val schedulerIdDBString = schedulerIdToDatabase(schedulerId)
  protected lazy val clusterMemberIdDBString = emptyToNull(clusterMemberId.string)

  protected def toEntityKey(taskId: TaskId) = java.lang.Integer.valueOf(taskId.value)

  protected def toEntity(o: TaskPersistent) =  {
    val e = new TaskEntity(o.taskId.value)
    e.schedulerId = schedulerIdDBString
    e.clusterMemberId = clusterMemberIdDBString
    e.jobPath = toDBString(o.jobPath)
    e.enqueueTime = instantToDatabase(o.enqueueTime)
    e.startTime = (o.startTimeOption map instantToDatabase).orNull
    e.parameterXml = emptyToNull(o.parametersXml)
    e.xml = emptyToNull(o.xml)
    e
  }

  def toObject(e: TaskEntity) =
    TaskPersistent(
      taskId = TaskId(e.taskId),
      jobPath = JobPath("/"+ e.jobPath),
      enqueueTime = databaseToInstant(e.enqueueTime),
      startTimeOption = Option(e.startTime) map databaseToInstant,
      parametersXml = nullToEmpty(e.parameterXml),
      xml = nullToEmpty(e.xml))
}

object TaskEntityConverter {
  def toDBString(o: JobPath) = o.withoutStartingSlash
}
