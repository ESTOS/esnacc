import StyledDiv from "../styles/StyledDiv";
import StyledGrid from "../styles/StyledGrid";
import ReactJson from "react-json-view";
import { css } from "goober";
import { useEffect, useRef } from "react";
import { OwnEvent } from "../types";

const InfoText = css({ color: "white", fontFamily: "monospace", padding: "0px 10px", margin: 0 });

const Comp = (Original: any, system: any) => (props: any) => {
    const { operationId } = props.operationProps.toJS();
    const events: OwnEvent[] = system.websocketSelectors.getEvents(system.oas3Selectors.selectedServer(), parseInt(operationId));

    useEffect(() => {
        const el = document.getElementById("event-info-" + operationId);
        if (el) {
            el.scrollTo({ left: 0, top: el.scrollHeight });
        }
    });

    return (
        <>
            <Original {...props} />
            {props.isShown ? (
                <>
                    <div className="opblock-section-header">
                        <h4>Message history</h4>
                    </div>
                    <StyledDiv padding="20px">
                        <StyledDiv
                            id={"event-info-" + operationId}
                            className="highlight-code"
                            style={{
                                background: "rgb(51, 51, 51)",
                                resize: "vertical",
                                borderRadius: "4px",
                                overflowY: "scroll",
                                height: "200px",
                                minHeight: "200px",
                            }}
                            fullWidth
                        >
                            <StyledGrid
                                rows={["min-content", "min-content", "min-content"]}
                                style={{
                                    alignContent: "end",
                                    minHeight: "100%",
                                }}
                            >
                                {events.slice(-100).map((item, index) => {
                                    //return <div>{JSON.stringify(item)}</div>;
                                    return (
                                        <>
                                            <StyledGrid gridColumn={"1/1000"} columns={["subgrid"]}>
                                                <p className={InfoText}>{item.direction}</p>
                                                <p className={InfoText}>{item.type}</p>
                                                <p
                                                    className={InfoText}
                                                >{`${item.time.getHours()}:${item.time.getMinutes()}:${item.time.getSeconds()}.${item.time.getMilliseconds()}`}</p>
                                            </StyledGrid>
                                            <StyledDiv gridColumn={"1/1000"} style={{ padding: "0px 10px" }}>
                                                <ReactJson
                                                    key={index}
                                                    name={false}
                                                    collapsed
                                                    style={{ background: "none", borderBottom: "1px solid white", padding: "10px 0px" }}
                                                    theme={"monokai"}
                                                    src={item.payload}
                                                />
                                            </StyledDiv>
                                        </>
                                    );
                                })}
                            </StyledGrid>
                        </StyledDiv>
                    </StyledDiv>
                </>
            ) : null}
        </>
    );
};

export default Comp;
