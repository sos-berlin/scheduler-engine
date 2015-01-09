package com.sos.scheduler.engine.minicom.comrpc.calls

import com.sos.scheduler.engine.minicom.types.{DISPID, IDispatch, IDispatchable}
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
trait Result

object EmptyResult extends Result

final case class CreateInstanceResult(iDispatch: IDispatchable)
extends Result

final case class QueryInterfaceResult(iDispatch: IDispatch)
extends Result

final case class GetIDsOfNamesResult(dispatchIds: immutable.Seq[DISPID])
extends Result

final case class InvokeResult(result: Any)
extends Result
