package com.sos.scheduler.engine.cplusplus.generator.javaproxy.procedure

import com.sos.scheduler.engine.cplusplus.generator.Configuration._
import com.sos.scheduler.engine.cplusplus.generator.cpp._
import com.sos.scheduler.engine.cplusplus.generator.cpp.Cpp.quoted
import com.sos.scheduler.engine.cplusplus.generator.cpp.Jni.mangled
import com.sos.scheduler.engine.cplusplus.generator.cpp.javabridge.JavaBridge
import com.sos.scheduler.engine.cplusplus.generator.javaproxy._
import com.sos.scheduler.engine.cplusplus.generator.javaproxy.clas.CppClass
import com.sos.scheduler.engine.cplusplus.generator.util.ClassOps._
import com.sos.scheduler.engine.cplusplus.generator.util.MyRichString._
import com.sos.scheduler.engine.cplusplus.generator.util.Parameter
import com.sos.scheduler.engine.cplusplus.generator.util.ProcedureSignature
import com.sos.scheduler.engine.cplusplus.scalautil.io.FileUtil._


class CppProcedure(cppClass: CppClass, m: ProcedureSignature)
{
    val name = Cpp.normalizedName(m.name)
    private val javaBridgeCall = javabridge.MethodCall(m.returnType)
    private val returnTypeIsClass = !isVoid(m.returnType)  &&  isClass(m.returnType)
    private val cppReturnType = if (returnTypeIsClass) CppName(m.returnType).fullName  else javaBridgeCall.returnType

    protected val parameterListDeclaration =
        m.parameters map { p => cppParameterDeclaration(p.typ) + " " + p.name } mkString ("(", ", ", ")")

    
    val classClassCode = new CppCode {
        def headerCode = {
            val c = if (m.isStatic) "Static_method"  else "Method"
            JavaBridge.namespace + "::" + c + " const " + variableName + ";"
        }
        def sourceCode = variableName + "(this, " + quoted(m.name) + ", " +
            quoted(Jni.methodTypeSignatureString(m.parameterTypes, m.returnType)) + ")"
    }


    val objectClassCode = new CppCode {
        def headerCode = "static " ? m.isStatic + cppReturnType + " " + name + parameterListDeclaration + ";"

        def sourceCode = {
            val declaration = cppReturnType + " " + cppClass.name.simpleName + "::" + name + parameterListDeclaration
            val jclass = if (m.isStatic) cppClass.cppClassClass.name + "::class_factory.clas()"  else "_class.get()"
            declaration + " {\n" + call(jclass, "get_jobject()") + "}\n"
        }

        def call(jclass: String, jobject: String) = {
            val parameterSetting = {
                val decl = "    " + JavaBridge.namespace + "::raw_parameter_list<" + m.parameterTypes.length + "> parameter_list;\n"
                val assignments = {
                    def paramValue(p: Parameter) = if (isClass(p.typ))  p.name + ".get_jobject()"  else p.name
                    (for ((p,i) <- m.parameters.zipWithIndex)
                        yield "    parameter_list._jvalues[" + i + "]." + Jni.jvalueUnionMember(p.typ) + " = " + paramValue(p) + ";\n")
                    .mkString
                }
                decl + assignments
            }

            val classSetting = "    " + cppClass.cppClassClass.name + "* cls = " + jclass + ";\n"
            
            val methodCall = {
                val obj = if (m.isStatic) "cls->get_jclass()"  else jobject
                "cls->" + variableName + "." + javaBridgeCall.callMethod + "(" + obj + ", parameter_list)"
            }

            def anyCall = m.returnType match {
                case t if isVoid(t) => voidCall
                case t if t.getName == "boolean" => booleanResultCall
                case t if isClass(t) => objectResultCall
                case _ => simpleResultCall
            }

            def voidCall = "    " + methodCall + ";\n"
            def booleanResultCall = "    return 0 != " + methodCall + ";\n"
            def objectResultCall =
                "    " + cppReturnType + " result;\n" +
                "    result.steal_local_ref(" + methodCall + ");\n" +
                "    return result;\n"
            def simpleResultCall = "    return " + methodCall + ";\n"

            parameterSetting + classSetting + anyCall
        }
    }

                 
    private def variableName = {
        def variableNameOfJavaName(name: String) = name match {
            case "<init>" => "_constructor"
            case x        => x
        }

        "_" + variableNameOfJavaName(mangled(name)) + "__" +
        mangled(m.parameterTypes map Jni.signatureString mkString) +
        "__method"
    }

    private def cppParameterDeclaration(t: Class[_]) = 
        if (isClass(t)) "const " + JavaBridge.hasProxyJobjectClassName(CppName(t).fullName) + "&"
        else Jni.typeName(t)
}
