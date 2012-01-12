package com.sos.scheduler.engine.test.util;

public class FileAlreadyLockedException extends RuntimeException {

	public FileAlreadyLockedException(String message,Exception e) {
		super(message,e);
	}
}
