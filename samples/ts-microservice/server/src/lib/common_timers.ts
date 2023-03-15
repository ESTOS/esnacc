/**
 * Own Timeout function as the node embedded uses an object NodeJS.Timout which is not recognizable as object.
 * So we better wrap it into an own object to be able to detect it (obj instanceof ETimeout)
 */
export class EOwnTimeout {
	private timeout?: NodeJS.Timeout;
	private args: unknown[];
	private callback: (...args: any[]) => void;

	/**
	 * Constructs the EOwnTimeout object
	 *
	 * @param callback - the function that has to be called on timeout
	 * @param ms - the timeout in msec
	 * @param args - arguments for the callback function
	 */
	public constructor(callback: (...args: any[]) => void, ms: number, ...args: any[]) {
		this.args = args;
		this.callback = callback;
		this.timeout = setTimeout(this.execute.bind(this), ms);
	}

	/**
	 * Executes the timeout and clears the internal timeout value
	 */
	private execute() {
		this.timeout = undefined;
		this.callback(...this.args);
	}

	/**
	 * Sets a new timeout for the underlying timeout object
	 *
	 * @param ms - the new timeout in msec
	 */
	public setNewTimeout(ms: number): void {
		if (this.timeout) {
			clearTimeout(this.timeout);
			this.timeout = undefined;
		}
		if (ms)
			this.timeout = setTimeout(this.execute.bind(this), ms);
	}

	/**
	 * Clears the timeout
	 */
	public clearTimeout(): void {
		if (this.timeout) {
			clearTimeout(this.timeout);
			this.timeout = undefined;
		}
	}

	/**
	 * Clears the timeout and executes the timeout right away
	 */
	public now(): void {
		if (this.timeout) {
			clearTimeout(this.timeout);
			this.execute();
		}
	}
}

/**
 * Own Timer function as the node embedded uses an object NodeJS.Timout which is not recognizable as object.
 * So we better wrap it into an own object to be able to detect it (obj instanceof Timer)
 */
export class EOwnInterval {
	private timeout?: NodeJS.Timeout;
	private args: unknown[];
	private ms: number;
	private callback: (...args: any[]) => void;

	/**
	 * Constructs the EOwnInterval object
	 *
	 * @param callback - the function that has to be called on interval
	 * @param ms - the interval in msec
	 * @param args - arguments for the callback function
	 */
	public constructor(callback: (...args: any[]) => void, ms: number, ...args: any[]) {
		this.args = args;
		this.ms = ms;
		this.callback = callback;
		this.timeout = setInterval(callback, ms, ...args);
	}

	/**
	 * Sets a new interval for the underlying timer object
	 *
	 * @param ms - the new timer in msec
	 */
	public setNewInterval(ms: number): void {
		if (this.timeout) {
			clearInterval(this.timeout);
			this.timeout = undefined;
		}
		if (ms) {
			this.timeout = setInterval(this.callback, ms, this.args);
			this.ms = ms;
		}
	}

	/**
	 * Executes the callback right now and resets the interval to the next regular interval
	 */
	public now(): void {
		this.setNewInterval(this.ms);
		setImmediate(this.callback, this.args);
	}

	/**
	 * Clears the interval
	 */
	public clearInterval(): void {
		if (this.timeout) {
			clearInterval(this.timeout);
			this.timeout = undefined;
		}
	}
}
