package com.sos.scheduler.engine.kernel.plugin;

import com.google.inject.Injector;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.scheduler.PrefixLogMock;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import static com.google.inject.Guice.createInjector;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.loadXml;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;

public final class PluginAdapterTest {
    private static final Logger logger = LoggerFactory.getLogger(PluginAdapterTest.class);
    private static final PluginConfiguration pluginConfiguration = new PluginConfiguration(PluginMock.class.getName(), ActivationMode.activateOnStart, null);
    private static final PluginConfiguration errorPluginConfiguration = new PluginConfiguration(ErrorPluginMock.class.getName(), ActivationMode.activateOnStart, null);

    private final PrefixLog log = PrefixLogMock.newLog(logger);
    private final PluginAdapter pluginAdapter = new PluginAdapter(pluginConfiguration, log);
    private final PluginAdapter errorPluginAdapter = new PluginAdapter(errorPluginConfiguration, log);
    private final Injector injector = createInjector();

    @Test public void testActivate() {
        pluginAdapter.activate(injector);
    }

    @Test(expected=RuntimeException.class) public void testActivate_error() {
        errorPluginAdapter.activate(injector);
    }

    @Test public void testGetXmState() {
        loadXml(pluginAdapter.getXmlState());
        errorPluginAdapter.getXmlState();
    }

    @Test public void testTryActivate() {
        pluginAdapter.tryActivate(injector);
        errorPluginAdapter.tryActivate(injector);
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