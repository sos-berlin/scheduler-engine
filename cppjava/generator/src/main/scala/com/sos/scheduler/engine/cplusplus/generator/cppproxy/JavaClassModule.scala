package com.sos.scheduler.engine.cplusplus.generator.cppproxy

import com.sos.scheduler.engine.cplusplus.generator.module._
import com.sos.scheduler.engine.cplusplus.generator.util._
import com.sos.scheduler.engine.cplusplus.generator.util.ClassOps._
import com.sos.scheduler.engine.cplusplus.runtime._

final class JavaClassModule(config: CppClassConfiguration, procedureSignatures: Seq[ProcedureSignature])
extends JavaModule {

  private val interface = config.interface
  private val suffix = "Impl"
  val name = interface.getName + suffix

  lazy val code = {
    val simpleName = interface.getSimpleName + suffix

    val sisterClass = parameterizedTypeOfRawType(interface.getGenericInterfaces, classOf[CppProxyWithSister[_]]) match {
      case Some(t) => t.getActualTypeArguments.head match {
        case o: Class[_] ⇒ o
        case o ⇒ sys.error(s"Unknown sister class '$o' of $interface")
      }
      case None => classOf[Sister]
    }

    /** Der Konstruktor verknüpft auch gleich die (neu angelegte) Java-Schwester */
    def constructor = {
      val sisterCode =
        if (interface.getFields exists {_.getName == "sisterType"})
        //"        requireContext(context);\n" +
          "        setSister(sisterType.sister(this, context));\n"
        else
          "        requireContextIsNull(context);\n"

      "    private " + simpleName + "(" + classOf[Sister].getName + " context) { // Nur für JNI zugänglich\n" +
          sisterCode +
      "    }\n"
    }

    val javaMethods = procedureSignatures map { new JavaMethod(_) }
    val methodsString = javaMethods map {m => m.javaImplementation + "\n" + m.javaNativeDeclaration + "\n\n"} mkString ""

    s"""package ${interface.getPackage.getName};
   |
   |@javax.annotation.Generated("C++/Java-Generator - SOS GmbH Berlin")
   |final class $simpleName
   |extends ${classOf[CppProxyImpl[_]].getName}<${sisterClass.getName}>
   |implements ${interface.getName} {
   |
   |$constructor
   |$methodsString}
   |""".stripMargin
  }
}
