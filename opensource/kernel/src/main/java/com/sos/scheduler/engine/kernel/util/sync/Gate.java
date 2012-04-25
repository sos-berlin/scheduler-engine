package com.sos.scheduler.engine.kernel.util.sync;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nullable;

import com.google.common.base.Joiner;
import com.sos.scheduler.engine.kernel.util.Time;

public class Gate<T> {
    private final BlockingQueue<T> queue = new ArrayBlockingQueue<T>(1);
    @Nullable private final String name;

    public Gate() {
        this.name = null;
    }

    public Gate(String name) {
        this.name = name;
    }


    public void put(T o) throws InterruptedException {
        queue.put(o);
    }

    public boolean offer(T o)  {
        return queue.offer(o);
    }

    public void expect(T o, Time t) throws InterruptedException {
        T result = tryPoll(t);
        if (result == null)  throw new RuntimeException(this +": Expected message '"+o+"' has not arrived within "+t);
        if (!result.equals(o))  throw new RuntimeException(this +": Message '"+o+"' has been expected, but '"+result+"' arrived");
    }

    public T poll(Time t) throws InterruptedException {
        T result = tryPoll(t);
        if (result == null)  throw new RuntimeException(this +": Expected message has not arrived within "+t);
        return result;
    }

    @Nullable private T tryPoll(Time t) throws InterruptedException {
        return queue.poll(t.getMillis(), TimeUnit.MILLISECONDS);
    }

    @Override public String toString() {
        return Joiner.on(" ").skipNulls().join("Gate", name);
    }
}
