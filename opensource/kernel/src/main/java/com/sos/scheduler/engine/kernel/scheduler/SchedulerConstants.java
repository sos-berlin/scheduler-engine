package com.sos.scheduler.engine.kernel.scheduler;

import com.google.common.base.Charsets;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.util.MavenProperties;

import java.nio.charset.Charset;

import static com.google.common.base.Charsets.ISO_8859_1;

public class SchedulerConstants {
    public static final Charset defaultEncoding = Charsets.UTF_8;
    public static final Charset schedulerEncoding = ISO_8859_1;
    public static final Charset logFileEncoding = schedulerEncoding;
    public static final MavenProperties mavenProperties = new MavenProperties(Scheduler.class);
    public static final String version = mavenProperties.getVersion();
    public static final String productName = "JobScheduler";
    public static final String productWithVersion = productName+" "+version;

    private SchedulerConstants() {}
}
