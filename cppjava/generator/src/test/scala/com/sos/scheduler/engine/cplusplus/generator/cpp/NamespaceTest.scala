package com.sos.scheduler.engine.cplusplus.generator.cpp

import org.junit._
import org.junit.Assert._

class NamespaceTest {
  val a = Namespace("a")
  val ab = Namespace("b",Namespace("a"))

  @Test def testSimpleName() {
    assertEquals("a", a.simpleName)
    assertEquals("::a", a.fullName)
  }

  @Test def test2() {
    assertEquals("b", ab.simpleName)
    assertEquals("::a::b", ab.fullName)
  }

  @Test def testNames() {
    assertEquals(List("a", "b"), ab.names)
  }

  @Test def testNestCode() {
    val result = (ab.nestedCode(" xx\n") + " ").replaceAll("[ \n]+", " ")
    assertEquals("namespace a { namespace b { xx }} ", result)
  }

  @Test def testPlus() {
    assertEquals("::a::b::c::d",(ab + Namespace("c::d")).fullName)
  }

  @Test def testPlusCppName() {
    assertEquals("::a::b::name",(ab + CppName("name")).fullName)
  }

  @Test def test3() {
    assertEquals("::a::b", Namespace("a::b").fullName)
    assertEquals("::a::b", Namespace("a :: b").fullName)
    assertEquals("::a::b", Namespace("::a::b").fullName)
  }
}
