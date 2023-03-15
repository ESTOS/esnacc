import { IASN1Transport, IReceiveInvokeContext } from "../stub/TSROSEBase";
import { theClient } from "../client";
import { IENetUC_Settings_ManagerROSE_Event_Handler } from "../stub/ENetUC_Settings_ManagerROSE_Interface";
import { ENetUC_Settings_ManagerROSE } from "../stub/ENetUC_Settings_ManagerROSE";
import { ENetUC_Settings_Manager } from "../stub/types";

/**
 * This module sends requests to the server to receive events
 */
export class SettingsManager implements IENetUC_Settings_ManagerROSE_Event_Handler {
	private server: ENetUC_Settings_ManagerROSE;

	/**
	 * Creates the SettingsManager object
	 *
	 * @param transport - the transport layer (the TSASN1Server instance that holds the hole ROSE ASN1 stuff)
	 */
	public constructor(transport: IASN1Transport) {
		this.server = new ENetUC_Settings_ManagerROSE(transport, true, this);
	}

	/**
	 * Sends settings to the server
	 * In case a property has changed the server will dispatch events
	 *
	 * @param enabled - the enable state
	 * @param username - the username
	 */
	public async setSettings(enabled: boolean, username: string): Promise<void> {
		const argument = new ENetUC_Settings_Manager.AsnSetSettingsArgument({
			settings: {
				bEnabled: enabled,
				u8sUsername: username
			}
		});
		const response = await this.server.invoke_asnSetSettings(argument);
		theClient.getResult(response, ENetUC_Settings_Manager.AsnSetSettingsResult);
	}

	/**
	 * Retrieve settings from the server
	 */
	public async getSettings(): Promise<{enabled?: boolean, username?: string} | undefined> {
		const argument = new ENetUC_Settings_Manager.AsnGetSettingsArgument();
		const response = await this.server.invoke_asnGetSettings(argument);
		const result = theClient.getResult(response, ENetUC_Settings_Manager.AsnGetSettingsResult);
		return {
			enabled: result.settings.bEnabled,
			username: result.settings.u8sUsername
		};
	}

	/**
	 * Sent to the clients in the case settings have changed
	 *
	 * @param argument - Argument for the asnSettingsChanged event
	 * @param invokeContext - Invokecontext from the asn.1 lib (containing invoke related data)
	 */
	public onEvent_asnSettingsChanged(argument: ENetUC_Settings_Manager.AsnSettingsChangedArgument, invokeContext: IReceiveInvokeContext): void {
		if (argument.settings.bEnabled !== undefined) {
			const enabled = document.getElementById("enabled") as HTMLInputElement;
			if (enabled)
				enabled.checked = argument.settings.bEnabled;
		}
		if (argument.settings.u8sUsername !== undefined) {
			const username = document.getElementById("username") as HTMLInputElement;
			if (username)
				username.value = argument.settings.u8sUsername;
		}
	}
}
