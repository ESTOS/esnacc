import React from "react";
import StyledButton from "../styles/StyledButton";
import { websocketStore } from "../store";

const Comp = (Original: any, system: any) => (props: any) => {
    if (props.currentServer.startsWith("ws") || props.currentServer.startsWith("wss")) {
        let ws = websocketStore[props.currentServer];
        if (ws == undefined || ws.readyState == ws.CLOSED) websocketStore[props.currentServer] = new WebSocket(props.currentServer);
    }

    return <Original {...props} />;
};

export default Comp;
