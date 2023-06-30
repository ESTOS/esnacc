import { IASN1Transport, IReceiveInvokeContext } from "../stub/TSROSEBase";
import { theClient } from "../client";
import { ENetUC_Event_Manager } from "../stub/types";
import { ENetUC_Event_ManagerROSE } from "../stub/ENetUC_Event_ManagerROSE";
import { IENetUC_Event_ManagerROSE_Event_Handler } from "../stub/ENetUC_Event_ManagerROSE_Interface";

/**
 * This module sends requests to the server to receive events
 */
export class EventManager implements IENetUC_Event_ManagerROSE_Event_Handler {
	// The networking layer that allows to send events to the server side
	private server: ENetUC_Event_ManagerROSE;

	/**
	 * Creates the SettingsManager object
	 * @param transport - the transport layer (the TSASN1Server instance that holds the hole ROSE ASN1 stuff)
	 */
	public constructor(transport: IASN1Transport) {
		this.server = new ENetUC_Event_ManagerROSE(transport, true, this);
	}

	/**
	 * Tells the server that it should send events to connected websocket clients
	 * @param count - the amount of events
	 * @param delay - the delay between the events
	 */
	public async getEvents(count: number, delay: number): Promise<void> {
		const argument = new ENetUC_Event_Manager.AsnCreateFancyEventsArgument({
			iEventCount: count,
			iEventDelay: delay
		});
		const response = await this.server.invoke_asnCreateFancyEvents(argument);
		theClient.getResult(response, ENetUC_Event_Manager.AsnCreateFancyEventsResult);
	}

	/**
	 * An event that is dispatched from the server to the clients
	 * @param argument - Argument for the AsnFancyEventArgument method
	 * @param invokeContext - Invokecontext from the asn.1 lib (containing invoke related data)
	 */
	public onEvent_asnFancyEvent(argument: ENetUC_Event_Manager.AsnFancyEventArgument, invokeContext: IReceiveInvokeContext): void {
	}
}
