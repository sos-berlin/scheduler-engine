package com.sos.scheduler.engine.cplusplus.generator.cpp

import org.junit._
import org.junit.Assert._

class CppNameTest {
  @Test def test1(): Unit = {
    val a = CppName("a")
    assertEquals("a", a.simpleName)
    assertEquals("::a", a.fullName)
  }

  @Test def test2(): Unit = {
    val ab = CppName("a::b")
    assertEquals("b", ab.simpleName)
    assertEquals("::a::b", ab.fullName)
  }
}
