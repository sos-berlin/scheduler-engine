package com.sos.scheduler.engine.tests.jira.js1689;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin;
import java.util.Map;
import javax.inject.Inject;
import static scala.collection.JavaConversions.mapAsJavaMap;

/**
 * @author Joacim Zschimmer
 */
public class TestPlugin extends AbstractPlugin {

    private final Scheduler scheduler;

    @Inject
    private TestPlugin(Scheduler scheduler) {
        this.scheduler = scheduler;
    }

    @Override
    public void onActivate() {
        try {
            assert(false);
            throw new RuntimeException("assert is switched off");
        } catch(AssertionError ignore) {
            System.out.println("This is the TestPlugin. assert() works.");
        }

        Map<String, String> mailDefaults = mapAsJavaMap(scheduler.mailDefaults());
        assert(mailDefaults.get("mail_on_error").equals("1"/*true*/));
        assert(mailDefaults.get("mail_on_warning").equals("1"/*true*/));
        assert(mailDefaults.get("smtp").equals("TEST-SMTP"));
      //assert(mailDefaults.get("user").equals(TEST-USER"));
      //assert(mailDefaults.get("password").equals(TEST-PASSWORD"));
        assert(mailDefaults.get("to").equals("TEST-TO"));
        assert(mailDefaults.get("from").equals("TEST-FROM"));
        assert(mailDefaults.get("from_name").startsWith("Scheduler "));
        assert(mailDefaults.get("from_name").contains(" -id=test"));
        assert(mailDefaults.get("bcc").equals("TEST-BCC"));
        assert(mailDefaults.get("cc").equals("TEST-CC"));
        assert(mailDefaults.get("subject").equals("TEST-SUBJECT"));
        assert(mailDefaults.get("queue_dir").equals("TEST-QUEUE-DIR"));
        assert(mailDefaults.get("queue_only").equals("0"));
    }
}
