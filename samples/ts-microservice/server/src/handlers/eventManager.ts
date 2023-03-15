import { IASN1Transport, IReceiveInvokeContext } from "../stub/TSROSEBase";
import { ILogData } from "uclogger";
import { ENetUC_Common, ENetUC_Event_Manager } from "../stub/types";
import { theClientConnectionManager } from "../globals";
import { ENetUC_Event_ManagerROSE } from "../stub/ENetUC_Event_ManagerROSE";
import { IENetUC_Event_ManagerROSE_Invoke_Handler } from "../stub/ENetUC_Event_ManagerROSE_Interface";

/**
 * This module creates events for clients
 */
export class EventManager implements IENetUC_Event_ManagerROSE_Invoke_Handler {
	/** Interface to call the other side */
	private rose: ENetUC_Event_ManagerROSE;

	/**
	 * Creates the SettingsManager object
	 *
	 * @param transport - the transport layer (the TSASN1Server instance that holds the hole ROSE ASN1 stuff)
	 */
	public constructor(transport: IASN1Transport) {
		this.rose = new ENetUC_Event_ManagerROSE(transport, true, this);
		this.sendEvent = this.sendEvent.bind(this);
	}

	/**
	 * The Loggers getLogData callback (used in all the log methods called in this class, add the classname to every log entry)
	 *
	 * @returns - an ILogData log data object provided additional data for all the logger calls in this class
	 */
	public getLogData(): ILogData {
		return {
			className: this.constructor.name
		};
	}

	/**
	 * Method that will create some events the server will then dispatch to the clients
	 *
	 * It´s not possible to call this method through rest as the client won´t receive these events
	 *
	 * @param argument - Argument to create fancy events on the server side
	 * @param invokeContext - Invokecontext from the asn.1 lib (containing invoke related data)
	 * @returns - AsnCreateFancyEventsResult on success, AsnRequestError on error or undefined if the function is not implemented
	 */
	public async onInvoke_asnCreateFancyEvents(argument: ENetUC_Event_Manager.AsnCreateFancyEventsArgument, invokeContext: IReceiveInvokeContext): Promise<ENetUC_Event_Manager.AsnCreateFancyEventsResult | ENetUC_Common.AsnRequestError | undefined> {
		// Check if the request comes from a websocket connection
		if (!invokeContext.clientConnectionID) {
			return new ENetUC_Common.AsnRequestError({
				u8sErrorString: "It is not possible to create events through a rest like request. Please use it via a websocket connection.",
				iErrorDetail: 1
			});
		}

		// Trigger the event dispatching
		if (argument.iEventCount > 0)
			setTimeout(this.sendEvent, argument.iEventDelay, 1, argument.iEventCount - 1, argument.iEventDelay);

		return new ENetUC_Event_Manager.AsnCreateFancyEventsResult();
	}

	/**
	 * Sends an event to all currently connected clients
	 *
	 * @param counter - the Counter that is increased with every event
	 * @param left - The amount of eveets that are left to be send
	 * @param delay - the delay for the next trigger
	 */
	private sendEvent(counter: number, left: number, delay: number): void {
		// Get the connectionids from the client connection manager
		// You may also implement the notify of the connection manager and have your own list (check theClientConnectionManager.addNotify(this))
		const connectionIDs = theClientConnectionManager.getClientConnectionIDs();
		if (connectionIDs.length) {
			// If we have connected clients, create the event argument and dispatch to all connected clients
			const argument = new ENetUC_Event_Manager.AsnFancyEventArgument({
				iEventCounter: counter,
				iEventsLeft: left
			});
			for (const connectionID of connectionIDs)
				this.rose.event_asnFancyEvent(argument, { clientConnectionID: connectionID });
		}
		// In case there are events left trigger a new timeout
		if (left > 0)
			setTimeout(this.sendEvent, delay, counter + 1, left - 1, delay);
	}
}
