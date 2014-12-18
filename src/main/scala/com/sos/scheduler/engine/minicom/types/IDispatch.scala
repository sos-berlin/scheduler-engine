package com.sos.scheduler.engine.minicom.types

import com.sos.scheduler.engine.minicom.types.IDispatch._
import java.lang.reflect.{InvocationTargetException, Method}
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
trait IDispatch extends IUnknown {

  def call(methodName: String, arguments: Seq[Any]): Any =
    invokeMethod(getClass.getMethods()(methodIndex(methodName)), arguments)

  def getIDsOfNames(names: Seq[String]): immutable.Seq[DISPID] = {
    require(names.size == 1)
    val name = names.head
    List(DISPID(methodIndex(name)))
  }

  private def methodIndex(name: String): Int = {
    val methods = getClass.getMethods
    val methodIndices = methods.indices filter { i ⇒ methods(i).getName.compareToIgnoreCase(name) == 0 }
    if (methodIndices.size < 1) throw new RuntimeException(s"Unknown method '$name'")
    if (methodIndices.size > 1) throw new RuntimeException(s"Ambiguous method name '$name'")
    methodIndices.head
  }

  def invoke(dispIds: Seq[DISPID], dispatchType: DispatchType, arguments: Seq[Any]): Any = {
    require(dispIds.size == 1)
    val method = getClass.getMethods()(dispIds.head.value)
    invokeMethod(method, arguments)
  }

  private def invokeMethod(method: Method, arguments: Seq[Any]): Any = {
    require(arguments.size == method.getParameterCount)
    val javaParameters = for ((t, v) ← method.getParameterTypes zip arguments ) yield convert(t.asInstanceOf[Class[_ <: AnyRef]], v)
    val result =
      try method.invoke(this, javaParameters.reverse: _*)
      catch { case e: InvocationTargetException ⇒ throw e.getTargetException }
    if (result == null) EmptyVariant else result
  }
}

private object IDispatch {
  private val StringClass = classOf[String]
  private val IntegerClass = classOf[java.lang.Integer]
  private val LongClass = classOf[java.lang.Long]
  private val BooleanClass = classOf[java.lang.Boolean]
  private val DoubleClass = classOf[java.lang.Double]
  private val VariantArraySerializableClass = classOf[VariantArray]

  private def convert[A <: AnyRef](c: Class[A], v: Any): A =
    (c match {
      case IntegerClass ⇒ v match {
        case o: Int ⇒ Int.box(o)
        case o: java.lang.Integer ⇒ o
      }
      case LongClass ⇒ v match {
        case o: Int ⇒ Long.box(o)
        case o: java.lang.Integer ⇒ o
        case o: Long ⇒ Long.box(o)
        case o: java.lang.Long ⇒ Long.box(o)
      }
      case BooleanClass ⇒ v match {
        case o: Boolean ⇒ Boolean.box(o)
        case o: java.lang.Boolean ⇒ o
      }
      case DoubleClass ⇒ v match {
        case o: Double ⇒ Double.box(o)
        case o: java.lang.Double ⇒ o
      }
      case StringClass ⇒ v.toString
      case VariantArraySerializableClass ⇒ v.asInstanceOf[VariantArray]
    }).asInstanceOf[A]
}
