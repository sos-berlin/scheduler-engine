package com.sos.scheduler.engine.cplusplus.generator.cppproxy

import com.sos.scheduler.engine.cplusplus.generator.module._
import com.sos.scheduler.engine.cplusplus.generator.util._
import com.sos.scheduler.engine.cplusplus.generator.util.ClassOps._
import com.sos.scheduler.engine.cplusplus.runtime._


class JavaClassModule(config: CppClassConfiguration, procedureSignatures: Seq[ProcedureSignature]) extends JavaModule {
    private val interface = config.interface
    private val suffix = "Impl"
    val name = interface.getName + suffix

    lazy val code = {
        val simpleName = interface.getSimpleName + suffix

        val sisterClass = parameterizedTypeOfRawType(interface.getGenericInterfaces, classOf[CppProxyWithSister[_]]) match {
            case Some(t) => t.getActualTypeArguments.head.asInstanceOf[Class[_]]
            case None => classOf[Sister]
        }

        /** Der Konstruktur verknüpft auch gleich die (neu angelegte) Java-Schwester */
        def constructor = {
            val sisterCode =
                if (interface.getFields exists { _.getName == "sisterType" })
                  //"        requireContext(context);\n" +
                    "        setSister(sisterType.sister(this, context));\n"
                else
                    "        requireContextIsNull(context);\n"

            "    private " + simpleName + "(" + classOf[Sister].getName + " context) { // Nur für JNI zugänglich\n" +
            sisterCode +
            "    }\n"
        }

        val javaMethods = procedureSignatures map { new JavaMethod(_) }
        val methodsString = javaMethods map { m => m.javaImplementation + "\n" + m.javaNativeDeclaration + "\n\n"} mkString ""

        "package " + interface.getPackage.getName + ";\n\n" +
        "final class " + simpleName + "\n" +
            "   extends " + classOf[CppProxyImpl[_]].getName + "<" + sisterClass.getName + ">\n" +
            "   implements " + interface.getName + "\n" +
            "{\n" +
        constructor + "\n" +
        methodsString +
        "}\n"
    }
}
