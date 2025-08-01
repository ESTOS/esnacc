// loads environment variables from a .env file into process.env
import * as dotenv from "dotenv";
import { IELoggerSettings } from "uclogger";

import expressInit from "./express/expressInit.js";
import { theClientConnectionManager, theConfig, theLogger, theLogStorage, theServer } from "./globals.js";
import { setDecoratorLogger } from "./lib/loggerdecorators.js";

dotenv.config();

const eLoggerSettings: IELoggerSettings = {
	fileLog: { logFilename: "microservicetemplate.log", logDirectory: theConfig.logDirectory },
	consoleLog: { logConsole: true },
	infrastructure: { environment: "dev", servername: "localhost", role: "microservicetemplate", role_instance: 0 },
	logLevel: "debug",
};

theLogger.init(eLoggerSettings);
setDecoratorLogger(theLogger);
theLogStorage.init();

theClientConnectionManager.init();
theServer.setClientConnectionHandler(theClientConnectionManager);
theServer.setLogger(theLogger);
theServer.init();

void expressInit();

theLogger.info("Server initialised", "init", undefined, { serverName: theConfig.econfServerFDQN });
