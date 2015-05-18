package com.sos.scheduler.engine.tests.scheduler.comapi.java;

import java.util.function.Consumer;
import sos.spooler.Job_impl;

/**
 * @author Andreas Liebert
 */
public final class MeasureApiTimeJob extends Job_impl {
    @Override
    public boolean spooler_process() throws Exception {
        DurationMeasurement infoDuration = measureTotal(20,
                x -> spooler_log.info("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"));
        DurationMeasurement debugDuration = measureTotal(20,
                x -> spooler_log.debug9("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"));

        spooler_log.info("spooler_log.info total_duration: " + infoDuration.totalAsMilliseconds() + "ms");
        spooler_log.info("spooler_log.info average_duration: " + infoDuration.averageAsMilliseconds() + "ms");
        spooler_log.info("spooler_log.debug9 total_duration: " + debugDuration.totalAsMilliseconds() + "ms");
        spooler_log.info("spooler_log.debug9 average_duration: " + debugDuration.averageAsMilliseconds() + "ms");
        return super.spooler_process();
    }

    private static DurationMeasurement measureTotal(int repeat, Consumer<String> consumer) {
        long duration = 0;
        for (int i = 0; i < repeat; i++) {
            duration += measureSingle(consumer);
        }
        return new DurationMeasurement(duration, duration / repeat);
    }

    private static long measureSingle(Consumer<String> consumer) {
        long startSingle = System.nanoTime();
        consumer.accept("");
        long endSingle = System.nanoTime();
        return endSingle - startSingle;
    }

    private static class DurationMeasurement {
        private final long totalDuration;
        private final long averageDuration;

        private static final int nano2msDivisor = 1000000;

        private DurationMeasurement(long total, long average) {
            totalDuration = total;
            averageDuration = average;
        }

        private long totalAsMilliseconds() {
            return totalDuration / nano2msDivisor;
        }

        private long averageAsMilliseconds() {
            return averageDuration / nano2msDivisor;
        }
    }
}
