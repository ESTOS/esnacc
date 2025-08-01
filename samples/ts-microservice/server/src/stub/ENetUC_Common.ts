// [PrintTSCodeOne]
// [PrintTSComments]
/*
 * ENetUC_Common.ts
 * "UC-Server-Access-Protocol-Common" ASN.1 stubs.
 * This file was generated by estos esnacc (V6.0.31, 29.07.2025)
 * based on Coral WinSnacc written by Deepak Gupta
 * NOTE: This is a machine generated file - editing not recommended
 */

// dprint-ignore-file
/* eslint-disable */
/**
 * Common interface definitions
 * ## Module description
 * This module contains common usable strutures for the other interfaces.
 * There should not be any function\/operation defintions in this module.
 */
// [PrintTSImports]
import * as asn1ts from "@estos/asn1ts";
// [PrintTSRootTypes]
export const MODULE_NAME = "ENetUC_Common";
export const MODULE_LASTCHANGE = "1970-01-01T00:00:00Z";
export const MODULE_MAJOR_VERSION = 0;
export const MODULE_MINOR_VERSION = 0;
export const MODULE_PATCH_VERSION = 0;
export const MODULE_VERSION = "0.0.0";

// [PrintTSTypeDefCode]
// [PrintTSimpleDefCode]
export type AsnSystemTime = number;

// [PrintTSTypeDefCode]
// [PrintTSChoiceDefCode]
export class AsnOptionalParamChoice {
	public constructor(that?: AsnOptionalParamChoice) {
		Object.assign(this, that);
	}

	private static initEmpty(): AsnOptionalParamChoice {
		return new AsnOptionalParamChoice();
	}

	public static type = "AsnOptionalParamChoice";

	public static getASN1Schema(params?: asn1ts.ConstructedParams): asn1ts.Choice {
		return new asn1ts.Choice({
			name: "AsnOptionalParamChoice",
			...params,
			value: [
				new asn1ts.Utf8String({ name: "stringdata" }),
				new asn1ts.OctetString({ name: "binarydata" }),
				new asn1ts.Integer({ name: "integerdata" })
			]
		});
	}

	public stringdata?: string;
	public binarydata?: Uint8Array;
	public integerdata?: number;
}

// [PrintTSTypeDefCode]
// [PrintTSSeqDefCode]
/**
 * Key value pair of strings.
 */
export class AsnStringPair {
	public constructor(that: AsnStringPair) {
		Object.assign(this, that);
	}

	private static initEmpty(): AsnStringPair {
		return new AsnStringPair({
			key: "",
			value: ""
		});
	}

	public static getOwnPropertyNames(bIncludeOptionals: boolean = true): string[] {
		const p = [
			"key",
			"value"
		];
		return p;
	}

	public static type = "AsnStringPair";

	public static getASN1Schema(params?: asn1ts.ConstructedParams): asn1ts.Sequence {
		return new asn1ts.Sequence({
			name: "AsnStringPair",
			...params,
			value: [
				new asn1ts.Utf8String({ name: "key" }),
				new asn1ts.Utf8String({ name: "value" }),
				new asn1ts.Extension()
			]
		});
	}

	public key!: string;
	public value!: string;
}

// [PrintTSTypeDefCode]
// [PrintTSSeqDefCode]
/**
 * Key value pair of integer
 */
export class AsnIntegerPair {
	public constructor(that: AsnIntegerPair) {
		Object.assign(this, that);
	}

	private static initEmpty(): AsnIntegerPair {
		return new AsnIntegerPair({
			key: 0,
			value: 0
		});
	}

	public static getOwnPropertyNames(bIncludeOptionals: boolean = true): string[] {
		const p = [
			"key",
			"value"
		];
		return p;
	}

	public static type = "AsnIntegerPair";

	public static getASN1Schema(params?: asn1ts.ConstructedParams): asn1ts.Sequence {
		return new asn1ts.Sequence({
			name: "AsnIntegerPair",
			...params,
			value: [
				new asn1ts.Integer({ name: "key" }),
				new asn1ts.Integer({ name: "value" }),
				new asn1ts.Extension()
			]
		});
	}

	public key!: number;
	public value!: number;
}

// [PrintTSTypeDefCode]
// [PrintTSSeqDefCode]
/**
 * Key value pair of &lt;string, integer&gt;
 */
