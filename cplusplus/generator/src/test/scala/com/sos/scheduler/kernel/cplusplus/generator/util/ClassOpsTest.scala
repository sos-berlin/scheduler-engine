package com.sos.scheduler.kernel.cplusplus.generator.util

import org.junit._
import java.lang.reflect.ParameterizedType
import java.lang.reflect.Type
import org.junit.Assert._
import ClassOps._


class ClassOpsTest {
    @Test def testAllInterfaces {
        trait T1
        trait T2
        trait T3
        trait T12 extends T1 with T2
        trait TestClass extends T12 with T3

        val expected = Set(classOf[T1],classOf[T2],classOf[T3],classOf[T12])
        assertEquals(expected, allInterfaces(classOf[TestClass]))
    }

    @Test def testTypeArgumentsOfInterface {
        trait T1[T]
        trait T2[T]
        class C extends T1[Int] with T2[String]
        assert(parameterizedTypeOfRawType(classOf[Object].getGenericInterfaces, classOf[T1[_]]).isEmpty)

        val a = parameterizedTypeOfRawType(classOf[C].getGenericInterfaces, classOf[T2[_]]).get.getActualTypeArguments
        assertEquals(1, a.size)
        assertEquals(classOf[String], a.head)
    }

    @Test def testParameterizedTypes {
        trait T1
        trait T2[T]
        class C extends T1 with T2[Int]

        val a = parameterizedTypes(classOf[C].getGenericInterfaces)
        assertEquals(1, a.size)
        assertEquals(classOf[T2[Int]], a.head.getRawType)
    }
}
