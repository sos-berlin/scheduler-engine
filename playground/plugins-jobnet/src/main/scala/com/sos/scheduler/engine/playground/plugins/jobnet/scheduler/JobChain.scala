package com.sos.scheduler.engine.playground.plugins.jobnet.scheduler

import com.sos.scheduler.engine.data.order.OrderId
import com.sos.scheduler.engine.kernel.folder.FileBased

final class JobChain extends FileBased {
  def onCppProxyInvalidated() {}

  def getFileBasedType = ???

  def getPath = ???

  def addOrder(o: Order) = ???

  def order(o: OrderId): Order = ???
}
