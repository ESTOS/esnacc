import { styled } from "goober";
import React from "react";
import { StyledDefaultArray, StyledDefaultProps } from "./defaults";

interface Props {}

const StyledComp = styled("div")<Props & StyledDefaultProps>([...StyledDefaultArray]);

export default StyledComp;
