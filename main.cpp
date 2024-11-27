#include "Card.h"

#include <cstdio>

#include <memory>

/*
 * main() also stays the same.
 */

int main(int argc, char **argv)
{
	try {
		int ret;
		const char *card;

		/* check which DRM device to open */
		if (argc > 1)
			card = argv[1];
		else
			card = "/dev/dri/card1";

		fprintf(stderr, "using card '%s'\n", card);

		auto ms = std::make_unique<Card>(card);

		/* prepare all connectors and CRTCs */
		ret = ms->prepare();
		if (ret)
			throw errcode_exception(ret, "modeset::prepare failed");

		bool modeset_success = ms->displays->setAllDisplaysModes();
		if (!modeset_success)
			throw errcode_exception(ret, "mode setting failed for some displays");
		
		/* draw some colors for 5seconds */
		ms->runDrawingLoop();

		ms = nullptr;
		fprintf(stderr, "exiting\n");
		return 0;
	
	} catch (const errcode_exception& ex) {
		fprintf(stderr, "%s\n", ex.what());
		return ex.errcode;
	}
}