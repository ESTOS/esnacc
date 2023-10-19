import React from "react";
import { RoseMessage } from "../types";

const Comp = (Original: any, system: any) => (props: any) => {
    //system.websocketActions.test();
    if (props.currentServer) system.websocketActions.initWebsocket(props.currentServer);
    return <Original {...props} />;
};

export default Comp;
