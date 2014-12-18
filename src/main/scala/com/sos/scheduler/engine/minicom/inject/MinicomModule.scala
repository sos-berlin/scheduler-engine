package com.sos.scheduler.engine.minicom.inject

import com.google.inject.TypeLiteral
import com.sos.scheduler.engine.common.guice.ScalaAbstractModule
import com.sos.scheduler.engine.minicom.IUnknownFactory
import com.sos.scheduler.engine.minicom.comrpc.CallExecutor._
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
final class MinicomModule(iUnknownFactories: immutable.Iterable[IUnknownFactory]) extends ScalaAbstractModule {

  protected def configure() = {
    bind(new TypeLiteral[ClsidToFactory] {}) toInstance (iUnknownFactories map { o ⇒ (o.clsid, o.iid) → o.apply _ }).toMap
  }
}
