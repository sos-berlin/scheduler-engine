package com.sos.scheduler.engine.kernel.util;

import java.util.ArrayList;
import java.util.List;

/** Misst die Last (volle Last: 1, Leerlauf: 0) über eine Anzahl Perioden.
 * - meter(1, now): Volllast beginnt
 * - meter(0, now): Volllast endet
 * Abgefragt werden kann dann die Last der letzten Perioden.
 *
 * Etwa so:
 *
    private LoadMeter loadMeter = new LoadMeter(10*1000, 6);    // 10s, 6 mal, also eine Minute

    public void onWaitBegin() {
        try {
            float load = loadMeter.getLoad();
            if (load > loadWarnLevel)  Nicht jedesmal die Meldung ausgeben. In PlugIn oder sowas auslagern, mit Abonnement.
                log().info("Load is " + above " + (int)(100 * loadWarnLevel) + "%");

            loadMeter.meter(0, new Date().getTime());
        } catch (Exception x) { log().warn("onWaitBegin: " + x); }
    }


    public void onWaitEnd() {
        try {
            loadMeter.meter(1, new Date().getTime());
        } catch (Exception x) { log().warn("onWaitEnd: " + x); }
    }
 *
 * @author Joacim Zschimmer
 */
public class LoadMeter {
    private static final int defaultNumberOfPeriods = 10;
    
    // Alle Zeiten in long, Millisekunden, kann aber jede andere Einheit sein. Nur toString() nimmt Millisekunden an.
    private final long periodDuration;
    private final int maxNumberOfPeriods;
    private boolean initialized = false;
    private long lastMeterTime;
    private long periodIndex;   // == clock / periodDuration
    private long rateDurationAccumulator = 0;      // load(0...1) * duration, Integral der Last über die aktuelle Periode
    private final History history;


    public LoadMeter(long periodDuration, int numberOfPeriods) {
        this.maxNumberOfPeriods = numberOfPeriods;
        this.periodDuration = periodDuration;
        this.history = new History();
    }


    public LoadMeter(long period) {
        this(period, defaultNumberOfPeriods);
    }


    public final void meter(int load, long now) {
        if (!(load == 0 || load == 1))  throw new IllegalArgumentException("not (load==0 || load==1)");
        long newPeriodIndex = now / periodDuration;

        if (!initialized) initialize(now);
        
        if (periodIndex == newPeriodIndex)
            accumulate(load, now - lastMeterTime);
        else
        if (periodIndex < newPeriodIndex) {
            finishPeriodAndSetHistory(load);    // Rest der noch aktuellen Period periodIndex
            history.fill(periodIndex + 1, newPeriodIndex, load);    // Ganze Perioden bis now
            accumulateForNewPeriod(load, now);    // Neue Periode
            periodIndex = newPeriodIndex;
        }
        lastMeterTime = now;
    }


    private void initialize(long now) {
        lastMeterTime = now;
        periodIndex = lastMeterTime / periodDuration;
        initialized = true;
    }


    private void finishPeriodAndSetHistory(int load) {
        long periodEndTime = (periodIndex + 1) * periodDuration;
        accumulate(load, periodEndTime - lastMeterTime);
        history.set(periodIndex, (float)rateDurationAccumulator / periodDuration);
        rateDurationAccumulator = 0;
    }

    
    private void accumulateForNewPeriod(int load, long nowMs) {
        long periodStart = nowMs / periodDuration * periodDuration;
        accumulate(load, nowMs - periodStart);
    }
    

    private void accumulate(int load, long durationMs) {
        rateDurationAccumulator += load * durationMs;
    }


    public final float getLoad() {
        return loadOfLastPeriods(maxNumberOfPeriods);
    }


    public final float loadOfLastPeriods(int n) {
        if (n <= 0 || n > maxNumberOfPeriods)  throw new IllegalArgumentException();
        float a = 0;
        for (int i = 0; i < n; i++)  a += history.get(periodIndex - 1 - i);
        return a / n;
    }


    public final int getNumberOfPeriods() {
        return maxNumberOfPeriods;
    }
    
    public final ArrayList<Float> getHistory() {
        ArrayList<Float> result = new ArrayList<Float>(maxNumberOfPeriods);
        for (int i = 0; i < maxNumberOfPeriods; i++)  result.add(history.get(periodIndex - i - 1));
        return result;
    }


    @Override public String toString() {
        return "average load last " + stringFromMillis(periodDuration) + ": " + stringPercent(loadOfLastPeriods(1)) +
            ", last " + stringFromMillis(periodDuration * maxNumberOfPeriods) +": " + stringPercent(loadOfLastPeriods(maxNumberOfPeriods));
    }


    private String stringFromMillis(long millis) {
        return (millis / 1000.0) + "s";
    }


    private String stringPercent(float a) {
        return (int)(100 * a) + "%";
    }

    
    private class History {
        private List<Float> periodLoads = new ArrayList<Float>(maxNumberOfPeriods);

        History() {
            for (int i = 0; i < maxNumberOfPeriods; i++)  periodLoads.add((float)0);
        }

        void fill(long begin, long end, float load) {
            if (end - begin < maxNumberOfPeriods) for (long i = begin; i < end; i++) set(i, load);
            else setAll(load);
        }

        void set(long number, float load) {
            periodLoads.set(index(number), load);
        }

        void setAll(float load) {
            for (int i = 0; i < maxNumberOfPeriods; i++)  periodLoads.set(i, load);
        }

        float get(long number) {
            return periodLoads.get(index(number));
        }

        private int index(long periodIndex) {
            return (int)mathModulo(periodIndex, maxNumberOfPeriods);
        }
    }

    private static long mathModulo(long a, long b) {
        long result = a % b;
        return result < 0? result + b : result;
    }
}
