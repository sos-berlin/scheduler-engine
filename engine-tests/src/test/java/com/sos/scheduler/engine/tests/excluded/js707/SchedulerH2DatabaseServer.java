package com.sos.scheduler.engine.tests.excluded.js707;

import static com.google.common.base.Throwables.propagate;

import java.sql.SQLException;

import org.h2.tools.Server;

import com.sos.scheduler.engine.kernel.util.Hostware;

public class SchedulerH2DatabaseServer {
    private final Server server;
    private final int tcpPort;

    private SchedulerH2DatabaseServer(Server server, int tcpPort) {
        this.server = server;
        this.tcpPort = tcpPort;
    }

    public void start() {
        try {
            server.start();
        } catch (SQLException x) { throw propagate(x); }
    }

    public void stop() {
        server.stop();
    }

    public String hostwarePath() {
        return "jdbc -class=org.h2.Driver " + Hostware.quoted("jdbc:h2:tcp://localhost:"+tcpPort+"/mem:schedulerTest");
    }

    public static SchedulerH2DatabaseServer newTcpServer(int tcpPort) {
        try {
            Server server = Server.createTcpServer(
                    //"-baseDir", directory.toString(),
                    "-tcpPort", Integer.toString(tcpPort),
                    "-tcpAllowOthers");
            return new SchedulerH2DatabaseServer(server, tcpPort);
        } catch (SQLException x) { throw propagate(x); }
    }
}
