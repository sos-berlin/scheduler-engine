package com.sos.scheduler.engine.kernel.xml

import CppXmlSchemaValidatorTest._
import com.google.common.io.Resources.getResource
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import java.net.URL
import javax.xml.XMLConstants._
import javax.xml.parsers.DocumentBuilderFactory
import javax.xml.transform.stream.StreamSource
import javax.xml.validation.SchemaFactory
import org.junit.runner.RunWith
import org.scalatest.FunSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import org.xml.sax.SAXParseException

@RunWith(classOf[JUnitRunner])
class CppXmlSchemaValidatorTest extends FunSpec {

  describe("CppXmlSchemaValidator") {
    lazy val validator = new CppXmlSchemaValidator(schemaResourceUrl.toExternalForm)

    it("should accept valid XML") {
      validator.validate(dom(validXmlUrl))
    }

    it("should reject invalid XML") {
      validationShouldFail { validator.validate(dom(invalidXmlUrl)) }
    }
  }

  describe("javax.xml.Validator") {
    lazy val validator = SchemaFactory.newInstance(W3C_XML_SCHEMA_NS_URI).newSchema(schemaResourceUrl).newValidator

    it("should accept valid XML") {
      validate(validXmlUrl)
    }

    it("should reject invalid XML") {
      validationShouldFail { validate(invalidXmlUrl) }
    }

    def validate(u: URL) {
      autoClosing(u.openStream()) { in ⇒
        validator.validate(new StreamSource(in))
      }
    }
  }
}

object CppXmlSchemaValidatorTest {
  private lazy val schemaResourceUrl = getResource("com/sos/scheduler/engine/kernel/xml/test.xsd")
  private lazy val validXmlUrl = getResource("com/sos/scheduler/engine/kernel/xml/testValid.xml")
  private lazy val invalidXmlUrl = getResource("com/sos/scheduler/engine/kernel/xml/testInvalid.xml")

  private def dom(u: URL) = {
    val factory = DocumentBuilderFactory.newInstance
    factory.setNamespaceAware(true)
    factory.newDocumentBuilder.parse(u.toURI.toString)
  }

  private def validationShouldFail(f: => Unit) {
    val x = intercept[SAXParseException] { f }
    x.getMessage should include ("INVALID_ATTRIBUTE")
  }
}
