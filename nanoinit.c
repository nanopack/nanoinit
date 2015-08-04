// -*- mode: c; tab-width: 4; indent-tabs-mode: 1; st-rulers: [80] -*-
// vim: ts=8 sw=8 ft=c noet

/*
 * Copyright (c) 2015 Pagoda Box Inc.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 */
 

#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <setjmp.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <stdarg.h>
#include "nanoinit.h"

extern char **environ;

int log_level = 0;
int signaled  = 0;
int alarmed   = 0;

#define LOG(NAME, LOG_LEVEL, FORMAT_TITLE) \
void \
NAME(const char *format, ...) \
{ \
	va_list args; \
	va_start(args, format); \
	if (log_level >= LOG_LEVEL) { \
		char *real_format = malloc(strlen(format) + strlen(FORMAT_TITLE) + 4); \
		memset(real_format, 0, strlen(format) + strlen(FORMAT_TITLE) + 4); \
		sprintf(real_format, FORMAT_TITLE": %s\n", format); \
		vfprintf(stderr, real_format, args); \
		free(real_format); \
	} \
	va_end(args); \
}

// Create functions for logging
LOG(error, LOG_LEVEL_ERROR, "Error");
LOG(warn,  LOG_LEVEL_WARN,  "Warning");
LOG(info,  LOG_LEVEL_INFO,  "Info");
LOG(debug, LOG_LEVEL_DEBUG, "Debug");

void
import_envvars(int clear_existing_environment, 
	int override_existing_environment)
{
	debug("import_envvars(%d, %d)", clear_existing_environment,
		override_existing_environment);
	struct stat s;
	env_list    *list = NULL;
	char        env_dir[] = "/etc/container_environment/";
	// populate the env_list
	if (stat(env_dir, &s) == 0 && S_ISDIR(s.st_mode)) {
		DIR           *envdir  = opendir(env_dir);
		env_list      *current = list;
		struct dirent *entry;
		while (entry = readdir(envdir)) {
			char filename[FILENAME_MAX];
			sprintf(filename, "%s%s", env_dir, entry->d_name);
			if (stat(filename, &s) == 0 && S_ISREG(s.st_mode)) {
				char *key   = strdup(entry->d_name);
				char *value = malloc(s.st_size + 1);
				memset(value, 0, s.st_size + 1);
				FILE *file;
				if ((file = fopen(filename, "r")) != NULL) {
					fscanf(file, "%s[^\n]", value);
					fclose(file);
				} else {
					error("Could not open %s: %s", filename, strerror(errno));
				}
				env_list *new_item = malloc(sizeof(env_list));
				if (list == NULL)
					list = new_item;
				if (current)
					current->next = new_item;
				current = new_item;
				current->key   = key;
				current->value = value;
				current->next  = NULL;
			}
		}
		closedir(envdir);
	} else {
		error("Could not open %s: %s", env_dir, strerror(errno));
	}

	// clear the environment?
	if (clear_existing_environment) {
		// not portable
		clearenv();
	}
	// set the environment?
	if (override_existing_environment) {
		for (env_list* i = list; i ; i = i->next) {
			setenv(i->key, i->value, 1);
		}
	}
	// cleanup
	for (env_list* i = list; i ; ) {
		env_list* to_free = i;
		i = i->next;
		if (to_free->key)
			free(to_free->key);
		if (to_free->value)
			free(to_free->value);
		free(to_free);
	}
}

int
countchar(char *string, char search)
{
	int count = 0;
	for (char *current = string; *current != '\0'; current++) {
		if (*current == search)
			count++;
	}
	return count;
}

char
*shquote(char *string)
{
	// is string is 0 length
	if (strlen(string) == 0)
		return strdup("''");
	// if there are no single quotes, wrap it in single quotes and call it good
	if (strstr(string, "'") == NULL) {
		char *dupstring = malloc(strlen(string) + 3);
		sprintf(dupstring, "'%s'", string);
		return dupstring;
	}
	// if there are single quotes, then replace them with '"'"' and wrap the
	// whole string in quotes.
	int new_len = strlen(string) + 5 * countchar(string, '\'');
	char *new_string = malloc(new_len + 1);
	memset(new_string, 0, new_len + 1);
	char *current = string;
	char *prev = current;
	current = strstr(current, "'");
	while (current) {
		strncat(new_string, prev, current - prev);
		strncat(new_string, "'\"'\"'", 5);
		prev = current + 1;
		current = strstr(current + 1, "'");
	}
	strcat(new_string, prev);
	return new_string;
}

