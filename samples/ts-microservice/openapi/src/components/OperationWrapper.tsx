import { css } from "goober";

const EventStyle = css({
    "& * > .opblock-post": {
        background: "rgba(97,175,254,.1) !important",
        borderColor: "#61affe !important",
    },
    "& * > .opblock-summary": {
        borderColor: "#61affe !important",
    },
    "& * > .tab-header span:after": {
        background: "#61affe !important",
    },
    "& * > .opblock-summary-method": {
        background: "#61affe !important",
    },
});

const Comp = (Original: any, system: any) => (props: any) => {
    const { path } = props.operation.toJS();
    let isInvoke = false;
    try {
        isInvoke = system.spec().get("json").get("paths").get(path).get("post").get("responses").get("200") != undefined;
    } catch (error) {}

    return (
        <div className={isInvoke ? "" : EventStyle}>
            <Original {...props} />
        </div>
    );
};

export default Comp;
