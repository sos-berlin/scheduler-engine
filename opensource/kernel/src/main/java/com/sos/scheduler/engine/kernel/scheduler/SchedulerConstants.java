package com.sos.scheduler.engine.kernel.scheduler;

import com.google.common.base.Charsets;
import org.joda.time.DateTimeZone;

import java.nio.charset.Charset;

import static com.google.common.base.Charsets.ISO_8859_1;
import static org.joda.time.DateTimeZone.UTC;

public final class SchedulerConstants {
    public static final Charset defaultEncoding = Charsets.UTF_8;
    public static final Charset schedulerEncoding = ISO_8859_1;
    public static final Charset logFileEncoding = schedulerEncoding;
    public static final DateTimeZone schedulerTimeZone = UTC;

    private SchedulerConstants() {}
}
