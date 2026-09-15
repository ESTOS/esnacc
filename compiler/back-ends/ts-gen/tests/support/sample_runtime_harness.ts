// Loopback runtime harness for glue tests (parity with cpp-lib/tests/test_support/sample_runtime_harness.h).
import {
	ASN1ClassInstanceType,
	PendingInvoke,
	TSASN1Base,
} from "../workdir/TSASN1Base.js";
import * as ENetUC_Common from "../workdir/ENetUC_Common.js";
import * as ENetUC_Event_Manager from "../workdir/ENetUC_Event_Manager.js";
import { ENetUC_Event_ManagerROSE } from "../workdir/ENetUC_Event_ManagerROSE.js";
import type { IENetUC_Event_ManagerROSE_Handler } from "../workdir/ENetUC_Event_ManagerROSE_Interface.js";
import * as ENetUC_Settings_Manager from "../workdir/ENetUC_Settings_Manager.js";
import { ENetUC_Settings_ManagerROSE, OperationIDs as SettingsOpIds } from "../workdir/ENetUC_Settings_ManagerROSE.js";
import type { IENetUC_Settings_ManagerROSE_Handler } from "../workdir/ENetUC_Settings_ManagerROSE_Interface.js";
import { ROSEMessage_Converter } from "../workdir/SNACCROSE_Converter.js";
import { ROSEError, ROSEMessage, ROSEInvoke, ROSEReject, ROSEResult } from "../workdir/SNACCROSE.js";
import { ConverterErrors } from "../workdir/TSConverterBase.js";
import { EASN1TransportEncoding } from "../workdir/TSInvokeContext.js";
import {
	asn1Decode,
	asn1Encode,
	createInvokeReject,
	type IASN1InvokeData,
	ReceiveInvokeContext,
	ROSEBase,
	SendInvokeContext,
} from "../workdir/TSROSEBase.js";
import { ROSE_TE_TRANSPORTFAILED } from "./rose_result_codes.js";

export type TransportEncoding = EASN1TransportEncoding;

export enum TransportActionKind {
	Deliver = "deliver",
	Drop = "drop",
	Fail = "fail",
	Queue = "queue",
}

export interface TransportAction {
	kind: TransportActionKind;
	result?: number;
}

export interface HandlerModes {
	implementGetSettings?: boolean;
	getSettingsReturnsError?: boolean;
	implementSetSettings?: boolean;
	setSettingsReturnsError?: boolean;
	implementCreateFancyEvents?: boolean;
	createFancyEventsReturnsError?: boolean;
}

/** In-memory transport forwarding encoded ROSE payloads to a peer TSASN1Base. */
export class LoopbackTransport extends TSASN1Base {
	public readonly sessionId: string;
	private peer?: LoopbackTransport;
	private readonly actions: TransportAction[] = [];
	private readonly queued: IASN1InvokeData[] = [];
	public dropNextResponse = false;

	public constructor(sessionId: string, role: "client" | "server") {
		super(
			EASN1TransportEncoding.JSON,
			role === "client" ? ASN1ClassInstanceType.TSASN1NodeClient : ASN1ClassInstanceType.TSASN1Server,
		);
		this.sessionId = sessionId;
	}

	public connectTo(peer: LoopbackTransport): void {
		this.peer = peer;
		peer.peer = this;
	}

	public enqueueAction(action: TransportAction): void {
		this.actions.push(action);
	}

	public queuedMessageCount(): number {
		return this.queued.length;
	}

	public flushQueuedMessages(): void {
		const pending = [...this.queued];
		this.queued.length = 0;
		for (const data of pending)
			void this.deliverToPeer(data);
	}

	public async sendInvoke(data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		if (!data.invoke.sessionID)
			data.invoke.sessionID = this.sessionId;

		const action = this.actions.shift() ?? { kind: TransportActionKind.Deliver };
		if (action.kind === TransportActionKind.Fail) {
			return createInvokeReject(
				data.invoke,
				action.result ?? ROSE_TE_TRANSPORTFAILED,
				"loopback transport failure",
			);
		}
		if (action.kind === TransportActionKind.Drop)
			return new Promise(() => {});
		if (action.kind === TransportActionKind.Queue) {
			this.queued.push(data);
			return new Promise(() => {});
		}

		const invokeId = data.invoke.invokeID;
		if (invokeId === 99999)
			return this.deliverToPeer(data);

		return new Promise((resolve) => {
			const ctxTimeout = data.invokeContext.invokeTimeout();
			const timeoutMs = ctxTimeout === undefined ? this.defaultTimeout : ctxTimeout;
			let timerId: ReturnType<typeof setTimeout> | undefined;
			if (timeoutMs) {
				timerId = setTimeout(() => {
					this.onROSETimeout(invokeId);
				}, timeoutMs);
			}
			this.pendingInvokes.set(invokeId, new PendingInvoke(data.invoke, resolve, timerId));

			void this.deliverToPeer(data).then((result) => {
				const pending = this.pendingInvokes.get(invokeId);
				if (!pending)
					return;
				this.pendingInvokes.delete(invokeId);
				if (result instanceof ROSEResult)
					pending.completed_result(result);
				else if (result instanceof ROSEError)
					pending.completed_error(result);
				else if (result instanceof ROSEReject)
					pending.completed_reject(result);
				else
					pending.completed_reject(createInvokeReject(data.invoke, ROSE_TE_TRANSPORTFAILED, "empty loopback response"));
			});
		});
	}

