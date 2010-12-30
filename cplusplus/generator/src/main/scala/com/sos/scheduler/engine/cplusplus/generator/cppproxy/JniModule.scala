package com.sos.scheduler.engine.cplusplus.generator.cppproxy

import com.sos.scheduler.engine.cplusplus.generator.Configuration
import com.sos.scheduler.engine.cplusplus.generator.Configuration._
import com.sos.scheduler.engine.cplusplus.generator.cpp._
import com.sos.scheduler.engine.cplusplus.generator.cpp.Cpp.quoted
import com.sos.scheduler.engine.cplusplus.generator.cpp.javabridge.JavaBridge
import com.sos.scheduler.engine.cplusplus.generator.module._
import com.sos.scheduler.engine.cplusplus.generator.util._
import com.sos.scheduler.engine.cplusplus.generator.util.MyRichString._
//import JniModule._  // Scala 2.8.0: "illegal cyclic reference involving value <import>"


class JniModule(config: CppClassConfiguration, procedureSignatures: Seq[ProcedureSignature], javaClassFullName: String)
extends CppModule {
    val interface = config.interface
    val className = CppName(config.className)
    val name = JniModule.namePrefix + className.simpleNames.mkString("__")
    val subdirectory = config.directory
    val include = config.includeOption
    
    val hasProxyClassName = "has_proxy< " + className + " >"

    lazy val headerCodeOption = None
    
    lazy val sourceCodeOption = Some {
        val jniMethods = procedureSignatures map { new JniMethod(this, _) }

        def proxyClassFactoryDefinition = JavaBridge.namespace.nestedCode {
            "    template<> const class_factory<Proxy_class> " + hasProxyClassName + "::proxy_class_factory(" + quoted(javaClassFullName)+ ");\n"
        }

        def cppNativeMethodImplementation = jniMethods map { _.nativeImplementation } mkString "\n"

        def cppRegisterNativesDefinition = if (jniMethods.isEmpty) ""  else
            "const static JNINativeMethod native_methods[] = {\n" +
            ( jniMethods map { "    " + _.registerNativeArrayEntry} ).mkString("", ",\n", "\n") +
            "};\n"

        def cppRegisterNativesFunction = {
            def declaration = "template<> void " + hasProxyClassName + "::register_cpp_proxy_class_in_java()"
            def body =
                "        Env env;\n" +
                "        Class* cls = " + hasProxyClassName + "::proxy_class_factory.clas();\n" +
                "        int ret = env->RegisterNatives(*cls, native_methods, " + jniMethods.size + ");\n" +
                "        if (ret < 0)  env.throw_java(\"RegisterNatives\");\n"

            JavaBridge.namespace.nestedCode(
                "    " + declaration + " {\n" +
                body ? jniMethods.nonEmpty +
                "    }\n")
        }

        ((config.includeOption ++ Configuration.cppStandardIncludes) map Cpp.includeQuoted).mkString + "\n" +
        (Configuration.cppStandardUsingNamespaces map { _.usingCode }).mkString + "\n" +
        proxyClassFactoryDefinition + "\n" +
        cppNativeMethodImplementation + "\n" +
        cppRegisterNativesDefinition + "\n" +
        cppRegisterNativesFunction
    }

    def registerCode = hasProxyClassName + "::register_cpp_proxy_class_in_java();"
}


object JniModule extends ModuleKind[JniModule] {
    val namePrefix = "jni__"

    def fileIsGeneratedAndCanBeDeleted(filename: String) =
        (filename startsWith namePrefix)  &&  CppModule.fileIsGeneratedAndCanBeDeleted(filename)
}
