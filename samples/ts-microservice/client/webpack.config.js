"use strict";
const fs = require('fs');
const path = require('path');
const dotenv = require("dotenv");
const wpdotenv = require("dotenv-webpack");
dotenv.config();

module.exports = function (env, args) {
	let https = false;
	if (process.env.MICROSERVICE_CLIENT_LISTEN_TLS && fs.existsSync(process.env.MICROSERVICE_CLIENT_KEYFILE) && fs.existsSync(process.env.MICROSERVICE_CLIENT_CERTFILE)) {
			https = {
					key: process.env.MICROSERVICE_CLIENT_KEYFILE,
					cert: process.env.MICROSERVICE_CLIENT_CERTFILE
			};
	}
	
	return {
		entry: './src/client.ts',
		devtool: 'source-map',
		mode: 'development',
		module: {
			rules: [
				{
					test: /\.ts?$/,
					use: 'ts-loader',
					exclude: /node_modules/,
				},
			],
		},
		resolve: {
			extensions: ['.tsx', '.ts', '.js'],
		},
		output: {
			filename: 'bundle.js',
			path: path.resolve(__dirname, 'dist'),
			libraryTarget: 'window'
		},
		devServer: {
			port: process.env.MICROSERVICE_CLIENT_LISTEN_PORT,
			hot: true,
			https: https,
			host: '::',
			allowedHosts: 'all',
			static: './',
			devMiddleware: {
				writeToDisk: true
			}
		},
		plugins: [
				new wpdotenv()
		]
	};
}