import EventViewer from "./EventViewer";

const Comp = (Original: any, system: any) => (props: any) => {
    return (
        <div>
            {props.oas3Selectors.selectedServer().startsWith("ws") || props.oas3Selectors.selectedServer().startsWith("wss") ? (
                <EventViewer></EventViewer>
            ) : null}
            <Original {...props} />
        </div>
    );
};

export default Comp;