	public sendEventSync(_data: IASN1InvokeData): boolean {
		return true;
	}

	public getSessionID(): string | undefined {
		return this.sessionId;
	}

	public async receiveRaw(rawData: object | Uint8Array, encoding: TransportEncoding): Promise<void> {
		const invokeContext = new ReceiveInvokeContext({ encoding });
		await this.receive(rawData, invokeContext);
	}

	private async deliverToPeer(data: IASN1InvokeData): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		const peer = this.peer;
		if (!peer)
			throw new Error("loopback peer not connected");

		const encoding = data.invokeContext.encoding ?? EASN1TransportEncoding.JSON;
		const wire = ROSEBase.encodeToTransport(data.payLoad, this.encodeContext);
		const receiveContext = new ReceiveInvokeContext({
			encoding,
			clientConnectionID: this.sessionId,
		});

		if (data.invoke.invokeID === 99999) {
			await peer.receive(wire.payLoad, receiveContext);
			return undefined;
		}

		const response = await peer.receive(wire.payLoad, receiveContext);
		if (!response?.payLoad)
			return undefined;

		if (this.dropNextResponse) {
			this.dropNextResponse = false;
			return new Promise(() => {});
		}

		// Deliver the encoded response through the client receive path (parity with real transport).
		const clientReceiveContext = new ReceiveInvokeContext({
			encoding,
			clientConnectionID: peer.sessionId,
		});
		await this.receive(response.payLoad, clientReceiveContext);
		return undefined;
	}
}

function settingsFromState(enabled: boolean, username: string): ENetUC_Settings_Manager.AsnSomeSettings {
	return new ENetUC_Settings_Manager.AsnSomeSettings({
		bEnabled: enabled,
		u8sUsername: username,
	});
}

/** Two-endpoint sample runtime with generated settings and event-manager modules. */
export class SampleRuntimeHarness {
	public readonly clientTransport: LoopbackTransport;
	public readonly serverTransport: LoopbackTransport;
	public readonly clientSettingsRose: ENetUC_Settings_ManagerROSE;
	public readonly serverSettingsRose: ENetUC_Settings_ManagerROSE;
	public readonly clientEventRose: ENetUC_Event_ManagerROSE;
	public readonly serverEventRose: ENetUC_Event_ManagerROSE;

	private serverEnabled = false;
	private serverUsername = "initial-user";
	private handlerModes: HandlerModes = {};
	private settingsEventCount = 0;
	private lastSettingsEventEnabled = false;
	private lastSettingsEventUsername = "";
	private fancyEvents: Array<{ counter: number; left: number }> = [];

	public constructor() {
		this.clientTransport = new LoopbackTransport("client-session", "client");
		this.serverTransport = new LoopbackTransport("server-session", "server");
		this.clientTransport.connectTo(this.serverTransport);

		this.serverSettingsRose = new ENetUC_Settings_ManagerROSE(this.serverTransport, true, this.createServerSettingsHandler());
		this.clientSettingsRose = new ENetUC_Settings_ManagerROSE(this.clientTransport, true, this.createClientSettingsHandler());
		this.serverEventRose = new ENetUC_Event_ManagerROSE(this.serverTransport, true, this.createServerEventHandler());
		this.clientEventRose = new ENetUC_Event_ManagerROSE(this.clientTransport, true, this.createClientEventHandler());
	}

	public setEncoding(encoding: TransportEncoding): void {
		this.clientTransport.setEncoding(encoding);
		this.serverTransport.setEncoding(encoding);
	}

	public configureServerHandlers(modes: HandlerModes): void {
		this.handlerModes = { ...modes };
	}

	public resetState(): void {
		this.handlerModes = {};
		this.serverEnabled = false;
		this.serverUsername = "initial-user";
		this.settingsEventCount = 0;
		this.lastSettingsEventEnabled = false;
		this.lastSettingsEventUsername = "";
		this.fancyEvents = [];
	}

	public getSettingsEventCount(): number {
		return this.settingsEventCount;
	}

	public getLastSettingsEventEnabled(): boolean {
		return this.lastSettingsEventEnabled;
	}

	public getLastSettingsEventUsername(): string {
		return this.lastSettingsEventUsername;
	}

	public getFancyEventCount(): number {
		return this.fancyEvents.length;
	}

	public fancyEventsSnapshot(): Array<{ counter: number; left: number }> {
		return [...this.fancyEvents];
	}

	public malformedPayload(encoding: TransportEncoding): object | Uint8Array {
		return encoding === EASN1TransportEncoding.JSON ? { notRose: true } : new Uint8Array([0xff, 0x00, 0x01]);
	}

