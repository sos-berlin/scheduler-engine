package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.command.Result;
import com.sos.scheduler.engine.kernel.command.ResultXmlizer;
import org.w3c.dom.Element;


public class PlugInCommandResultXmlizer implements ResultXmlizer {
    @Override public Class<? extends Result> getResultClass() {
        return PlugInCommandResult.class;
    }

    @Override public Element toElement(Result result) {
        PlugInCommandResult r = (PlugInCommandResult)result;
        return r.getElement();
    }
}
