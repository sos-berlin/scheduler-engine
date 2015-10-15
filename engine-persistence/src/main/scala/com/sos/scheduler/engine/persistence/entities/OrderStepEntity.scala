package com.sos.scheduler.engine.persistence.entities

import javax.annotation.Nullable
import javax.persistence.TemporalType.TIMESTAMP
import javax.persistence._

/**
 * @author Joacim Zschimmer
 */
@Entity
@Table(name="SCHEDULER_ORDER_STEP_HISTORY")
@IdClass(classOf[OrderStepEntityKey])
class OrderStepEntity {

  private[entities] def this(k: OrderStepEntityKey) {
    this()
    orderHistoryId = k.orderHistoryId
    step = k.step
  }

  @Column(name=""""HISTORY_ID"""", nullable = false) @Id
  var orderHistoryId: Int = _

  @Column(name=""""STEP"""", nullable = false) @Id
  var step: Int= _

  @Column(name=""""TASK_ID"""", nullable = false)
  var taskId: Int = _

  @Column(name=""""STATE"""", nullable = false)
  var state: String = _

  @Column(name=""""START_TIME"""", nullable = false) @Temporal(TIMESTAMP)
  var startTime: java.util.Date = _

  @Column(name=""""END_TIME"""") @Nullable @Temporal(TIMESTAMP)
  var endTime: java.util.Date = _

  @Column(name=""""ERROR"""") @Nullable
  var isError: java.lang.Boolean = _

  @Column(name=""""ERROR_CODE"""") @Nullable
  var errorCode: String = _

  @Column(name=""""ERROR_TEXT"""") @Nullable
  var errorText: String = _

  override def toString = s"OrderStepEntity($orderHistoryId-$step $state)"
}
