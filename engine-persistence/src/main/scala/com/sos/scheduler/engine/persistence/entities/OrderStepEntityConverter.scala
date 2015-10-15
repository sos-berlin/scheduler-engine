package com.sos.scheduler.engine.persistence.entities

import com.google.common.base.Strings.emptyToNull
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.persistence.SchedulerDatabases.{instantToDatabase, _}
import com.sos.scheduler.engine.persistence.entity.ObjectEntityConverter

trait OrderStepEntityConverter extends ObjectEntityConverter[OrderStepPersistentState, OrderStepPersistentState.Key, OrderStepEntity] {

  final def toObject(e: OrderStepEntity) = OrderStepPersistentState(
    orderHistoryId = OrderHistoryId(e.orderHistoryId),
    step = e.step,
    taskId = TaskId(e.taskId),
    orderState = OrderState(e.state),
    startTime = databaseToInstant(e.startTime),
    endTimeOption = Option(e.endTime) map databaseToInstant,
    isError = e.isError != null && e.isError,
    errorText = Option(e.errorText) getOrElse "",
    errorCodeOption = Option(e.errorCode) map MessageCode.apply)

  final def toEntity(o: OrderStepPersistentState): OrderStepEntity = {
    val e = new OrderStepEntity(toEntityKey(o.key))
    e.orderHistoryId = o.orderHistoryId.value
    e.step = o.step
    e.taskId = o.taskId.value
    e.state = emptyToNull(o.orderState.string)
    e.errorCode = (o.errorCodeOption map { _.string }).orNull
    e.errorText = emptyToNull(o.errorText)
    e.startTime = instantToDatabase(o.startTime)
    e.endTime = (o.endTimeOption map instantToDatabase).orNull
    e
  }

  final def toEntityKey(k: OrderStepPersistentState.Key) = OrderStepEntityKey(k.orderHistoryId.value, k.step)
}
