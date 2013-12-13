package com.sos.scheduler.engine.persistence.entities

import com.google.common.base.Strings
import com.google.common.base.Strings.emptyToNull
import com.sos.scheduler.engine.data.base.IsString.stringOrNull
import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderState, OrderId, OrderPersistent, OrderKey}
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.persistence.SchedulerDatabases._
import com.sos.scheduler.engine.persistence.SchedulerDatabases.instantToDatabase
import com.sos.scheduler.engine.persistence.entity.ObjectEntityConverter

trait OrderEntityConverter extends ObjectEntityConverter[OrderPersistent, OrderKey, OrderEntity] {
  protected val schedulerId: SchedulerId

  final def toObject(e: OrderEntity) = OrderPersistent(
      jobChainPath = JobChainPath("/"+ e.jobChainPath),
      orderId = new OrderId(e.orderId),
      distributedNextTimeOption = Option(e.distributedNextTime) map databaseToInstant,
      occupyingClusterIdOption = Option(e.occupyingClusterMemberId) map ClusterMemberId,
      priority = e.priority,
      ordering = e.ordering,
      stateOption = Option(e.state) map OrderState.apply,
      initialStateOption = Option(e.initialState) map OrderState.apply,
      title = Strings.nullToEmpty(e.title),
      creationTimestampOption = Option(e.creationTimestamp) map databaseToInstant,
      modificationTimestampOption = Option(e.modificationTimestamp) map databaseToInstant,
      payloadXmlOption = Option(e.payload),
      runtimeXmlOption =  Option(e.runtime),
      xmlOption = Option(e.xml))

  final def toEntity(o: OrderPersistent): OrderEntity = {
    val e = new OrderEntity(toEntityKey(o.key))
    e.distributedNextTime = (o.distributedNextTimeOption map instantToDatabase).orNull
    e.occupyingClusterMemberId = stringOrNull(o.occupyingClusterIdOption)
    e.priority = o.priority
    e.ordering = o.ordering
    e.state = stringOrNull(o.stateOption)
    e.initialState = stringOrNull(o.initialStateOption)
    e.title = emptyToNull(o.title)
    e.creationTimestamp = (o.creationTimestampOption map instantToDatabase).orNull
    e.modificationTimestamp = (o.modificationTimestampOption map instantToDatabase).orNull
    e.payload = o.payloadXmlOption.orNull
    e.runtime = o.runtimeXmlOption.orNull
    e.xml = o.xmlOption.orNull
    e
  }

  final def toEntityKey(k: OrderKey) = OrderEntityKey(
    schedulerIdToDatabase(schedulerId),
    k.jobChainPath.withoutStartingSlash,
    k.id.string)
}