export class AsnStringIntegerPair {
	public constructor(that: AsnStringIntegerPair) {
		Object.assign(this, that);
	}

	private static initEmpty(): AsnStringIntegerPair {
		return new AsnStringIntegerPair({
			u8sStr: "",
			iInt: 0
		});
	}

	public static getOwnPropertyNames(bIncludeOptionals: boolean = true): string[] {
		const p = [
			"u8sStr",
			"iInt"
		];
		return p;
	}

	public static type = "AsnStringIntegerPair";

	public static getASN1Schema(params?: asn1ts.ConstructedParams): asn1ts.Sequence {
		return new asn1ts.Sequence({
			name: "AsnStringIntegerPair",
			...params,
			value: [
				new asn1ts.Utf8String({ name: "u8sStr" }),
				new asn1ts.Integer({ name: "iInt" }),
				new asn1ts.Extension()
			]
		});
	}

	public u8sStr!: string;
	public iInt!: number;
}

// [PrintTSTypeDefCode]
// [PrintTSSeqDefCode]
/**
 * Generic error sequence. See the documentation of the operations specific descriptions.
 */
export class AsnRequestError {
	public constructor(that: AsnRequestError) {
		Object.assign(this, that);
	}

	private static initEmpty(): AsnRequestError {
		return new AsnRequestError({
			iErrorDetail: 0,
			u8sErrorString: ""
		});
	}

	public static getOwnPropertyNames(bIncludeOptionals: boolean = true): string[] {
		const p = [
			"iErrorDetail",
			"u8sErrorString"
		];
		return p;
	}

	public static type = "AsnRequestError";

	public static getASN1Schema(params?: asn1ts.ConstructedParams): asn1ts.Sequence {
		return new asn1ts.Sequence({
			name: "AsnRequestError",
			...params,
			value: [
				new asn1ts.Integer({ name: "iErrorDetail" }),
				new asn1ts.Utf8String({ name: "u8sErrorString" }),
				new asn1ts.Extension()
			]
		});
	}

	/** A number representing the error */
	public iErrorDetail!: number;
	/** A string representing the error */
	public u8sErrorString!: string;
}

// [PrintTSTypeDefCode]
// [PrintTSSeqDefCode]
/**
 * A three tuple to identify contact data.
 * The three tuple EntryID, EntryIDDB and EntryIDStore identifies contact data including its
 * source database, so that contact data from different data sources can be differentiated from each other.
 * It is an own sequence, so it can be tranferred without any concrete contact data to save bandwidth.
 *
 * The AsnNetDatabaseContact does not contain a AsnNetDatabaseContactID object, it contains the three
 * members by itself.
 */
export class AsnNetDatabaseContactID {
	public constructor(that: AsnNetDatabaseContactID) {
		Object.assign(this, that);
	}

	private static initEmpty(): AsnNetDatabaseContactID {
		return new AsnNetDatabaseContactID({
			u8sEntryID: "",
			u8sEntryIDDB: "",
			u8sEntryIDStore: ""
		});
	}

	public static getOwnPropertyNames(bIncludeOptionals: boolean = true): string[] {
		const p = [
			"u8sEntryID",
			"u8sEntryIDDB",
			"u8sEntryIDStore"
		];
		return p;
	}

	public static type = "AsnNetDatabaseContactID";

	public static getASN1Schema(params?: asn1ts.ConstructedParams): asn1ts.Sequence {
		return new asn1ts.Sequence({
			name: "AsnNetDatabaseContactID",
			...params,
			value: [
				new asn1ts.Utf8String({ name: "u8sEntryID" }),
				new asn1ts.Utf8String({ name: "u8sEntryIDDB" }),
				new asn1ts.Utf8String({ name: "u8sEntryIDStore" }),
				new asn1ts.Extension()
			]
		});
	}

	/** ID of the contact data in the source database. */
	public u8sEntryID!: string;
	/** ID of the database (type). */
	public u8sEntryIDDB!: string;
	/** Additional ID of a store in the database (if needed). */
	public u8sEntryIDStore!: string;
}

// [PrintTSTypeDefCode]
// [PrintTSSeqDefCode]
/**
 * Encapsulated key value pair, including a definition of the value type.
 * Sequences can change over time of the development of the product. To ensure, that possible breaking changes can be avoided
 * within the life cycle of a major version, most sequences contain a list of AsnOptionalParam.
 *
 * Because of the generic nature of the this sequence, the documention of the content is defined in the sections
 * using the AsnOptionalParam.
 */
