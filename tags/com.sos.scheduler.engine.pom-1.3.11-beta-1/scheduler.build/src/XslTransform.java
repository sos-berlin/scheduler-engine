import javax.xml.transform.*;
import javax.xml.transform.stream.StreamSource;
import javax.xml.transform.stream.StreamResult;
import java.io.*;

public class XslTransform {

	public static void transform(String xmlFile, String xslFile,
			String resultFile) {
		try {
			TransformerFactory fact = TransformerFactory.newInstance();
			Source xsl = new StreamSource(new FileInputStream(xslFile));
			Source xml = new StreamSource(new FileInputStream(xmlFile));
			Result output = new StreamResult(new FileOutputStream(resultFile));
			Transformer transformer = fact.newTransformer(xsl);
			transformer.transform(xml, output);
		} catch (TransformerException e) {
			e.printStackTrace();
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		}
	}
}
