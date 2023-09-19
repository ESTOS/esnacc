import { IReceiveInvokeContextParams, ISendInvokeContextParams } from "./TSROSEBase";

// prettier-ignore
/* eslint-disable */

/**
 * This is the central deprecated callback
 *
 * The asn1 files allow to flag objects or methods as deprecated
 * If that is the case the compiler adds callbacks in case such an object is created or such a method is called
 */
export interface IASN1DeprecatedCallback {
	/**
	 * A deprecated object has been created
	 *
	 * @param deprecatedSince - unix time when the object has been flagged deprecated
	 * @param moduleName - the module in which the object is located
	 * @param objectName - the name of the object that is about to get created
	 * @param callStack - the call stack that shows where the object has been created
	 */
	deprecatedObject(deprecatedSince: number, moduleName: string, objectName: string, callStack: IASN1CallStackEntry[]): void;

	/**
	 * A deprecated method has been called
	 *
	 * @param deprecatedSince - unix time when the method has been flagged deprecated
	 * @param moduleName - the module in which the object is located
	 * @param methodName - the name of the method that has been called
	 * @param direction - whether the call was inbound or outbound
	 * @param invokeContext - the invokeContext that shows more details about the invoke
	 * @param callStack - the call stack that shows where the object has been created
	 */
	deprecatedMethod(deprecatedSince: number, moduleName: string, methodName: string, direction: "IN" | "OUT", invokeContext: IReceiveInvokeContextParams | ISendInvokeContextParams | undefined, callStack: IASN1CallStackEntry[]): void;
}

/**
 * Result of the getCallingFunction
 */
export interface IASN1CallStackEntry {
	class?: string;
	method: string | undefined;
}

/**
 * This is the class that holds the central deprecated callback 
 */
export class TSDeprecatedCallback {
	// This is the central deprecated callback, see ASN1DeprecatedCallback for details.
	private static deprecatedCallback?: IASN1DeprecatedCallback;
	
	/**
	 * Sets the callback that is informed about deprecaetd method calls, deprecated object creation
	 *
	 * @param callback - the callback that shall get notified
	 */
	public static setDeprecatedCallback(callback: IASN1DeprecatedCallback | undefined): void {
		TSDeprecatedCallback.deprecatedCallback = callback;
	}

	/**
	 * This method is called in case an object is created which is flagged deprecated
	 *
	 * @param deprecatedSince - unix time when the object has been flagged deprecated
	 * @param moduleName - the module in which the object is located
	 * @param obj - the object that has been created
	 */
	public static deprecatedObject(deprecatedSince: number, moduleName: string, obj: object): void {
		if (!TSDeprecatedCallback.deprecatedCallback)
			return;
		const name = obj.constructor.name;
		const stack = this.getCallStack();
		TSDeprecatedCallback.deprecatedCallback.deprecatedObject(deprecatedSince, moduleName, name, stack);
	}

	/**
	 * This method is called in case a method is called which is flagged deprecated
	 *
	 * @param deprecatedSince - unix time when the method has been flagged deprecated
	 * @param moduleName - the module in which the object is located
	 * @param methodName - the name of the method that has been called
	 * @param direction - whether the call was inbound or outbound
	 * @param invokeContext - the invokeContext that shows more details about the invoke
	 */
	public static deprecatedMethod(deprecatedSince: number, moduleName: string, methodName: string, direction: "IN" | "OUT", invokeContext: IReceiveInvokeContextParams | ISendInvokeContextParams | undefined): void {
		if (!TSDeprecatedCallback.deprecatedCallback)
			return;
		const stack = this.getCallStack();
		TSDeprecatedCallback.deprecatedCallback.deprecatedMethod(deprecatedSince, moduleName, methodName, direction, invokeContext, stack);
	}

		/**
	 * Retrieves the stack of the calling function
	 * We can remove some levels in case we are in a handling function and do not want to see the call to that handling function but the call before
	 *
	 * @param back - how many levels on the stack do we want to go back
	 * @returns - the stack that allows to see where a call has come from
	 */
		private static getCallStack(back = 1): IASN1CallStackEntry[] {
			const result: IASN1CallStackEntry [] = [];
			const stack = new Error().stack;
			if (stack) {
				const elements = stack.split("\n");
				for (let iCount = 2 + back; iCount++; iCount < elements.length) {
					const caller = elements[iCount];
					if (!caller)
						break;
					let bIsNode = true;
					const reg1 = / at (.*) /;
					let method = reg1.exec(caller);
					if (!method) {
						const reg2 = /(.*)@/;
						method = reg2.exec(caller);
						bIsNode = false;
					}
					if (method && method[1]) {
						let split: RegExpExecArray | null = null;
						if (bIsNode) {
							// node js root element, we stop here as the root level gives no additional insights...
							if (method[1] === "Object.<anonymous>")
								break;
							const reg3 = /(.*)\.(.*)/;
							split = reg3.exec(method[1]);
						}
						if (split)
							result.push({ class: split[1], method: split[2] });
						else
							result.push({ method: method[1] });
						if (!bIsNode) {
							// After the componentDidMount in react we need no further entires (just blurries the callstack)
							if (method[1] === "componentDidMount")
								break;
						}
					}
				}
			}
			return result;
		}
}