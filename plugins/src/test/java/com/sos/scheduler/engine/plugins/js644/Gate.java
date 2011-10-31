package com.sos.scheduler.engine.plugins.js644;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nullable;

import com.sos.scheduler.engine.kernel.util.Time;

public class Gate<T> {
    private final BlockingQueue<T> queue = new ArrayBlockingQueue<T>(1);

    public void put(T o) throws InterruptedException {
        queue.put(o);
    }

    public void expect(T o, Time t) throws InterruptedException {
        T result = tryPoll(t);
        if (result == null)  throw new RuntimeException("Expected message '"+o+"' has not arrived within "+t);
        if (!result.equals(o))  throw new RuntimeException("Message '"+o+"' has been expected, but '"+result+"' arrived");
    }

    public T poll(Time t) throws InterruptedException {
        T result = tryPoll(t);
        if (result == null)  throw new RuntimeException("Expected message has not arrived within "+t);
        return result;
    }

    @Nullable private T tryPoll(Time t) throws InterruptedException {
        return queue.poll(t.getMillis(), TimeUnit.MILLISECONDS);
    }
}
