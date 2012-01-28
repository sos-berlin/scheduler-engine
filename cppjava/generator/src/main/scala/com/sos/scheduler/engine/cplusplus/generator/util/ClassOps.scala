package com.sos.scheduler.engine.cplusplus.generator.util

import java.lang.reflect._
import scala.collection.mutable
import java.lang.annotation.Annotation
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp

object ClassOps {
    type JavaClass = Class[_]

    def allDependendClasses(clas: JavaClass) = {
        var newClasses = Set[JavaClass](clas)
        var result = mutable.Set[JavaClass]() ++ newClasses
        while (newClasses.nonEmpty) {
            newClasses = (newClasses flatMap directlyUsedJavaClasses) -- result;
            result ++= newClasses
        }
        result.toSet - clas
    }

    def allInterfaces(clas: JavaClass): Set[Class[_]] = {
        //TODO Derselbe Algorithmus wie allDependedClasses. Der könnte abstrahiert werden.
        var newInterfaces = Set[JavaClass](clas)
        var result = mutable.Set[JavaClass]() ++ newInterfaces
        while (newInterfaces.nonEmpty) {
            newInterfaces = (newInterfaces flatMap { _.getInterfaces }) -- result
            result ++= newInterfaces
        }
        result.toSet - clas
    }

    def directlyUsedJavaClasses(clas: JavaClass): Set[JavaClass] = {
        val types =
            superclass(clas).toSet ++
            (validConstructors(clas) flatMap { _.getParameterTypes }) ++
            (validMethods(clas) flatMap { _.getParameterTypes }) ++
            (validMethods(clas) map { _.getReturnType }) ++
            clas.getInterfaces

        types filter { t => typeIsValidClass(t) && t != clas }
    }

    def validConstructors(c: JavaClass) = c.getConstructors.toList filter memberIsValid

    def validMethods(c: JavaClass) = withoutOverriddenVariantMethods(c.getDeclaredMethods.toList filter memberIsValid)

    private def memberIsValid(m: Member) = Modifier.isPublic(m.getModifiers) && memberIsForCppAnnotated(m)

    /** Wenn die Klasse mit {@link ForCpp} annotiert ist, werden nur ebenso annotierte Member für C++ übernommen. */
    private def memberIsForCppAnnotated(m: Member) =
        memberIsAnnotated(m, classOf[ForCpp]) || !classIsAnnotated(m.getDeclaringClass, classOf[ForCpp])

    private def classIsAnnotated(c: Class[_], a: Class[_ <: Annotation]) = c.getAnnotation(a) != null

    private def memberIsAnnotated(m: Member, clas: Class[_ <: Annotation]) = m match {
        case c: Constructor[_] => c.getAnnotation(clas) != null
        case m: Method => m.getAnnotation(clas) != null
    }

    def typeIsValidClass(t: JavaClass) =
        !t.isPrimitive &&
        !t.isArray &&
        !t.isLocalClass &&
        !t.isMemberClass &&
        Modifier.isPublic(t.getModifiers) &&
        t != classOf[Void]

    /** Covariante Methoden eines Interfaces können doppelt implementiert sein,
     * jeweils mit den covarianten Rückgabetypen.
     * Zum Beispiel ist Writer::append() implementiert mit der Rückgabe Writer und Appendable.
     * Wir nehmen hier nur die Implementierung mit der Unterklasse.
     */
    def withoutOverriddenVariantMethods(methods: List[Method]): List[Method] = {
        class MethodVariant(val name: String, val parameterTypes: Seq[JavaClass]) {
            override def equals(o: Any) = o.isInstanceOf[MethodVariant] && eq(o.asInstanceOf[MethodVariant])
            def eq(o: MethodVariant) = name == o.name  &&  parameterTypes.corresponds(o.parameterTypes){ _ == _ }
            override def hashCode = name.hashCode + parameterTypes.foldLeft(0){ _ + _.hashCode }
        }

        def methodWithBestReturnType(methods: Traversable[Method]): Method = {
            var result = methods.head
            for (m <- methods.tail)  if (result.getReturnType isAssignableFrom m.getReturnType)  result = m
            result
        }

        val map = mutable.Map[MethodVariant,mutable.ListBuffer[Method]]()
        methods foreach { m =>
            val v = new MethodVariant(m.getName, m.getParameterTypes)
            if (!map.contains(v))  map.put(v, mutable.ListBuffer())
            map(v).append(m)
        }

        (map.values map methodWithBestReturnType).toList
    }


    def neededClasses(clas: JavaClass) = superclasses(clas).toSet - clas

    def superclasses(clas: JavaClass) = {
        def superclassList(clas: JavaClass): List[JavaClass] = clas.getSuperclass match {
            case null => superclass(clas).toList
            case superclass => superclass :: superclassList(superclass)
        }
        superclassList(clas).toSet
    }

    def superclass(clas: JavaClass) = clas.getSuperclass match {
        case null if clas.isInterface => Some(classOf[Object])   // In C++ wäre das eine virtuelle Superklasse
        case null if clas == classOf[Object] => None
        case null => throw new RuntimeException("Class without superclass: " + clas)
        case c => Some(c)
    }

    def parameterizedTypeOfRawType(types: Seq[Type], rawType: Class[_]): Option[ParameterizedType] =
        (parameterizedTypes(types) filter { _.getRawType == rawType } ensuring { _.size <= 1 }).headOption

    def parameterizedTypes(types: Seq[Type]): List[ParameterizedType] =
        types.toList filter { _.isInstanceOf[ParameterizedType] } map { _.asInstanceOf[ParameterizedType]}

    def isVoid(t: Class[_]) = t == classOf[Void]  ||  t.getName == "void"
    def isClass(t: Class[_]) = classOf[Object] isAssignableFrom t
    def isStringClass(t: Class[_]) = classOf[String] isAssignableFrom t
    def isByteArrayClass(t: Class[_]) = t.isArray && t.getComponentType.getName == "byte"

    def compareClassSeqs(a: Seq[Class[_]], b: Seq[Class[_]]): Int =
        a zip b map { x => compareClasses(x._1, x._2) } find { _ != 0 } match {
            case None => a.length compare b.length
            case Some(c) => c
        }

    def compareClasses(a: Class[_], b: Class[_]): Int = a.getName compare b.getName
}
