package com.sos.scheduler.engine.minicom.types

import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
trait IDispatch extends IDispatchable {

  //def call(methodName: String, arguments: Seq[Any] = Nil): Any

  def getIdOfName(name: String): DISPID

  def invoke(dispId: DISPID, dispatchType: DispatchType, arguments: immutable.Seq[Any]): Any
}
