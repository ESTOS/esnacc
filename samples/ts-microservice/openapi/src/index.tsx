import SwaggerUI from "swagger-ui-react";
import React from "react";
import ReactDOM from "react-dom";
import "swagger-ui/dist/swagger-ui.css";
import plugin from "./plugin";
import { setup } from "goober";
import { shouldForwardProp } from "goober/should-forward-prop";

setup(React.createElement);

ReactDOM.render(<SwaggerUI url="/schema/ENetUC_Event_Manager.json" plugins={[plugin]}></SwaggerUI>, document.body);