export class AsnOptionalParam {
	public constructor(that: AsnOptionalParam) {
		Object.assign(this, that);
	}

	private static initEmpty(): AsnOptionalParam {
		return new AsnOptionalParam({
			key: "",
			value: AsnOptionalParamChoice["initEmpty"].call(0)
		});
	}

	public static getOwnPropertyNames(bIncludeOptionals: boolean = true): string[] {
		const p = [
			"key",
			"value"
		];
		return p;
	}

	public static type = "AsnOptionalParam";

	public static getASN1Schema(params?: asn1ts.ConstructedParams): asn1ts.Sequence {
		return new asn1ts.Sequence({
			name: "AsnOptionalParam",
			...params,
			value: [
				new asn1ts.Utf8String({ name: "key" }),
				AsnOptionalParamChoice.getASN1Schema({ name: "value" }),
				new asn1ts.Extension()
			]
		});
	}

	public key!: string;
	public value!: AsnOptionalParamChoice;
}

// [PrintTSTypeDefCode]
// [PrintTSSetOfDefCode]
// [PrintTSListClass]
export class AsnStringPairList extends Array<AsnStringPair> {
	public static getASN1Schema(params?: asn1ts.SequenceOfParams): asn1ts.SequenceOf {
		return new asn1ts.SequenceOf({
			...params,
			value: AsnStringPair.getASN1Schema()
		});
	}
}

// [PrintTSTypeDefCode]
// [PrintTSSeqDefCode]
/**
 * Header data for a RFC-7519 JSON Web Token
 */
export class AsnJSONWebTokenHeader {
	public constructor(that: AsnJSONWebTokenHeader) {
		Object.assign(this, that);
	}

	private static initEmpty(): AsnJSONWebTokenHeader {
		return new AsnJSONWebTokenHeader({
			u8sTYP: "",
			u8sALG: ""
		});
	}

	public static getOwnPropertyNames(bIncludeOptionals: boolean = true): string[] {
		const p = [
			"u8sTYP",
			"u8sALG"
		];
		if (bIncludeOptionals) {
			p.push(
				"u8sCTY",
				"optionals"
			);
		}
		return p;
	}

	public static type = "AsnJSONWebTokenHeader";

	public static getASN1Schema(params?: asn1ts.ConstructedParams): asn1ts.Sequence {
		return new asn1ts.Sequence({
			name: "AsnJSONWebTokenHeader",
			...params,
			value: [
				new asn1ts.Utf8String({ name: "u8sTYP" }),
				new asn1ts.Utf8String({ name: "u8sALG" }),
				new asn1ts.Utf8String({ name: "u8sCTY", idBlock: { optionalID: 0 } }),
				AsnStringPairList.getASN1Schema({ name: "optionals", idBlock: { optionalID: 1 } }),
				new asn1ts.Extension()
			]
		});
	}

	/** Defines the type of the token. This is currently by default JWT to specify it as RFC-7519 JSON Web Token */
	public u8sTYP!: string;
	/** Identifies which algorithm is used to generate the signature, HS256 indicates that this token is signed using HMAC-SHA256. */
	public u8sALG!: string;
	/** Defines the content type if the typ is not JWT (JSON Web Token) */
	public u8sCTY?: string;
	public optionals?: AsnStringPairList;
}

// [PrintTSTypeDefCode]
// [PrintTSSeqDefCode]
/**
 * Payload data for a RFC-7519 JSON Web Token
 */
export class AsnJSONWebTokenPayLoad {
	public constructor(that: AsnJSONWebTokenPayLoad) {
		Object.assign(this, that);
	}

	private static initEmpty(): AsnJSONWebTokenPayLoad {
		return new AsnJSONWebTokenPayLoad({
			u8sJTI: ""
		});
	}

	public static getOwnPropertyNames(bIncludeOptionals: boolean = true): string[] {
		const p = [
			"u8sJTI"
		];
		if (bIncludeOptionals) {
			p.push(
				"u8sISS",
				"u8sSUB",
				"u8sAUD",
				"utcEXP",
				"utcNBF",
				"utcIAT",
				"iUserType",
				"optionals"
			);
		}
		return p;
	}

	public static type = "AsnJSONWebTokenPayLoad";

