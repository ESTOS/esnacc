import assert from "node:assert/strict";
import { type ChildProcess, spawn } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { after, before, describe, test } from "node:test";
import { fileURLToPath } from "node:url";

import {
	assertInvokeResult,
	createEventIntegration,
	createSettingsIntegration,
	releaseIntegration,
	waitForCount,
} from "./integration_client.js";
import * as ENetUC_Event_Manager from "./stub/ENetUC_Event_Manager.js";
import * as ENetUC_Settings_Manager from "./stub/ENetUC_Settings_Manager.js";
import { EASN1TransportEncoding } from "./stub/TSInvokeContext.js";

const testDir = path.dirname(fileURLToPath(import.meta.url));
const nodeClientRoot = path.resolve(testDir, "..");
const nodeServerRoot = process.env.SNACC_TS_SERVER_ROOT ?? path.resolve(nodeClientRoot, "../node-server");
const testPort = Number(process.env.SNACC_TS_TEST_PORT ?? "13020");
const logDirectory = process.env.SNACC_TS_LOG_DIRECTORY ?? path.join(nodeServerRoot, "log", "integration-tests");

const transportEncodings = [{ label: "JSON", encoding: EASN1TransportEncoding.JSON }, {
	label: "BER",
	encoding: EASN1TransportEncoding.BER,
}] as const;

let serverProcess: ChildProcess | undefined;
let serverOutput = "";

function serverEnv(): NodeJS.ProcessEnv {
	return {
		...process.env,
		MICROSERVICE_ENVIRONMENT: "development",
		MICROSERVICE_LOG_LEVEL: "error",
		MICROSERVICE_LOG_TO_CONSOLE: "0",
		MICROSERVICE_LOG_DIRECTORY: logDirectory,
		MICROSERVICE_SERVER_FQDN: "localhost",
		MICROSERVICE_SERVER_LISTEN_PORT_TCP: String(testPort),
	};
}

function appendServerOutput(stream: NodeJS.WriteStream, chunk: Buffer): void {
	serverOutput += chunk.toString("utf8");
	if (process.env.SNACC_TS_VERBOSE === "1")
		stream.write(chunk);
}

async function waitForServerReady(timeoutMs = 60000): Promise<void> {
	const deadline = Date.now() + timeoutMs;
	const url = `http://127.0.0.1:${testPort}/echo`;

	while (Date.now() < deadline) {
		if (serverProcess?.exitCode !== null && serverProcess?.exitCode !== undefined) {
			throw new Error(
				`node-server exited before becoming ready (code ${serverProcess.exitCode}). Output:\n${serverOutput}`,
			);
		}

		try {
			const response = await fetch(url, { method: "GET" });
			if (response.ok)
				return;
		} catch {
			// Server not ready yet.
		}
		await new Promise((resolve) => setTimeout(resolve, 200));
	}

	throw new Error(`Server on port ${testPort} did not become ready in time. Output:\n${serverOutput}`);
}

async function startServer(): Promise<void> {
	fs.mkdirSync(logDirectory, { recursive: true });
	serverOutput = "";

	serverProcess = spawn(process.execPath, ["index.js"], {
		cwd: nodeServerRoot,
		env: serverEnv(),
		stdio: ["ignore", "pipe", "pipe"],
	});

	serverProcess.stdout?.on("data", (chunk: Buffer) => {
		appendServerOutput(process.stdout, chunk);
	});
	serverProcess.stderr?.on("data", (chunk: Buffer) => {
		appendServerOutput(process.stderr, chunk);
	});
	serverProcess.on("exit", (code, signal) => {
		if (code !== null && code !== 0) {
			serverOutput += `\n[node-server exited with code ${code}]`;
		}
		if (signal) {
			serverOutput += `\n[node-server terminated by signal ${signal}]`;
		}
	});

	await waitForServerReady();
}

async function stopServer(): Promise<void> {
	if (!serverProcess)
		return;

	const proc = serverProcess;
	serverProcess = undefined;

	if (proc.exitCode !== null) {
		proc.kill();
		return;
	}

	proc.kill("SIGTERM");
	await new Promise<void>((resolve) => {
		const timer = setTimeout(() => {
			proc.kill("SIGKILL");
			resolve();
		}, 5000);
		proc.once("exit", () => {
			clearTimeout(timer);
			resolve();
		});
	});
}

before(async () => {
	await startServer();
});

after(async () => {
	await stopServer();
});

