package com.sos.scheduler.engine.cplusplus.generator.javaproxy.clas

import com.sos.scheduler.engine.cplusplus.generator.cpp._
import com.sos.scheduler.engine.cplusplus.generator.cpp.javabridge.JavaBridge

class StringCppObjectClass(cppClass: CppClass) extends CppObjectClass(cppClass)
{
    private val superclassConstructorsCode = super.constructorsCode

    override protected def constructorsCode = new CppCode {
        def headerCode = superclassConstructorsCode.headerCode +
            "    String(const char*);\n" +     // Kompatibilität zu C++-string
            "    String(const string&);\n" +
            "    String(const ::zschimmer::com::Bstr&);\n" +
            "    operator string() const;\n"
            
        def sourceCode = {
            def stringConstructor(parameter: String) =
                "String::String(" + parameter + ") " +
                "{ assign_(" + JavaBridge.namespace + "::Env().jstring_from_string(s)); }\n"

            superclassConstructorsCode.sourceCode +
            "\n" +
            stringConstructor("const char* s") +
            stringConstructor("const string& s") +
            stringConstructor("const ::zschimmer::com::Bstr& s") +
            "\n" +
            "String::operator string() const { return " + JavaBridge.namespace + "::Env().string_from_jstring( (jstring)get_jobject() ); }\n"
        }
    }
}
