/**
 * Copyright 2015 Rapid7
 * @brief Test harness
 * @file main.c
 */

#include <getopt.h>
#include <libgen.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "argv_split.h"
#include "log.h"
#include "mettle.h"

static void usage(const char *name)
{
	printf("Usage: %s [options]\n", name);
	printf("  -h, --help        display help\n");
	printf("  -u, --uri [uri]   add connection URI\n");
	printf("  -U, --uuid [uuid] set the UUID (base64)\n");
	printf("  -d, --debug       enable debug output\n");
	printf("  -o, --out [file]  write debug output to a file\n");
	printf("\n");
	exit(1);
}

static void start_logger(const char *out)
{
	FILE *l = stderr;
	if (out) {
	      FILE *f = fopen(out, "w");
	      if (f) l = f;
	}
	log_init_file(l);
	log_init_flush_thread();
}

static int parse_cmdline(int argc, char * const argv[], struct mettle *m)
{
	int c = 0;
	int index = 0;

	struct option options[] = {
		{"debug", no_argument, NULL, 'd'},
		{"out", required_argument, NULL, 'o'},
		{"uri", required_argument, NULL, 'u'},
		{"uuid", required_argument, NULL, 'U'},
		{ 0, 0, NULL, 0 }
	};
	const char *short_options = "hu:U:do:";
	const char *out = NULL;
	bool debug = false;
	int log_level = 0;

	while ((c = getopt_long(argc, argv, short_options, options, &index)) != -1) {
		switch (c) {
		case 'u':
			mettle_add_server_uri(m, optarg);
			break;
		case 'U':
			mettle_set_uuid_base64(m, optarg);
			break;
		case 'd':
			debug = true;
			log_set_level(++log_level);
			break;
		case 'o':
			out = optarg;
			break;
		case 'h':
		default:
			usage("mettle");
		}
	}

	if (debug) {
		start_logger(out);
	}

	return 0;
}

int parse_default_args(int argc, char * const argv[], struct mettle *m)
{
	static char default_opts[] = "DEFAULT_OPTS"
		"                                                               "
		"                                                               "
		"                                                               "
		"                                                               ";

	if (strncasecmp(default_opts, "default_opts", strlen("default_opts"))) {
		size_t argc = 0;
		char **argv = NULL;
		argv = argv_split(default_opts, argv, &argc);
		if (argv) {
			parse_cmdline(argc, argv, m);
			return 0;
		}
	}

	return -1;
}

int main(int argc, char * argv[])
{
	/*
	 * Disable SIGPIPE process aborts.
	 */
	signal(SIGPIPE, SIG_IGN);

	/*
	 * Allocate the main dispatcher
	 */
	struct mettle *m = mettle();
	if (m == NULL) {
		log_error("could not initialize");
		return 1;
	}

	/*
	 * Check to see if we were injected by metasploit
	 */
	if (strcmp(argv[0], "m") == 0) {
		/*
		 * There is a fd sitting here, trust me
		 */
		mettle_add_tcp_sock(m, (int)((long *)argv)[1]);
		parse_default_args(argc, argv, m);
	} else {
		if (parse_default_args(argc, argv, m)) {
			parse_cmdline(argc, argv, m);
		}
	}

	/*
	 * Start mettle and event loop
	 */
	mettle_start(m);

	mettle_free(m);

	return 0;
}
