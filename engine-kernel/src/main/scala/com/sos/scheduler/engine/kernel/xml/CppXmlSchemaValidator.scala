package com.sos.scheduler.engine.kernel.xml

import com.sos.jobscheduler.common.utils.JavaResource
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.xml.CppXmlSchemaValidator._
import java.net.URL
import javax.xml.XMLConstants.W3C_XML_SCHEMA_NS_URI
import javax.xml.transform.dom.DOMSource
import javax.xml.validation.SchemaFactory
import org.w3c.dom.Document

@ForCpp
final class CppXmlSchemaValidator @ForCpp()(urlString: String) {
  private val url = if (urlString.isEmpty) schedulerXmlSchemaResource.url else new URL(urlString)
  private val validator = SchemaFactory.newInstance(W3C_XML_SCHEMA_NS_URI).newSchema(url).newValidator

  @ForCpp private[xml] def validate(document: Document): Unit = {
    validator.validate(new DOMSource(document))
  }
}

object CppXmlSchemaValidator {
  private lazy val schedulerXmlSchemaResource = JavaResource("com/sos/scheduler/enginedoc/common/scheduler.xsd")
}
