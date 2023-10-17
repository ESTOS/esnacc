import { CSSAttribute } from "goober";
import { Dictionary } from "../types";

export interface StyledDefaultProps {
    fullWidth?: true;
}

const _StyledDefaultArray: Array<(props: StyledDefaultProps) => Dictionary<CSSAttribute, any> | Dictionary<CSSAttribute, any>> = [
    ({ fullWidth }) => (fullWidth ? { width: "100%" } : {}),
];

export const StyledDefaultArray: Array<any> = _StyledDefaultArray;
