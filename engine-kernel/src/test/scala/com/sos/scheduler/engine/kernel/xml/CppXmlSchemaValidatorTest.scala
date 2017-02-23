package com.sos.scheduler.engine.kernel.xml

import com.sos.jobscheduler.common.scalautil.AutoClosing.autoClosing
import com.sos.jobscheduler.common.utils.JavaResource
import com.sos.scheduler.engine.kernel.xml.CppXmlSchemaValidatorTest._
import java.net.URL
import javax.xml.XMLConstants._
import javax.xml.parsers.DocumentBuilderFactory
import javax.xml.transform.stream.StreamSource
import javax.xml.validation.SchemaFactory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import org.xml.sax.SAXParseException

@RunWith(classOf[JUnitRunner])
class CppXmlSchemaValidatorTest extends FreeSpec {

  "CppXmlSchemaValidator" - {
    lazy val validator = new CppXmlSchemaValidator(SchemaResourceUrl.toExternalForm)

    "should accept valid XML" in {
      validator.validate(dom(ValidXmlUrl))
    }

    "should reject invalid XML" in {
      validationShouldFail { validator.validate(dom(InvalidXmlUrl)) }
    }
  }

  "javax.xml.Validator" - {
    lazy val validator = SchemaFactory.newInstance(W3C_XML_SCHEMA_NS_URI).newSchema(SchemaResourceUrl).newValidator

    "should accept valid XML" in {
      validate(ValidXmlUrl)
    }

    "should reject invalid XML" in {
      validationShouldFail { validate(InvalidXmlUrl) }
    }

    def validate(u: URL): Unit = {
      autoClosing(u.openStream()) { in ⇒
        validator.validate(new StreamSource(in))
      }
    }
  }
}

object CppXmlSchemaValidatorTest {
  private lazy val SchemaResourceUrl = JavaResource("com/sos/scheduler/engine/kernel/xml/test.xsd").url
  private lazy val ValidXmlUrl = JavaResource("com/sos/scheduler/engine/kernel/xml/testValid.xml").url
  private lazy val InvalidXmlUrl = JavaResource("com/sos/scheduler/engine/kernel/xml/testInvalid.xml").url

  private def dom(u: URL) = {
    val factory = DocumentBuilderFactory.newInstance
    factory.setNamespaceAware(true)
    factory.newDocumentBuilder.parse(u.toURI.toString)
  }

  private def validationShouldFail(f: => Unit): Unit = {
    val x = intercept[SAXParseException] { f }
    x.getMessage should include ("INVALID_ATTRIBUTE")
  }
}
