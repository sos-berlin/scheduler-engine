package com.sos.scheduler.engine.minicom.inject

import com.google.inject.Provides
import com.sos.scheduler.engine.common.guice.ScalaAbstractModule
import com.sos.scheduler.engine.minicom.IDispatchFactory
import com.sos.scheduler.engine.minicom.comrpc.CallExecutor
import com.sos.scheduler.engine.minicom.comrpc.CallExecutor._
import com.sos.scheduler.engine.minicom.comrpc.calls.{Call, Result}
import com.sos.scheduler.engine.minicom.types.{CLSID, IDispatchable, IID}
import javax.inject.Singleton
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
final class MinicomModule(iUnknownFactories: immutable.Iterable[IDispatchFactory]) extends ScalaAbstractModule {

  protected def configure() = {}

  @Provides @Singleton
  private def executeCall(o: CallExecutor): Call ⇒ Result = o.execute

  @Provides @Singleton
  private def createInstanceByCLSID: CreateIDispatchableByCLSID = {
    val clsidToFactoryMap = (iUnknownFactories map { o ⇒ o.clsid → o }).toMap
    def createIDispatchable(clsId: CLSID, iid: IID): IDispatchable = {
      val factory = clsidToFactoryMap(clsId)
      require(factory.iid == iid, s"IID $iid is not supported by $factory")
      factory()
    }
    createIDispatchable
  }
}
