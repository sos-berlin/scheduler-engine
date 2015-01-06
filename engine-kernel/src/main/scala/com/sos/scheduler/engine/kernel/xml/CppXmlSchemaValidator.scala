package com.sos.scheduler.engine.kernel.xml

import CppXmlSchemaValidator._
import com.google.common.io.Resources._
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import java.net.URL
import javax.xml.XMLConstants.W3C_XML_SCHEMA_NS_URI
import javax.xml.transform.dom.DOMSource
import javax.xml.validation.SchemaFactory
import org.w3c.dom.Document

@ForCpp
final class CppXmlSchemaValidator @ForCpp()(urlString: String) {
  private val url = if (urlString.isEmpty) schedulerXmlSchemaUrl else new URL(urlString)
  private val validator = SchemaFactory.newInstance(W3C_XML_SCHEMA_NS_URI).newSchema(url).newValidator

  @ForCpp private[xml] def validate(document: Document): Unit = {
    validator.validate(new DOMSource(document))
  }
}

object CppXmlSchemaValidator {
  private lazy val schedulerXmlSchemaUrl = getResource("com/sos/scheduler/enginedoc/common/scheduler.xsd")
}
