import { CSSAttribute } from "goober";
import { Dictionary } from "../types";
import { Property } from "csstype";

export interface StyledDefaultProps {
    fullWidth?: true;
    padding?: string;
    gridColumn?: Property.GridColumn;
}

const _StyledDefaultArray: Array<(props: StyledDefaultProps) => Dictionary<CSSAttribute, any>> = [
    ({ fullWidth }) => (fullWidth ? { width: "100%" } : {}),
    ({ padding }) => (padding ? { padding: padding } : {}),
    ({ gridColumn }) => (gridColumn ? { gridColumn: gridColumn } : {}),
];

export const StyledDefaultArray: Array<any> = _StyledDefaultArray;
