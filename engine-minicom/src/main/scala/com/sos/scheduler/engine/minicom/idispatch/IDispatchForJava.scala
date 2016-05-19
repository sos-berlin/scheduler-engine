package com.sos.scheduler.engine.minicom.idispatch

import com.sos.scheduler.engine.minicom.idispatch.IDispatchForJava._
import scala.collection.JavaConversions._

/**
  * @author Joacim Zschimmer
  */
final class IDispatchForJava(iDispatch: IDispatch) {

  def getIdOfName(name: String): DISPID =
    iDispatch.getIdOfName(name)

  def invoke(
    dispId: DISPID,
    dispatchTypes: java.util.Set[DispatchType],
    arguments: java.util.List[Object],
    namedArguments: java.util.List[(DISPID, Object)]): Object
  =
    anyToAnyRef(iDispatch.invoke(dispId, dispatchTypes.toSet, arguments, namedArguments))
}

object IDispatchForJava {
  def anyToAnyRef(o: Any): AnyRef = o.asInstanceOf[AnyRef]  // ???
}
