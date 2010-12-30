package com.sos.scheduler.kernel.cplusplus.generator.javaproxy.clas

import com.sos.scheduler.kernel.cplusplus.generator.Configuration._
import com.sos.scheduler.kernel.cplusplus.generator.cpp._
import com.sos.scheduler.kernel.cplusplus.generator.cpp.javabridge.JavaBridge
import com.sos.scheduler.kernel.cplusplus.generator.javaproxy._
import com.sos.scheduler.kernel.cplusplus.generator.util.ClassOps._


/** Liefert die Klasse, die die Java-Klasse beschreibt. */
class CppClassClass(cppClass: CppClass)  extends CppCode
{
    val name = cppClass.javaClass.getSimpleName + "__class"

    def headerCode = "struct " + name + ";"

    def sourceCode =
        sourceOnlyDeclarations +
        staticVariables.sourceCode + "\n" +
        constructorCode.sourceCode + "\n" +
        destructorCode.sourceCode + "\n"

    def sourceOnlyDeclarations =
        "struct " + name + " : " + JavaBridge.namespace + "::Class\n" +
        "{\n" +
            constructorCode.headerCode +
            destructorCode.headerCode + "\n" +
            methodVariables.headerCode + "\n" +
            staticVariables.headerCode +
        "};\n\n"

    private val constructorCode = new CppCode {
        def headerCode = "    " + declaration + ";\n"

        def sourceCode =
            name + "::" + declaration + " :\n" +
            "    " + JavaBridge.namespace + "::Class(class_name)\n" +
            methodVariables.sourceCode +
            "{}\n"

        def declaration = name + "(const string& class_name)"
    }

    private val destructorCode = new CppCode {
        def headerCode = "   ~" + name + "();\n"
        def sourceCode = name + "::~" + name + "() {}\n"
    }

    private val methodVariables = new CppCode {
        def headerCode = cppClass.cppProcedures map { "    " + _.classClassCode.headerCode + "\n" } mkString
        def sourceCode = (cppClass.cppProcedures map { "    ," + _.classClassCode.sourceCode } mkString "\n")
    }

    private val staticVariables = new CppCode {
        def headerCode = "    static " + classFactoryTypename + " class_factory;\n"
        def sourceCode = classFactoryTypename +" " + name + "::class_factory (\"" + cppClass.javaClass.getName + "\");\n"
        def classFactoryTypename = "const " + JavaBridge.namespace + "::class_factory< " + name + " >";
    }
}