	public static getASN1Schema(params?: asn1ts.ConstructedParams): asn1ts.Sequence {
		return new asn1ts.Sequence({
			name: "AsnJSONWebTokenPayLoad",
			...params,
			value: [
				new asn1ts.Utf8String({ name: "u8sISS", idBlock: { optionalID: 0 } }),
				new asn1ts.Utf8String({ name: "u8sSUB", idBlock: { optionalID: 1 } }),
				new asn1ts.Utf8String({ name: "u8sAUD", idBlock: { optionalID: 2 } }),
				new asn1ts.Real({ name: "utcEXP", idBlock: { optionalID: 3 } }),
				new asn1ts.Real({ name: "utcNBF", idBlock: { optionalID: 4 } }),
				new asn1ts.Real({ name: "utcIAT", idBlock: { optionalID: 5 } }),
				new asn1ts.Utf8String({ name: "u8sJTI" }),
				new asn1ts.Integer({ name: "iUserType", idBlock: { optionalID: 6 } }),
				AsnStringPairList.getASN1Schema({ name: "optionals", idBlock: { optionalID: 7 } }),
				new asn1ts.Extension()
			]
		});
	}

	/** Identifies principal that issued the JWT. */
	public u8sISS?: string;
	/** Identifies the subject of the JWT. */
	public u8sSUB?: string;
	/** Identifies the recipients that the JWT is intended for. Each principal intended to process the JWT must identify itself with a value in the audience claim. If the principal processing the claim does not identify itself with a value in the aud claim when this claim is present, then the JWT must be rejected. */
	public u8sAUD?: string;
	/** Identifies the expiration time on and after which the JWT must not be accepted for processing. The value must be a NumericDate[10]: either an integer or decimal, representing seconds past 1970-01-01 00:00:00Z. */
	public utcEXP?: Date;
	/** Identifies the time on which the JWT will start to be accepted for processing. The value must be a NumericDate. */
	public utcNBF?: Date;
	/** Identifies the time at which the JWT was issued. The value must be a NumericDate. */
	public utcIAT?: Date;
	/** Case sensitive unique identifier of the token even among different issuers. */
	public u8sJTI!: string;
	/** Defines a user type in the context of using the token. The implementer has to define a enum value list this value belongs to */
	public iUserType?: number;
	public optionals?: AsnStringPairList;
}

// [PrintTSTypeDefCode]
// [PrintTSSetOfDefCode]
// [PrintTSListClass]
export class AsnOptionalParameters extends Array<AsnOptionalParam> {
	public static getASN1Schema(params?: asn1ts.SequenceOfParams): asn1ts.SequenceOf {
		return new asn1ts.SequenceOf({
			...params,
			value: AsnOptionalParam.getASN1Schema()
		});
	}
}

// [PrintTSTypeDefCode]
// [PrintTSSeqDefCode]
/**
 * Sequence\/object containing contact data.
 * AsnNetDatabaseContact is widly used almost everywhere in the API. It contains the contact data of a contact and from which databse it comes from.
 * All fields in this sequence are optional, because not every field must have content, but the sequence is broadly used, so every field which is
 * not transmitted saves bandwidth.
 */
export class AsnNetDatabaseContact {
	public constructor(that?: AsnNetDatabaseContact) {
		Object.assign(this, that);
	}

	private static initEmpty(): AsnNetDatabaseContact {
		return new AsnNetDatabaseContact();
	}

