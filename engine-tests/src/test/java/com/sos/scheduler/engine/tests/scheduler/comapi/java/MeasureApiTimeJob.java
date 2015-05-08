package com.sos.scheduler.engine.tests.scheduler.comapi.java;

import sos.spooler.Job_impl;

import java.util.function.Consumer;

/**
 * Created by Andreas Liebert on 08.05.2015.
 */
public class MeasureApiTimeJob extends Job_impl{
    @Override
    public boolean spooler_process() throws Exception {
        DurationMeasurement infoDuration = measureTotal(20,
                x -> spooler_log.info("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"));
        DurationMeasurement debugDuration = measureTotal(20,
                x -> spooler_log.debug9("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"));

        spooler_log.info("spooler_log.info total_duration: "+infoDuration.totalDuration/100000+"ms");
        spooler_log.info("spooler_log.info average_duration: "+infoDuration.averageDuration/100000+"ms");
        spooler_log.info("spooler_log.debug9 total_duration: "+debugDuration.totalDuration/100000+"ms");
        spooler_log.info("spooler_log.debug9 average_duration: "+debugDuration.averageDuration/100000+"ms");
        return super.spooler_process();
    }


    private long measureSingle(Consumer<String> consumer){
        long startSingle = System.nanoTime();
        consumer.accept("");
        long endSingle = System.nanoTime();
        long singleDuration = endSingle-startSingle;
        return singleDuration;
    }

    private DurationMeasurement measureTotal(int repeat, Consumer<String> consumer){
        long duration = 0;
        for (int i=0; i<repeat; i++){
             duration += measureSingle(consumer);
        }
        return new DurationMeasurement(duration, duration/repeat);
    }


    private class DurationMeasurement {
        long totalDuration;
        long averageDuration;

        public DurationMeasurement(long total, long average){
            totalDuration = total;
            averageDuration = average;
        }
    }
}
