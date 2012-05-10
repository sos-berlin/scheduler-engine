package com.sos.scheduler.engine.cplusplus.generator.cppproxy

import com.sos.scheduler.engine.cplusplus.generator.util.ProcedureSignature
import com.sos.scheduler.engine.cplusplus.runtime.annotation.JavaOnlyInterface
import java.lang.reflect.Member
import java.lang.reflect.Modifier
import CppProxyModulePair._

class CppProxyModulePair(val interface: Class[_]) {
    require(interface.isInterface, "Not an interface: " + interface)

    private val config = CppClassConfiguration(interface)
    private val procedureSignatures = (interface.getMethods.toList filter isCppMethod map ProcedureSignature.apply).sorted
    val javaModule = new JavaClassModule(config, procedureSignatures)
    val jniModule = new JniModule(config, procedureSignatures, javaModule.name)
}

object CppProxyModulePair {
    private def isCppMethod(m: Member) = Modifier.isPublic(m.getModifiers)  &&  !classIsJavaOnlyInterface(m.getDeclaringClass)

    private def classIsJavaOnlyInterface(c: Class[_]) = c.isAnnotationPresent(classOf[JavaOnlyInterface])
}