for (const { label, encoding } of transportEncodings) {
	describe(`TypeScript ASN.1 integration (${label})`, { concurrency: 1 }, () => {
		test("REST asnGetSettings returns a result", async () => {
			const { client, settingsRose } = createSettingsIntegration(testPort, encoding);
			try {
				client.useRest();

				const response = await settingsRose.invoke_asnGetSettings(new ENetUC_Settings_Manager.AsnGetSettingsArgument());
				const result = assertInvokeResult(response, ENetUC_Settings_Manager.AsnGetSettingsResult, "asnGetSettings");
				assert.ok(result.settings);
			} finally {
				await releaseIntegration(client, { settingsRose });
			}
		});

		test("REST asnSetSettings round-trips Unicode username through asnGetSettings", async () => {
			const { client, settingsRose } = createSettingsIntegration(testPort, encoding);
			try {
				client.useRest();

				const username = "Müller-äöü";
				const setResponse = await settingsRose.invoke_asnSetSettings(
					new ENetUC_Settings_Manager.AsnSetSettingsArgument({ settings: { bEnabled: true, u8sUsername: username } }),
				);
				assertInvokeResult(setResponse, ENetUC_Settings_Manager.AsnSetSettingsResult, "asnSetSettings");

				const getResponse = await settingsRose.invoke_asnGetSettings(
					new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
				);
				const getResult = assertInvokeResult(
					getResponse,
					ENetUC_Settings_Manager.AsnGetSettingsResult,
					"asnGetSettings after Unicode set",
				);
				assert.equal(getResult.settings.u8sUsername, username);
			} finally {
				await releaseIntegration(client, { settingsRose });
			}
		});

		test("REST asnSetSettings round-trips through asnGetSettings", async () => {
			const { client, settingsRose } = createSettingsIntegration(testPort, encoding);
			try {
				client.useRest();

				const setResponse = await settingsRose.invoke_asnSetSettings(
					new ENetUC_Settings_Manager.AsnSetSettingsArgument({
						settings: { bEnabled: true, u8sUsername: "integration-user" },
					}),
				);
				assertInvokeResult(setResponse, ENetUC_Settings_Manager.AsnSetSettingsResult, "asnSetSettings");

				const getResponse = await settingsRose.invoke_asnGetSettings(
					new ENetUC_Settings_Manager.AsnGetSettingsArgument(),
				);
				const getResult = assertInvokeResult(
					getResponse,
					ENetUC_Settings_Manager.AsnGetSettingsResult,
					"asnGetSettings after set",
				);
				assert.equal(getResult.settings.bEnabled, true);
				assert.equal(getResult.settings.u8sUsername, "integration-user");
			} finally {
				await releaseIntegration(client, { settingsRose });
			}
		});

		test("WebSocket asnSetSettings dispatches asnSettingsChanged", async () => {
			const { client, collector, settingsRose } = createEventIntegration(testPort, { settings: true }, encoding);
			try {
				client.useWebSocket();
				assert.equal(await client.connect(), true);

				const setResponse = await settingsRose!.invoke_asnSetSettings(
					new ENetUC_Settings_Manager.AsnSetSettingsArgument({
						settings: { bEnabled: false, u8sUsername: "event-user" },
					}),
				);
				assertInvokeResult(setResponse, ENetUC_Settings_Manager.AsnSetSettingsResult, "asnSetSettings over ws");

				const events = await waitForCount(() => collector.settingsChangedEvents, 1);
				assert.equal(events[0]?.settings.bEnabled, false);
				assert.equal(events[0]?.settings.u8sUsername, "event-user");
			} finally {
				await releaseIntegration(client, { settingsRose });
			}
		});

		test("WebSocket asnCreateFancyEvents dispatches ordered fancy events", async () => {
			const { client, collector, eventRose } = createEventIntegration(testPort, { events: true }, encoding);
			try {
				client.useWebSocket();
				assert.equal(await client.connect(), true);

				const response = await eventRose!.invoke_asnCreateFancyEvents(
					new ENetUC_Event_Manager.AsnCreateFancyEventsArgument({ iEventDelay: 0, iEventCount: 3 }),
				);
				assertInvokeResult(response, ENetUC_Event_Manager.AsnCreateFancyEventsResult, "asnCreateFancyEvents");

				const events = await waitForCount(() => collector.fancyEvents, 3);
				assert.deepEqual(events.map((event) => [event.iEventCounter, event.iEventsLeft]), [[1, 2], [2, 1], [3, 0]]);
			} finally {
				await releaseIntegration(client, { eventRose });
			}
		});
	});
}
