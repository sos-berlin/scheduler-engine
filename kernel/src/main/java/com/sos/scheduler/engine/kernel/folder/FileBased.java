package com.sos.scheduler.engine.kernel.folder;

import com.sos.scheduler.engine.kernel.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.kernel.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.kernel.cplusplus.runtime.Sister;
import com.sos.scheduler.kernel.cplusplus.runtime.annotation.ForCpp;


@ForCpp
abstract public class FileBased<T extends FileBased<T,CPPPROXY>, CPPPROXY extends CppProxyWithSister<T>> 
  extends AbstractHasPlatform implements Sister
{
    private final CPPPROXY cppProxy;


    protected FileBased(Platform p, CPPPROXY proxy) {
        super(p);
        cppProxy = proxy;
    }
}
