package com.sos.scheduler.engine.kernel.plugin;

import com.google.inject.Injector;
import com.sos.scheduler.engine.kernel.scheduler.Platform;
import com.sos.scheduler.engine.kernel.scheduler.PlatformMock;
import org.junit.Test;

import static com.google.inject.Guice.createInjector;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.loadXml;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;

public final class PluginAdapterTest {
    private static final PluginConfiguration pluginConfiguration = new PluginConfiguration(PluginMock.class.getName(), ActivationMode.activateOnStart, null);
    private static final PluginConfiguration errorPluginConfiguration = new PluginConfiguration(ErrorPluginMock.class.getName(), ActivationMode.activateOnStart, null);

    private final Platform platform = PlatformMock.newInstance();
    private final PluginAdapter pluginAdapter = new PluginAdapter(pluginConfiguration, platform.log());
    private final PluginAdapter errorPluginAdapter = new PluginAdapter(errorPluginConfiguration, platform.log());
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