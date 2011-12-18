package com.sos.scheduler.engine.cplusplus.generator.cppproxy

import com.sos.scheduler.engine.cplusplus.generator.cpp.Cpp.quoted
import com.sos.scheduler.engine.cplusplus.generator.cpp._
import com.sos.scheduler.engine.cplusplus.generator.util._
import com.sos.scheduler.engine.cplusplus.generator.util.ClassOps._
import com.sos.scheduler.engine.cplusplus.generator.util.MyRichString._
import com.sos.scheduler.engine.cplusplus.generator.util.Util._
import com.sos.scheduler.engine.cplusplus.runtime.CppProxy


class JniMethod(jniModule: JniModule, m: ProcedureSignature) {
    val jniName = CppName(Jni.simpleMethodName(m.name, m.parameterTypes), CppName(jniModule.interface).namespace)
    val jniReturnTypeName = Jni.typeName(m.returnType)

    def nativeImplementation = {
        val jniParameterDeclarations = m.parameters map { p => Jni.typeName(p.typ) + " " + p.name }
        val declaration = "static " + jniReturnTypeName + " JNICALL " + jniName.simpleName +
            inParentheses("JNIEnv* jenv" :: "jobject" :: "jlong cppReference" :: jniParameterDeclarations)
        val call = "o_->" + m.name + inParentheses(m.parameters map cppExpressionStringOfJni)
        val code =
            "    Env env = jenv;\n" +
            "    try {\n" +
            "        " + jniModule.className + "* o_ = " + cppObjectByReference(jniModule.className, "cppReference") + ";\n" +
            "        " + "return " ? m.hasReturnType + jniExpressionStringOfCpp(m.returnType, call) + ";\n" +
            "    }\n" +
            "    catch(const exception& x) {\n" +
            "        env.set_java_exception(x);\n" +
           ("        return " + jniReturnTypeName + "();\n" when m.hasReturnType) +
            "    }\n"
            
        jniName.namespace.nestedCode(declaration + "\n{\n" + code + "}\n")
    }

    private def cppExpressionStringOfJni(p: Parameter): String =
        if (classOf[String] isAssignableFrom p.typ)
            "env.string_from_jstring(" + p.name + ")"
        else
        if (isClass(p.typ))
            cppObjectByReference(CppName(p.typ), p.name)
        else
        if (p.typ.getName == "boolean")
            p.name + " != 0"
        else
            p.name

    private def cppObjectByReference(cppClassName: CppName, reference: String) = {
        val debugString = (cppClassName.fullName + " in " when cppClassName != jniModule.className) +
            jniModule.className.fullName + "::" + m.name + "()"
        "has_proxy< " +cppClassName + " >::of_cpp_reference(" + reference + "," + quoted(debugString) + ")"
    }

    private def jniExpressionStringOfCpp(typ: Class[_], expr: String) =
        if (typ.isArray)
            "java_array_from_c("+ expr +")"
        else
        if (isStringClass(typ))
            "env.jstring_from_string(" + expr + ")"
        else
        if (classOf[CppProxy] isAssignableFrom typ)
            "Has_proxy::jobject_of(" + expr + ")"
        else
        if (isClass(typ))  // Method returns a real Java object, that means the method is used only for Java calls.
            "(" + expr + ").local_ref()"  // Global reference is lost after call, therefore we make a local reference
        else
          "(" + expr + ")"

    def registerNativeArrayEntry =
        List("(char*)" + quoted(m.nativeJavaName), "(char*)" + quoted(signatureString), "(void*)" + jniName)
        .mkString("{ ", ", ", " }")

    private def signatureString = Jni.methodTypeSignatureString(classOf[Long] :: m.parameterTypes, m.returnType)
}
