package com.sos.scheduler.engine.kernel.variable

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.kernel.cppproxy.Variable_setC
import scala.collection.JavaConversions._

@ForCpp
private[kernel] final class VariableSet(cppProxy: Variable_setC) extends Sister {

  def onCppProxyInvalidated(): Unit = {}

  def toMap: Map[String, String] = (cppProxy.java_names map { o ⇒ o → apply(o) }).toMap

  def apply(key: String): String = cppProxy.get_string(key)

  def update(key: String, value: String): Unit = cppProxy.set_var(key, value)
}

object VariableSet {
  final class Type extends SisterType[VariableSet, Variable_setC] {
    def sister(proxy: Variable_setC, context: Sister) = {
      assert(context == null)
      new VariableSet(proxy)
    }
  }
}
