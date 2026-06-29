import * as ENetUC_Common from "./stub/ENetUC_Common.js";
import * as ENetUC_Event_Manager from "./stub/ENetUC_Event_Manager.js";
import { ENetUC_Event_ManagerROSE } from "./stub/ENetUC_Event_ManagerROSE.js";
import { IENetUC_Event_ManagerROSE_Handler } from "./stub/ENetUC_Event_ManagerROSE_Interface.js";
import * as ENetUC_Settings_Manager from "./stub/ENetUC_Settings_Manager.js";
import { ENetUC_Settings_ManagerROSE } from "./stub/ENetUC_Settings_ManagerROSE.js";
import { IENetUC_Settings_ManagerROSE_Handler } from "./stub/ENetUC_Settings_ManagerROSE_Interface.js";
import { TSASN1NodeClient } from "./stub/TSASN1NodeClient.js";
import { EASN1TransportEncoding } from "./stub/TSInvokeContext.js";
import { AsnInvokeProblem } from "./stub/TSROSEBase.js";

export class EventCollector
	implements Partial<IENetUC_Settings_ManagerROSE_Handler>, Partial<IENetUC_Event_ManagerROSE_Handler> {
	public readonly settingsChangedEvents: ENetUC_Settings_Manager.AsnSettingsChangedArgument[] = [];
	public readonly fancyEvents: ENetUC_Event_Manager.AsnFancyEventArgument[] = [];

	public onEvent_asnSettingsChanged(argument: ENetUC_Settings_Manager.AsnSettingsChangedArgument): void {
		this.settingsChangedEvents.push(argument);
	}

	public onEvent_asnFancyEvent(argument: ENetUC_Event_Manager.AsnFancyEventArgument): void {
		this.fancyEvents.push(argument);
	}
}

export class IntegrationClient extends TSASN1NodeClient {
	public readonly restBase: string;
	public readonly wsBase: string;

	public constructor(port: number, encoding: EASN1TransportEncoding = EASN1TransportEncoding.JSON) {
		super(encoding);
		this.encodeContext.bAddTypes = false;
		this.defaultTimeout = 10000;
		this.restBase = `http://127.0.0.1:${port}/rest`;
		this.wsBase = `ws://127.0.0.1:${port}/ws`;
	}

	public useRest(): void {
		this.setTarget(this.restBase);
	}

	public useWebSocket(): void {
		this.setTarget(this.wsBase);
	}
}

export function assertInvokeResult<T>(
	response: T | ENetUC_Common.AsnRequestError | AsnInvokeProblem,
	classType: { new(...args: never[]): T; },
	label: string,
): T {
	if (response instanceof classType)
		return response;

	if (response instanceof ENetUC_Common.AsnRequestError)
		throw new Error(`${label}: ${response.u8sErrorString ?? "request error"}`);

	if (response instanceof AsnInvokeProblem)
		throw new Error(`${label}: ${response.details ?? "invoke problem"}`);

	throw new Error(`${label}: unexpected response type`);
}

/** REST invoke tests: one client, settings module only, no inbound event handlers. */
export interface SettingsIntegration {
	client: IntegrationClient;
	settingsRose: ENetUC_Settings_ManagerROSE;
}

export function createSettingsIntegration(port: number): SettingsIntegration {
	const client = new IntegrationClient(port);
	const settingsRose = new ENetUC_Settings_ManagerROSE(client, false);
	return { client, settingsRose };
}

/** WebSocket event tests: one client; register only the ROSE modules needed for that test. */
export interface EventIntegration {
	client: IntegrationClient;
	collector: EventCollector;
	settingsRose?: ENetUC_Settings_ManagerROSE;
	eventRose?: ENetUC_Event_ManagerROSE;
}

export interface CreateEventIntegrationOptions {
	settings?: boolean;
	events?: boolean;
}

export function createEventIntegration(port: number, options: CreateEventIntegrationOptions): EventIntegration {
	const client = new IntegrationClient(port);
	const collector = new EventCollector();
	let settingsRose: ENetUC_Settings_ManagerROSE | undefined;
	let eventRose: ENetUC_Event_ManagerROSE | undefined;

	if (options.settings)
		settingsRose = new ENetUC_Settings_ManagerROSE(client, true, collector);
	if (options.events)
		eventRose = new ENetUC_Event_ManagerROSE(client, true, collector);

	return { client, collector, settingsRose, eventRose };
}

export async function releaseIntegration(
	client: IntegrationClient,
	roses?: { settingsRose?: ENetUC_Settings_ManagerROSE; eventRose?: ENetUC_Event_ManagerROSE; },
): Promise<void> {
	roses?.settingsRose?.removeHandler();
	roses?.eventRose?.removeHandler();
	await client.disconnect(true);
}

export async function waitForCount<T>(
	getItems: () => readonly T[],
	expectedCount: number,
	timeoutMs = 10000,
	pollMs = 50,
): Promise<T[]> {
	const deadline = Date.now() + timeoutMs;
	while (Date.now() < deadline) {
		const items = getItems();
		if (items.length >= expectedCount)
			return [...items];
		await new Promise((resolve) => setTimeout(resolve, pollMs));
	}

	throw new Error(`Timed out waiting for ${expectedCount} item(s), got ${getItems().length}`);
}
