package com.sos.scheduler.engine.kernelcpptest.excluded.js746;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.io.ByteArrayInputStream;
import java.io.StringWriter;
import java.io.Writer;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathFactory;

import org.apache.xml.serialize.OutputFormat;
import org.apache.xml.serialize.XMLSerializer;
import org.joda.time.DateTime;
import org.joda.time.format.DateTimeFormat;
import org.joda.time.format.DateTimeFormatter;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.Assert;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;

import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import com.sos.scheduler.engine.kernel.util.Time;

/**
 * \file JS746.java
 * \brief js-746: test show_calendar
 * 
 * \class JS746
 * \brief js-746: test show_calendar
 * 
 * \details
 * The class is for testing the behaviour of the command show_calendar for different run_times (single_start,
 * repeat and absolute_repeat).
 * 
 * \code
   \endcode
 * 
 * \author ss 
 * \version 1.0 - 30.09.2011 09:12:21 
 * <div class="sos_branding">
 * <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
@SuppressWarnings("deprecation")
public class JS746 extends SchedulerTest {

	private static final Time schedulerTimeout = Time.of(10);
	private static Logger logger;
	
	private static DateTimeFormatter format = DateTimeFormat.forPattern("dd.MM.yy HH:mm:ss");;

	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		logger = LoggerFactory.getLogger(JS746.class);
		logger.debug("starting test for " + JS746.class.getName());
	}

	@Test
	public void test() throws Exception {
		runScheduler(schedulerTimeout, "-e log-level=warn");

		// single_start: taeglich 12 Uhr
		testRange("2011-09-01T11:59:00", "2011-09-05T12:01:00", "single_start",	5);

		// single_start: taeglich 12 Uhr
		testRange("2011-09-01T12:00:00", "2011-09-01T12:01:00", "single_start",	1);

		// single_start_monday: montags 15 und 18 Uhr
		testRange("2011-09-01T18:00:00", "2011-09-05T19:00:00", "single_start_monday",	2);

		// absolute_repeat: period_1 von 10-18Uhr 1x stündlich
		testRange("2011-09-01T15:30:00", "2011-09-05T12:00:00", "period_1", 5);

		// absolute_repeat: period_1 von 10-18Uhr 1x stündlich
		testRange("2011-09-01T17:30:00", "2011-09-01T20:00:00", "period_1", 0);

		// repeat: period_2 von 10-18Uhr 1x stündlich
		testRange("2011-09-01T17:30:00", "2011-09-01T20:00:00", "period_2", 1);
	}

	private void testRange(String from, String to, String jobname,
			int expectedHits) throws Exception {
		DateTime dtFrom = new DateTime(from);
		DateTime dtTo = new DateTime(to);
		Document doc = show_calendar(from, to);
		testResult(doc, jobname, dtFrom, dtTo, expectedHits);
	}

	private Document show_calendar(String from, String to) throws Exception {
		String result = scheduler.executeXml("<show_calendar before='" + to
				+ "' from='" + from + "' limit='100' what='orders'/>");
		DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
		DocumentBuilder db = dbf.newDocumentBuilder();
		Document document = db.parse(new InputSource(new ByteArrayInputStream(
				result.getBytes("utf-8"))));
		logger.debug(formatXml(document));
		return document;
	}

	/**
	 * \brief selects a nodelist from the response for a given job 
	 * \detail
	 *
	 * @param doc - the DOM document with the result
	 * @param jobname - the jobname
	 * @return a NodeList containing at or period elements
	 * @throws Exception
	 */
	private static NodeList getResultSet(Document doc, String jobname)
			throws Exception {
		XPath xpath = XPathFactory.newInstance().newXPath();
		String path = "//*[@job='/" + jobname + "']";
		logger.debug("xpath=" + path);
		XPathExpression expr = xpath.compile(path);
		Object result = expr.evaluate(doc, XPathConstants.NODESET);
		NodeList nodes = (NodeList) result;
		logger.debug("result=" + result);
		return nodes;
	}

	/**
	 * \brief check if the response is correct for a job
	 * \detail
	 *
	 * @param doc - the DOM document with the result
	 * @param jobname - the jobname
	 * @param dtFrom - begin of the period of the show_calendar command
	 * @param dtTo - end of the period of the show_calendar command
	 * @param expectedHits - expected occurences for the job in the response
	 * @throws Exception
	 */
	private static void testResult(Document doc, String jobname, DateTime dtFrom,
			DateTime dtTo, int expectedHits) throws Exception {
		NodeList result = getResultSet(doc, jobname);
		assertEquals(jobname + " [" + dtFrom.toString(format) + "-" + dtTo.toString(format)
				+ "]", expectedHits, result.getLength());
		for (int i = 0; i < result.getLength(); i++) {
			Node n = result.item(i);
			if (n.getNodeName().equals("at")) {
				DateTime dtNodeAt = getDateTimeAttribute(n, "at");
				assertTrue ("(period start) " + getMessage(jobname,dtNodeAt,dtFrom,dtTo), dtFrom.isBefore(dtNodeAt) || dtFrom.equals(dtNodeAt) );
				assertTrue ("(period end) " + getMessage(jobname,dtNodeAt,dtFrom,dtTo), dtTo.isAfter(dtNodeAt));
			} else {
				DateTime dtNodeFrom = getDateTimeAttribute(n, "begin");
				DateTime dtNodeTo = getDateTimeAttribute(n, "end");
				assertTrue ("(period start) " + getMessage(jobname,dtNodeTo,dtFrom,dtTo), dtFrom.isBefore(dtNodeTo) );
				assertTrue ("(period end) " + getMessage(jobname,dtNodeFrom,dtFrom,dtTo), dtTo.isAfter(dtNodeFrom));
			}
		}
	}
	
	private static String getMessage(String jobname, DateTime timestamp, DateTime from, DateTime to) {
		return "job " + jobname + ": " + timestamp.toString(format) + " is outside the period " + from.toString(format) + "-" + to.toString(format);
	}

	private static DateTime getDateTimeAttribute(Node node, String attributeName) {
		String strTime = node.getAttributes().getNamedItem(attributeName).getTextContent();
		return new DateTime(strTime);
	}

	private String formatXml(Document doc) throws Exception {
		OutputFormat format = new OutputFormat(doc);
		format.setLineWidth(65);
		format.setIndenting(true);
		format.setIndent(2);
		Writer out = new StringWriter();
		XMLSerializer serializer = new XMLSerializer(out, format);
		serializer.serialize(doc);
		return out.toString();
	}

}
