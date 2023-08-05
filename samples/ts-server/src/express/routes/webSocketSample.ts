import express from "express";
import WebSocket from "ws";
import net from "net";
import { theClientConnectionManager, theLogger } from "../../globals";
import { IEModule } from "../expressRouter";
import { ILogData } from "uclogger";
import { IClientConnectionConstructorArguments } from "../../lib/IClientConnection";
import { IVerifyClientOptions, VerifyClientFunction, WebSocketRoute } from "../webSocketRoute";

/**
 * The express websocket sample route
 */
class WebSocketSample extends WebSocketRoute implements IEModule {
	// the Path this websocket sample is handling on
	private path = "/ws";

	/**
	 * Add specific routings
	 * @param router - parent router
	 */
	public override init(router: express.Router): void {
		super.init(router);
	}

	/**
	 * The Loggers getLogData callback (used in all the log methods called in this class, add the classname to every log entry)
	 * @returns - an ILogData log data object provided additional data for all the logger calls in this class
	 */
	public getLogData(): ILogData {
		return { className: "WebSocketSample" };
	}

	/**
	 * Verifies if a client is allowed to connect this route
	 * @param options - the verfiy context object that is handed over from the caller
	 * @param func - the function to be called to signal that the connection is allowed or not
	 * @returns a Promis resolving to void
	 */
	protected async verifyClient(options: IVerifyClientOptions, func: VerifyClientFunction): Promise<void> {
		func(true);
	}

	/**
	 * This handler is called if the servers onupgrade is called.
	 * Each route is then asked if it wants to handle the request
	 * If we handle it we return true, otherwise false to let the next handler check it
	 * @param request - the express request
	 * @param socket - the associated socket
	 * @param head - the head object
	 * @returns - true if we handle the request, false otherwise
	 */
	public onUpgrade(request: express.Request, socket: net.Socket, head: Buffer): boolean {
		try {
			if (request.url.substr(0, this.path.length) === this.path && this.webSocketServer) {
				this.webSocketServer.handleUpgrade(request, socket, head, async (ws: WebSocket) => {
					const args: IClientConnectionConstructorArguments = {
						request
					};
					const con = theClientConnectionManager.createClientConnection(args);
					con.setWebSocket(ws);
					return true;
				});
			}
		} catch (error) {
			theLogger.error("Upgrading connection failed", "onUpgrade", this, { request }, error);
		}
		return false;
	}
}

export = new WebSocketSample();
