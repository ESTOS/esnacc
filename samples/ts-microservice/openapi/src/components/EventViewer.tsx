import React from "react";
import StyledGrid from "../styles/StyledGrid";

const Comp = (props: any) => {
    return (
        <StyledGrid fullWidth rows={["1fr"]} style={{ height: "500px", overflowY: "auto", margin: "10px 0px", backgroundColor: "white" }}>
            <div>asdsa</div>
        </StyledGrid>
    );
};

export default Comp;
