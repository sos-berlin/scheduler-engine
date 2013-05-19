package com.sos.scheduler.engine.test;

import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.data.log.ErrorLogEvent;
import com.sos.scheduler.engine.kernel.util.ResourcePath;
import com.sos.scheduler.engine.test.binary.CppBinariesDebugMode;
import scala.Function1;
import scala.runtime.AbstractFunction1;

import javax.annotation.Nullable;

public final class TestConfigurationBuilder {
    private static final Function1<ErrorLogEvent, Boolean> defaultExpectedErrorLogEventPredicate = new AbstractFunction1<ErrorLogEvent, Boolean>() {
        @Override public Boolean apply(@Nullable ErrorLogEvent o) { return false; }
    };

    private final Class<?> testClass;
    private ResourcePath resourcePath;
    private Function1<ErrorLogEvent, Boolean> expectedErrorLogEventPredicate = defaultExpectedErrorLogEventPredicate;
    private CppBinariesDebugMode debugMode = CppBinariesDebugMode.debug;
    private String logCategories = System.getProperty("scheduler.logCategories");
    private boolean useDatabase = false;
    @Nullable private ImmutableMap<String,String> nameMap = null;
    @Nullable private ResourceToFileTransformer fileTransformer = null;

    public TestConfigurationBuilder(Class<?> testClass) {
        this.testClass = testClass;
        resourcePath = new ResourcePath(testClass.getPackage());
    }

    public TestConfigurationBuilder resourcesPackage(Package p) {
        resourcePath = new ResourcePath(p);
        return this;
    }

    public TestConfigurationBuilder nameMap(@Nullable ImmutableMap<String,String> o) {
        nameMap = o;
        return this;
    }

    public TestConfigurationBuilder resourceToFileTransformer(@Nullable ResourceToFileTransformer o) {
        fileTransformer = o;
        return this;
    }

    public TestConfigurationBuilder debugMode(CppBinariesDebugMode debugMode) {
        this.debugMode = debugMode;
        return this;
    }

    public TestConfigurationBuilder logCategories(String o) {
        this.logCategories = o;
        return this;
    }

    public TestConfigurationBuilder useDatabase(Boolean o) {
        this.useDatabase = o;
        return this;
    }

//    public TestConfiguration build() {
//        return new TestConfiguration(testClass, new Some(resourcePath), binariesDebugMode, logCategories,
//                useDatabase, expectedErrorLogEventPredicate,
//                nameMap, fileTransformer);
//    }
}
