package com.sos.scheduler.engine.cplusplus.generator.cpp

import org.junit._
import org.junit.Assert._

class NamespaceTest {
  val a = Namespace("a")
  val ab = Namespace("b",Namespace("a"))

  @Test def testSimpleName(): Unit = {
    assertEquals("a", a.simpleName)
    assertEquals("::a", a.fullName)
  }

  @Test def test2(): Unit = {
    assertEquals("b", ab.simpleName)
    assertEquals("::a::b", ab.fullName)
  }

  @Test def testNames(): Unit = {
    assertEquals(List("a", "b"), ab.names)
  }

  @Test def testNestCode(): Unit = {
    val result = (ab.nestedCode(" xx\n") + " ").replaceAll("[ \n]+", " ")
    assertEquals("namespace a { namespace b { xx }} ", result)
  }

  @Test def testPlus(): Unit = {
    assertEquals("::a::b::c::d",(ab + Namespace("c::d")).fullName)
  }

  @Test def testPlusCppName(): Unit = {
    assertEquals("::a::b::name",(ab + CppName("name")).fullName)
  }

  @Test def test3(): Unit = {
    assertEquals("::a::b", Namespace("a::b").fullName)
    assertEquals("::a::b", Namespace("a :: b").fullName)
    assertEquals("::a::b", Namespace("::a::b").fullName)
  }
}
