// Copyright (C) 2022, 2023 by Mark Melton
//

#include "core/util/tool.h"

int tool_main(int argc, const char *argv[]) {
    ArgParse opts
	(
	 argFlag<'v'>("verbose", "Verbose diagnostics")
	 );
    opts.parse(argc, argv);
    return 0;
}
