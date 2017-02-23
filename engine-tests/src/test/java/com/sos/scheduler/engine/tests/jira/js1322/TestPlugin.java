package com.sos.scheduler.engine.tests.jira.js1322;

import com.sos.jobscheduler.data.filebased.AbsolutePath;
import com.sos.scheduler.engine.data.filebased.FileBasedType;
import com.sos.scheduler.engine.data.job.JobPath;
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin;
import com.sos.scheduler.engine.kernel.plugin.Plugins;
import com.sos.scheduler.engine.kernel.plugin.XmlConfigurationChangingPlugin;
import javax.inject.Inject;
import javax.inject.Named;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;

import static com.google.common.base.Preconditions.checkArgument;
import static com.google.common.base.Strings.nullToEmpty;
import static com.sos.scheduler.engine.common.javautils.ScalaInJava.toScalaSet;
import static com.sos.scheduler.engine.common.xml.CppXmlUtils.loadXml;
import static com.sos.scheduler.engine.common.xml.CppXmlUtils.toXmlBytes;
import static java.nio.charset.StandardCharsets.UTF_8;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * Simplified Plugin, which append a string to every file based configured job's titles.
 * Usage
 * <pre>
 *    &lt;plugin java_class="com.sos.scheduler.engine.tests.jira.js1322.TestPlugin">
 *       &lt;plugin.config append-to-title="TEST-SUFFIX"/>
 *    &lt;/plugin>
 * </pre>
 *
 * @author Joacim Zschimmer
 */
public final class TestPlugin extends AbstractPlugin implements XmlConfigurationChangingPlugin {

    private final String appendToTitle;

    @Inject
    private TestPlugin(@Named(Plugins.configurationXMLName) Element pluginElement) {
        // Missing: check for valid configuration
        appendToTitle = pluginElement.getAttribute("append-to-title");
    }

    @Override public scala.collection.immutable.Set<FileBasedType> fileBasedTypes() {
        return toScalaSet(FileBasedType.Job);
    }

    @Override public byte[] changeXmlConfiguration(FileBasedType typ, AbsolutePath path, byte[] xmlBytes) {
        // Code is for example only. It loads the job XML as a DOM tree and manipulates it.
        assertEquals(typ, FileBasedType.Job);
        if (path.equals(new JobPath("/error"))) throw new RuntimeException("TestPlugin does not like " + path);
        assertTrue(path.equals(new JobPath("/test")));

        Document doc = xmlBytesToDom(xmlBytes);
        modifyJobElement(doc.getDocumentElement());
        return domToXmlBytes(doc);
    }

    private void modifyJobElement(Element element) {
        checkArgument(element.getLocalName().equals("job"));
        String title = nullToEmpty(element.getAttribute("title"));
        element.setAttribute("title", title + " - " + appendToTitle);
    }

    private static Document xmlBytesToDom(byte[] xmlBytes) {
        String encoding = "";
        return loadXml(xmlBytes, encoding);
    }

    private static byte[] domToXmlBytes(Node node) {
        boolean indent = false;
        return toXmlBytes(node, UTF_8, indent);
    }
}
