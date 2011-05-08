import java.io.FileWriter;
import java.io.IOException;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerConfigurationException;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.apache.log4j.Logger;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

public class XMLDom {

	private static Logger logger = Logger.getLogger(XMLDom.class);

	public static final int DEFAULT_INDENT = 3;
	public static final String DEFAULT_ENCODING = "iso-8859-1";

	private final TransformerFactory tfactory;
	private final Transformer transformer;

	public XMLDom() {
		tfactory = TransformerFactory.newInstance();
		Transformer t = null;
		try {
			t = tfactory.newTransformer();
		} catch (TransformerConfigurationException e) {
			logger.trace("unable to create a transformer instance.", e);
		}
		transformer = t;
		setTransformerProperties(DEFAULT_INDENT, DEFAULT_ENCODING);
	}

	public Element createDom(Document doc) {
		Element node = doc.createElement("empty");
		return node;
	}

	public void setTransformerProperties(int indent, String encoding) {
		tfactory.setAttribute("indent-number", indent); // Einr�ckungstiefe
														// definieren
		transformer.setOutputProperty(OutputKeys.INDENT, "yes"); // Parameter
																	// setzen:
																	// Einr�cken
		transformer.setOutputProperty(OutputKeys.METHOD, "xml"); // Ausgabe-Typ:
																	// xml
		transformer.setOutputProperty(OutputKeys.MEDIA_TYPE, "text/xml"); // Content-Type
		transformer.setOutputProperty(OutputKeys.ENCODING, encoding); // Encoding
																		// setzen
	}

	public void writeToFile(Document doc, String fileName) {
		try {
			logger.debug("resultfile is " + fileName);
			FileWriter fw = new FileWriter(fileName);
			transformer.transform(new DOMSource(doc), new StreamResult(fw));
		} catch (TransformerException e) {
			logger.trace("error transforming XML content.", e);
		} catch (IOException e) {
			logger.trace("error writing file" + fileName + ".", e);
		}
	}

	public static Document getDocument() {
		Document doc = null;
		try {
			DocumentBuilder docBuilder = DocumentBuilderFactory.newInstance().newDocumentBuilder();
			doc = docBuilder.newDocument();
		} catch (ParserConfigurationException e) {
			logger.trace("error creating a xml Document.", e);
		}
		return doc;
	}

}
