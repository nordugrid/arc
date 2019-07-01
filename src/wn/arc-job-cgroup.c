#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define STR1(x) #x
#define STR(x) STR1(x)

static void usage(char *pname) {
    fprintf(stderr, "Usage: %s {-m|-c} {-d|-n NAME}\n");
    fprintf(stderr, "Options:\n  -m      Use memory controller\n  -c      Use cpuacct controller\n  -d      Delete cgroup (move processes to parent cgroup)\n  -n NAME Name of the nested cgroup to create\n\n", pname);
}

// Provide the path to the mountpoint where requested cgroup controller tree is mounted
int get_cgroup_mount_root(const char *req_controller, char **cgroup_root) {
    FILE *proc_mount_fd = NULL;
    proc_mount_fd = fopen("/proc/mounts", "re");

    if (!proc_mount_fd) {
        fprintf(stderr, "ERROR: Cannot read the /proc/mounts\n");
        return 1;
    }

    char *fs_spec = malloc(sizeof(char) * (FILENAME_MAX + 1));
    char *fs_mount = malloc(sizeof(char) * (FILENAME_MAX + 1));
    char fs_type[4096];
    char fs_mntopts[4096];
    int fs_freq, fs_passno;
    char *mntopt;
    char *mntopt_pos;
    int found = 0;
    while (!found && fscanf(proc_mount_fd, "%s %s %s %s %d %d", fs_spec, fs_mount, fs_type, fs_mntopts, &fs_freq, &fs_passno) == 6 ) {
        if (strcmp(fs_type, "cgroup")) continue;
        // split mount options to find the requested controller
        mntopt = strtok_r(fs_mntopts, ",", &mntopt_pos);
        while (mntopt) {
            if (strncmp(req_controller, mntopt, strlen(req_controller) + 1) == 0) {
                *cgroup_root = strdup(fs_mount);
                found = 1;
                break;
            }
            mntopt = strtok_r(NULL, ",", &mntopt_pos);
        }
    }
    free(fs_spec);
    free(fs_mount);
    fclose(proc_mount_fd);

    if (!found) {
        fprintf(stderr, "ERROR: Cannot find cgroup mountpoint for %s controller\n", req_controller);
        return 1;
    }
    return 0;
}

// Provide current path to process cgroup for requested controller (relative to the cgroup tree mount point)
int get_cgroup_controller_path(pid_t pid, const char *req_controller, char **cgroup_path) {
    char pid_cgroup_file[32];
    FILE *pid_cgroup_fd = NULL;
    // open the /proc/<PID>/cgroup file
    sprintf(pid_cgroup_file, "/proc/%d/cgroup", (int)pid);
    pid_cgroup_fd = fopen(pid_cgroup_file, "re");
    if (!pid_cgroup_fd) {
        fprintf(stderr, "ERROR: Failed to open %s\n", pid_cgroup_file);
        return 1;
    }

    char controllers[64];
    char *controllers_path = malloc(sizeof(char) * (FILENAME_MAX + 1));
    int idx;
    char *controller = NULL;
    char *controller_pos = NULL;
    int found = 0;
    // and find the requested controller path
    while (!found && fscanf(pid_cgroup_fd, "%d:%64[^:]:%"STR(FILENAME_MAX)"s\n", &idx, controllers, controllers_path) == 3 ){
        controller = strtok_r(controllers, ",", &controller_pos);
        while (controller) {
            if (strncmp(req_controller, controller, strlen(req_controller) + 1) == 0) {
                // remove slash in the end if present
                if (controllers_path[strlen(controllers_path) - 1] == '/') {
                    controllers_path[strlen(controllers_path) - 1] = '\0';
                }
                *cgroup_path = strdup(controllers_path);
                found = 1;
                break;
            }
            controller = strtok_r(NULL, ",", &controller_pos);
        }
    }
    free(controllers_path);
    fclose(pid_cgroup_fd);

    if (!found) {
        fprintf(stderr, "ERROR: Cannot find %s cgroup controller for PID %d\n", req_controller, pid);
        return 1;
    }
    return 0;
}

