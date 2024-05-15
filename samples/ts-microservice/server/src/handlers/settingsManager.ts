import { ILogData } from "uclogger";

import { theClientConnectionManager } from "../globals";
import { ENetUC_Settings_ManagerROSE } from "../stub/ENetUC_Settings_ManagerROSE";
import { IENetUC_Settings_ManagerROSE_Invoke_Handler } from "../stub/ENetUC_Settings_ManagerROSE_Interface";
import { IASN1Transport, IReceiveInvokeContext } from "../stub/TSROSEBase";
import { ENetUC_Common, ENetUC_Settings_Manager } from "../stub/types";

/**
 * This module implements the settings class that allows to get and set settings and creates events for settings that have changed
 */
export class SettingsManager implements Partial<IENetUC_Settings_ManagerROSE_Invoke_Handler> {
	/** Interface to call the other side */
	private rose: ENetUC_Settings_ManagerROSE;

	/**
	 * Creates the SettingsManager object
	 *
	 * @param transport - the transport layer (the TSASN1Server instance that holds the hole ROSE ASN1 stuff)
	 */
	public constructor(transport: IASN1Transport) {
		this.rose = new ENetUC_Settings_ManagerROSE(transport, true, this);
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
	 * Method to retrieve settings from the server side
	 *
	 * @param argument - Argument for the asnGetSettings method
	 * @param invokeContext - Invokecontext from the asn.1 lib (containing invoke related data)
	 * @returns - AsnGetSettingsResult on success, AsnRequestError on error or undefined if the function is not implemented
	 */
	public async onInvoke_asnGetSettings(argument: ENetUC_Settings_Manager.AsnGetSettingsArgument, invokeContext: IReceiveInvokeContext): Promise<ENetUC_Settings_Manager.AsnGetSettingsResult | ENetUC_Common.AsnRequestError | undefined> {
		return new ENetUC_Settings_Manager.AsnGetSettingsResult({
			settings: this.settings
		});
	}

	/**
	 * Method to store settings on the server side
	 *
	 * If a settings property has changed the client will get notified by an event
	 *
	 * @param argument - Argument for the asnSetSettings method
	 * @param invokeContext - Invokecontext from the asn.1 lib (containing invoke related data)
	 * @returns - AsnSetSettingsResult on success, AsnRequestError on error or undefined if the function is not implemented
	 */
	public async onInvoke_asnSetSettings(argument: ENetUC_Settings_Manager.AsnSetSettingsArgument, invokeContext: IReceiveInvokeContext): Promise<ENetUC_Settings_Manager.AsnSetSettingsResult | ENetUC_Common.AsnRequestError | undefined> {
		let bChanged = false;
		const changedSettings = new ENetUC_Settings_Manager.AsnSomeSettings();
		if (this.settings.bEnabled !== argument.settings.bEnabled) {
			this.settings.bEnabled = argument.settings.bEnabled;
			changedSettings.bEnabled = argument.settings.bEnabled;
			bChanged = true;
		}
		if (this.settings.u8sUsername !== argument.settings.u8sUsername) {
			this.settings.u8sUsername = argument.settings.u8sUsername;
			changedSettings.u8sUsername = argument.settings.u8sUsername;
			bChanged = true;
		}
		if (bChanged) {
			this.settings.stTime = new Date();
			const connectionIDs = theClientConnectionManager.getClientConnectionIDs();
			if (connectionIDs.length) {
				const argument = new ENetUC_Settings_Manager.AsnSettingsChangedArgument({ settings: changedSettings });
				for (const connectionID of connectionIDs)
					this.rose.event_asnSettingsChanged(argument, { clientConnectionID: connectionID });
			}
		}
		return new ENetUC_Settings_Manager.AsnSetSettingsResult();
	}

	private settings = new ENetUC_Settings_Manager.AsnSomeSettings();
}
