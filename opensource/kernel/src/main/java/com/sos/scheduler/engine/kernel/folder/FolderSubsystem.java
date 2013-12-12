package com.sos.scheduler.engine.kernel.folder;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.data.folder.AbsolutePath;
import com.sos.scheduler.engine.kernel.cppproxy.Folder_subsystemC;

public final class FolderSubsystem implements FileBasedSubsystem {
    private final Folder_subsystemC cppProxy;

    public FolderSubsystem(Folder_subsystemC cppProxy) {
        this.cppProxy = cppProxy;
    }

    public ImmutableList<String> names(AbsolutePath path, String typeName) {
        String cppTypeName = typeName.replaceAll("([A-Z])", "_$1").toLowerCase();  // "jobChain" -> "job_chain"
        return ImmutableList.copyOf(cppProxy.java_names(path.string(), cppTypeName));
    }

    /** @return true, wenn ein {@link FileBased} geladen worden ist. */
    public boolean updateFolders() {
        return cppProxy.update_folders_now();
    }

//    /** @return true, wenn ein {@link FileBased} geladen worden ist. */
//    public boolean updateFolders(double minimumAge) {
//        return cppProxy.handle_folders(minimumAge);
//    }
}
