// This file is embedded as resource file in the esnacc.exe ASN1 Compiler
// Do NOT edit or modify this code as it is machine generated
// and will be overwritten with every code generation of the esnacc.exe

// dprint-ignore-file
/* eslint-disable */

import * as ENetUC_Common from "./ENetUC_Common.js";
import {
	ConverterError,
	ConverterErrors,
	ConverterErrorType,
	DecodeContext,
	EncodeContext,
	INamedType,
	TSConverter,
} from "./TSConverterBase.js";
import { roseDebugBreak } from "./TSBaseUtils.js";

/**
 * UCServer wire value for one optional parameter entry (integer, UTF-8 string, or base64 binary wrapper).
 */
export type IUCServerOptionalParam = number | string | { binarydata: string };

/**
 * UCServer map notation for AsnOptionalParameters — Redux-safe plain object keyed by parameter name.
 * Differs from IAsnOptionalParameters (ASN.1 array of IAsnOptionalParam) used by standard JSON encoding.
 */
export type IUCServerOptionalParameters = Record<string, IUCServerOptionalParam> & INamedType;

/**
 * Helper class that contains the code to do the custom conversion the UCServer is doing for the AsnOptionalParameters
 */
export class EAsnOptionalParametersConverter {
	/**
	 * Encodes an AsnOptionalParameters object to JSON string encoding in the proprietary Notation the UCServer is using
	 *
	 * @param obj - the optional parameters to encode
	 * @param errors - errors that occured while parsing
	 * @param context - the encoding context
	 * @param parametername - the parent parameter value name this object is held in
	 * @returns true on success
	 */
	public static toJSON(
		obj: ENetUC_Common.AsnOptionalParameters,
		errors?: ConverterErrors,
		context?: EncodeContext,
		parametername?: string,
	): IUCServerOptionalParameters | undefined {
		errors ||= new ConverterErrors();
		errors.storeState();
		const newContext = TSConverter.addEncodeContext(context, parametername, "AsnOptionalParameters");

		if (!newContext?.bUCServerOptionalParams) {
			roseDebugBreak();
			return undefined;
		}

		const result = {} as IUCServerOptionalParameters;
		if (newContext?.bAddTypes)
			result._type = "IUCServerOptionalParameters";

		for (const [id, element] of obj.entries()) {
			if (element) {
				if (element.value.integerdata !== undefined)
					result[element.key] = element.value.integerdata;
				else if (element.value.stringdata !== undefined)
					result[element.key] = element.value.stringdata;
				else if (element.value.binarydata !== undefined)
					result[element.key] = { binarydata: TSConverter.encode64(element.value.binarydata) };
				else {
					roseDebugBreak();
					errors.push(
						new ConverterError(
							ConverterErrorType.PROPERTY_TYPEMISMATCH,
							newContext.context,
							`Unable to read ${id}, not integer, not string, not binary`,
						),
					);
				}
			}
		}

		if (errors.validateResult(newContext, "AsnOptionalParameters"))
			return result;

		return undefined;
	}

	/**
	 * Encodes an AsnOptionalParam from the dataarg that holds the params in the proprietary Notation the UCServer is using
	 *
	 * @param dataarg - the argument to decode
	 * @param obj - the object that is returned
	 * @param errors - errors that occured while parsing
	 * @param context - the decoding context
	 * @param parametername - the parent parameter value name this object is held in
	 * @param optional - true if this is an optional object (no error on not existing)
	 * @returns true on success
	 */
	public static fromJSON(
		dataarg: string | object | undefined,
		obj: ENetUC_Common.AsnOptionalParameters,
		errors?: ConverterErrors,
		context?: DecodeContext,
		parametername?: string,
		optional?: boolean,
	): boolean {
		const bFirstCaller = context ? context.context === "" : false;
		if (errors == null)
			errors = new ConverterErrors();
		const newContext = TSConverter.addDecodeContext(context, parametername, "AsnOptionalParameters");

		if (dataarg == null) {
			if (!(optional === true)) {
				if (bFirstCaller)
					errors.push(new ConverterError(undefined, undefined, "Errors while decoding AsnOptionalParameters"));
				errors.push(new ConverterError(ConverterErrorType.PROPERTY_MISSING, newContext.context, "property missing"));
			}
			return false;
		}
		const data = TSConverter.prepareJSONData(dataarg, errors, newContext) as IUCServerOptionalParameters;
		if (data) {
			for (const key in data) {
				if (!Object.prototype.hasOwnProperty.call(data, key))
					continue;
				if (key === "_type")
					continue;
				const value = data[key];
				if (typeof value === "number")
					obj.push(new ENetUC_Common.AsnOptionalParam({ key, value: { integerdata: value } }));
				else if (typeof value === "string")
					obj.push(new ENetUC_Common.AsnOptionalParam({ key, value: { stringdata: value } }));
				else if (value?.binarydata) {
					obj.push(
						new ENetUC_Common.AsnOptionalParam({ key, value: { binarydata: TSConverter.decode64(value.binarydata) } }),
					);
				} else {
					errors.push(
						new ConverterError(
							ConverterErrorType.PROPERTY_TYPEMISMATCH,
							newContext.context,
							`Error while decoding AsnOptionalParameter ${key}`,
						),
					);
				}
			}
		}

		if (bFirstCaller && errors.length)
			errors.unshift(new ConverterError(undefined, undefined, "Errors while decoding AsnOptionalParameters"));

		return newContext.bLaxDecoding ? true : errors.length === 0;
	}
}
