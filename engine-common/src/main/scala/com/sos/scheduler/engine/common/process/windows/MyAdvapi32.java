package com.sos.scheduler.engine.common.process.windows;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.WString;
import com.sun.jna.platform.win32.WinBase.FILETIME;
import java.util.List;
import static java.util.Arrays.asList;

/**
 * @author Joacim Zschimmer
 */
public class MyAdvapi32 {
    private MyAdvapi32() {}

    public static final int CRED_TYPE_GENERIC = 1;

    public static final class CREDENTIAL extends Structure {
        @SuppressWarnings("unused") public int flags;
        @SuppressWarnings("unused") public int typ;
        @SuppressWarnings("unused") public WString targetName;
        @SuppressWarnings("unused") public WString comment;
        @SuppressWarnings("unused") public FILETIME lastWritten;
        @SuppressWarnings("unused") public int credentialBlobSize;
        @SuppressWarnings("unused") public Pointer credentialBlob;
        @SuppressWarnings("unused") public int persist;
        @SuppressWarnings("unused") public int attributeCount;
        @SuppressWarnings("unused") public Pointer attributes;
        @SuppressWarnings("unused") public WString targetAlias;
        @SuppressWarnings("unused") public WString userName;

        public CREDENTIAL(Pointer p) {
            super(p);
        }

        @Override
        protected List<String> getFieldOrder() {
            return asList("flags", "typ", "targetName", "comment", "lastWritten",
                "credentialBlobSize", "credentialBlob", "persist", "attributeCount", "attributes", "targetAlias", "userName");
        }
    }

    public static final class ACCESS_MASK {
        private ACCESS_MASK() {}

        public static final int MAXIMUM_ALLOWED = 25;
        public static final int GENERIC_EXECUTE = 29;
    }
}
