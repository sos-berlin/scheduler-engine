package com.sos.scheduler.engine.common.scalautil

import HasCloserTest._
import com.google.common.io.Closer
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class HasCloserTest extends FunSuite {

  test("onClose") {
    val a = new A
    a.closed shouldBe false
    a.close()
    a.closed shouldBe true
  }

  test("implicit Closer.apply") {
    import HasCloser.implicits._
    implicit val closer = Closer.create()
    var a = false
    closer { a = true }
    a shouldBe false
    closer.close()
    a shouldBe true
  }

  test("registerCloseable") {
    import HasCloser.implicits._
    implicit val closer = Closer.create()
    val b = (new B).registerCloseable
    b.closed shouldBe false
    closer.close()
    b.closed shouldBe true
  }
}


private object HasCloserTest {

  private class A extends HasCloser {
    var closed = false
    onClose { closed = true }
  }

  private class B {
    var closed = false
    def close() {
      closed = true
    }
  }
}
