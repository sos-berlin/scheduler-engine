package com.sos.scheduler.engine.kernel.util;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.concurrent.TimeUnit;

public final class Time {
    public static final Time eternal = new Time(Long.MAX_VALUE, TimeUnit.DAYS);
    
    public final long value;
    public final TimeUnit unit;

    public Time(long value, TimeUnit timeUnit) {
        this.value = value;
        this.unit = timeUnit;
    }

    public long getMillis() {
        return unit.toMillis(value);
    }

//    public long getNanosWithoutMillis() {
//        return unit.excessNanos(getMillis(), value);
//    }

    public final String toXml() {
        return new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss'Z'").format(new Date(getMillis()));
    }

    @Override public String toString() {
        return (unit.toMillis(value) / 1000.0) + "s";
    }


    public static Time of(long value, TimeUnit u) {
        return new Time(value, u);
    }

    public static Time of(double seconds) {
        return new Time((long)(seconds * 1000000000), TimeUnit.NANOSECONDS);
    }

    public static Time of(Date o) {
        return new Time(o.getTime(), TimeUnit.MILLISECONDS);
    }
    //public static Time of(Date d) { return ofMillis(d.getTime()); }

    public static Time ofMillis(long value) {
        return new Time(value, TimeUnit.MILLISECONDS);
    }
}
