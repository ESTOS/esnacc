// ROSE result / reject constants for glue tests (parity with cpp-lib/include/SnaccROSEInterfaces.h).
import * as ENetUC_Common from "../stub/ENetUC_Common.js";
import { InvokeProblemenum } from "../stub/SNACCROSE.js";
import { AsnInvokeProblem } from "../stub/TSROSEBase.js";
import { CustomInvokeProblemEnum } from "../stub/TSROSEBase.js";

export const ROSE_NOERROR = 0;
export const ROSE_TE_TRANSPORTFAILED = 0x00000001;
export const ROSE_TE_TIMEOUT = 0x00000003;
export const ROSE_REJECT_UNKNOWNOPERATION = 0x00000100;
export const ROSE_REJECT_MISTYPEDARGUMENT = 0x00000200;
export const ROSE_REJECT_FUNCTIONMISSING = 0x00000300;

export function isRoseReject(code: number): boolean {
	return (code & 0x00000f00) !== 0;
}

/** Asserts the invoke returned an AsnInvokeProblem with the expected problem value. */
export function assertAsnInvokeProblem(result: unknown, expected: number): AsnInvokeProblem {
	if (!(result instanceof AsnInvokeProblem)) {
		throw new Error(`expected AsnInvokeProblem, got ${typeof result}`);
	}
	if (result.value !== expected) {
		throw new Error(`expected invoke problem ${expected}, got ${result.value}`);
	}
	return result;
}

/** Asserts the invoke returned an application error with the expected detail code. */
export function assertAsnRequestError(result: unknown, expectedDetail: number): ENetUC_Common.AsnRequestError {
	if (!(result instanceof ENetUC_Common.AsnRequestError)) {
		throw new Error(`expected AsnRequestError, got ${typeof result}`);
	}
	if (result.iErrorDetail !== expectedDetail) {
		throw new Error(`expected error detail ${expectedDetail}, got ${result.iErrorDetail}`);
	}
	return result;
}

export { CustomInvokeProblemEnum, InvokeProblemenum };
