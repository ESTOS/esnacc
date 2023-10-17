import { websocketStore } from "./store"
import { getInvokeId } from "./utils"

export async function invokeRest(operationID: number, url: string, body: any) {
	const options: RequestInit = {
		method: "POST",
		headers: {
			"Content-Type": "application/json"
		},
		body: JSON.stringify(
			{
				"invoke": {
					"invokeID": getInvokeId(),
					operationID: operationID,
					"argument": body
				}
			}
		)
	}
	console.log({
		"invoke": {
			"invokeId": Date.now(),
			operationID,
			"argument": body
		}
	})
	const resp = await fetch(url, options)
	return resp
}

export function invokeWs(operationID: number, url: string, body: any) {
	return new Promise(async (res, rej) => {
		let ws = websocketStore[url]

		if (ws && ws.readyState == WebSocket.OPEN) {
			const invokeID = getInvokeId();
			const awaiter = (message: MessageEvent<any>) => {
				console.log(message.data)
				const data = JSON.parse(message.data);
				if (data.result && data.result.invokeID == invokeID) {
					if (ws)
						ws.removeEventListener("message", awaiter);
					res(data.result.result)
				}
			}
			ws.addEventListener("message", awaiter)
			const payload = {
				"invoke": {
					invokeID,
					operationID: operationID,
					"argument": body
				}
			}
			ws.send(JSON.stringify(payload))
			console.log("Event was emitted to " + url, payload)
			setTimeout(() => rej(new Error("Timeout (1 minute)")), 1000 * 60);
		}
		else {
			throw new Error("Websocket not open!")
		}
	})
}