	public static getOwnPropertyNames(bIncludeOptionals: boolean = true): string[] {
		const p: string[] = [];
		if (bIncludeOptionals) {
			p.push(
				"u8sFound",
				"u8sDatabaseName",
				"u8sEntryIDDB",
				"u8sEntryIDStore",
				"u8sEntryID",
				"u8sCtiServerUserName",
				"u8sDisplayName",
				"u8sFirstName",
				"u8sLastName",
				"u8sJobTitle",
				"u8sCompany",
				"u8sDepartment",
				"u8sOfficeRoomNumber",
				"u8sCustomerID",
				"u8sBusinessAddressStreet",
				"u8sBusinessAddressPostalCode",
				"u8sBusinessAddressCity",
				"u8sBusinessAddressState",
				"u8sBusinessAddressCountry",
				"u8sPrivateAddressStreet",
				"u8sPrivateAddressPostalCode",
				"u8sPrivateAddressCity",
				"u8sPrivateAddressState",
				"u8sPrivateAddressCountry",
				"u8sOtherAddressStreet",
				"u8sOtherAddressPostalCode",
				"u8sOtherAddressCity",
				"u8sOtherAddressState",
				"u8sOtherAddressCountry",
				"u8sEMail",
				"u8sEMail2",
				"u8sEMail3",
				"u8sSIPAddress",
				"u8sWebPageURL",
				"u8sPhoneBusiness",
				"u8sPhoneBusiness2",
				"u8sCompanyMainTelephoneNumber",
				"u8sAssistantTelephoneNumber",
				"u8sPhoneHome",
				"u8sPhoneHome2",
				"u8sPrimaryTelephoneNumber",
				"u8sPhoneMobile",
				"u8sCarTelephoneNumber",
				"u8sRadioTelephoneNumber",
				"u8sPagerTelephoneNumber",
				"u8sOtherTelephoneNumber",
				"u8sCallbackTelephoneNumber",
				"u8sISDNTelephoneNumber",
				"u8sTTYTTDTelephoneNumber",
				"u8sFaxBusiness",
				"u8sFaxHome",
				"u8sBody",
				"u8sDirectWebLink",
				"customFields",
				"bValid",
				"iPrivateContact",
				"iCtiServerUser",
				"optionalParams"
			);
		}
		return p;
	}

	public static type = "AsnNetDatabaseContact";

