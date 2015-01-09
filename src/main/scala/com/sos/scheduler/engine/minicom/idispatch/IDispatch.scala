package com.sos.scheduler.engine.minicom.idispatch

import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
trait IDispatch extends IDispatchable {

  def getIdOfName(name: String): DISPID

  def invoke(dispId: DISPID, dispatchType: DispatchType, arguments: immutable.Seq[Any]): Any
}
