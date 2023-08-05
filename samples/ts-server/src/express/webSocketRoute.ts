import * as express from "express";
import WebSocket from "ws";
import http from "http";

export interface IVerifyClientOptions {
	origin: string;
	secure: boolean;
	req: http.IncomingMessage;
}

export type VerifyClientFunction = (res: boolean, code?: number, message?: string, headers?: http.OutgoingHttpHeaders) => void

/**
 * The base websocket implementation
 */
export abstract class WebSocketRoute {
	protected webSocketServer?: WebSocket.Server;

	/**
	 * Initializes the websocket route object
	 * @param router - parent router
	 */
	public init(router: express.Router): void {
		this.webSocketServer = 	new WebSocket.Server({
			noServer: true,
			verifyClient: this.verifyClient.bind(this)
		});
	}

	protected abstract verifyClient(options: IVerifyClientOptions, func: VerifyClientFunction): Promise<void>;
}
