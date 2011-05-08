package com.sos.scheduler.engine.cplusplus.generator.javaproxy.clas

import com.sos.scheduler.engine.cplusplus.generator.Configuration
import com.sos.scheduler.engine.cplusplus.generator.cpp._
import com.sos.scheduler.engine.cplusplus.generator.javaproxy.procedure._
import com.sos.scheduler.engine.cplusplus.generator.util._
import com.sos.scheduler.engine.cplusplus.generator.util.ClassOps._
import com.sos.scheduler.engine.cplusplus.runtime.CppProxy
import java.lang.Class
import scala.collection.mutable;


class CppClass(val javaClass: Class[_], val knownClasses: Set[Class[_]])
extends CppCode
{
    private val isCppProxy = classOf[CppProxy] isAssignableFrom javaClass
    private val suppressMethods = isCppProxy

    val name = CppName(javaClass)
    val cppClassClass = new CppClassClass(this)
    val cppObjectClass = if (javaClass == classOf[String]) new StringCppObjectClass(this)  else new CppObjectClass(this)

    val cppConstructors = {
        val signatures = validConstructors(javaClass) filter { c => typesAreKnown(c.getParameterTypes) } map ProcedureSignature.apply
        signatures.sorted map { new CppConstructor(this, _) }
    }

    val cppMethods = {
        val signatures = if (suppressMethods) Nil  else
            (validMethods(javaClass) filter { m => typesAreKnown(m.getParameterTypes) && typeIsKnown(m.getReturnType) }
            map ProcedureSignature.apply)
        signatures.sorted map { new CppMethod(this, _) }
    }

    val cppProcedures = cppConstructors ++ cppMethods

    private def typesAreKnown(types: Seq[Class[_]]) = types forall typeIsKnown
    private def typeIsKnown(t: Class[_]) = t.isPrimitive  ||  typeIsValidClass(t)  &&  knownClasses.contains(t)

    
    def headerPreprocessorMacro =
        ("_" + Configuration.generatedJavaProxyNamespace.simpleName + "_" + javaClass.getName.replace('.', '_') + "_H_").toUpperCase

    def neededForwardDeclarations = (directlyUsedJavaClasses map forwardDeclaration).toSeq.sorted.mkString

    private def forwardDeclaration(c: Class[_]) = {
        val cppName = CppName(c)
        cppName.namespace.nestedCode("struct " + cppName.simpleName + "; ")
    }

    def javaSuperclasses = superclasses(javaClass)

    val directlyUsedJavaClasses: Set[Class[_]] =
        ( ClassOps.directlyUsedJavaClasses(javaClass) filter { t => typeIsValidClass(t) && t != javaClass && typeIsKnown(t) } ) +
        classOf[String]  // Für obj_name()

    override def headerCode = name.namespace.nestedCode(
        cppObjectClass.forwardDeclaration + "\n" +
        cppClassClass.headerCode + "\n\n" +
        cppObjectClass.headerCode
    )

    override def sourceCode = name.namespace.nestedCode(
        cppClassClass.sourceCode + "\n\n" +
        cppObjectClass.sourceCode
    )
}
