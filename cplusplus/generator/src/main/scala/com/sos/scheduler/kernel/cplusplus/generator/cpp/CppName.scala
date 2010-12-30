package com.sos.scheduler.kernel.cplusplus.generator.cpp

import com.sos.scheduler.kernel.cplusplus.generator.Configuration


case class CppName(simpleName: String, namespace: Namespace) extends Ordered[CppName] {
    require(!simpleName.contains(':'), "Not a simple name: " + simpleName)
    
    val fullName = namespace.fullName + "::" + simpleName

    def simpleNames = namespace.names ::: simpleName :: Nil

    def compare(o: CppName) = fullName compare o.fullName
    
    override def toString = fullName
}


object CppName {
    def apply(fullName: String): CppName = {
        val NamespaceAndNameRegex = """^(.+)::([^:]+)$""".r
        fullName match {
            case NamespaceAndNameRegex(namespace, simpleName) => new CppName(simpleName, Namespace(namespace))
            case simpleName => new CppName(simpleName, Namespace.root)
        }
    }

    def apply(c: Class[_]): CppName = Configuration.generatedJavaProxyNamespace + CppName(c.getName.replace(".", "::"))
}
