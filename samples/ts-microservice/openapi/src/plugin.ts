
import OperationSummaryWrapper from "./components/OperationSummaryWrapper";
import ResponseWrapper from "./components/ResponseWrapper";
import ExecuteWrapper from "./components/ExecuteWrapper";
import ServerWrapper from "./components/ServerWrapper"
import { userExecute } from "./fn/execute"

/**
 *
 * @param system
 */
const Plugin = function (system: any) {
    return {
        wrapComponents: {
            //responses: ResponseWrapper,
            //execute: ExecuteWrapper,
            Servers: ServerWrapper,
            OperationSummary: OperationSummaryWrapper
        },
        fn: {
            execute: userExecute(system),
        },
    };
};

export default Plugin;