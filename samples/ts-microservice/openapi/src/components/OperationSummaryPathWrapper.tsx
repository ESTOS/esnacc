import React, { useEffect, useRef, useState } from "react";
import { OwnEvent } from "../types";

const Comp = (Original: any, system: any) => (props: any) => {
    const { path, operationId } = props.operationProps.toJS();

    let isInvoke = false;
    try {
        isInvoke = system.spec().get("json").get("paths").get(path).get("post").get("responses").get("200") != undefined;
    } catch (error) {}

    const events: OwnEvent[] = system.websocketSelectors.getEvents(system.oas3Selectors.selectedServer(), parseInt(operationId));
    const [length, setLength] = useState(0);
    const [timeOut, setTimeOut] = useState<null | NodeJS.Timeout>(null);
    const ref = useRef<HTMLSpanElement>(null);

    useEffect(() => {
        if (length == 0 && events.length != 0) {
            setLength(events.length);
        } else if (length != events.length) {
            setLength(events.length);
            if (ref.current) {
                ref.current.style.setProperty("background", "black", "important");
                const timeout = () => {
                    setTimeOut(null);
                    if (ref.current) ref.current.style.background = "";
                };
                if (timeOut != null) clearTimeout(timeOut);
                setTimeOut(setTimeout(timeout, 400));
            }
        }
    }, [events]);

    return (
        <>
            <span className="opblock-summary-method" ref={ref} style={{ transition: "all 200ms" }}>
                {isInvoke ? "Invoke" : "Event"}
            </span>
            <Original {...props} />
        </>
    );
};

export default Comp;