void
export_envvars(int to_dir)
{
	debug("export_envvars(%d)", to_dir);
	char shell_file_name[] = "/etc/container_environment.sh";
	FILE *shell_file;
	if ((shell_file = fopen(shell_file_name, "w")) != NULL) {
		for (int i = 0; environ[i]; i++) {
			char *envvar = environ[i];
			// don't export it if it is one of the following:
			// HOME, USER, GROUP, UID, GID, SHELL

			if (strncmp("HOME=", envvar, 5) != 0 &&
				strncmp("USER=", envvar, 5) != 0 &&
				strncmp("GROUP=", envvar, 6) != 0 &&
				strncmp("UID=", envvar, 4) != 0 &&
				strncmp("GID=", envvar, 4) != 0 &&
				strncmp("SHELL=", envvar, 6) != 0) {
				size_t len = strstr(envvar, "=") - envvar;
				char *key = malloc(len + 1);
				memset(key, 0, len + 1);
				strncpy(key, envvar, len);
				char *value = strdup(envvar + len + 1);
				char *safe_key = shquote(key);
				char *safe_value = shquote(value);
				if (to_dir) {
					char filename[FILENAME_MAX];
					sprintf(filename, "/etc/container_environment/%s", key);
					FILE *file;
					if ((file = fopen(filename, "w")) != NULL) {  
						fprintf(file, "%s", value);
						fclose(file);
					} else {
						error("Failed to open file %s: %s", filename, 
							strerror(errno));
					}
				}
				fprintf(shell_file, "export %s=%s\n", safe_key, safe_value);
				free(key);
				free(value);
				free(safe_key);
				free(safe_value);
			}
		}
		fclose(shell_file);
	} else {
		error("Failed to open file %s: %s", shell_file_name, strerror(errno));
	}
}

