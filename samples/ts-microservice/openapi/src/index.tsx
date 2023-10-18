import App from "./app";
import { setup } from "goober";
import ReactDOM from "react-dom";
import React from "react";

setup(React.createElement);

ReactDOM.render(<App></App>, document.body);
