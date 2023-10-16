import { Request } from "express";
import path from "path";
import { ILogData } from "uclogger";
import url from "url";
import { v4 as uuidv4 } from "uuid";

import { theConfig } from "../globals";

/**
 * Class with different static helper methods
 */
export class Common {
	/**
	 * The Loggers getLogData callback (used in all the log methods called in this class, add the classname to every log entry)
	 *
	 * @returns - an ILogData log data object provided additional data for all the logger calls in this class
	 */
	public static getLogData(): ILogData {
		return {
			className: this.constructor.name
		};
	}

	/**
	 * Generates a uuidv4
	 *
	 * @returns - the uuidv4
	 */
	public static generateGUID(): string {
		return uuidv4().split("-").join("");
	}

	/**
	 * Adds a platform specific directory seperator if the string is not empty and not closed with the approrpriate one
	 *
	 * @param dir - Directory path where to add the separator
	 * @returns - the directory with the seperator
	 */
	public static addDirSeperator(dir: string): string {
		if (dir.length && dir.substring(dir.length - 1) !== path.sep)
			dir += path.sep;
		return dir;
	}

	/**
	 * Adds a linux directory seperator if the string is not empty and not closed with the approrpriate one
	 *
	 * @param dir - Directory path where to add the separator
	 * @returns - the directory with the seperator
	 */
	public static addLinuxDirSeperator(dir: string): string {
		if (dir.length && dir.substring(dir.length - 1) !== "/")
			dir += "/";
		return dir;
	}

	/**
	 * Retrieve a random string A-Z, a-z, 0-9, of a given length
	 *
	 * @param length - the length we want to retrieve
	 * @returns - the random string
	 */
	public static getRandomString(length: number): string {
		let result = "";
		const characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
		for (let i = 0; i < length; i++)
			result += characters.charAt(Math.floor(Math.random() * characters.length));
		return result;
	}

	/**
	 * Retreive a random number as string with a given length
	 *
	 * @param length - the length we want to retrieve
	 * @returns - the random string
	 */
	public static getRandomNumberAsString(length: number): string {
		let result = "";
		const characters = "0123456789";
		for (let i = 0; i < length; i++)
			result += characters.charAt(Math.floor(Math.random() * characters.length));
		return result;
	}

	/**
	 * Retrieve a random integer value between 0 and max
	 *
	 * @param max - the maximum value we want to retrieve (exclusive this value - 2 returns 1 or 0)
	 * @returns - the integer value
	 */
	public static getRandomInt(max: number): number {
		return Math.floor(Math.random() * Math.floor(max));
	}

	/**
	 * Get the time as 00:00:00:000 value
	 *
	 * @returns - the time
	 */
	public static getTime(): string {
		const time = new Date();
		let result = "";
		if (time.getHours() < 10)
			result += "0";
		result += time.getHours().toString();
		result += ":";
		if (time.getMinutes() < 10)
			result += "0";
		result += time.getMinutes().toString();
		result += ":";
		if (time.getSeconds() < 10)
			result += "0";
		result += time.getSeconds().toString();
		result += ":";
		if (time.getMilliseconds() < 10)
			result += "00";
		else if (time.getMilliseconds() < 100)
			result += "0";
		result += time.getMilliseconds().toString();
		return result;
	}

	/**
	 * Get the URL from the Request object
	 *
	 * @param req - the Request to parse
	 * @returns - the url to hand back
	 */
	public static getURLFromRequest(req: Request): URL {
		const path = url.format(
			{
				protocol: req.protocol,
				host: req.get("host"),
				pathname: req.originalUrl
			}
		);
		return new URL(path);
	}

	/**
	 * Rounds a value to roundto values
	 *
	 * @param value - the value to round
	 * @param roundto - the modulo integer we want to round to (e.g. 5) : 7.5 will be rounded to 5
	 * @returns - the rounded value
	 */
	public static roundValueTo(value: number, roundto: number): number {
		return Math.round(Math.round(value) / roundto) * roundto;
	}

	/**
	 * Converts a JavaScript value to a JavaScript Object Notation (JSON) string.
	 * In dev mode we do it pretty printed, in prod or staging we do it plain
	 *
	 * @param value - A JavaScript value, usually an object or array, to be converted.
	 * @returns - the converted JSON as string
	 */
	public static stringify(value: unknown): string {
		if (theConfig.environment === "development")
			return JSON.stringify(value, null, "\t");
		else
			return JSON.stringify(value);
	}

	/**
	 * Exits the process and writes that info as error to the console
	 *
	 * @param text - text for the console output
	 * @param code - the code to exit
	 */
	public static exit(text: string, code: number): void {
		(console as Console).error({ code, text });
		process.exit(code);
	}

	/**
	 * Helper method to overcome a certain typescript error
	 *
	 * You have a class property in method that is nulled/undefined at the beginning and filled at the end
	 * In between you have async await methods.
	 * If you undefine it at the beginning the typescript compiler assumes that it is filled at the end, but
	 * due to the await it´s not guaranteed if you call the method multiple times at ones.
	 *
	 * So we use this method to check whether the object is really filled or not
	 *
	 * @param element - the element to check
	 * @returns - true if element is defined, otherwise false
	 */
	public static getDefined<T>(element: T | undefined, className: { new(...args: any[]): T }): T | undefined {
		return element;
	}
}
