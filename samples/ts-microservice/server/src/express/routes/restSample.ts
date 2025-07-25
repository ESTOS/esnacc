import express from "express";
import { ILogData } from "uclogger";

import { theLogger, theServer } from "../../globals.js";
import { ReceiveInvokeContext } from "../../stub/TSROSEBase.js";
import { IEModule } from "../expressRouter.js";

/**
 * The express rest function call sample route
 */
class RestSample implements IEModule {
	/**
	 * Add specific routings
	 * @param router - parent router
	 */
	public init(router: express.Router): void {
		router.use("/rest", this.restRequest.bind(this));
	}

	/**
	 * The Loggers getLogData callback (used in all the log methods called in this class, add the classname to every log entry)
	 * @returns - an ILogData log data object provided additional data for all the logger calls in this class
	 */
	public getLogData(): ILogData {
		return { className: "RestSample" };
	}

	/**
	 * Takes any client request and forwards it to the asn theServer instance
	 * @param req - http request from client
	 * @param res - http response to client
	 */
	public async restRequest(req: express.Request, res: express.Response): Promise<void> {
		if (req.method === "OPTIONS")
			res.send("");
		else {
			try {
				const invokeContext = new ReceiveInvokeContext({
					clientIP: req.ip,
					headers: req.headers,
					url: req.url
				});
				const response = await theServer.receive(req.body, invokeContext);
				if (response) {
					if (typeof response.payLoad === "string") {
						res.writeHead(response.httpStatusCode, {
							"Content-Type": "application/json",
							"Content-Length": response.payLoad.length
						});
						res.end(response.payLoad, "ascii");
					} else {
						res.writeHead(response.httpStatusCode, {
							"Content-Type": "application/octet-stream",
							"Content-Length": response.payLoad.length
						});
						res.end(response.payLoad, "binary");
					}
				}
			} catch (error) {
				debugger;
				theLogger.error("exception", "restRequest", this, undefined, error);
				res.status(500).send("Catched an unhandled exception while processing request");
			}
		}
	}
}

export default new RestSample();
