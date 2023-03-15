import express from "express";
import http from "http";
import https from "https";
import fs from "fs";
import path from "path";
import net from "net";
import { ILogData } from "uclogger";
import { theLogger } from "../globals";
const router = express.Router();

// Routes add themselves to the router.
// The route needs to fullfill the EModule interface declaration
// If you need additional routes just add them to the routes directory and everything will go its way
// You can easily add and remove routes also in different environments with the existance of the appropriate route file

export interface IEModule {
	// Every Module that wants to be added to the express routing needs to expose this Init Method
	// It is called while the module is loaded
	init(router: express.Router): void;

	// If the module needs to know the webservice object this handler is called
	// e.g. to setup the upgrade callback
	setServer?(server: http.Server | https.Server): void;

	// Checks if this module wants to handle the onUpgrade request (returns true then)
	onUpgrade?(request: express.Request, socket: net.Socket, head: Buffer): boolean;
}

/**
 * The router that initializes the routes under /routes
 */
class ERouter {
	// The list of modules as loaded with getRoutes
	private static modules: IEModule[] = [];

	/**
	 * The Loggers getLogData callback (used in all the log methods called in this class, add the classname to every log entry)
	 *
	 * @returns - an ILogData log data object provided additional data for all the logger calls in this class
	 */
	public static getLogData(): ILogData {
		return {
			className: "ERouter"
		};
	}

	/**
	 * Fetches the available Routes to add them to the express routing table
	 *
	 * @returns - the router which is a static in this scope to be handed over to the express app.use
	 */
	public static getRoutes(): express.Router {
		this.getFiles().forEach((file: string) => {
			// eslint-disable-next-line @typescript-eslint/no-var-requires
			const mod = require("./routes/" + file) as IEModule;
			if (mod) {
				if (mod.init)
					mod.init(router);
				this.modules.push(mod);
			}
		});
		return router;
	}

	/**
	 * Allows to handle server related stuff in the Module (e.g. upgrade to websocket)
	 *
	 * @param server - the express server object to add the upgrade callback to
	 */
	public static setServer(server: http.Server | https.Server): void {
		server.on("upgrade", (request: express.Request, socket: net.Socket, head: Buffer) => {
			try {
				for (const module of this.modules) {
					if (module.onUpgrade && module.onUpgrade(request, socket, head))
						break;
				}
			} catch (error) {
				theLogger.error("Upgrading connection failed", "setServer", this, { request }, error);
			}
		});
	}

	/**
	 * Retrieves the available modules in the same path
	 *
	 * @returns - the list of modules in the path
	 */
	private static getFiles(): string[] {
		const routes = new Array<string>(0);
		const normalizedPath = path.join(__dirname, "routes");
		fs.readdirSync(normalizedPath).forEach((file: string) => {
			const ext = path.extname(file).toLowerCase();
			if (ext === ".js")
				routes.push(file);
		});
		return routes;
	}
}

export { ERouter };
