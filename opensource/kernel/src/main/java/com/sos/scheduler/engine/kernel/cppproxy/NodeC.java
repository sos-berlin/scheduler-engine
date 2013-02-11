package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.order.jobchain.Node;
import com.sos.scheduler.engine.kernel.scheduler.HasInjector;

@CppClass(clas="sos::scheduler::order::job_chain::Node", directory="scheduler", include="spooler.h")
public interface NodeC extends NodeCI, CppProxyWithSister<Node> {
    SisterType <Node, NodeC> sisterType = new SisterType<Node, NodeC>() {
        @Override public Node sister(NodeC proxy, Sister context) {
            NodeCI nodeCI = proxy;
            return nodeCI instanceof Order_queue_nodeC?
                    Order_queue_nodeC.sisterType.sister((Order_queue_nodeC)nodeCI, context)
                    : new Node(proxy, ((HasInjector)context).injector());
        }
    };
}
