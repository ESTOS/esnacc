import React from "react";
import StyledButton from "../styles/StyledButton";

const Comp = (Original: any, system: any) => (props: any) => {
    if (props.oas3Selectors.selectedServer().startsWith("ws") || props.oas3Selectors.selectedServer().startsWith("wss")) {
        system.specActions.clearResponse(props.path, props.method);
        system.specActions.clearRequest(props.path, props.method);
    }
    console.log(props);
    return (
        <div>
            {props.oas3Selectors.selectedServer().startsWith("ws") || props.oas3Selectors.selectedServer().startsWith("wss") ? (
                <StyledButton
                    fullWidth
                    onClick={() => {
                        system.specActions.updateLoadingStatus("loading");
                    }}
                >
                    Execute
                </StyledButton>
            ) : (
                <Original {...props} />
            )}
        </div>
    );
};

export default Comp;
