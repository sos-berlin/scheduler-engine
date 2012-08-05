package com.sos.scheduler.engine.cplusplus.generator.javaproxy.clas

import com.sos.scheduler.engine.cplusplus.generator.util.ClassOps._
import java.io.Writer
import org.junit._
import org.junit.Assert._

class CppClassTest {
  @Ignore @Test def test() {
    val cppClass = new CppClass(classOf[java.lang.Boolean], Set(classOf[java.lang.Boolean]))

    val separator = "\n\n//------------------\n\n\n"

    System.out.println(
      cppClass.neededForwardDeclarations + separator +
      cppClass.headerCode + separator +
      cppClass.sourceCode )
  }

//    @Test def testWithAllClasses {
//        val classes = CppClass.withAllClasses(classOf[Object])
//        assertTrue( "Class erwartet, statt " + classes, classes.contains( classOf[Class[_]]) )
//    }

  @Test def testNeededClasses() {
    val classes = neededClasses(classOf[String])
    assert(classes.contains( classOf[Object]))
  }

  @Test def testWithoutOverriddenVariantMethods() {
    // Writer implementiert append doppelt: Appendable append(char) und Writer append(char)

    val appendCharMethods = classOf[Writer].getDeclaredMethods.toList filter
      { m => m.getName == "append" && m.getParameterTypes.size == 1 && m.getParameterTypes.head == classOf[Char] }
    assertEquals(2, appendCharMethods.size)

    val result = withoutOverriddenVariantMethods(appendCharMethods)
    assert(result.size==1)
  }
}
