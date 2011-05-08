package com.sos.scheduler.engine.cplusplus.generator.cppproxy

import com.sos.scheduler.engine.cplusplus.generator.Configuration
import com.sos.scheduler.engine.cplusplus.generator.Configuration._
import com.sos.scheduler.engine.cplusplus.generator.cpp.Cpp.includeQuoted
import com.sos.scheduler.engine.cplusplus.generator.module._
import com.sos.scheduler.engine.cplusplus.generator.util.Util._


class RegisterNativeClassesModule(includes: Seq[String], jniModules: Iterable[JniModule])
extends CppModule {
    //TODO Nicht eindeutig, wenn verschiedene JniModule.directory vorliegen.
    //Es wird für alle directory derselbe Funktionsname im selben Namespace javaproxy generiert.
    //Bisher genügt das, weil wir nur ein directory haben, nämlich "scheduler".
    //Lösung: Über Z_INIT() statisch verketten, wie "Message_code_text error_codes".
    //Der Aufruf registerNativeClasses() baut dann aus der Liste das Array auf und übergibt es JNI.

    val name = "jni__register_native_classes"

    private val declaration = "void register_native_classes()"

    lazy val headerCodeOption = Some(declaration + ";\n")

    lazy val sourceCodeOption = Some(
        ((includes ++ Configuration.cppStandardIncludes) map includeQuoted).mkString + "\n" +
        (Configuration.cppStandardUsingNamespaces map { _.usingCode }).mkString + "\n" +
        declaration + "{ \n" +
        (jniModules.toSeq sortBy { _.name } map { "    " + _.registerCode + "\n" }).mkString +
        "}\n"
    )
}
