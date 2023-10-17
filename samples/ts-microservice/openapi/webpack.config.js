"use strict";
const fs = require("fs");
const path = require("path");
const dotenv = require("dotenv");
const wpdotenv = require("dotenv-webpack");
const MiniCssExtractPlugin = require("mini-css-extract-plugin");

dotenv.config();

module.exports = function (env, args) {
    let https = false;

    return {
        entry: "./src/index.tsx",
        devtool: "source-map",
        mode: "development",
        module: {
            rules: [
                {
                    test: /\.tsx?$/,
                    use: "ts-loader",
                    exclude: /node_modules/,
                },
                {
                    test: /\.jsx?$/,
                    exclude: /node_modules/,
                    loader: "babel-loader",
                },
                {
                    test: /\.css$/i,
                    use: [MiniCssExtractPlugin.loader, "css-loader"],
                },
            ],
        },
        resolve: {
            extensions: [".tsx", ".ts", ".js"],
        },
        output: {
            filename: "bundle.js",
            path: path.resolve(__dirname, "dist"),
            libraryTarget: "window",
            clean: true,
        },
        devServer: {
            port: process.env.MICROSERVICE_CLIENT_LISTEN_PORT,
            hot: true,
            https: https,
            host: "::",
            allowedHosts: "all",
            static: "./",
            devMiddleware: {
                writeToDisk: true,
            },
        },
        plugins: [new wpdotenv(), new MiniCssExtractPlugin()],
    };
};
