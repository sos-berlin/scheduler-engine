package com.sos.scheduler.engine.persistence.entities

import com.google.common.base.Strings
import com.google.common.base.Strings.emptyToNull
import com.sos.jobscheduler.base.generic.IsString.stringOrNull
import com.sos.jobscheduler.data.order.OrderId
import com.sos.jobscheduler.data.scheduler.SchedulerId
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order.{OrderKey, OrderPersistentState}
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.persistence.SchedulerDatabases.{instantToDatabase, _}
import com.sos.scheduler.engine.persistence.entity.ObjectEntityConverter

trait OrderEntityConverter extends ObjectEntityConverter[OrderPersistentState, OrderKey, OrderEntity] {
  protected val schedulerId: SchedulerId

  final def toObject(e: OrderEntity) = OrderPersistentState(
      jobChainPath = JobChainPath("/"+ e.jobChainPath),
      orderId = OrderId(e.orderId),
      distributedNextTimeOption = Option(e.distributedNextTime) map databaseToInstant,
      occupyingClusterIdOption = Option(e.occupyingClusterMemberId) map ClusterMemberId.apply,
      priority = e.priority,
      ordering = e.ordering,
      nodeIdOption = Option(e.state) map NodeId.apply,
      initialNodeIdOption = Option(e.initialState) map NodeId.apply,
      title = Strings.nullToEmpty(e.title),
      creationTimestampOption = Option(e.creationTimestamp) map databaseToInstant,
      modificationTimestampOption = Option(e.modificationTimestamp) map databaseToInstant,
      payloadXmlOption = Option(e.payload),
      runtimeXmlOption =  Option(e.runtime),
      xmlOption = Option(e.xml))

  final def toEntity(o: OrderPersistentState): OrderEntity = {
    val e = new OrderEntity(toEntityKey(o.key))
    e.distributedNextTime = (o.distributedNextTimeOption map instantToDatabase).orNull
    e.occupyingClusterMemberId = stringOrNull(o.occupyingClusterIdOption)
    e.priority = o.priority
    e.ordering = o.ordering
    e.state = stringOrNull(o.nodeIdOption)
    e.initialState = stringOrNull(o.initialNodeIdOption)
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
