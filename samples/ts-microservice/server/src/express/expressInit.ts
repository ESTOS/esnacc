import cors from "cors";
import express from "express";
import fs from "fs";
import http from "http";
import https from "https";
import { ILogData } from "uclogger";

import { ERouter } from "./expressRouter";
import { theConfig, theLogger } from "../globals";
import { CONF_EXIT_CODES } from "../lib/exithandler";

const expressInit = async (): Promise<boolean> => {
	const app = express();

	app.use(express.raw());
	app.use(express.json());
	app.use(express.urlencoded({ extended: false }));
	app.use(cors());
	app.use("/", ERouter.getRoutes());

	const promises: Promise<boolean>[] = [];
	const global: ILogData = { className: "global" };
	let listening = false;

	if (theConfig.econfServerListenPortTLS && theConfig.econfServerCertfile && theConfig.econfServerKeyfile) {
		const certFile = theConfig.econfServerCertfile;
		const keyFile = theConfig.econfServerKeyfile;
		const prom: Promise<boolean> = new Promise((resolve, reject): void => {
			const options = {
				cert: fs.readFileSync(certFile),
				key: fs.readFileSync(keyFile)
			};
			const server = https.createServer(options, app);
			server.on("error", (e) => {
				theLogger.error("failed to create https server", "expressInit", global, undefined, e);
				reject(e);
			});
			ERouter.setServer(server);
			server.listen(theConfig.econfServerListenPortTLS, theConfig.econfServerListenIP, (): void => {
				theLogger.info(`Server is running https://${theConfig.econfServerListenIP}:${theConfig.econfServerListenPortTLS}...`, "expressInit", global);
				listening = true;
				resolve(true);
			});
		});
		promises.push(prom);
	}
	if (theConfig.econfServerListenPortTCP) {
		const prom: Promise<boolean> = new Promise((resolve, reject): void => {
			const server = http.createServer(app);
			server.on("error", (e) => {
				theLogger.error("failed to create http server", "expressInit", global, undefined, e);
				reject(e);
			});
			ERouter.setServer(server);
			server.listen(theConfig.econfServerListenPortTCP, theConfig.econfServerListenIP, (): void => {
				theLogger.info(`Server is running http://${theConfig.econfServerListenIP}:${theConfig.econfServerListenPortTCP}...`, "expressInit", global);
				listening = true;
				resolve(true);
			});
		});
		promises.push(prom);
	}

	await Promise.all(promises);
	if (!listening) {
		theLogger.error("failed to listen TCP or TLS", "expressInit", global);
		process.exit(CONF_EXIT_CODES.EXIT_CODE_NOT_LISTENING);
	}

	return true;
};

export = expressInit;