// Create a new cgroup and move process with defined PID to it
int move_pid_to_cgroup(pid_t pid, const char *cgroup_path) {
    FILE *cgroup_tasks = NULL;
    // create cgroup
    if ( mkdir(cgroup_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 ) {
        switch (errno) {
            case EPERM:
                fprintf(stderr, "ERROR: No permissions to create cgroup %s. Do you have a SUID bit set?\n", cgroup_path);
                break;
            case ENOENT:
                fprintf(stderr, "ERROR: Failed to create a cgroup. A component of the cgroup path prefix specified by %s does not name an existing cgroup.\n", cgroup_path);
                break;
            default:
                fprintf(stderr, "ERROR: Failed to create cgroup %s: %s\n.", cgroup_path, strerror(errno));
        }
        return 1;
    }
    // move pid to cgroup
    char *task_path = malloc(sizeof(char) * (FILENAME_MAX + 1));
    snprintf(task_path, FILENAME_MAX, "%s/tasks", cgroup_path);
    int retval = 1;
    while (1) {
        cgroup_tasks = fopen(task_path, "we");
        if (!cgroup_tasks) {
            switch (errno) {
                case EPERM:
                    fprintf(stderr, "ERROR: No permissions to add task to cgroup. Do you have a SUID set?\n");
                    break;
                default:
                    fprintf(stderr, "ERROR: Failed to add task to cgroup: %s\n", strerror(errno));
            }
            break;
        }
        if (fprintf(cgroup_tasks, "%d", pid) < 0) {
            fprintf(stderr, "ERROR: Failed to add task to cgroup: %s\n", strerror(errno));
            break;
        }
        if (fflush(cgroup_tasks) < 0) {
            fprintf(stderr, "ERROR: Failed to add task to cgroup: %s\n", strerror(errno));
            break;
        }
        fprintf(stderr, "Cgroup %s has been successfully created for PID %d\n", cgroup_path, pid);
        retval = 0;
        break;
    }
    free(task_path);
    fclose(cgroup_tasks);
    return retval;
}

// Remove cgroup (moving all processed to parent cgroup)
int move_pids_to_parent_cgroup(const char *cgroup_path, const char *cgroup_root) {
    char *last_slash = NULL;
    char *parent_cgroup_path =  malloc(sizeof(char) * (FILENAME_MAX + 1));
    char *current_cgroup_path =  malloc(sizeof(char) * (FILENAME_MAX + 1));

    strcpy(current_cgroup_path, cgroup_path);
    strcpy(parent_cgroup_path, cgroup_path);
    // check for top-level cgroup
    if (parent_cgroup_path[strlen(parent_cgroup_path) - 1] == '/') {
        parent_cgroup_path[strlen(parent_cgroup_path) - 1] = '\0';
    }
    FILE *cgroup_tasks_fd = NULL;
    FILE *pcgroup_tasks_fd = NULL;
    int retval = 1;
    while(1) {
        if ( strcmp(parent_cgroup_path, cgroup_root) == 0 ) {
            fprintf(stderr, "Process is already belongs to top-level cgroup. Nothing to do.\n");
            retval = 0;
            break;
        }
        // crop last part of the path
        last_slash = strrchr(parent_cgroup_path, '/');
        last_slash[0] = '\0';
        // open tasks files for both cgroups
        strcat(current_cgroup_path, "/tasks");
        strcat(parent_cgroup_path, "/tasks");
        cgroup_tasks_fd = fopen(current_cgroup_path, "re");
        if (!cgroup_tasks_fd) {
            fprintf(stderr, "ERROR: Failed to open %s for reading. %s\n", cgroup_tasks_fd, strerror(errno));
            break;
        }
        pcgroup_tasks_fd = fopen(parent_cgroup_path, "we");
        if (!pcgroup_tasks_fd) {
            fprintf(stderr, "ERROR: Failed to open %s for writing. %s\n", pcgroup_tasks_fd, strerror(errno));
            break;
        }
        // migrate processes
        int cgpid;
        while(fscanf(cgroup_tasks_fd, "%d", &cgpid) == 1) {
            if (fprintf(pcgroup_tasks_fd, "%d", cgpid) < 0) {
                fprintf(stderr, "WARNING: Failed to migrate process %d to parent cgroup. %s\n", cgpid, strerror(errno));
                continue;
            }
            // one process per write() call
            if (fflush(pcgroup_tasks_fd) < 0) {
                fprintf(stderr, "WARNING: Failed to migrate process %d to parent cgroup. %s\n", cgpid, strerror(errno));
                continue;
            }
        }
        // delete cgroup
        if (rmdir(cgroup_path) != 0) {
            fprintf(stderr, "ERROR: Failed to remove the cgroup: %s. %s\n", cgroup_path, strerror(errno));
            break;
        }
        // info
        fprintf(stderr, "Cgroup %s has been successfully removed\n", cgroup_path);
        retval = 0;
        break;
    }
    free(parent_cgroup_path);
    free(current_cgroup_path);
    fclose(pcgroup_tasks_fd);
    fclose(cgroup_tasks_fd);
    return retval;
}

int main(int argc, char *argv[]) {
    const char *cgroup_name = NULL;
    const char *controller = NULL;
    char opt;
    int delete_mode = 0;
    // Parse command line options
    while ((opt = getopt(argc, argv, "dmcn:")) != -1) {
        switch (opt) {
            case 'n':
                cgroup_name = optarg;
                break;
            case 'd':
                delete_mode = 1;
                break;
            case 'm':
                controller = "memory";
                break;
            case 'c':
                controller = "cpuacct";
                break;
            default:
                fprintf(stderr, "ERROR: Unknown option %s", opt);
                usage(argv[0]);
                return 1;
        }
    }
    if ( !delete_mode && !cgroup_name ) {
        fprintf(stderr, "ERROR: Name of child cgroup to be created should be specified\n");
        usage(argv[0]);
        return 1;
    }
    if (!controller) {
        fprintf(stderr, "ERROR: Controller type (-m or -c) should be specified\n");
        usage(argv[0]);
        return 1;
    }
    char *cgroup_root = malloc(sizeof(char) * (FILENAME_MAX + 1));
    char *controller_path = malloc(sizeof(char) * (FILENAME_MAX + 1));
    char *cgroup_path = malloc(sizeof(char) * (FILENAME_MAX + 1));
    int retval = 1;
    while (1) {
        // get cgroup root mount
        if (get_cgroup_mount_root(controller, &cgroup_root) != 0) {
            break;
        }
        // get controller-specific path for the parent process
        pid_t ppid = getppid();
        if (get_cgroup_controller_path(ppid, controller, &controller_path) != 0) {
            break;
        }
        // print info
        fprintf(stderr, "Found %s controller for PID %d: %s%s\n", controller, (int)ppid, cgroup_root, controller_path);
        // check root
        uid_t euid = geteuid();
        if ( euid ) {
            fprintf(stderr, "WARNING: The command is running as non-root and most probably have no access rights. It is designed with SUID bit in mind.\n");
        }
        // Delete vs Create
        if (delete_mode) {
            // delete PPID cgroup (moving all processed to parent cgroup)
            sprintf(cgroup_path, "%s%s", cgroup_root, controller_path);
            if (move_pids_to_parent_cgroup(cgroup_path, cgroup_root) != 0 ) {
                break;
            }
        } else {
            // create a child cgroup and mode PPID to it
            snprintf(cgroup_path, FILENAME_MAX, "%s%s/%s", cgroup_root, controller_path, cgroup_name);
            if (move_pid_to_cgroup(ppid, cgroup_path) != 0 ) {
                break;
            }
            // print full path to new cgroup to the stdout
            printf("%s\n", cgroup_path);
            retval = 0;
            break;
        }
    }
    free(cgroup_root);
    free(controller_path);
    free(cgroup_path);
    return retval;
}
