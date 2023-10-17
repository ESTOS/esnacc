const Comp = (Original: any, system: any) => (props: any) => {
    return (
        <>
            <div>
                <h3>TESSSSSSSSSSSSSSSST</h3>
            </div>
            <Original {...props} />
            {props.isShown ? (
                <div>
                    <h3>TESSSSSSSSSSSSSSSST</h3>
                </div>
            ) : null}
        </>
    );
};

export default Comp;
