import { existsSync, readFileSync } from "fs";
import { resolve } from "path";
import { defineConfig, loadEnv } from "vite";

export default defineConfig(({ command, mode }) => {
	// Load env file based on `mode` in the current working directory.
	// Set the third parameter to '' to load all env regardless of the `VITE_` prefix.
	const env = loadEnv(mode, process.cwd(), "");
	let useTLS = env.MICROSERVICE_CLIENT_LISTEN_TLS === "1" || env.MICROSERVICE_CLIENT_LISTEN_TLS === "true";
	const sslCertPath = resolve(__dirname, env.MICROSERVICE_CLIENT_CERTFILE);
	const sslKeyPath = resolve(__dirname, env.MICROSERVICE_CLIENT_KEYFILE);
	let port = parseInt(env.MICROSERVICE_CLIENT_LISTEN_PORT || "3000");
	let dnsName = env.MICROSERVICE_CLIENT_DNS_NAME || "localhost";

	if (useTLS && (!existsSync(sslCertPath) || !existsSync(sslKeyPath))) {
		console.warn("Listening is configured for TLS but either the cert or the key file is missing. Switching to TCP");
		useTLS = 0;
	}
	if (useTLS === 0 && port === 443)
		port = 80;

	if (useTLS) {
		dnsName = "https://" + dnsName;
		if (port !== 443)
			dnsName += ":" + port;
	}
	else {
		dnsName = "http://" + dnsName;
		if (port !== 80)
			dnsName += ":" + port;
	}

	return {
		server: {
			port: port,
			host: "0.0.0.0",
			open: dnsName,
			https: useTLS ? { key: readFileSync(sslKeyPath), cert: readFileSync(sslCertPath) } : false,
		},
		build: {
			rollupOptions: {
				// Add any build-specific configurations here
			},
		},
	};
});
