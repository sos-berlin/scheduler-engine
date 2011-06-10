package com.sos.scheduler.engine.cplusplus.generator.cppproxy

import com.sos.scheduler.engine.cplusplus.generator.util._
import com.sos.scheduler.engine.cplusplus.generator.util.ClassOps._
import com.sos.scheduler.engine.cplusplus.generator.util.MyRichString._
import com.sos.scheduler.engine.cplusplus.generator.util.Util._
import com.sos.scheduler.engine.cplusplus.runtime.CppProxy


class JavaMethod(m: ProcedureSignature) {
    private val javaParameterDeclarations = m.parameters map { p => p.typ.getName + " " + p.name }

    def javaImplementation = {
        val declaration =
            "    @Override public " +
            m.returnType.getName + " " +
            m.name +
            inParentheses(javaParameterDeclarations)

        val call = m.nativeJavaName + inParentheses("cppReference()" :: (m.parameters map javaCallArgument))

        val code = m.returnType match {
            case t if isClass(t) =>
                "            " + t.getName + " result = " + call + ";\n" +
                "            " + "checkIsNotReleased(" + t.getName + ".class, result);\n" +
                "            " + "return result;\n"
            case t if isVoid(t) =>
                "            " + call + ";\n"
            case _ =>
                "            " + "return " + call + ";\n"
        }

        def threadSafeCode = {
            val threadLock = classOf[CppProxy].getName + ".threadLock"
            "        " + threadLock  + ".lock();\n" +
            "        try {\n" +
            code +
            "        }\n" +
            "        finally {\n" +
            "            " + threadLock + ".unlock();\n" +
            "        }\n"
        }

        declaration + " {\n" + 
        (if (m.isThreadSafe) code else threadSafeCode) +
        "    }\n"
    }

    def javaCallArgument(p: Parameter) =
        if (isStringClass(p.typ))
            p.name
        else
        if (isClass(p.typ))
            p.name + ".cppReference()"
        else
            p.name

    def javaNativeDeclaration =
        "    private static native " +
        m.returnType.getName + " " +
        m.nativeJavaName +
        inParentheses("long cppReference" :: javaParameterDeclarations) +
        ";\n"
}
