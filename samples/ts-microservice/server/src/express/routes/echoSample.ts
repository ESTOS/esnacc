import * as express from "express";
import { IEModule } from "../expressRouter";

/**
 * The express echo sample route
 */
class Echo implements IEModule {
	/**
	 * TODOC
	 * @param router - TODOC
	 */
	public init(router: express.Router): void {
		router.use("/echo", this.echo);
	}

	/**
	 * TODOC
	 * @param req - TODOC
	 * @param res - TODOC
	 */
	public async echo(req: express.Request, res: express.Response): Promise<void> {
		// status(200) is optional
		if (typeof (req.body) === "string" && req.body.length)
			res.status(200).send(req.body);
		else
			res.status(200).send("Nothing to echo");
	}
}

export = new Echo();
