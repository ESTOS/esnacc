// math.test.ts

import { Sequence } from "@estos/asn1ts";
import * as axios from "axios";

import { theConfig } from "../src/globals.js";
import { AsnGetSettingsArgument, AsnGetSettingsResult, AsnSetSettingsArgument, AsnSetSettingsResult } from "../src/stub/ENetUC_Settings_Manager.js";
import { AsnGetSettingsArgument_Converter, AsnGetSettingsResult_Converter, AsnSetSettingsArgument_Converter, AsnSetSettingsResult_Converter } from "../src/stub/ENetUC_Settings_Manager_Converter.js";
import { ROSEReject } from "../src/stub/SNACCROSE.js";
import { ROSEReject_Converter } from "../src/stub/SNACCROSE_Converter.js";
import { ConverterErrors, DecodeContext, EncodeContext } from "../src/stub/TSConverterBase.js";
import { EASN1TransportEncoding } from "../src/stub/TSInvokeContext.js";
import { ROSEBase, asn1Decode, asn1Encode } from "../src/stub/TSROSEBase.js";

const theAxios = axios.default;

/**
 * Retrieves the method for a http post operation
 *
 * @param method - the method we want to call
 * @returns the URL
 */
function getURL(method: string): string {
	let strURL = "http://";
	strURL += theConfig.econfServerFDQN;
	strURL += ":";
	strURL += theConfig.econfServerListenPortTCP;
	strURL += "/rest/";
	strURL += method;
	return strURL;
}

/**
 * Returns a default EncodeContext
 *
 * @returns the encode context
 */
function getEncodeContext(): EncodeContext {
	return new EncodeContext({
		// If encoding lax the encoder will sum up errors but will not stop encoding, errors are collected but the function always returns true
		bLaxEncoding: false,
		// to get naked code (not pretty printed with newlines, and tabs) set this to false
		bPrettyPrint: true,
		// Adds the type name to the generated output as _type="Typename"
		bAddTypes: true,
		// Encodes optional params in the hand coded UCServer notation
		bUCServerOptionalParams: false
	});
}

/**
 * Returns a default EncodeContext
 *
 * @returns the encode context
 */
function getDecodeContext(): DecodeContext {
	return new DecodeContext({
		// If decoding lax the parser will sum up errors but will not stop parsing, errors are collected but the function always returns true
		bLaxDecoding: false
	});
}

const jsonHeader = {
	headers: {
		"Content-Type": "application/json",
		Accept: "application/json"
	}
};

const berHeader: axios.AxiosRequestConfig = {
	responseType: "arraybuffer",
	headers: {
		"Content-Type": "application/octet-stream",
		Accept: "application/octet-stream"
	}
};

