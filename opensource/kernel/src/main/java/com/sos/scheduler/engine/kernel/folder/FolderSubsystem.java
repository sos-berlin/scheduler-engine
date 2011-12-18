package com.sos.scheduler.engine.kernel.folder;

import com.sos.scheduler.engine.kernel.cppproxy.Folder_subsystemC;
import com.sos.scheduler.engine.kernel.scheduler.Subsystem;

public class FolderSubsystem implements FileBasedSubsystem {
    private final Folder_subsystemC cppProxy;

    public FolderSubsystem(Folder_subsystemC cppProxy) {
        this.cppProxy = cppProxy;
    }

//    public ImmutableList<AbsolutePath> list(AbsolutePath path) {
//        return list(path, "*");
//    }

//    public ImmutableList<AbsolutePath> list(AbsolutePath path, String typeName) {
//
//    }

    /** @see {@link #updateFolders(double)}. */
    public boolean updateFolders() {
        return cppProxy.handle_folders(0);
    }

    /** @return true, wenn ein {@link FileBased} geladen worden ist. */
    public boolean updateFolders(double minimumAge) {
        return cppProxy.handle_folders(minimumAge);
    }
}
