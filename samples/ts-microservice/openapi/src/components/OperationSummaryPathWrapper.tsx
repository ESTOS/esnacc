import React, { useEffect, useRef, useState } from "react";
import { currentServer, getStoreSignal } from "../store";

const Comp = (Original: any, system: any) => (props: any) => {
    const { path, operationId } = props.operationProps.toJS();

    let isInvoke = false;
    try {
        isInvoke = system.spec().get("json").get("paths").get(path).get("post").get("responses").get("200") != undefined;
    } catch (error) {}

    const events = getStoreSignal(currentServer.value, parseInt(operationId));
    const [length, setLength] = useState(0);
    const [timeOut, setTimeOut] = useState<null | NodeJS.Timeout>(null);
    const ref = useRef<HTMLSpanElement>(null);

    useEffect(() => {
        if (length == 0 && events.value.length != 0) {
            setLength(events.value.length);
        } else if (length != events.value.length) {
            setLength(events.value.length);
            if (ref.current) {
                console.log(ref.current.style.background);
                ref.current.style.setProperty("background", "black", "important");
                const timeout = () => {
                    setTimeOut(null);
                    if (ref.current) ref.current.style.background = "";
                };
                if (timeOut != null) clearTimeout(timeOut);
                setTimeOut(setTimeout(timeout, 400));
            }
        }
    }, [events.value]);

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
