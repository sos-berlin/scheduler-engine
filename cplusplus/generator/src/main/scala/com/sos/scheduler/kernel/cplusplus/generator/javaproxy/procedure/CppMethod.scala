package com.sos.scheduler.kernel.cplusplus.generator.javaproxy.procedure

import java.lang.reflect.Method
import com.sos.scheduler.kernel.cplusplus.generator.javaproxy._
import com.sos.scheduler.kernel.cplusplus.generator.javaproxy.clas.CppClass
import com.sos.scheduler.kernel.cplusplus.generator.util.ProcedureSignature


class CppMethod(cppClass: CppClass, s: ProcedureSignature) extends CppProcedure(cppClass, s)


object CppMethod{ 
    def apply(cppClass: CppClass, m: Method) =
        new CppMethod(cppClass, ProcedureSignature(m.getName, m.getReturnType, m.getParameterTypes.toList))
}
