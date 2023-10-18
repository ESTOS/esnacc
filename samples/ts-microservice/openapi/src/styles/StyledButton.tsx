import { styled } from "goober";
import React from "react";
import { StyledDefaultArray, StyledDefaultProps } from "./defaults";

interface StyledButtonProps {}

const StyledButton = styled("button")<StyledButtonProps & StyledDefaultProps>([
    { backgroundColor: "#4990e2", border: "none", color: "white", padding: "10px" },
    ...StyledDefaultArray,
]);

export default StyledButton;
