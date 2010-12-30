package com.sos.scheduler.kernel.cplusplus.generator.javaproxy.procedure

import com.sos.scheduler.kernel.cplusplus.generator.cpp._
import com.sos.scheduler.kernel.cplusplus.generator.javaproxy._
import com.sos.scheduler.kernel.cplusplus.generator.javaproxy.clas.CppClass
import com.sos.scheduler.kernel.cplusplus.generator.util.ProcedureSignature
import java.lang.reflect.Constructor


class CppConstructor(cppClass: CppClass, s: ProcedureSignature) extends CppProcedure(cppClass, s)
{
    object objectInstantiator extends CppCode {
        val myName = cppClass.name.simpleName

        def headerCode = "static " + myName + " new_instance" + parameterListDeclaration + ";"

        def sourceCode = {
            val declaration = myName + " " + myName + "::new_instance" + parameterListDeclaration
            val body =
                "{\n" +
                    "    " + myName + " result;\n" +
                    "    " + "result.java_object_allocate_();\n" +
                             objectClassCode.call("result._class.get()", "result.get_jobject()") +
                    "    " + "return result;\n" +
                 "}\n"
            declaration + " " + body
        }
    }
}


object CppConstructor {
    def apply(cppClass: CppClass, c: Constructor[_]) = new CppConstructor(cppClass, ProcedureSignature(c))
}
