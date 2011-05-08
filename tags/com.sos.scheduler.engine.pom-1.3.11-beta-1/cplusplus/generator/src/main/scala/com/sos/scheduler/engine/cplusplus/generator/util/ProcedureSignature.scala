package com.sos.scheduler.engine.cplusplus.generator.util

import com.sos.scheduler.engine.cplusplus.generator.cpp._
import com.sos.scheduler.engine.cplusplus.generator.util.ClassOps._
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppThreadSafe
import java.lang.reflect.Constructor
import java.lang.reflect.Method
import java.lang.reflect.Modifier


case class ProcedureSignature(name: String, returnType: Class[_], parameterTypes: List[Class[_]], 
  isStatic: Boolean = false, isThreadSafe: Boolean = false)
extends Ordered[ProcedureSignature] {
    val nativeJavaName = name + "__native"
    val parameters = for ((t,i) <- parameterTypes.zipWithIndex)  yield Parameter(t, "p" + i)
    def hasReturnType = !isVoid(returnType)
    def classes = parameterTypes.toSet + returnType

    def compare(o: ProcedureSignature) = name compare o.name match {
        case 0 =>
            compareClassSeqs(parameterTypes, o.parameterTypes) match {
                case 0 => compareClasses(returnType, o.returnType)
                case c => c
            }
        case c => c
    }
}


object ProcedureSignature {
    def apply(m: Method) = new ProcedureSignature(m.getName, m.getReturnType, m.getParameterTypes.toList,
        isStatic = Modifier.isStatic(m.getModifiers),
        isThreadSafe = m.getAnnotation(classOf[CppThreadSafe]) != null)

    def apply(c: Constructor[_]) = ofConstructor(c.getParameterTypes)
    def ofConstructor(t: Seq[Class[_]]) = new ProcedureSignature("<init>", classOf[Void], t.toList)
}
