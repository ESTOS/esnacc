import OperationSummaryWrapper from "./components/OperationSummaryWrapper";
import ServerWrapper from "./components/ServerWrapper";
import EmptyWrapper from "./components/EmptyWrapper";
import OperationWrapper from "./components/OperationWrapper";
import OperationSummaryPathWrapper from "./components/OperationSummaryPathWrapper";
import { executeRequestWrapper, userExecute } from "./fn/execute";
import * as websocketActions from "./websocket/actions";
import * as websocketReducers from "./websocket/reducers";
import * as websocketSelectors from "./websocket/selectors";

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
            execute: userExecute,
        },
        statePlugins: {
            spec: {
                wrapActions: {
                    executeRequest: executeRequestWrapper,
                },
            },
            websocket: {
                actions: {
                    ...websocketActions,
                },
                reducers: {
                    ...websocketReducers,
                },
                selectors: {
                    ...websocketSelectors,
                },
            },
        },
    };
};

export default Plugin;
