package com.sos.scheduler.engine.persistence.entities

import com.google.common.base.Strings.emptyToNull
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.scheduler.SchedulerId
import com.sos.scheduler.engine.persistence.SchedulerDatabases.{instantToDatabase, _}
import com.sos.scheduler.engine.persistence.entity.ObjectEntityConverter

trait OrderHistoryEntityConverter extends ObjectEntityConverter[OrderHistoryPersistentState, OrderHistoryId, OrderHistoryEntity] {
  protected val schedulerId: SchedulerId

  final def toObject(e: OrderHistoryEntity) = OrderHistoryPersistentState(
    historyId = OrderHistoryId(e.orderHistoryId),
    jobChainPath = JobChainPath(s"/${e.jobChainPath}"),
    orderId = OrderId(e.orderId),
    schedulerId = SchedulerId(e.schedulerId),
    orderState = OrderState(e.state),
    stateText = Option(e.stateText) getOrElse "",
    title = Option(e.title) getOrElse "",
    startTime = databaseToInstant(e.startTime),
    endTimeOption = Option(e.endTime) map databaseToInstant,
    log = Option(e.log) getOrElse "")

  final def toEntity(o: OrderHistoryPersistentState): OrderHistoryEntity = {
    val e = new OrderHistoryEntity(toEntityKey(o.key))
    e.jobChainPath = o.jobChainPath.withoutStartingSlash
    e.orderId = o.orderId.string
    e.schedulerId = o.schedulerId.string
    e.state = emptyToNull(o.orderState.string)
    e.stateText = emptyToNull(o.stateText)
    e.title = emptyToNull(o.title)
    e.startTime = instantToDatabase(o.startTime)
    e.endTime = (o.endTimeOption map instantToDatabase).orNull
    e.log = emptyToNull(o.log)
    e
  }

  final def toEntityKey(k: OrderHistoryId) = k
}
