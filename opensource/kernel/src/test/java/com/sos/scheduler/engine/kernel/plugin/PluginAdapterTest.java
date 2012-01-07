package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.scheduler.Platform;
import com.sos.scheduler.engine.kernel.scheduler.PlatformMock;
import org.junit.Test;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.loadXml;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;

public class PluginAdapterTest {
    private final Platform platform = PlatformMock.newInstance();
    private final PluginAdapter pluginAdapter = new PluginAdapter(new PluginMock(), "plugInMock", platform.log());
    private final PluginAdapter errorPluginAdapter = new PluginAdapter(new ErrorPluginMock(), "errorPluginMock", platform.log());

    @Test public void testActivate() {
        pluginAdapter.activate();
    }

    @Test(expected=RuntimeException.class) public void testActivate_error() {
        errorPluginAdapter.activate();
    }

    @Test public void testGetXmState() {
        loadXml(pluginAdapter.getXmlState());
        errorPluginAdapter.getXmlState();
    }

    @Test public void testTryActivate() {
        pluginAdapter.tryActivate();
        errorPluginAdapter.tryActivate();
    }

    @Test public void testTryClose() {
        pluginAdapter.tryClose();
        errorPluginAdapter.tryClose();
    }

    @Test public void testGetXmlState() {
        loadXml(pluginAdapter.getXmlState());
        loadXml(errorPluginAdapter.getXmlState());
    }

    @Test public void testToString() {
        assertThat(pluginAdapter.toString(), startsWith("Plugin "));
    }
}