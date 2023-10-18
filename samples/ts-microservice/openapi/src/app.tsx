import SwaggerUI from "swagger-ui-react";
import React, { useState } from "react";
import "swagger-ui/dist/swagger-ui.css";
import plugin from "./plugin";
import { setup } from "goober";
import Select from "react-select";

setup(React.createElement);

const options = [
    { value: "/schema/ENetUC_Common.json", label: "ENetUC_Common" },
    { value: "/schema/ENetUC_Settings_Manager.json", label: "ENetUC_Settings_Manager" },
    { value: "/schema/ENetUC_Event_Manager.json", label: "ENetUC_Event_Manager" },
];

const Comp = () => {
    let [selected, setSelected] = useState(options[2]);

    return (
        <>
            <Select
                options={options}
                defaultValue={selected}
                onChange={(x) => {
                    if (x) setSelected(x);
                }}
            />
            <SwaggerUI url={selected!.value} plugins={[plugin]}></SwaggerUI>
        </>
    );
};

export default Comp;
