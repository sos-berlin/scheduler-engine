package com.sos.scheduler.engine.cplusplus.generator.cppproxy

import com.sos.scheduler.engine.cplusplus.generator.cppproxy.JavaMethod._
import com.sos.scheduler.engine.cplusplus.generator.util.ClassOps._
import com.sos.scheduler.engine.cplusplus.generator.util.Util._
import com.sos.scheduler.engine.cplusplus.generator.util._
import com.sos.scheduler.engine.cplusplus.runtime.{CppProxies, CppProxy}

final class JavaMethod(m: ProcedureSignature) {
  private val javaParameterDeclarations = m.parameters map { p => classToString(p.typ) + " " + p.name }

  private val returnTypeName = m.returnType match {
    case c if classIsByteArray(c) => "byte[]"
    case c if c.isArray && c.getComponentType == classOf[java.lang.String] => "String[]"
    case c if c.isArray && c.getComponentType == classOf[java.lang.String] => "String[]"
    case c if c.isArray && "int[]".equals(c.getCanonicalName) => c.getCanonicalName
    case c if c.isArray && "long[]".equals(c.getCanonicalName) => c.getCanonicalName
    case c if c.isArray => "Object[]"
    case c => c.getName
  }

  def javaImplementation = {
    val declaration =
      "    @Override public " +
        returnTypeName + " " +
        m.name +
        inParentheses(javaParameterDeclarations)

    val call = m.nativeJavaName + inParentheses("cppReference()" :: (m.parameters map { _.name }))

    val code = m.returnType match {
      case t if isClass(t) =>
        "            " + returnTypeName + " result = " + call + ";\n" +
          "            " + "checkIsNotReleased(" + returnTypeName + ".class, result);\n" +
          "            " + "return result;\n"
      case t if isVoid(t) =>
        "            " + call + ";\n"
      case _ =>
        "            " + "return " + call + ";\n"
    }

    def threadSafeCode = {
      val cppProxy = classOf[CppProxy].getName + "$.MODULE$"
      s"""        $cppProxy.requireCppThread();
         |        try {
         |$code        }
         |        catch (Exception x) { throw ${ classOf[CppProxies].getName }.propagateCppException(x, this); }
         |""".stripMargin
    }

    declaration + " {\n" +
      (if (m.isThreadSafe) code else threadSafeCode) +
      "    }\n"
  }

  def javaNativeDeclaration =
    "    private static native " +
      returnTypeName + " " +
      m.nativeJavaName +
      inParentheses("long cppReference" :: javaParameterDeclarations) +
      ";\n"
}

object JavaMethod {
  private def classToString[A](c: Class[A]) =
    if (classIsIntArray(c))
      "int[]"
    else if (classIsLongArray(c))
      "long[]"
    else
      c.getName
}
