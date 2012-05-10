package com.sos.scheduler.engine.cplusplus.generator.cpp

import com.sos.scheduler.engine.cplusplus.generator.util.ClassOps._
import java.lang.reflect.Method

object Jni {
  private case class TypeMetadata(cppName: String, signature: String) {
    def jvalueUnionMember = signature.head.toLower    // Element vom C++-union jvalue
  }

  private val jniTypeMap: Map[Class[_],TypeMetadata] = Map(
    classOf[Void]    -> TypeMetadata("void"    , "V"),
    classOf[Boolean] -> TypeMetadata("jboolean", "Z"),
    classOf[Byte]    -> TypeMetadata("jbyte"   , "B"),
    classOf[Char]    -> TypeMetadata("jchar"   , "C"),
    classOf[Int]     -> TypeMetadata("jint"    , "I"),
    classOf[Short]   -> TypeMetadata("jshort"  , "S"),
    classOf[Long]    -> TypeMetadata("jlong"   , "J"),
    classOf[Float]   -> TypeMetadata("jfloat"  , "F"),
    classOf[Double]  -> TypeMetadata("jdouble" , "D"),
    classOf[Object]  -> TypeMetadata("jobject" , signatureStringOfClass(classOf[Object])),
    classOf[String]  -> TypeMetadata("jstring" , signatureStringOfClass(classOf[String]))
  )

  private def metadata(t: Class[_]) = {
    val myType = if (isVoid(t))  classOf[Void]  else t
    jniTypeMap.get(myType)
  }


  def typeName(t: Class[_]) = metadata(t) match {
    case Some(m) => m.cppName
    case None => requireIsClass(t); "jobject"
  }

  private def requireIsClass(t: Class[_]) { require(isClass(t), "Unknown type '" + t + "'") }

  def signatureString(t: Class[_]): String = metadata(t) match {
    case Some(m) => m.signature
    case None =>
      if (t.isArray)  "[" + signatureString(t.getComponentType)
      else signatureStringOfClass(t)
  }

  private def signatureStringOfClass(c: Class[_]) = {
      requireIsClass(c)
      "L" + c.getName.replace('.', '/') + ";"
  }

  def jvalueUnionMember(typ: Class[_]) = jniTypeMap(if (jniTypeMap contains typ) typ else classOf[Object]).jvalueUnionMember

  //def signatureString(m: Method): String = mangled(m.getName) + parameterListSignatureString(m) + signatureString(m.getReturnType)

  def methodTypeSignatureString(m: Method): String = methodTypeSignatureString(m.getParameterTypes, m.getReturnType)

  def methodTypeSignatureString(parameterTypes: Seq[Class[_]], returnType: Class[_]) =
      (parameterTypes map signatureString mkString ("(", "", ")")) + signatureString(returnType)

  //def methodName(m: Method) = "Java_" + mangled(m.getDeclaringClass.getName + "." + methodNameWithoutClass(m))

  def simpleMethodName(m: Method): String = simpleMethodName(m.getName, m.getParameterTypes)

  def simpleMethodName(name: String, parameterTypes: Seq[Class[_]]) =
      mangled(name) + whenFilled("__", parameterTypes map mangled mkString)

  private def whenFilled(prefix: String, string: String) = if (string.isEmpty) ""  else prefix + string

  private def mangled(t: Class[_]): String = mangled(signatureString(t))

  def mangled(name: String): String = name
    .replace("_", "_1")
    .replace(";", "_2")
    .replace("[", "_3")
    .replace('.', '_')
    .replace('/', '_')
}