	public static getASN1Schema(params?: asn1ts.ConstructedParams): asn1ts.Sequence {
		return new asn1ts.Sequence({
			name: "AsnNetDatabaseContact",
			...params,
			value: [
				new asn1ts.Utf8String({ name: "u8sFound", idBlock: { optionalID: 0 } }),
				new asn1ts.Utf8String({ name: "u8sDatabaseName", idBlock: { optionalID: 1 } }),
				new asn1ts.Utf8String({ name: "u8sEntryIDDB", idBlock: { optionalID: 2 } }),
				new asn1ts.Utf8String({ name: "u8sEntryIDStore", idBlock: { optionalID: 3 } }),
				new asn1ts.Utf8String({ name: "u8sEntryID", idBlock: { optionalID: 4 } }),
				new asn1ts.Utf8String({ name: "u8sCtiServerUserName", idBlock: { optionalID: 5 } }),
				new asn1ts.Utf8String({ name: "u8sDisplayName", idBlock: { optionalID: 6 } }),
				new asn1ts.Utf8String({ name: "u8sFirstName", idBlock: { optionalID: 7 } }),
				new asn1ts.Utf8String({ name: "u8sLastName", idBlock: { optionalID: 8 } }),
				new asn1ts.Utf8String({ name: "u8sJobTitle", idBlock: { optionalID: 9 } }),
				new asn1ts.Utf8String({ name: "u8sCompany", idBlock: { optionalID: 10 } }),
				new asn1ts.Utf8String({ name: "u8sDepartment", idBlock: { optionalID: 11 } }),
				new asn1ts.Utf8String({ name: "u8sOfficeRoomNumber", idBlock: { optionalID: 12 } }),
				new asn1ts.Utf8String({ name: "u8sCustomerID", idBlock: { optionalID: 13 } }),
				new asn1ts.Utf8String({ name: "u8sBusinessAddressStreet", idBlock: { optionalID: 14 } }),
				new asn1ts.Utf8String({ name: "u8sBusinessAddressPostalCode", idBlock: { optionalID: 15 } }),
				new asn1ts.Utf8String({ name: "u8sBusinessAddressCity", idBlock: { optionalID: 16 } }),
				new asn1ts.Utf8String({ name: "u8sBusinessAddressState", idBlock: { optionalID: 17 } }),
				new asn1ts.Utf8String({ name: "u8sBusinessAddressCountry", idBlock: { optionalID: 18 } }),
				new asn1ts.Utf8String({ name: "u8sPrivateAddressStreet", idBlock: { optionalID: 19 } }),
				new asn1ts.Utf8String({ name: "u8sPrivateAddressPostalCode", idBlock: { optionalID: 20 } }),
				new asn1ts.Utf8String({ name: "u8sPrivateAddressCity", idBlock: { optionalID: 21 } }),
				new asn1ts.Utf8String({ name: "u8sPrivateAddressState", idBlock: { optionalID: 22 } }),
				new asn1ts.Utf8String({ name: "u8sPrivateAddressCountry", idBlock: { optionalID: 23 } }),
				new asn1ts.Utf8String({ name: "u8sOtherAddressStreet", idBlock: { optionalID: 24 } }),
				new asn1ts.Utf8String({ name: "u8sOtherAddressPostalCode", idBlock: { optionalID: 25 } }),
				new asn1ts.Utf8String({ name: "u8sOtherAddressCity", idBlock: { optionalID: 26 } }),
				new asn1ts.Utf8String({ name: "u8sOtherAddressState", idBlock: { optionalID: 27 } }),
				new asn1ts.Utf8String({ name: "u8sOtherAddressCountry", idBlock: { optionalID: 28 } }),
				new asn1ts.Utf8String({ name: "u8sEMail", idBlock: { optionalID: 29 } }),
				new asn1ts.Utf8String({ name: "u8sEMail2", idBlock: { optionalID: 30 } }),
				new asn1ts.Utf8String({ name: "u8sEMail3", idBlock: { optionalID: 31 } }),
				new asn1ts.Utf8String({ name: "u8sSIPAddress", idBlock: { optionalID: 32 } }),
				new asn1ts.Utf8String({ name: "u8sWebPageURL", idBlock: { optionalID: 33 } }),
				new asn1ts.Utf8String({ name: "u8sPhoneBusiness", idBlock: { optionalID: 34 } }),
				new asn1ts.Utf8String({ name: "u8sPhoneBusiness2", idBlock: { optionalID: 35 } }),
				new asn1ts.Utf8String({ name: "u8sCompanyMainTelephoneNumber", idBlock: { optionalID: 36 } }),
				new asn1ts.Utf8String({ name: "u8sAssistantTelephoneNumber", idBlock: { optionalID: 37 } }),
				new asn1ts.Utf8String({ name: "u8sPhoneHome", idBlock: { optionalID: 38 } }),
				new asn1ts.Utf8String({ name: "u8sPhoneHome2", idBlock: { optionalID: 39 } }),
				new asn1ts.Utf8String({ name: "u8sPrimaryTelephoneNumber", idBlock: { optionalID: 40 } }),
				new asn1ts.Utf8String({ name: "u8sPhoneMobile", idBlock: { optionalID: 41 } }),
				new asn1ts.Utf8String({ name: "u8sCarTelephoneNumber", idBlock: { optionalID: 42 } }),
				new asn1ts.Utf8String({ name: "u8sRadioTelephoneNumber", idBlock: { optionalID: 43 } }),
				new asn1ts.Utf8String({ name: "u8sPagerTelephoneNumber", idBlock: { optionalID: 44 } }),
				new asn1ts.Utf8String({ name: "u8sOtherTelephoneNumber", idBlock: { optionalID: 45 } }),
				new asn1ts.Utf8String({ name: "u8sCallbackTelephoneNumber", idBlock: { optionalID: 46 } }),
				new asn1ts.Utf8String({ name: "u8sISDNTelephoneNumber", idBlock: { optionalID: 47 } }),
				new asn1ts.Utf8String({ name: "u8sTTYTTDTelephoneNumber", idBlock: { optionalID: 48 } }),
				new asn1ts.Utf8String({ name: "u8sFaxBusiness", idBlock: { optionalID: 49 } }),
				new asn1ts.Utf8String({ name: "u8sFaxHome", idBlock: { optionalID: 50 } }),
				new asn1ts.Utf8String({ name: "u8sBody", idBlock: { optionalID: 51 } }),
				new asn1ts.Utf8String({ name: "u8sDirectWebLink", idBlock: { optionalID: 52 } }),
				AsnStringPairList.getASN1Schema({ name: "customFields", idBlock: { optionalID: 53 } }),
				new asn1ts.Boolean({ name: "bValid", idBlock: { optionalID: 54 } }),
				new asn1ts.Integer({ name: "iPrivateContact", idBlock: { optionalID: 55 } }),
				new asn1ts.Integer({ name: "iCtiServerUser", idBlock: { optionalID: 56 } }),
				AsnOptionalParameters.getASN1Schema({ name: "optionalParams", idBlock: { optionalID: 57 } }),
				new asn1ts.Extension()
			]
		});
	}

