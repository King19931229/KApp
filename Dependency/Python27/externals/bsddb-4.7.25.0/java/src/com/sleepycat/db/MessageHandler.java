/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997,2008 Oracle.  All rights reserved.
 *
 * $Id: MessageHandler.java 63573 2008-05-23 21:43:21Z trent.nelson $
 */
package com.sleepycat.db;

/**
An interface specifying a callback function to be called to display
informational messages.
*/
public interface MessageHandler {
    /**
    A callback function to be called to display informational messages.
    <p>
    There are interfaces in the Berkeley DB library which either directly
    output informational messages or statistical information, or configure
    the library to output such messages when performing other operations,
    {@link com.sleepycat.db.EnvironmentConfig#setVerboseDeadlock EnvironmentConfig.setVerboseDeadlock} for example.
    <p>
    The {@link com.sleepycat.db.EnvironmentConfig#setMessageHandler EnvironmentConfig.setMessageHandler} and
    {@link com.sleepycat.db.DatabaseConfig#setMessageHandler DatabaseConfig.setMessageHandler} methods are used to
    display these messages for the application.
    <p>                 
    @param environment  
    The enclosing database environment handle.
    <p>
    @param message
    An informational message string.
    */
    void message(Environment environment, String message);
}