void
ignore_signals(int signame)
{
	signal(SIGTERM, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signaled = 1;
}

void
alarm_handler(int signame)
{
	alarmed = 1;
}

void
kill_all_processes(int timeout)
{
	int done = 0;
	int ret = 0;
	int status = 0;

	info("Killing all processes...");
	kill(-1, SIGTERM);
	alarm(timeout);
	while (done == 0 && alarmed == 0) {
		// wait for all children
		ret = waitpid(-1, &status, 0);
		if (ret == -1 && errno == ECHILD)
			done = 1;
	}
	if (done == 0) {
		warn("Not all processes have exited in time. Forcing them to exit.");
		// try killing with stronger signal
		kill(-1, SIGKILL);
	}
	// cancel the alarm if it hasn't gone off yet
	alarm(0);
}

pid_t
run_init(char *args[])
{
	debug("starting service manager");
	pid_t	init_pid;
	char	*default_args[] = {"/opt/gonano/sbin/runsvdir", "-P",
		"/etc/service", 0};
	if (args == NULL)
		args = default_args;
	// char *args[] = {"/bin/sleep", "10", 0};

	if ((init_pid = fork()) == 0) {
		import_envvars(1, 1);
		if (execve(args[0], args, environ) == -1) {
			error("Failed to exec the init process: %s", strerror(errno));
			exit(1);
		}
	} else if (init_pid < 0) {
		error("Failed to fork init process: %s", strerror(errno));
	}

	debug("init pid: %d", init_pid);
	return init_pid;
}

void
run_rc_local()
{
	struct stat	s;
	char		*rc_local[] = {"/etc/rc.local", 0};
	pid_t		pid;
	int         status;
	if (stat(rc_local[0], &s) == 0 && 
		(S_ISREG(s.st_mode) || S_ISLNK(s.st_mode)) && 
		(s.st_mode | S_IXUSR)) {
		if ((pid = fork()) == 0) {
			import_envvars(1, 1);
			if (execve(rc_local[0], rc_local, environ) == -1) {
				error("Failed to exec the rc.local: %s", strerror(errno));
				exit(1);
			}
		} else if (pid < 0) {
			error("Failed to fork rc.local: %s", strerror(errno));
		} else {
			debug("rc.local pid: %d", pid);
			do {
				waitpid(pid, &status, 0);
			} while (WIFEXITED(status) == 0 && signaled == 0);
			if (WEXITSTATUS(status))
				warn("rc.local exited with status: %d", WEXITSTATUS(status));
			if (WIFSIGNALED(status))
				warn("rc.local killed with signal: %d", WIFSIGNALED(status));
		}
	} else {
		debug("Can not run /etc/rc.local: %s", strerror(errno));
	}
}

void
run_nanoinit_d(char *action)
{
	struct stat		s;
	struct dirent	**name_list;
	pid_t			pid;
	int				status;
	int				n;
	char    	    init_dir[] = "/etc/nanoinit.d/";
	if (stat(init_dir, &s) == 0 && S_ISDIR(s.st_mode)) {
		if ((n = scandir(init_dir, &name_list, 0, alphasort)) >= 0) {
			for (int i = 0; i < n; i++) {
				char filename[FILENAME_MAX];
				sprintf(filename, "%s%s", init_dir, name_list[i]->d_name);
				if (stat(filename, &s) == 0 &&
					(S_ISREG(s.st_mode) || S_ISLNK(s.st_mode)) &&
					(s.st_mode | S_IXUSR)) {
					info("Running: \"%s %s\"", filename, action);
					if ((pid = fork()) == 0) {
						import_envvars(1,1);
						char *cmd[] = {filename, action, 0};
						if (execve(cmd[0], cmd, environ) == -1) {
							error("Failed to exec \"%s %s\":", filename, action,
								strerror(errno));
							exit(1);
						}
					} else if (pid < 0) {
						error("Failed to fork \"%s %s\": %s", filename, action,
							strerror(errno));
					} else {
						debug("\"%s %s\" pid: %d", filename, action, pid);
						do {
							waitpid(pid, &status, 0);
						} while (WIFEXITED(status) == 0 && signaled == 0);
						if (WEXITSTATUS(status))
							warn("\"%s %s\" exited with status: %d", filename,
								action, WEXITSTATUS(status));
						if (WIFSIGNALED(status))
							warn("\"%s %s\" killed with signal: %d", filename,
								action, WIFSIGNALED(status));
					}
				} else {
					warn("Can't run: \"%s %s\"", filename, action);
				}
				free(name_list[i]);
			}
		} else {
			error("/etc/nanoinit.d: %s", strerror(errno));
		}
		free(name_list);
	} else {
		debug("/etc/nanoinit.d: %s", strerror(errno));
	}
}

int
main(int argc, char *argv[])
{
	log_level = LOG_LEVEL_DEBUG;
	info("                         _       _ _ ");  
	info(" _ __   __ _ _ __   ___ (_)_ __ (_) |_ ");
	info("| '_ \\ / _` | '_ \\ / _ \\| | '_ \\| | __|");
	info("| | | | (_| | | | | (_) | | | | | | |_ ");
	info("|_| |_|\\__,_|_| |_|\\___/|_|_| |_|_|\\__|");
	info("booting...");
	pid_t init_pid = 0;
	pid_t child_pid = 0;
	int status = 0;
	int ret = 0;
	int arg_offset;
	int killall = 1;

	for (arg_offset = 1; arg_offset < argc; arg_offset++) {
		if (strncmp("--",argv[arg_offset],2) != 0)
			break;
		if (strncmp("--quiet",argv[arg_offset],7) == 0)
			log_level = LOG_LEVEL_WARN;
		if (strncmp("--no-killall",argv[arg_offset],12) == 0)
			killall = 0;
	}

	struct sigaction alarm;
	struct sigaction other;
	alarm.sa_handler = alarm_handler;
	sigfillset(&alarm.sa_mask);
	alarm.sa_flags = 0;
	other.sa_handler = ignore_signals;
	sigfillset(&other.sa_mask);
	other.sa_flags = 0;
	debug("setting signal handlers");
	sigaction(SIGALRM, &alarm, NULL);
	sigaction(SIGINT, &other, NULL);
	sigaction(SIGTERM, &other, NULL);

	import_envvars(0, 0);
	export_envvars(1);

	run_nanoinit_d("start");
	run_rc_local();

	if ((init_pid = run_init((arg_offset == argc) ? NULL : argv + arg_offset )) > 0) {
		do {
			child_pid = waitpid(-1, &status, 0);
			if (child_pid > 0)
				debug("reaped process %d", child_pid);
		} while (child_pid != init_pid && signaled == 0);
	}

	if (signaled)
		warn("Init system aborted.");
	run_nanoinit_d("stop");
	if (killall)
		kill_all_processes(KILL_ALL_PROCESSES_TIMEOUT);
	return ret;
}