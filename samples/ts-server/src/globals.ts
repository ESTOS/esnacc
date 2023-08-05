// This file is used for the global available singletons which will otherwise produce circular dependencies.
// Add any global singleton to this file and import them when needed using this file

// Please do not forget to check for circular dependencies every now and then using
// npm run check

import { Config, IConfig } from "./singletons/config";
import { ClientConnectionManager } from "./singletons/clientConnectionManager";
import { Server } from "./singletons/server";
import { ELogger } from "uclogger";
import { LogLocalStorage } from "./singletons/asyncLocalStorage";

export const theConfig: IConfig = new Config().config;
export const theClientConnectionManager = ClientConnectionManager.getInstance();
export const theServer = Server.getInstance();
export const theLogger = new ELogger();
export const theLogStorage = LogLocalStorage.getInstance();

theServer.setClientConnectionHandler(theClientConnectionManager);
theServer.setLogger(theLogger);
