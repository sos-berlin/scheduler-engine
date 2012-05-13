package com.sos.scheduler.engine.kernel.xml

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import org.w3c.dom.Document
import javax.xml.transform.TransformerFactory
import javax.xml.transform.dom.{DOMResult, DOMSource}
import scala.collection.JavaConversions._

@ForCpp
class CppXsltStylesheet @ForCpp()(document: Document) {
  private val transformer = TransformerFactory.newInstance().newTransformer(new DOMSource(document))

  @ForCpp def apply(document: Document, parameters: java.util.HashMap[String,String]): Document = {
    parameters foreach { case (name, value) => transformer.setParameter(name, value) }
    val result = new DOMResult
    transformer.transform(new DOMSource(document), result);
    result.getNode.asInstanceOf[Document]
  }
}
