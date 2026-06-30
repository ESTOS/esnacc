// This file is used for the global available singletons which will otherwise produce circular dependencies.
// Add any global singleton to this file and import them when needed using this file

// Please do not forget to check for circular dependencies every now and then using
// npm run check

import { ELogger } from "uclogger";

import { LogLocalStorage } from "./singletons/asyncLocalStorage.js";
import { ClientConnectionManager } from "./singletons/clientConnectionManager.js";
import { Config, IConfig } from "./singletons/config.js";
import { Server } from "./singletons/server.js";

export const theConfig: IConfig = new Config().config;
export const theClientConnectionManager = ClientConnectionManager.getInstance();
export const theServer = Server.getInstance();
export const theLogger = new ELogger();
export const theLogStorage = LogLocalStorage.getInstance();

theServer.setClientConnectionHandler(theClientConnectionManager);
theServer.setLogger(theLogger);
