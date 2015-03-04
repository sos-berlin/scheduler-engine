package com.sos.scheduler.engine.minicom.idispatch

/**
 * @author Joacim Zschimmer
 */
trait IDispatch extends IDispatchable {

  def getIdOfName(name: String): DISPID

  def invoke(dispId: DISPID, dispatchTypes: Set[DispatchType], arguments: Seq[Any], namedArguments: Seq[(DISPID, Any)] = Nil): Any
}