describe("Test HTTP Post RPC calls", () => {
	test("asnGetSettings JSON encoded payload", async () => {
		const argument = new AsnGetSettingsArgument();
		const errors = new ConverterErrors();
		const encoded = asn1Encode(EASN1TransportEncoding.JSON, argument, AsnGetSettingsArgument_Converter, errors, getEncodeContext());
		expect(errors.length).toBe(0);
		expect(encoded).toBeDefined();
		expect(encoded).toBeInstanceOf(Object);
		if (encoded) {
			const data = ROSEBase.encodeToTransport(encoded, getEncodeContext());
			const response = await theAxios.post(getURL("asnGetSettings"), data.payLoad, jsonHeader);
			expect(response.status).toBe(200);
			expect(response.data).toBeDefined();
			expect(response.data).toBeInstanceOf(Object);
			const decoded = asn1Decode<AsnGetSettingsResult>(response.data, AsnGetSettingsResult_Converter, errors, getDecodeContext());
			expect(errors.length).toBe(0);
			expect(decoded).toBeInstanceOf(AsnGetSettingsResult);
		}
	});

	test("asnGetSettings BER encoded payload", async () => {
		const argument = new AsnGetSettingsArgument();
		const errors = new ConverterErrors();
		const encoded = asn1Encode(EASN1TransportEncoding.BER, argument, AsnGetSettingsArgument_Converter, errors, getEncodeContext());
		expect(encoded).toBeDefined();
		expect(encoded).toBeInstanceOf(Sequence);
		expect(errors.length).toBe(0);
		if (encoded) {
			const data = ROSEBase.encodeToTransport(encoded, getEncodeContext());
			const response = await theAxios.post(getURL("asnGetSettings"), data.payLoad, berHeader);
			expect(response.status).toBe(200);
			expect(response.data).toBeDefined();
			expect(response.data).toBeInstanceOf(Uint8Array);
			const decoded = asn1Decode<AsnGetSettingsResult>(response.data, AsnGetSettingsResult_Converter, errors, getDecodeContext());
			expect(errors.length).toBe(0);
			expect(decoded).toBeInstanceOf(AsnGetSettingsResult);
		}
	});

	test("asnGetSettings JSON no payload", async () => {
		const response = await theAxios.post(getURL("asnGetSettings"), undefined, jsonHeader);
		expect(response.status).toBe(200);
		expect(response.data).toBeDefined();
		expect(response.data).toBeInstanceOf(Object);
		const errors = new ConverterErrors();
		const decoded = asn1Decode<AsnGetSettingsResult>(response.data, AsnGetSettingsResult_Converter, errors, getDecodeContext());
		expect(response.data instanceof Object).toBe(true);
		expect(errors.length).toBe(0);
		expect(decoded).toBeInstanceOf(AsnGetSettingsResult);
	});

	test("asnGetSettings BER no payload", async () => {
		const response = await theAxios.post(getURL("asnGetSettings"), undefined, berHeader);
		expect(response.status).toBe(200);
		expect(response.data).toBeDefined();
		expect(response.data).toBeInstanceOf(Uint8Array);
		const errors = new ConverterErrors();
		const decoded = asn1Decode<AsnGetSettingsResult>(response.data, AsnGetSettingsResult_Converter, errors, getDecodeContext());
		expect(response.data instanceof Uint8Array).toBe(true);
		expect(errors.length).toBe(0);
		expect(decoded).toBeInstanceOf(AsnGetSettingsResult);
	});

	test("calling not existing method in JSON", async () => {
		const url = getURL("notExistingMethod");
		try {
			await theAxios.post(url, undefined, jsonHeader);
			fail("Should not have returned successfully");
		} catch (error: unknown) {
			const axiosError = error as axios.AxiosError;
			expect(axiosError.response).toBeDefined();
			if (axiosError.response) {
				expect(axiosError.response.status).toBe(500);
				expect(axiosError.response.data).toBeInstanceOf(Object);
				const errors = new ConverterErrors();
				const decoded = asn1Decode<ROSEReject>(axiosError.response.data as object, ROSEReject_Converter, errors, getDecodeContext()) as ROSEReject;
				expect(decoded).toBeInstanceOf(ROSEReject);
				expect(decoded.reject?.invokeProblem).toBe(1);
			}
			console.log(error);
		}
	});

	test("calling not existing method in BER", async () => {
		const url = getURL("notExistingMethod");
		try {
			await theAxios.post(url, undefined, berHeader);
			fail("Should not have returned successfully");
		} catch (error: unknown) {
			const axiosError = error as axios.AxiosError;
			expect(axiosError.response).toBeDefined();
			if (axiosError.response) {
				expect(axiosError.response.status).toBe(500);
				expect(axiosError.response.data).toBeInstanceOf(Uint8Array);
				const errors = new ConverterErrors();
				const decoded = asn1Decode<ROSEReject>(axiosError.response.data as Uint8Array, ROSEReject_Converter, errors, getDecodeContext()) as ROSEReject;
				expect(decoded).toBeInstanceOf(ROSEReject);
				expect(decoded.reject?.invokeProblem).toBe(1);
			}
		}
	});

	test("asnSetSettings asnGetSettings JSON encoded payload", async () => {
		const setArgument = new AsnSetSettingsArgument({
			settings: {
				bEnabled: true,
				u8sUsername: "test"
			}
		});
		const errors = new ConverterErrors();
		const transportEncodedSetArgument = asn1Encode(EASN1TransportEncoding.JSON, setArgument, AsnSetSettingsArgument_Converter, errors, getEncodeContext());
		expect(errors.length).toBe(0);
		expect(transportEncodedSetArgument).toBeDefined();
		expect(transportEncodedSetArgument).toBeInstanceOf(Object);
		if (transportEncodedSetArgument) {
			const data = ROSEBase.encodeToTransport(transportEncodedSetArgument, getEncodeContext());
			const setResponse = await theAxios.post(getURL("asnSetSettings"), data.payLoad, jsonHeader);
			expect(setResponse.status).toBe(200);
			expect(setResponse.data).toBeDefined();
			expect(setResponse.data).toBeInstanceOf(Object);
			const decodedSetResponse = asn1Decode<AsnSetSettingsResult>(setResponse.data, AsnSetSettingsResult_Converter, errors, getDecodeContext());
			expect(errors.length).toBe(0);
			expect(decodedSetResponse).toBeInstanceOf(AsnSetSettingsResult);

			const getArgument = new AsnGetSettingsArgument();
			const transportEncodedGetArgument = asn1Encode(EASN1TransportEncoding.JSON, getArgument, AsnGetSettingsArgument_Converter, errors, getEncodeContext());
			expect(errors.length).toBe(0);
			expect(transportEncodedGetArgument).toBeDefined();
			expect(transportEncodedGetArgument).toBeInstanceOf(Object);
			if (transportEncodedGetArgument) {
				const data = ROSEBase.encodeToTransport(transportEncodedGetArgument, getEncodeContext());
				const getResponse = await theAxios.post(getURL("asnGetSettings"), data.payLoad, jsonHeader);
				expect(getResponse.status).toBe(200);
				expect(getResponse.data).toBeDefined();
				expect(getResponse.data).toBeInstanceOf(Object);
				const decodedGetResponse = asn1Decode<AsnGetSettingsResult>(getResponse.data, AsnGetSettingsResult_Converter, errors, getDecodeContext());
				expect(errors.length).toBe(0);
				expect(decodedGetResponse).toBeInstanceOf(AsnGetSettingsResult);
				expect(decodedGetResponse?.settings.bEnabled).toBe(true);
				expect(decodedGetResponse?.settings.u8sUsername).toBe("test");
			}
		}
	});

	test("asnSetSettings asnGetSettings BER encoded payload", async () => {
		const setArgument = new AsnSetSettingsArgument({
			settings: {
				bEnabled: true,
				u8sUsername: "test"
			}
		});
		const errors = new ConverterErrors();
		const transportEncodedSetArgument = asn1Encode(EASN1TransportEncoding.BER, setArgument, AsnSetSettingsArgument_Converter, errors, getEncodeContext());
		expect(errors.length).toBe(0);
		expect(transportEncodedSetArgument).toBeDefined();
		expect(transportEncodedSetArgument).toBeInstanceOf(Sequence);
		if (transportEncodedSetArgument) {
			const data = ROSEBase.encodeToTransport(transportEncodedSetArgument, getEncodeContext());
			const setResponse = await theAxios.post(getURL("asnSetSettings"), data.payLoad, berHeader);
			expect(setResponse.status).toBe(200);
			expect(setResponse.data).toBeDefined();
			expect(setResponse.data).toBeInstanceOf(Uint8Array);
			const decodedSetResponse = asn1Decode<AsnSetSettingsResult>(setResponse.data, AsnSetSettingsResult_Converter, errors, getDecodeContext());
			expect(errors.length).toBe(0);
			expect(decodedSetResponse).toBeInstanceOf(AsnSetSettingsResult);

			const getArgument = new AsnGetSettingsArgument();
			const transportEncodedGetArgument = asn1Encode(EASN1TransportEncoding.BER, getArgument, AsnGetSettingsArgument_Converter, errors, getEncodeContext());
			expect(errors.length).toBe(0);
			expect(transportEncodedGetArgument).toBeDefined();
			expect(transportEncodedGetArgument).toBeInstanceOf(Sequence);
			if (transportEncodedGetArgument) {
				const data = ROSEBase.encodeToTransport(transportEncodedGetArgument, getEncodeContext());
				const getResponse = await theAxios.post(getURL("asnGetSettings"), data.payLoad, berHeader);
				expect(getResponse.status).toBe(200);
				expect(getResponse.data).toBeDefined();
				expect(getResponse.data).toBeInstanceOf(Uint8Array);
				const decodedGetResponse = asn1Decode<AsnGetSettingsResult>(getResponse.data, AsnGetSettingsResult_Converter, errors, getDecodeContext());
				expect(errors.length).toBe(0);
				expect(decodedGetResponse).toBeInstanceOf(AsnGetSettingsResult);
				expect(decodedGetResponse?.settings.bEnabled).toBe(true);
				expect(decodedGetResponse?.settings.u8sUsername).toBe("test");
			}
		}
	});
});
