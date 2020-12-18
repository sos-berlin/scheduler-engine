package com.sos.scheduler.engine.tests.jira.js1049

import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime.DurationRichInt
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.job.{JobDescription, JobPath}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderDetailed, OrderFinished, OrderId, OrderKey, OrderSuspended}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerEncoding
import com.sos.scheduler.engine.kernel.variable.SchedulerVariableSet
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1049.JS1049IT._
import java.nio.charset.Charset
import java.nio.charset.StandardCharsets.{UTF_16BE, UTF_8}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.xml.XML

@RunWith(classOf[JUnitRunner])
final class JS1049IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val httpPort = findRandomFreeTcpPort()
  protected override lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = s"-http-port=$httpPort" :: Nil,
    logCategories = "scheduler.mainlog")
  private lazy val client = new StandardWebSchedulerClient(s"http://127.0.0.1:$httpPort").closeWithCloser

  override def onBeforeSchedulerActivation(): Unit = {
    for (i ← JobIncludeSettings flatMap { _.includes })
      (controller.environment.liveDirectory / i.filename).write(i.content, i.encoding)
  }

  "XML configuration files" - {
    "scheduler.xml in UTF-8" in {
      instance[SchedulerVariableSet].apply("Å") shouldEqual "å"
    }

    "job.xml in UTF-8" in {
      jobView[JobDescription](JobPath("/test-a")).description shouldEqual "å"
    }

    for (j ← JobIncludeSettings; d = j.descriptionInclude; s = j.scriptInclude)
      s"Include $d, $s" in {
        (controller.environment.liveDirectory / d.filename).contentString(d.encoding) shouldEqual d.content
        jobView[JobDescription](j.jobPath).description shouldEqual d.content
        job(j.jobPath).scriptText shouldEqual s.content
      }
  }

  "Job description by executeXml" in {
    (scheduler executeXml <show_job job={JobPath("/test-a").string} what="description"/>).string should include ("<description>å</description>")
  }

  "Job script with include" in {
    runJob(TextIncludeJobPath)
  }

  "XML Schema check" in {
    controller.suppressingTerminateOnError {
      intercept[Exception] { scheduler executeXml <show_state INVALID-ATTRIBUTE="xx"/> } .getMessage should include ("INVALID-ATTRIBUTE")
    }
  }

  "Invalid XML" in {
    controller.suppressingTerminateOnError {
      intercept[Exception] { scheduler executeXml "<>" } .getMessage should include ("SAXParseException")
    }
  }

  "Order.xml_payload" in {
    eventBus.awaiting[OrderFinished](XmlPayloadOrderKey) {
      scheduler executeXml OrderCommand(XmlPayloadOrderKey)
    }
  }

  "JS-1901 permanent order parameter with umlaut in XML" - {
    "ISO-8859-1 (1)" in {
      runOrderEncodingTest(1, OrderId("ISO-8859-1"))
    }

    "ISO-8859-1 (2)" in {
      runOrderEncodingTest(2, OrderId("ISO-8859-1"))
    }

    "UTF-8 (1)" in {
      runOrderEncodingTest(3, OrderId("UTF-8"))
    }

    "UTF-8 (2)" in {
      runOrderEncodingTest(4, OrderId("UTF-8"))
    }
  }

  private def runOrderEncodingTest(runNumber: Int, orderId: OrderId): Unit = {
    val orderKey = JobChainPath("/test") orderKey orderId
    val expected = s"TEST-${orderId.string}-ÄÖÜ"
    logParameter(orderKey)
    assert(orderDetailed(orderKey).variables("PARAMETER") == expected)
    assert(httpOrderDetailed(orderKey).variables("PARAMETER") == expected)

    eventBus.awaiting[OrderSuspended.type](orderKey) {
      scheduler executeXml <modify_order job_chain="/test" order={orderKey.id.string} at="now"/>
    }
    logParameter(orderKey)
    assert(orderDetailed(orderKey).variables("PARAMETER") == expected)
    assert(httpOrderDetailed(orderKey).variables("PARAMETER") == expected)
    assert(orderLog(orderKey) contains s"#$runNumber PARAMETER=$expected")

    eventBus.awaiting[OrderFinished](orderKey) {
      scheduler executeXml <modify_order job_chain="/test" order={orderKey.id.string} suspended="false"/>
    }
    logParameter(orderKey)
    assert(orderDetailed(orderKey).variables("PARAMETER") == expected)
    assert(httpOrderDetailed(orderKey).variables("PARAMETER") == expected)
    assert(orderDetailed(orderKey).variables("PARAMETER") == expected)
    assert(httpShowOrderVariables(orderKey)("PARAMETER") == expected)
  }


  private def logParameter(orderKey: OrderKey): Unit = {
    val p = orderDetailed(orderKey).variables("PARAMETER")
    logger.info("PARAMETER=" + p + "  0x" + (p.filter(c => (c & 0x80) != 0).map(_.toInt).map(c => f"$c%02X").mkString))
  }

  private def httpOrderDetailed(orderKey: OrderKey): OrderDetailed =
    client.order[OrderDetailed](orderKey).await(TestTimeout).value

  private def httpShowOrderVariables(orderKey: OrderKey): Map[String, String] = {
    val responseXml = XML.loadString(
      client.execute(<show_order what="payload" job_chain={orderKey.jobChainPath.string} order={orderKey.id.string}/>)
        .await(99.s))
    (responseXml \ "answer" \ "order" \ "payload" \ "params" \ "param")
      .collect {
        case e: xml.Elem => e.attributes.asAttrMap("name") -> e.attributes.asAttrMap("value")
      }
      .toMap
  }
}

object JS1049IT
{
  private val logger = Logger(getClass)

  private case class Include(filename: String, encoding: Charset, content: String) {
    override def toString = s"$filename $encoding"
  }

  private case class JobIncludeSetting(
      jobPath: JobPath,
      descriptionInclude: Include,
      scriptInclude: Include) {
    def includes = List(descriptionInclude, scriptInclude)
  }

  private val TextIncludeJobPath = JobPath("/test-text-include")
  private val JobIncludeSettings = List(
    JobIncludeSetting(
      TextIncludeJobPath,
      descriptionInclude = Include("test-description.txt", schedulerEncoding, "ö"),
      scriptInclude = Include("test-script.txt", schedulerEncoding, "exit 0")),
    JobIncludeSetting(
      JobPath("/test-xml-include"),
      descriptionInclude = Include("test-description.xhtml", UTF_8, "<p>ü</p>"),
      scriptInclude = Include("test-script.xml", UTF_8, "<p>ß</p>")),
    JobIncludeSetting(
      JobPath("/test-xml-prolog-include"),
      descriptionInclude = Include("test-description-prolog.xhtml", UTF_16BE, "<?xml version='1.0' encoding='UTF-16BE'?><p>°</p>"),
      scriptInclude = Include("test-script-prolog.xml", UTF_16BE, "<?xml version='1.0' encoding='UTF-16BE'?><p>§</p>")))
  private val XmlPayloadOrderKey = JobChainPath("/test-xml-payload") orderKey "1"
}
