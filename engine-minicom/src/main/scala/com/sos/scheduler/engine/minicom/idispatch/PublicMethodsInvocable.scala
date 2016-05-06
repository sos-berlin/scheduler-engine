package com.sos.scheduler.engine.minicom.idispatch

import java.lang.reflect.Method

/**
  * All public methods are callable via [[OverridingInvocableIDispatch]]
  *
  * @author Joacim Zschimmer
  */
trait PublicMethodsInvocable extends Invocable {

  final def invocableMethods: Seq[Method] = getClass.getMethods
}
