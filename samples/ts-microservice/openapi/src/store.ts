import { Signal, signal } from "@preact/signals-react";

export interface OwnEvent {
    time: Date,
    direction: "OUT" | "IN",
    payload: any,
    type: "invoke" | "result" | "reject"
}

export const currentServer = signal("");

export const eventStore: {
    [serverUrl: string]: {
        [operationId: string]: Signal<Array<OwnEvent>>
    }
} = {}

export const websocketStore: { [url: string]: WebSocket } = {}

export function getStoreSignal(serverUrl: string, operationId: number) {

    if (eventStore[serverUrl] == undefined) eventStore[serverUrl] = {}
    if (eventStore[serverUrl]![operationId] == undefined) eventStore[serverUrl]![operationId] = signal(new Array())
    return eventStore[serverUrl]![operationId]!
}

export function storeEvent(serverUrl: string, operationId: number, ev: OwnEvent) {

    console.log("New Websocket Message on: " + serverUrl, ev)
    if (eventStore[serverUrl] == undefined) eventStore[serverUrl] = {}
    if (eventStore[serverUrl]![operationId] == undefined) eventStore[serverUrl]![operationId] = signal(new Array(ev))
    else {
        eventStore[serverUrl]![operationId]!.value = eventStore[serverUrl]![operationId]!.value.concat(ev)
    }
}