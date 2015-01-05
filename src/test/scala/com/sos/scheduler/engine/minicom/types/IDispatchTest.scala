package com.sos.scheduler.engine.minicom.types

import com.sos.scheduler.engine.minicom.types.IDispatchTest._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class IDispatchTest extends FreeSpec {

  private val iDispatch = IDispatch(A)

  "call" in {
    iDispatch.call("int", List(7)) shouldEqual 8
    iDispatch.call("int", List(Int box 7)) shouldEqual 8
    iDispatch.call("boxedInteger", List(7)) shouldEqual 8
    iDispatch.call("boxedInteger", List(Int box 7)) shouldEqual 8
    iDispatch.call("long", List(ALong)) shouldEqual ALong + 1
    iDispatch.call("long", List(Long box ALong)) shouldEqual ALong + 1
    iDispatch.call("long", List(7)) shouldEqual 8
    iDispatch.call("long", List(Int box 7)) shouldEqual 8
    iDispatch.call("boxedLong", List(7L)) shouldEqual 8
    iDispatch.call("boxedLong", List(Long box 7)) shouldEqual 8
    iDispatch.call("boxedLong", List(7)) shouldEqual 8
    iDispatch.call("boxedLong", List(Int box 7)) shouldEqual 8
    iDispatch.call("double", List(1.2)) shouldEqual 1.3
    iDispatch.call("double", List(Double box 1.2)) shouldEqual 1.3
    iDispatch.call("boxedDouble", List(1.2)) shouldEqual 1.3
    iDispatch.call("boxedDouble", List(Double box 1.2)) shouldEqual 1.3
    iDispatch.call("boolean", List(false)) shouldEqual true
    iDispatch.call("boolean", List(Boolean box false)) shouldEqual true
    iDispatch.call("boxedBoolean", List(false)) shouldEqual true
    iDispatch.call("boxedBoolean", List(Boolean box false)) shouldEqual true
    iDispatch.call("string", List(1, ALong, 1.2, true, "x")) shouldEqual s"1 $ALong 1.2 true x"
    iDispatch.call("array", List(VariantArray(Vector[Any](1L, 1.2)))) shouldEqual 2.2
  }

  "GetIDsOfNames and Invoke" in {
    val dispIds = iDispatch.getIDsOfNames(List("DOUBLE"))
    iDispatch.invoke(dispIds, DISPATCH_METHOD, List(1.2)) shouldEqual 1.3
  }
}

private object IDispatchTest {
  private val ALong = 111222333444555666L
  private object A extends IDispatch {
    def int(o: Int) = o + 1
    def boxedInteger(o: java.lang.Integer): java.lang.Integer = o + 1
    def long(o: Long) = o + 1
    def boxedLong(o: java.lang.Long): java.lang.Long = o + 1
    def double(o: Double) = o + 0.1
    def boxedDouble(o: java.lang.Double): java.lang.Double = o + 0.1
    def boolean(o: Boolean) = !o
    def boxedBoolean(o: Boolean): java.lang.Boolean = !o
    def string(i: Int, l: Long, d: Double, b: Boolean, o: String) = s"$i $l $d $b $o"
    def array(a: VariantArray) = a.indexedSeq(0).asInstanceOf[Long] + a.indexedSeq(1).asInstanceOf[Double]
  }
}
