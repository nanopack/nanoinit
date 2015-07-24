// -*- mode: c; tab-width: 4; indent-tabs-mode: 1; st-rulers: [80] -*-
// vim: ts=8 sw=8 ft=c noet

/*
 * Copyright (c) 2015 Pagoda Box Inc.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 */

#define KILL_PROCESS_TIMEOUT 5
#define KILL_ALL_PROCESSES_TIMEOUT 5

#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_INFO  3
#define LOG_LEVEL_DEBUG 4

typedef struct env_list_t {
	char		*key;
	char		*value;
	struct env_list_t	*next;
} env_list;

