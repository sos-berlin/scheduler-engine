package com.sos.scheduler.taskserver.comrpc.calls

import com.sos.scheduler.taskserver.comrpc.IUnknown
import com.sos.scheduler.taskserver.comrpc.types.DISPID
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
trait Result

object EmptyResult extends Result

final case class CreateInstanceResult(iUnknown: IUnknown)
  extends Result

final case class QueryInterfaceResult(iUnknown: IUnknown)
  extends Result

final case class GetIDsOfNamesResult(dispatchIds: immutable.Seq[DISPID])
  extends Result

final case class InvokeResult(result: Any)
  extends Result

final case class CallResult(result: Any)
  extends Result
