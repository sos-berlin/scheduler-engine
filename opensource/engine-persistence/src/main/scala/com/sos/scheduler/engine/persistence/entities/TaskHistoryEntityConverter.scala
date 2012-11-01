package com.sos.scheduler.engine.persistence.entities

import com.google.common.base.Strings._
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskHistoryEntry
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.persistence.SchedulerDatabases._
import com.sos.scheduler.engine.persistence.entity.ObjectEntityConverter

trait TaskHistoryEntityConverter extends ObjectEntityConverter[TaskHistoryEntry, Int, TaskHistoryEntity] {

  protected def toEntity(o: TaskHistoryEntry) = {
    val e = new TaskHistoryEntity
    e.id = o.id
    e.schedulerId = schedulerIdToDatabase(o.schedulerId)
    e.clusterMemberId = emptyToNull(o.clusterMemberId.string)
    e.jobPath = o.jobPath.withoutStartingSlash
    e.startTime = new java.util.Date(o.startTime.getMillis)
    e.endTime = (o.endTimeOption map instantToDatabase).orNull
    e.cause = o.cause
    e.steps = (o.stepsOption map Integer.valueOf).orNull
    e.errorCode = emptyToNull(o.errorCode)
    e.errorText = emptyToNull(o.errorText)
    e.processId = (o.processIdOption map Integer.valueOf).orNull
    //e.log = o.log
    e
  }

  protected def toEntityKey(id: Int) = Integer.valueOf(id)

  protected def toObject(e: TaskHistoryEntity) = new TaskHistoryEntry {
    val id = e.id
    val schedulerId = schedulerIdFromDatabase(e.schedulerId)
    val clusterMemberId = new ClusterMemberId(nullToEmpty(e.clusterMemberId))
    val jobPath = JobPath.of("/" + e.jobPath)
    val startTime = databaseToDateTime(e.startTime)
    val endTimeOption = Option(e.startTime) map databaseToDateTime
    val cause = nullToEmpty(e.cause)
    val stepsOption = Option(e.steps) map { _ + 0 }
    val errorCode = nullToEmpty(e.errorCode)
    val errorText = nullToEmpty(e.errorText)
    val parameterXml = nullToEmpty(e.parameterXml)
    val processIdOption = Option(e.processId) map { _ + 0 }
  }
}

object TaskHistoryEntityConverter extends TaskHistoryEntityConverter
