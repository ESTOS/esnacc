import { eventStore, storeEvent, websocketStore } from "../store"
import { RoseMessage } from "../types"
import { eventOperationId, getInvokeId } from "../utils"

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
	const resp = await fetch(url, options)
	return resp
}

export function invokeWs(operationID: number, url: string, body: any): Promise<{ status: 200 | 500, data: any }> {
	return new Promise(async (res, rej) => {
		let ws = websocketStore[url]

		if (ws && ws.readyState == WebSocket.OPEN) {
			const invokeID = getInvokeId();
			const awaiter = (message: MessageEvent<any>) => {
				try {
					const data: RoseMessage<any> = JSON.parse(message.data);

					if (data.result && data.result.invokeID == invokeID) {
						if (ws)
							ws.removeEventListener("message", awaiter);
						storeEvent(url, operationID, { time: new Date(), direction: "OUT", payload: structuredClone(data.result.result.result), type: "result" });
						res({ status: 200, data: data.result.result.result })
					}
					else if (data.reject && data.reject.invokedID.invokedID == invokeID) {
						if (ws)
							ws.removeEventListener("message", awaiter);
						storeEvent(url, operationID, { time: new Date(), direction: "OUT", payload: structuredClone(data.reject.details), type: "reject" });
						res({ status: 500, data: data.reject.details })
					}
				} catch (error) {
					console.log(error)
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
			storeEvent(url, operationID, { time: new Date(), direction: "OUT", payload: structuredClone(body), type: "invoke" });

			setTimeout(() => {
				if (ws)
					ws.removeEventListener("message", awaiter);
				rej(new Error("Timeout (10 seconds)"))
			}, 10000);
		}
		else {
			rej(new Error("Websocket not open!"));
		}
	})
}

export function eventWs(operationID: number, url: string, body: any) {
	let ws = websocketStore[url]

	if (ws && ws.readyState == WebSocket.OPEN) {
		const payload = {
			"invoke": {
				invokeID: eventOperationId,
				operationID: operationID,
				"argument": body
			}
		}
		ws.send(JSON.stringify(payload))
		storeEvent(url, operationID, { time: new Date(), direction: "OUT", payload: structuredClone(body), type: "invoke" });
	}
	else {
		throw new Error("Websocket not open!")
	}
}