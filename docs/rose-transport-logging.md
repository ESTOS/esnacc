---
title: ROSE transport logging
scope: cpp-lib, compiler/back-ends/ts-gen/gluecode
owner_repo: esnacc
entry_for:
  - EAsnLogLevel
  - LogTransportData
  - setTransportLogLevel
purpose: Human-readable contract for ROSE transport logs, including every justified C++/TypeScript deviation.
read_when:
  - Changing EAsnLogLevel, LogTransportData, or the TypeScript transport log
  - Checking C++ and TypeScript ROSE parity before a commit, push, or merge request
related_docs:
  - ../AGENTS.md
  - ../cpp-lib/tests/runtime_correctness_notes.md
  - ../.cursor/rules/rose-cross-language-parity.mdc
  - rose-telemetry.md
---

# ROSE transport logging

Every ROSE PDU is written once for each notation the bitmask enables. Invoke, result, error, reject, and event are covered in both directions.

`EAsnLogLevel` is the same bitmask in both runtimes.

| Bit | Value | Notation |
| --- | --- | --- |
| `DISABLED` | 0 | Nothing from this switch |
| `JSON` | 1 | Pretty JSON of the decoded PDU |
| `BER` | 2 | Uppercase hex of a BER payload, bytes separated by spaces |
| `JSON_AND_BER` | 3 | Both lines |
| `JSON_ALWAYS_PRETTY_PRINTED` | 4 | Combined with `JSON`. Forces a newline when the JSON text has none |

A JSON payload never produces a hex line. The BER bit applies only when the wire encoding is BER. When both bits are set, the hex line is wrapped as `"BER" : "..."`. BER alone is the raw hex. A reject envelope is context tag `[4]`, so that hex starts with `A4`.

The JSON line contains a newline and the ROSE choice key (`invoke`, `result`, `error`, `reject`). It does not contain the `"BER"` wrapper. Invoke lines also carry the operation name.

A decode failure logs the raw inbound payload and an error record whose JSON contains the key `exception`.

`logRawTransport` remains the single-notation switch. It applies only while the bitmask for that direction is `DISABLED`.

## API

| C++ | TypeScript | Role |
| --- | --- | --- |
| `EAsnLogLevel` | `EAsnLogLevel` | Same numeric bits |
| `GetLogLevel` | `getTransportLogLevel` | Read the one bitmask |
| Test harness `SetLogLevels` | `setTransportLogLevel` | Set the one bitmask |
| `LogTransportData` | `logTransport` | Write the enabled notations |
| `PrintJSONToLog` | The logger `debug` / `error` callback | Deliver one line |

Sources:

- `cpp-lib/include/SnaccROSEInterfaces.h`
- `cpp-lib/src/SnaccROSEBase.cpp`
- `compiler/back-ends/ts-gen/gluecode/TSASN1Base.ts`

## Deviations

| Topic | C++ | TypeScript | Why this is allowed |
| --- | --- | --- | --- |
| JSON wire text | `LogTransportData` writes the JSON bytes that were sent. The C++ encoder already inserts newlines. | The JSON notation is `JSON.stringify` of the decoded value with tabs. The TypeScript JSON wire is compact. | The logged notation is the decoded PDU. The compact wire stays unchanged. |
| Missing-handler details | The reject details token is `functionMissing`. | The details sentence is `<operation> is not implemented`. | The log contains that details string. The reject text itself is the existing TypeScript sentence. |
| Log level | `GetLogLevel` takes a direction so the derived class can answer. The library stores no level. The test harness stores two and every test sets them to the same value. | One `logLevel` on the transport. `setTransportLogLevel` takes that one value. | The library does not keep a separate inbound and outbound level. |
| Sink | `PrintJSONToLog` appends to the process ASN.1 log file. | `setLogger` delivers each line to that transport's logger. | A TypeScript process hosts many connections. Each transport has its own logger. |
| File configuration | `configureFileLogging` opens the C++ log file. | There is no file logger. The product logger is the sink. | File output is a C++ process facility. |

## Tests

| Behavior | C++ | TypeScript |
| --- | --- | --- |
| BER invoke and result, both notations | `BerInvokeAndResultLoggedAsPrettyJsonAndHex` | `BerInvokeAndResultLoggedAsPrettyJsonAndHex (BER)` |
| BER error, both notations | `BerErrorLoggedAsPrettyJsonAndHex` | `BerErrorLoggedAsPrettyJsonAndHex (BER)` |
| BER reject, both notations | `BerRejectLoggedAsPrettyJsonAndHex` | `BerRejectLoggedAsPrettyJsonAndHex (BER)` |
| BER event, both notations | `BerEventLoggedAsPrettyJsonAndHex` | `BerEventLoggedAsPrettyJsonAndHex (BER)` |
| JSON reject, no hex | `JsonRejectLoggedAsPrettyJsonWithoutHex` | `JsonRejectLoggedAsPrettyJsonWithoutHex (JSON)` |
| JSON wire with both bits, no hex | `JsonTransportWithBothBitsLogsJsonWithoutHex` | `JsonTransportWithBothBitsLogsJsonWithoutHex (JSON)` |
| BER-only reject hex | `BerOnlyRejectLoggedAsHexWithoutDecodedJson` | `BerOnlyRejectLoggedAsHexWithoutDecodedJson (BER)` |
| Pretty flag inserts newlines | `JsonPrettyLevelLogsNewlines` | `JsonPrettyLevelLogsNewlines (JSON)` |
| Malformed inbound payload | `MalformedInboundPayloadLogsRawDataAndError*` | `MalformedInboundPayloadLogsRawDataAndError (JSON)` / `(BER)` |

C++ spec: `cpp-lib/tests/logging/logging_tests.cpp`. TypeScript spec: `typescript/TSTransportLogging.test.ts`.
