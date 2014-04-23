package com.sos.scheduler.engine.cplusplus.generator.util

import ProcedureSignatureTest._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class ProcedureSignatureTest extends FreeSpec {
  "String return type" in {
    val c = classOf[Simple]
    ProcedureSignature(c, c.getMethod("simple")) should have (
      'name ("simple"),
      'static (false),
      'hasReturnType (true),
      'returnType (classOf[String]),
      'parameterTypes (Nil)
    )
  }

  "Generic return type" in {
    val c = classOf[Y]
    ProcedureSignature(c, c.getMethod("generic")) should have (
      'name ("generic"),
      'static (false),
      'hasReturnType (true),
      'returnType (classOf[U]),
      'parameterTypes (Nil)
    )
  }

  "Generic collection return type" in {
    val c = classOf[Y]
    ProcedureSignature(c, c.getMethod("genericList")) should have (
      'name ("genericList"),
      'static (false),
      'hasReturnType (true),
      'returnType (classOf[java.util.List[_]]),
      'parameterTypes (Nil)
    )
  }

  "method with return type using type parameter of its class" in {
    trait K[A <: T] {
      def generic: A
    }
    val c = classOf[K[_]]
    ProcedureSignature(c, c.getMethod("generic")) should have (
      'name ("generic"),
      'static (false),
      'hasReturnType (true),
      'returnType (classOf[T]),
      'parameterTypes (Nil)
    )
  }

  "method with type parameter used in return type" in {
    trait K {
      def generic[A <: T]: A
    }
    val c = classOf[K]
    ProcedureSignature(c, c.getMethod("generic")) should have (
      'name ("generic"),
      'static (false),
      'hasReturnType (true),
      'returnType (classOf[T]),
      'parameterTypes (Nil)
    )
  }
}

object ProcedureSignatureTest {
  trait Simple {
    def simple(): String
  }

  trait T
  trait U extends T

  trait X[A <: T] {
    def generic(): A
    def genericList(): java.util.List[A]
  }

  trait Y extends X[U]
}
