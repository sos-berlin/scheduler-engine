package com.sos.scheduler.engine.util;

import org.slf4j.bridge.SLF4JBridgeHandler;

public final class LoggingFunctions {
    public static void enableJavaUtilLoggingOverSLF4J() {
        disableJavaUtilLogging();
        SLF4JBridgeHandler.install();
    }

    public static void disableJavaUtilLogging() {
        java.util.logging.Logger rootLogger = java.util.logging.LogManager.getLogManager().getLogger("");
        for (java.util.logging.Handler h: rootLogger.getHandlers())
            rootLogger.removeHandler(h);
    }

    private LoggingFunctions() {}
}