	public u8sFound?: string;
	public u8sDatabaseName?: string;
	public u8sEntryIDDB?: string;
	public u8sEntryIDStore?: string;
	public u8sEntryID?: string;
	public u8sCtiServerUserName?: string;
	public u8sDisplayName?: string;
	public u8sFirstName?: string;
	public u8sLastName?: string;
	public u8sJobTitle?: string;
	public u8sCompany?: string;
	public u8sDepartment?: string;
	public u8sOfficeRoomNumber?: string;
	public u8sCustomerID?: string;
	public u8sBusinessAddressStreet?: string;
	public u8sBusinessAddressPostalCode?: string;
	public u8sBusinessAddressCity?: string;
	public u8sBusinessAddressState?: string;
	public u8sBusinessAddressCountry?: string;
	public u8sPrivateAddressStreet?: string;
	public u8sPrivateAddressPostalCode?: string;
	public u8sPrivateAddressCity?: string;
	public u8sPrivateAddressState?: string;
	public u8sPrivateAddressCountry?: string;
	public u8sOtherAddressStreet?: string;
	public u8sOtherAddressPostalCode?: string;
	public u8sOtherAddressCity?: string;
	public u8sOtherAddressState?: string;
	public u8sOtherAddressCountry?: string;
	public u8sEMail?: string;
	public u8sEMail2?: string;
	public u8sEMail3?: string;
	public u8sSIPAddress?: string;
	public u8sWebPageURL?: string;
	public u8sPhoneBusiness?: string;
	public u8sPhoneBusiness2?: string;
	public u8sCompanyMainTelephoneNumber?: string;
	public u8sAssistantTelephoneNumber?: string;
	public u8sPhoneHome?: string;
	public u8sPhoneHome2?: string;
	public u8sPrimaryTelephoneNumber?: string;
	public u8sPhoneMobile?: string;
	public u8sCarTelephoneNumber?: string;
	public u8sRadioTelephoneNumber?: string;
	public u8sPagerTelephoneNumber?: string;
	public u8sOtherTelephoneNumber?: string;
	public u8sCallbackTelephoneNumber?: string;
	public u8sISDNTelephoneNumber?: string;
	public u8sTTYTTDTelephoneNumber?: string;
	public u8sFaxBusiness?: string;
	public u8sFaxHome?: string;
	/** thats also called \"note\" */
	public u8sBody?: string;
	public u8sDirectWebLink?: string;
	/** additional fields, mostly filled with runtime information */
	public customFields?: AsnStringPairList;
	public bValid?: boolean;
	public iPrivateContact?: number;
	public iCtiServerUser?: number;
	/**
	 * Additionally fields added later in the key-value-field for backward compatibility.
	 * - key \"jpegPhoto\", Type byte[], a jpeg photo of the contact; Note: it is planned to not transmit the photo with
	 * the contact but a hash code instead in the future.
	 */
	public optionalParams?: AsnOptionalParameters;
}

// [PrintTSTypeDefCode]
// [PrintTSSetOfDefCode]
// [PrintTSListClass]
export class AsnIntegerPairList extends Array<AsnIntegerPair> {
	public static getASN1Schema(params?: asn1ts.SequenceOfParams): asn1ts.SequenceOf {
		return new asn1ts.SequenceOf({
			...params,
			value: AsnIntegerPair.getASN1Schema()
		});
	}
}

// [PrintTSTypeDefCode]
// [PrintTSSetOfDefCode]
// [PrintTSListClass]
export class AsnStringIntegerPairList extends Array<AsnStringIntegerPair> {
	public static getASN1Schema(params?: asn1ts.SequenceOfParams): asn1ts.SequenceOf {
		return new asn1ts.SequenceOf({
			...params,
			value: AsnStringIntegerPair.getASN1Schema()
		});
	}
}

// [PrintTSTypeDefCode]
// [PrintTSSetOfDefCode]
// [PrintTSListClass]
export class AsnRequestErrorList extends Array<AsnRequestError> {
	public static getASN1Schema(params?: asn1ts.SequenceOfParams): asn1ts.SequenceOf {
		return new asn1ts.SequenceOf({
			...params,
			value: AsnRequestError.getASN1Schema()
		});
	}
}

