import { CSSAttribute, styled } from "goober";
import React from "react";
import { StyledDefaultArray, StyledDefaultProps } from "./defaults";
import { Property } from "csstype";

interface StyledGridProps {
    columns?: Property.GridTemplateColumns[];
    rows?: Property.GridTemplateRows[];
}

const StyledGrid = styled("div")<StyledGridProps & StyledDefaultProps>([
    ({ columns }) => (columns ? { gridTemplateColumns: columns.join(",") } : {}),
    ({ rows }) => (rows ? { gridTemplateColumns: rows.join(",") } : {}),
    ...StyledDefaultArray,
]);

export default StyledGrid;
