package com.sos.scheduler.engine.tests.jira.js655;

import sos.spooler.Job_impl;
import sos.spooler.Web_service_operation;
import sos.spooler.Web_service_response;

public class JavaA extends Job_impl {
    @Override
    public final boolean spooler_process() {
        Web_service_operation op = spooler_task.order().web_service_operation();
        //String request_string = new String( request.binary_content(), request.charset_name() );
        Web_service_response response = op.response();
        response.set_content_type("text/plain");
        response.set_charset_name("ISO-8859-1");
        response.set_string_content("Bye!");
        response.send();
        return true;
    }
}
