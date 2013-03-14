package com.sos.scheduler.engine.test;

import com.google.common.base.Predicate;
import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.data.log.ErrorLogEvent;
import com.sos.scheduler.engine.kernel.util.ResourcePath;
import com.sos.scheduler.engine.test.binary.CppBinariesDebugMode;

import javax.annotation.Nullable;

public final class TestSchedulerControllerBuilder {
    private static final Predicate<ErrorLogEvent> defaultExpectedErrorLogEventPredicate = new Predicate<ErrorLogEvent>() {
        @Override public boolean apply(@Nullable ErrorLogEvent o) { return false; }
    };

    private final Class<?> testClass;
    private ResourcePath resourcePath;
    private Predicate<ErrorLogEvent> expectedErrorLogEventPredicate = defaultExpectedErrorLogEventPredicate;
    private CppBinariesDebugMode debugMode = CppBinariesDebugMode.debug;
    private String logCategories;
    @Nullable private ImmutableMap<String,String> nameMap = null;
    @Nullable private ResourceToFileTransformer fileTransformer = null;

    public TestSchedulerControllerBuilder(Class<?> testClass) {
        this.testClass = testClass;
        resourcePath = new ResourcePath(testClass.getPackage());
    }

    public TestSchedulerControllerBuilder resourcesPackage(Package p) {
        resourcePath = new ResourcePath(p);
        return this;
    }

    public TestSchedulerControllerBuilder nameMap(@Nullable ImmutableMap<String,String> o) {
        nameMap = o;
        return this;
    }

    public TestSchedulerControllerBuilder resourceToFileTransformer(@Nullable ResourceToFileTransformer o) {
        fileTransformer = o;
        return this;
    }

    public TestSchedulerControllerBuilder expectedErrorLogEventPredicate(Predicate<ErrorLogEvent> p) {
        expectedErrorLogEventPredicate = p;
        return this;
    }

    public TestSchedulerControllerBuilder debugMode(CppBinariesDebugMode debugMode) {
        this.debugMode = debugMode;
        return this;
    }

    public TestSchedulerControllerBuilder logCategories(String o) {
        this.logCategories = o;
        return this;
    }

    public TestSchedulerController build() {
        TestSchedulerController r = new TestSchedulerController(testClass, resourcePath, nameMap, fileTransformer, expectedErrorLogEventPredicate, debugMode);
        r.setLogCategories(logCategories);
        return r;
    }
}
