package com.sos.scheduler.engine.kernel.folder;

import com.sos.scheduler.engine.kernel.Subsystem;
import com.sos.scheduler.engine.kernel.cppproxy.Folder_subsystemC;

public class FolderSubsystem implements Subsystem {
    private final Folder_subsystemC cppProxy;

    public FolderSubsystem(Folder_subsystemC cppProxy) {
        this.cppProxy = cppProxy;
    }

    /** @return true, wenn ein {@link FileBased} geladen worden ist. */
    public boolean updateFolders(double minimumAge) {
        return cppProxy.handle_folders(minimumAge);
    }
}
