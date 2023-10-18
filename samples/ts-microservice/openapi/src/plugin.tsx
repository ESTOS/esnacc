import OperationSummaryWrapper from "./components/OperationSummaryWrapper";
import ServerWrapper from "./components/ServerWrapper";
import EmptyWrapper from "./components/EmptyWrapper";
import OperationWrapper from "./components/OperationWrapper";
import OperationSummaryPathWrapper from "./components/OperationSummaryPathWrapper";
import { userExecute } from "./fn/execute";

/**
 *
 * @param system
 */
const Plugin = function (system: any) {
    return {
        wrapComponents: {
            Servers: ServerWrapper,
            OperationSummary: OperationSummaryWrapper,
            OperationSummaryMethod: EmptyWrapper,
            OperationSummaryPath: OperationSummaryPathWrapper,
            operation: OperationWrapper,
        },
        fn: {
            execute: userExecute(system),
        },
    };
};

export default Plugin;
