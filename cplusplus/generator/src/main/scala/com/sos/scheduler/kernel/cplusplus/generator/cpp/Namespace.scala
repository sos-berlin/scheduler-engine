package com.sos.scheduler.kernel.cplusplus.generator.cpp

import java.io.File
import scala.collection.mutable


case class Namespace(simpleName: String, parent: Option[Namespace]) {
    require(!simpleName.isEmpty || parent.isEmpty, "Namespace with empty name but parent=" + parent)

    def nestedCode(code: String): String = {
        val separator = if (code contains '\n')  "\n" else ""
        val names = this.names
        (names map { "namespace " + _ + " { " } mkString) + separator*2 +
        code +
        separator + "}" * names.length + "\n"
    }

    def usingCode = "using namespace " + this.fullName + ";\n"


    def fullName: String = parent match {
        case Some(a) => a.fullName + "::" + simpleName
        case None => simpleName
    }

    def names: List[String] = {
        var result = List[String]()
        var o: Option[Namespace] = Some(this)
        while (o.isDefined  &&  o.get.simpleName != "") {
            result = o.get.simpleName :: result
            o = o.get.parent
        }
        result.toList ensuring { _ forall { _.nonEmpty } }
    }

    def filePath = names mkString File.separator

    def +(o: Namespace): Namespace = Namespace(names ::: o.names)
    def +(o: CppName): CppName = CppName(o.simpleName, this + o.namespace)
    
    override def toString = fullName
}

object Namespace {
    val root = new Namespace("", None)

    def apply(fullName: String): Namespace = Namespace( fullName.replace(" ","") split "::" dropWhile { _.isEmpty } )

    def apply(names: Seq[String]): Namespace = {
        var result = root
        for (n <- names)  result = Namespace(n, result)
        result
    }

    def apply(simpleName: String, namespace: Namespace): Namespace = Namespace(simpleName, Some(namespace))
}
