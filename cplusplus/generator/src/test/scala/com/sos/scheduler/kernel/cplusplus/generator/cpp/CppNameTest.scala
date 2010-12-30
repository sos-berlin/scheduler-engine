package com.sos.scheduler.kernel.cplusplus.generator.cpp
import org.junit._
import org.junit.Assert._

class CppNameTest {
    @Test def test1 {
        val a = CppName("a")
        assertEquals("a", a.simpleName)
        assertEquals("::a", a.fullName)
    }

    @Test def test2 {
        val ab = CppName("a::b")
        assertEquals("b", ab.simpleName)
        assertEquals("::a::b", ab.fullName)
    }
}
