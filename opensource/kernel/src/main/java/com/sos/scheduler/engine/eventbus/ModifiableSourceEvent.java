package com.sos.scheduler.engine.eventbus;

/** Markiert ein {@link Event}, dass dessen {@link EventSource} unter bestimmten Bedingungen
 * von einen @{@link HotEventHandler} verändert werden darf. */
public interface ModifiableSourceEvent {}
