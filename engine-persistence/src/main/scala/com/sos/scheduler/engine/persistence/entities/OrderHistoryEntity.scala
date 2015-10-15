package com.sos.scheduler.engine.persistence.entities

import com.sos.scheduler.engine.data.order.OrderHistoryId
import javax.annotation.Nullable
import javax.persistence.TemporalType.TIMESTAMP
import javax.persistence._

/**
 * @author Joacim Zschimmer
 */
@Entity
@Table(name="SCHEDULER_ORDER_HISTORY")
class OrderHistoryEntity {

  private[entities] def this(k: OrderHistoryId) {
    this()
    orderHistoryId = k.value
  }

  @Column(name=""""HISTORY_ID"""", nullable = false) @Id
  var orderHistoryId: Int = _

  @Column(name=""""JOB_CHAIN"""", nullable = false)
  var jobChainPath: String = _

  @Column(name=""""ORDER_ID"""", nullable = false)
  var orderId: String = _

  @Column(name=""""SPOOLER_ID"""", nullable = false)
  var schedulerId: String = _

  @Column(name=""""TITLE"""") @Nullable
  var title: String = _

  @Column(name=""""STATE"""") @Nullable
  var state: String = _

  @Column(name=""""STATE_TEXT"""") @Nullable
  var stateText: String = _

  @Column(name=""""START_TIME"""", nullable = false) @Temporal(TIMESTAMP)
  var startTime: java.util.Date = _

  @Column(name=""""END_TIME"""") @Nullable @Temporal(TIMESTAMP)
  var endTime: java.util.Date = _

  @Column(name=""""LOG"""") @Nullable
  var log: String = _

  override def toString = s"OrderHistoryEntity($jobChainPath:$orderId $orderHistoryId)"
}
