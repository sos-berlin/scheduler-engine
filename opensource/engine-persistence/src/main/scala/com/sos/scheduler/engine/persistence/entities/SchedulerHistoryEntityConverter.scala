package com.sos.scheduler.engine.persistence.entities

import com.google.common.base.Strings._
import com.sos.scheduler.engine.data.job.SchedulerHistoryEntry
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.persistence.SchedulerDatabases._
import com.sos.scheduler.engine.persistence.entities.TaskHistoryEntity.schedulerDummyJobPath
import com.sos.scheduler.engine.persistence.entity.ObjectEntityConverter

class SchedulerHistoryEntityConverter extends ObjectEntityConverter[SchedulerHistoryEntry, Int, TaskHistoryEntity] {

  protected def toEntity(o: SchedulerHistoryEntry) = {
    val e = new TaskHistoryEntity
    e.id = o.id
    e.schedulerId = schedulerIdToDatabase(o.schedulerId)
    e.clusterMemberId = emptyToNull(o.clusterMemberId.string)
    e.jobPath = schedulerDummyJobPath
    e.startTime = instantToDatabase(o.startTime)
    e.endTime = (o.endTimeOption map instantToDatabase).orNull
    e.errorCode = emptyToNull(o.errorCode)
    e.errorText = emptyToNull(o.errorText)
    e.processId = o.processId
    e
  }

  protected def toEntityKey(id: Int) = Integer.valueOf(id)

  protected def toObject(e: TaskHistoryEntity) = new SchedulerHistoryEntry {
    val id = e.id
    val schedulerId = schedulerIdFromDatabase(e.schedulerId)
    val clusterMemberId = new ClusterMemberId(nullToEmpty(e.clusterMemberId))
    val startTime = databaseToDateTime(e.startTime)
    val endTimeOption = Option(e.startTime) map databaseToDateTime
    val errorCode = nullToEmpty(e.errorCode)
    val errorText = nullToEmpty(e.errorText)
    val parameterXml = nullToEmpty(e.parameterXml)
    val processId = e.processId + 0
  }
}
