package com.sos.scheduler.engine.kernel.variable

import com.google.common.collect.ImmutableMap
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.SisterType
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.cppproxy.Variable_setC
import scala.collection.JavaConversions._

@ForCpp
final class VariableSet(cppProxy: Variable_setC)
extends UnmodifiableVariableSet with Sister with Iterable[(String, String)] {

  def onCppProxyInvalidated(): Unit = {}

  override def size: Int =
    cppProxy.count

  override def apply(name: String): String =
    cppProxy.get_string(name)
    //get(name) getOrElse error(s"Unknown variable name '$name'")

  def keys: Iterable[String] =
    cppProxy.java_names

  def getNames: java.util.Collection[String] =
    cppProxy.java_names

  def toMap: Map[String, String] =
    (getNames map { o => o -> apply(o) }).toMap

  def toGuavaMap: ImmutableMap[String,String] = {
    val result = new ImmutableMap.Builder[String, String]
    for (name <- cppProxy.java_names)
      result.put(name, apply(name))
    result.build()
  }

  def iterator = new Iterator[(String, String)] {
    val i = cppProxy.java_names.iterator
    def hasNext = i.hasNext
    def next() = {
      val name = i.next
      name -> cppProxy.get_string(name)
    }
  }

//  private def get(name: String): Option[String] =
//    Option(cppProxy.get_string(name))

  def update(name: String, value: String): Unit = {
    cppProxy.set_var(name, value)
  }
}

object VariableSet {
  final class Type extends SisterType[VariableSet, Variable_setC] {
    def sister(proxy: Variable_setC, context: Sister) = {
      assert(context == null)
      new VariableSet(proxy)
    }
  }
}
