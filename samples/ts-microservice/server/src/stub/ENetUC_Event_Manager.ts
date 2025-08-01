// [PrintTSCodeOne]
// [PrintTSComments]
/*
 * ENetUC_Event_Manager.ts
 * "UC-Server-Access-Protocol-Event-Manager" ASN.1 stubs.
 * This file was generated by estos esnacc (V6.0.31, 29.07.2025)
 * based on Coral WinSnacc written by Deepak Gupta
 * NOTE: This is a machine generated file - editing not recommended
 */

// dprint-ignore-file
/* eslint-disable */
/**
 * This module is used to create and dispatch events.
 * Clients may request events from the server
 * The server then sends events to all websocket connected clients
 * Requesting events through rest is not supported and rejected
 * Methods and Events
 * Methods:
 * 4000 asnCreateFancyEvents			- A method that will create events on the server side
 * Events:
 * 4050 asnFancyEvent					- An event that is fired by the server in case a client request an event
 */
// [PrintTSImports]
import * as asn1ts from "@estos/asn1ts";
import * as ENetUC_Common from "./ENetUC_Common.js";
// [PrintTSRootTypes]
export const MODULE_NAME = "ENetUC_Event_Manager";
export const MODULE_LASTCHANGE = "1970-01-01T00:00:00Z";
export const MODULE_MAJOR_VERSION = 0;
export const MODULE_MINOR_VERSION = 0;
export const MODULE_PATCH_VERSION = 0;
export const MODULE_VERSION = "0.0.0";

// [PrintTSTypeDefCode]
// [PrintTSSeqDefCode]
/**
 * Argument to create fancy events on the server side
 */
export class AsnCreateFancyEventsArgument {
	public constructor(that: AsnCreateFancyEventsArgument) {
		Object.assign(this, that);
	}

	private static initEmpty(): AsnCreateFancyEventsArgument {
		return new AsnCreateFancyEventsArgument({
			iEventDelay: 0,
			iEventCount: 0
		});
	}

	public static getOwnPropertyNames(bIncludeOptionals: boolean = true): string[] {
		const p = [
			"iEventDelay",
			"iEventCount"
		];
		return p;
	}

	public static type = "AsnCreateFancyEventsArgument";

	public static getASN1Schema(params?: asn1ts.ConstructedParams): asn1ts.Sequence {
		return new asn1ts.Sequence({
			name: "AsnCreateFancyEventsArgument",
			...params,
			value: [
				new asn1ts.Integer({ name: "iEventDelay" }),
				new asn1ts.Integer({ name: "iEventCount" }),
				new asn1ts.Extension()
			]
		});
	}

	/** the delay in msec between the events (the first event will also be delayed by that delay) */
	public iEventDelay!: number;
	/** the amount of events to send */
	public iEventCount!: number;
}

// [PrintTSTypeDefCode]
// [PrintTSSeqDefCode]
/**
 * Result for the asnCreateFancyEvents method
 */
export class AsnCreateFancyEventsResult {
	public constructor(that?: AsnCreateFancyEventsResult) {
		Object.assign(this, that);
	}

	private static initEmpty(): AsnCreateFancyEventsResult {
		return new AsnCreateFancyEventsResult();
	}

	public static getOwnPropertyNames(bIncludeOptionals: boolean = true): string[] {
		const p: string[] = [];
		return p;
	}

	public static type = "AsnCreateFancyEventsResult";

	public static getASN1Schema(params?: asn1ts.ConstructedParams): asn1ts.Sequence {
		return new asn1ts.Sequence({
			name: "AsnCreateFancyEventsResult",
			...params,
			value: [
				new asn1ts.Extension()
			]
		});
	}
}

// [PrintTSTypeDefCode]
// [PrintTSSeqDefCode]
/**
 * Argument for the AsnFancyEventArgument method
 */
export class AsnFancyEventArgument {
	public constructor(that: AsnFancyEventArgument) {
		Object.assign(this, that);
	}

	private static initEmpty(): AsnFancyEventArgument {
		return new AsnFancyEventArgument({
			iEventCounter: 0,
			iEventsLeft: 0
		});
	}

	public static getOwnPropertyNames(bIncludeOptionals: boolean = true): string[] {
		const p = [
			"iEventCounter",
			"iEventsLeft"
		];
		return p;
	}

	public static type = "AsnFancyEventArgument";

	public static getASN1Schema(params?: asn1ts.ConstructedParams): asn1ts.Sequence {
		return new asn1ts.Sequence({
			name: "AsnFancyEventArgument",
			...params,
			value: [
				new asn1ts.Integer({ name: "iEventCounter" }),
				new asn1ts.Integer({ name: "iEventsLeft" }),
				new asn1ts.Extension()
			]
		});
	}

	/** The count that is incremented with every event */
	public iEventCounter!: number;
	/** The events that are left to get dispatched */
	public iEventsLeft!: number;
}
