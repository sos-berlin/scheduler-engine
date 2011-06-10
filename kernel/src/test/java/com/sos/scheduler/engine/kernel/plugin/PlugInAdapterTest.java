package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.PlatformMock;
import org.junit.Test;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.loadXml;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;


public class PlugInAdapterTest {
    private final Platform platform = PlatformMock.newInstance();
    private final PluginAdapter plugInAdapter = new PluginAdapter(new PlugInMock(), "plugInMock", platform.log());
    private final PluginAdapter errorPlugInAdapter = new PluginAdapter(new ErrorPlugInMock(), "errorPlugInMock", platform.log());


    @Test public void testActivate() {
        plugInAdapter.activate();
    }


    @Test(expected=RuntimeException.class) public void testActivate_error() {
        errorPlugInAdapter.activate();
    }


    @Test public void testClose() {
        plugInAdapter.close();
    }


    @Test(expected=RuntimeException.class) public void testClose_error() {
        errorPlugInAdapter.close();
    }


    @Test public void testGetXmState() {
        loadXml(plugInAdapter.getXmlState());
        errorPlugInAdapter.getXmlState();
    }

    
    @Test public void testTryActivate() {
        plugInAdapter.tryActivate();
        errorPlugInAdapter.tryActivate();
    }


    @Test public void testTryClose() {
        plugInAdapter.tryClose();
        errorPlugInAdapter.tryClose();
    }

    @Test public void testGetXmlState() {
        loadXml(plugInAdapter.getXmlState());
        loadXml(errorPlugInAdapter.getXmlState());
    }

    @Test public void testToString() {
        assertThat(plugInAdapter.toString(), startsWith("Plugin "));
    }
}