	/** Sends a raw ROSE invoke through the client loopback transport (negative tests). */
	public async sendClientInvokeMessage(
		invoke: ROSEInvoke,
		argument: object | undefined,
		encoding: TransportEncoding,
		timeoutMs = 250,
	): Promise<ROSEReject | ROSEResult | ROSEError | undefined> {
		if (argument !== undefined)
			invoke.argument = argument;
		const errors = new ConverterErrors();
		const payLoad = asn1Encode(encoding, new ROSEMessage({ invoke }), ROSEMessage_Converter, errors, this.clientTransport.getEncodeContext());
		const invokeContext = new SendInvokeContext({ encoding, invokeTimeoutMs: timeoutMs, invokeID: invoke.invokeID });
		return this.clientTransport.sendInvoke({
			invoke,
			payLoad: payLoad!,
			invokeContext,
		});
	}

	private createServerSettingsHandler(): IENetUC_Settings_ManagerROSE_Handler {
		const self = this;
		return {
			async onInvoke_asnGetSettings(
				_argument: ENetUC_Settings_Manager.AsnGetSettingsArgument,
				_ctx,
			): Promise<ENetUC_Settings_Manager.AsnGetSettingsResult | ENetUC_Common.AsnRequestError | undefined> {
				if (self.handlerModes.implementGetSettings === false)
					return undefined;
				if (self.handlerModes.getSettingsReturnsError) {
					return new ENetUC_Common.AsnRequestError({
						iErrorDetail: 7001,
						u8sErrorString: "get settings handler returned error",
					});
				}
				return new ENetUC_Settings_Manager.AsnGetSettingsResult({
					settings: settingsFromState(self.serverEnabled, self.serverUsername),
				});
			},
			async onInvoke_asnSetSettings(
				argument: ENetUC_Settings_Manager.AsnSetSettingsArgument,
				_ctx,
			): Promise<ENetUC_Settings_Manager.AsnSetSettingsResult | ENetUC_Common.AsnRequestError | undefined> {
				if (self.handlerModes.implementSetSettings === false)
					return undefined;
				if (self.handlerModes.setSettingsReturnsError) {
					return new ENetUC_Common.AsnRequestError({
						iErrorDetail: 7002,
						u8sErrorString: "set settings handler returned error",
					});
				}
				self.serverEnabled = argument.settings.bEnabled ?? false;
				self.serverUsername = argument.settings.u8sUsername ?? "";
				self.serverSettingsRose.event_asnSettingsChanged(
					new ENetUC_Settings_Manager.AsnSettingsChangedArgument({
						settings: settingsFromState(self.serverEnabled, self.serverUsername),
					}),
					{ encoding: self.serverTransport.getEncoding() },
				);
				return new ENetUC_Settings_Manager.AsnSetSettingsResult({});
			},
			onEvent_asnSettingsChanged(): void {
				// server does not receive this event in these tests
			},
		};
	}

	private createClientSettingsHandler(): IENetUC_Settings_ManagerROSE_Handler {
		const self = this;
		return {
			async onInvoke_asnGetSettings(): Promise<undefined> {
				return undefined;
			},
			async onInvoke_asnSetSettings(): Promise<undefined> {
				return undefined;
			},
			onEvent_asnSettingsChanged(argument: ENetUC_Settings_Manager.AsnSettingsChangedArgument): void {
				++self.settingsEventCount;
				self.lastSettingsEventEnabled = argument.settings.bEnabled ?? false;
				self.lastSettingsEventUsername = argument.settings.u8sUsername ?? "";
			},
		};
	}

	private createServerEventHandler(): IENetUC_Event_ManagerROSE_Handler {
		const self = this;
		return {
			async onInvoke_asnCreateFancyEvents(
				argument: ENetUC_Event_Manager.AsnCreateFancyEventsArgument,
				_ctx,
			): Promise<ENetUC_Event_Manager.AsnCreateFancyEventsResult | ENetUC_Common.AsnRequestError | undefined> {
				if (self.handlerModes.implementCreateFancyEvents === false)
					return undefined;
				if (self.handlerModes.createFancyEventsReturnsError) {
					return new ENetUC_Common.AsnRequestError({
						iErrorDetail: 7003,
						u8sErrorString: "create fancy events handler returned error",
					});
				}
				const count = argument.iEventCount ?? 0;
				for (let index = 0; index < count; ++index) {
					self.serverEventRose.event_asnFancyEvent(
						new ENetUC_Event_Manager.AsnFancyEventArgument({
							iEventCounter: index + 1,
							iEventsLeft: count - index - 1,
						}),
						{ encoding: self.serverTransport.getEncoding() },
					);
				}
				return new ENetUC_Event_Manager.AsnCreateFancyEventsResult({});
			},
		};
	}

	private createClientEventHandler(): IENetUC_Event_ManagerROSE_Handler {
		const self = this;
		return {
			onEvent_asnFancyEvent(argument: ENetUC_Event_Manager.AsnFancyEventArgument): void {
				self.fancyEvents.push({
					counter: argument.iEventCounter ?? 0,
					left: argument.iEventsLeft ?? 0,
				});
			},
		};
	}
}
