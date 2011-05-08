package com.sos.scheduler.engine.playground.zschimmer

import javax.xml.transform.TransformerFactory
import javax.xml.transform.dom.DOMSource
import javax.xml.transform.sax.SAXResult
import org.w3c.dom.Node
import scala.xml.TopScope
import scala.xml.parsing.NoBindingFactoryAdapter


object XMLs {
//    def fromJavaDom(doc: Document) {
//        val charWriter = new CharArrayWriter()
//        TransformerFactory.newInstance.newTransformer.transform(new DOMSource(doc), new StreamResult(charWriter))
//        XML.load(new CharArrayReader(charWriter.toCharArray))
//    }

    def fromJavaDom(node: Node) = {
        val saxHandler = new NoBindingFactoryAdapter()
        saxHandler.scopeStack.push(TopScope)
        TransformerFactory.newInstance.newTransformer.transform(new DOMSource(node), new SAXResult(saxHandler))
        saxHandler.scopeStack.pop
        saxHandler.rootElem
    }
}
