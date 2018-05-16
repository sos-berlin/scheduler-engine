package com.sos.scheduler.engine.cplusplus.generator.javaproxy.procedure

import com.sos.scheduler.engine.cplusplus.generator.cpp._
import com.sos.scheduler.engine.cplusplus.generator.javaproxy.clas.CppClass
import com.sos.scheduler.engine.cplusplus.generator.util.ProcedureSignature
import java.lang.reflect.Constructor

final class CppConstructor(cppClass: CppClass, s: ProcedureSignature) extends CppProcedure(cppClass, s) {

  object objectInstantiator extends CppCode {
    val myName = cppClass.name.simpleName

    def headerCode = s"static $myName new_instance$parameterListDeclaration;"

    def sourceCode = {
      val declaration = s"$myName $myName::new_instance$parameterListDeclaration"
      val body =
        s"""{
           |    $myName result;
           |    result.java_object_allocate_();
           |${ objectClassCode.call("result._class.get()", "result.get_jobject()") }    return result;
           |}
           |""".stripMargin
      s"$declaration $body"
    }
  }

}

object CppConstructor {
  def apply(cppClass: CppClass, c: Constructor[_]) = new CppConstructor(cppClass, ProcedureSignature(c))
}
