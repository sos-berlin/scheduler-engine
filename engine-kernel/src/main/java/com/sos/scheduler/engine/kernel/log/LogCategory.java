package com.sos.scheduler.engine.kernel.log;

import com.sos.scheduler.engine.base.generic.StringValue;


public class LogCategory extends StringValue {
    public static final LogCategory scheduler = new LogCategory("scheduler");

    public LogCategory(String x) { super(x); }
}
