import React from "react";
import { currentServer, storeEvent, websocketStore } from "../store";
import { RoseMessage } from "../types";

const Comp = (Original: any, system: any) => (props: any) => {
    if (currentServer.value != props.currentServer) {
        currentServer.value = props.currentServer ?? "";
    }
    if (props.currentServer && (props.currentServer.startsWith("ws") || props.currentServer.startsWith("wss"))) {
        let ws = websocketStore[props.currentServer];
        if (ws == undefined || ws.readyState == ws.CLOSED) {
            let newWS = new WebSocket(props.currentServer);
            //let schemaUrl = system.spec().get("url");
            newWS.addEventListener("message", (m) => {
                try {
                    let payload: RoseMessage<any> = JSON.parse(m.data);
                    if (payload.result) {
                    } else if (payload.invoke) {
                        storeEvent(props.currentServer, payload.invoke.operationID, {
                            time: new Date(),
                            direction: "IN",
                            payload: structuredClone(payload.invoke.argument),
                            type: "invoke",
                        });
                    }
                } catch (error) {
                    console.log(error);
                }
            });
            websocketStore[props.currentServer] = newWS;
        }
    }

    return <Original {...props} />;
};

export default Comp;
