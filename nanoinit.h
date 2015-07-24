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

