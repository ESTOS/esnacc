import { ILogData } from "uclogger";

import { theLogger } from "../globals.js";
import { IClientConnection, IClientConnectionNotify } from "./IClientConnection.js";

/**
 * Handles the notifies as beeing exposed by IConferenceControllerNotify
 */
export class ClientConnectionNotifies extends Set<IClientConnectionNotify> {
	/**
	 * The Loggers getLogData callback (used in all the log methods called in this class, add the classname to every log entry)
	 *
	 * @returns - an ILogData log data object provided additional data for all the logger calls in this class
	 */
	public getLogData(): ILogData {
		return { className: this.constructor.name };
	}

	/**
	 * Adds a notify to the conference controller callbacks
	 *
	 * @param callback - the notify to add
	 */
	public addNotify(callback: IClientConnectionNotify): void {
		if (!this.has(callback))
			this.add(callback);
		else {
			debugger;
			theLogger.error("added an already existing notify", "addNotify", this, { callback });
		}
	}

	/**
	 * Removes a notify from the conference controller callbacks
	 *
	 * @param callback - the notify to remove
	 */
	public removeNotify(callback: IClientConnectionNotify): void {
		if (!this.delete(callback)) {
			debugger;
			theLogger.error("tried to remove a non existing notify", "removeNotify", this, { callback });
		}
	}

	/**
	 * Notifies that the client connection has a new rtt (round trip time value)
	 *
	 * @param con - the affected client connection
	 * @param rtt - the new round trip time in msec
	 */
	public fire_On_RTTChanged(con: IClientConnection, rtt: number): void {
		for (const notify of this.values()) {
			if (notify.on_RTTChanged)
				notify.on_RTTChanged(con, rtt);
		}
	}

	/**
	 * Notifies that a client is disconnecting from the server
	 * The notify has a slightly different name than the IClientConnectionManagerNotify as this event comes prior to the on_ClientDisconnected
	 * While this notify is processed the client is still in the list of connections in the clientConnectionManager
	 *
	 * @param con - the disconnected client connection
	 */
	public fire_On_ClientDisconnecting(con: IClientConnection): void {
		for (const notify of this.values()) {
			if (notify.on_ClientDisconnecting)
				notify.on_ClientDisconnecting(con);
		}
	}

	/**
	 * Notifies that a client timed out in sending a keepalive to the server
	 *
	 * @param con - the connected client connection object
	 */
	public fire_On_ClientKeepAliveTimedOut(con: IClientConnection): void {
		for (const notify of this.values()) {
			if (notify.on_ClientKeepAliveTimedOut)
				notify.on_ClientKeepAliveTimedOut(con);
		}
	}
}
