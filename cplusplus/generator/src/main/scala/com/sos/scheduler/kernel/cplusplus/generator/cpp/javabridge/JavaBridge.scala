package com.sos.scheduler.kernel.cplusplus.generator.cpp.javabridge

import com.sos.scheduler.kernel.cplusplus.generator.cpp.CppName
import com.sos.scheduler.kernel.cplusplus.generator.cpp.Namespace


/** Gegenstück zum C++-Namespace javabridge */
object JavaBridge {
    val namespace = Namespace("zschimmer::javabridge")

    //def hasProxyClassName(cppClassName: CppName) =  namespace + "::has_proxy< " + cppClassName + " >"

    /** Typsicheres jobject eins has_proxy<> zur Parameterübergabe an Java. */
    def hasProxyJobjectClassName(cppClassName: String) = namespace + "::proxy_jobject< " + cppClassName + " >"
}
