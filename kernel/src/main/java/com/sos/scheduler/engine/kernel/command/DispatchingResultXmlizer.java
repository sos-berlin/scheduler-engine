package com.sos.scheduler.engine.kernel.command;

import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.kernel.SchedulerException;
import org.w3c.dom.Element;


class DispatchingResultXmlizer implements ResultXmlizer {
    private final ImmutableMap<Class<?>,ResultXmlizer> resultXmlizers;


    DispatchingResultXmlizer(Iterable<ResultXmlizer> xmlizers) {
        resultXmlizers = mapOfXmlizers(xmlizers);
    }


    private static ImmutableMap<Class<?>,ResultXmlizer> mapOfXmlizers(Iterable<ResultXmlizer> rx) {
        ImmutableMap.Builder<Class<?>,ResultXmlizer> b = new ImmutableMap.Builder<Class<?>,ResultXmlizer>();
        for (ResultXmlizer r: rx) b.put(r.getResultClass(), r);
        return b.build();
    }


    @Override public Class<? extends Result> getResultClass() {
        return Result.class;   // Dummy
    }


    @Override public Element toElement(Result r) {
        return resultXmlizer(r.getClass()).toElement(r);
    }


    private ResultXmlizer resultXmlizer(Class<?> clas) {
        ResultXmlizer result = resultXmlizers.get(clas);
        if (result == null)  throw new SchedulerException("No " + ResultXmlizer.class.getName() + " for " + clas.getName());
        return result;
    }
}