// [PrintTSTypeDefCode]
// [PrintTSSetOfDefCode]
// [PrintTSListClass]
export class SEQInteger extends Array<number> {
	public static getASN1Schema(params?: asn1ts.SequenceOfParams): asn1ts.SequenceOf {
		return new asn1ts.SequenceOf({
			...params,
			value: new asn1ts.Integer()
		});
	}
}

// [PrintTSTypeDefCode]
// [PrintTSSetOfDefCode]
// [PrintTSListClass]
export class UTF8StringList extends Array<string> {
	public static getASN1Schema(params?: asn1ts.SequenceOfParams): asn1ts.SequenceOf {
		return new asn1ts.SequenceOf({
			...params,
			value: new asn1ts.Utf8String()
		});
	}
}

// [PrintTSTypeDefCode]
// [PrintTSSetOfDefCode]
// [PrintTSListClass]
export class AsnNetDatabaseContactList extends Array<AsnNetDatabaseContact> {
	public static getASN1Schema(params?: asn1ts.SequenceOfParams): asn1ts.SequenceOf {
		return new asn1ts.SequenceOf({
			...params,
			value: AsnNetDatabaseContact.getASN1Schema()
		});
	}
}

// [PrintTSTypeDefCode]
// [PrintTSSetOfDefCode]
// [PrintTSListClass]
export class AsnNetDatabaseContactIDList extends Array<AsnNetDatabaseContactID> {
	public static getASN1Schema(params?: asn1ts.SequenceOfParams): asn1ts.SequenceOf {
		return new asn1ts.SequenceOf({
			...params,
			value: AsnNetDatabaseContactID.getASN1Schema()
		});
	}
}

// [PrintTSTypeDefCode]
// [PrintTSSeqDefCode]
/**
 * Property bag definition for storing runtime defined content for objects (like users, computers, groups etc.).
 * To make it possible to transport and store runtime defined content in some objects, these object contain
 * a property bag, which is simply a key value store. Which keys (and therefor which data) is available
 * is depending on the object and the business logic, which uses it. So see the according sequence (object) documentation
 * for more details.
 */
export class AsnUserPropertyBag {
	public constructor(that: AsnUserPropertyBag) {
		Object.assign(this, that);
	}

	private static initEmpty(): AsnUserPropertyBag {
		return new AsnUserPropertyBag({
			asnProperties: new AsnStringPairList()
		});
	}

	public static getOwnPropertyNames(bIncludeOptionals: boolean = true): string[] {
		const p = [
			"asnProperties"
		];
		return p;
	}

	public static type = "AsnUserPropertyBag";

	public static getASN1Schema(params?: asn1ts.ConstructedParams): asn1ts.Sequence {
		return new asn1ts.Sequence({
			name: "AsnUserPropertyBag",
			...params,
			value: [
				AsnStringPairList.getASN1Schema({ name: "asnProperties" }),
				new asn1ts.Extension()
			]
		});
	}

	public asnProperties!: AsnStringPairList;
}

// [PrintTSTypeDefCode]
// [PrintTSSeqDefCode]
/**
 * Data for a RFC-7519 JSON Web Token
 */
export class AsnJSONWebToken {
	public constructor(that: AsnJSONWebToken) {
		Object.assign(this, that);
	}

	private static initEmpty(): AsnJSONWebToken {
		return new AsnJSONWebToken({
			header: AsnJSONWebTokenHeader["initEmpty"].call(0),
			payload: AsnJSONWebTokenPayLoad["initEmpty"].call(0),
			signature: ""
		});
	}

	public static getOwnPropertyNames(bIncludeOptionals: boolean = true): string[] {
		const p = [
			"header",
			"payload",
			"signature"
		];
		return p;
	}

	public static type = "AsnJSONWebToken";

	public static getASN1Schema(params?: asn1ts.ConstructedParams): asn1ts.Sequence {
		return new asn1ts.Sequence({
			name: "AsnJSONWebToken",
			...params,
			value: [
				AsnJSONWebTokenHeader.getASN1Schema({ name: "header" }),
				AsnJSONWebTokenPayLoad.getASN1Schema({ name: "payload" }),
				new asn1ts.Utf8String({ name: "signature" }),
				new asn1ts.Extension()
			]
		});
	}

	/** The header content of the JWT */
	public header!: AsnJSONWebTokenHeader;
	/** The body content of the JWT */
	public payload!: AsnJSONWebTokenPayLoad;
	/** The signature of the base64 encoded header and payload */
	public signature!: string;
}
