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
#include "nanit.h"

extern char **environ;

int log_level = 0;
int signaled = 0;
int alarmed = 0;

void
error(char *message)
{
	if (log_level >= LOG_LEVEL_ERROR)
		fprintf(stderr, "Error: %s\n", message);
}

void
warn(char *message)
{
	if (log_level >= LOG_LEVEL_WARN)
		fprintf(stderr, "Warning: %s\n", message);
}

void
info(char *message)
{
	if (log_level >= LOG_LEVEL_INFO)
		fprintf(stderr, "Info: %s\n", message);
}

void
debug(char *message)
{
	if (log_level >= LOG_LEVEL_DEBUG)
		fprintf(stderr, "Debug: %s\n", message);
}

void
import_envvars(int clear_existing_environment, 
	int override_existing_environment)
{
	struct stat s;
	env_list *list = NULL;
	// populate the env_list
	if (stat("/etc/container_environment", &s) == 0 && S_ISDIR( s.st_mode )) {
		DIR *envdir = opendir("/etc/container_environment/");
		env_list *current = list;
		struct dirent *entry;
		while (entry = readdir(envdir)) {
			char filename[FILENAME_MAX];
			sprintf(filename, "/etc/container_environment/", entry->d_name);
			if (stat(filename, &s) == 0 && S_ISREG(s.st_mode)) {
				char *key = strdup(entry->d_name);
				char *value = NULL;
				char temp_value[4096];
				FILE *file = fopen(filename, "r");
				fscanf(file, "%s[^\n]", &temp_value);
				fclose(file);
				value = strdup(temp_value);
				env_list *new_item = malloc(sizeof(env_list));
				if (list == NULL)
					list = new_item;
				if (current)
					current->next = new_item;
				current = new_item;
				current->key = key;
				current->value = value;
				current->next = NULL;
			}
		}
		closedir(envdir);
	}

	// clear the environment?
	if (clear_existing_environment) {
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
	FILE *shell_file = fopen("/etc/container_environment.sh", "w");
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
				FILE *file = fopen(filename, "w");
				fprintf(file, "%s", value);
				fclose(file);
			}
			fprintf(shell_file, "export %s=%s\n", safe_key, safe_value);
			free(key);
			free(value);
			free(safe_key);
			free(safe_value);
		}
	}
	fclose(shell_file);
}

void
ignore_signals(int signame)
{
	signal(SIGTERM, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	info("received interrupt");
	signaled = 1;
}

void
alarm_handler(int signame)
{
	info("ALARM!");
	alarmed = 1;
}

void
kill_all_processes(int timeout)
{
	int done = 0;
	int ret = 0;
	int status = 0;

	info("Killing all processes...");
	(void) kill(-1, SIGTERM);
	(void) alarm(timeout);
	while (done == 0 && alarmed == 0) {
		// wait for all children
		ret = waitpid(-1, &status, 0);
		if (ret == -1 && errno == ECHILD)
			done = 1;
	}
	if (done == 0) {
		warn("Not all processes have exited in time. Forcing them to exit.");
		// try killing with stronger signal
		(void) kill(-1, SIGKILL);
	}
	// cancel the alarm if it hasn't gone off yet
	(void) alarm(0);
}

pid_t
run_init()
{
	pid_t init_pid;
	// char** env = get_env();
	char *args[] = {"/opt/gonano/sbin/runsvdir", "-P", "/etc/service", 0};
	// char *args[] = {"/bin/sleep", "3600", 0};

	if ((init_pid = fork()) == 0) {
		import_envvars(1, 1);
		execve(args[0], args, environ);
	}

	return init_pid;
}

int
main(int argc, char *argv[])
{
	pid_t init_pid = 0;
	pid_t child_pid = 0;
	int status = 0;
	int ret = 0;

	log_level = LOG_LEVEL_INFO;
	info("setting signal handlers");
	struct sigaction alarm;
	struct sigaction other;
	alarm.sa_handler = alarm_handler;
	sigfillset(&alarm.sa_mask);
	alarm.sa_flags = 0;
	other.sa_handler = ignore_signals;
	sigfillset(&other.sa_mask);
	other.sa_flags = 0;

	sigaction(SIGALRM, &alarm, NULL);
	sigaction(SIGINT, &other, NULL);
	sigaction(SIGTERM, &other, NULL);
	// signal(SIGTERM, ignore_signals);
	// signal(SIGINT, ignore_signals);
	// signal(SIGALRM, alarm_handler);

	info("importing envvars");
	import_envvars(0, 0);
	info("exporting envvars");
	export_envvars(1);

	info("running init");
	init_pid = run_init();
	if (init_pid > 0) {
		while (child_pid != init_pid && signaled == 0) {
			child_pid = waitpid(-1, &status, 0);
			if (child_pid == -1 && errno == EINTR)
				info("Wait interruped");
		}
	}

	if (signaled)
		warn("Init system aborted.");

	kill_all_processes(KILL_ALL_PROCESSES_TIMEOUT);
	return ret;
}