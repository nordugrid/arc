#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define STR1(x) #x
#define STR(x) STR1(x)

#define isoctal(x) (((x) & ~7) == '0')

static void usage(char *pname) {
    fprintf(stderr, "Usage: %s {-m|-c} {-d|-n NAME}\n", pname);
    fprintf(stderr, "Options:\n  -m      Use memory controller\n  -c      Use cpuacct controller\n  -d      Delete cgroup (move processes to parent cgroup)\n  -n NAME Name of the nested cgroup to create\n\n");
}

// Provide the path to the mountpoint where requested cgroup controller tree is mounted
int get_cgroup_mount_root(const char *req_controller, char *cgroup_root_ptr) {
    FILE *proc_mount_fd = NULL;
    proc_mount_fd = fopen("/proc/mounts", "r");
    if (!proc_mount_fd) {
        fprintf(stderr, "ERROR: Cannot read the /proc/mounts\n");
        return 1;
    }
    // parse /proc/mounts looking for req_contrller in mount options
    char *fs_spec = malloc(sizeof(char) * (FILENAME_MAX + 1));
    char *fs_mount = malloc(sizeof(char) * (FILENAME_MAX + 1));
    char fs_type[4096+1];
    char fs_mntopts[4096+1];
    int fs_freq, fs_passno;
    char *mntopt;
    char *mntopt_pos;
    int found = 0;
    while (!found && fscanf(proc_mount_fd, "%" STR(FILENAME_MAX) "s %" STR(FILENAME_MAX) "s %4096s %4096s %d %d\n", fs_spec, fs_mount, fs_type, fs_mntopts, &fs_freq, &fs_passno) == 6 ) {
        if (strcmp(fs_type, "cgroup")) continue;
        // split mount options to find the requested controller
        mntopt = strtok_r(fs_mntopts, ",", &mntopt_pos);
        while (mntopt) {
            if ((strncmp(req_controller, mntopt, strlen(req_controller) + 1) == 0) && (mntopt[strlen(req_controller)] == ',' || mntopt[strlen(req_controller)] == '\0')) {
                found = 1;
                // mounts use \oct encoding for unwanted characters
                size_t sz = 0;
                char *mount = fs_mount;
                char *cgroup_root = cgroup_root_ptr;
                while(*mount && sz < FILENAME_MAX) {
                    if (*mount == '\\' && sz + 3 < FILENAME_MAX && 
                        isoctal(mount[1]) && isoctal(mount[2]) && isoctal(mount[3])) {
                        *cgroup_root++ = 64*(mount[1] & 7) + 8*(mount[2] & 7) + (mount[3] & 7);
                        mount += 4;
                        sz += 4;
                    } else {
                        *cgroup_root++ = *mount++;
                        sz++;
                    }
                }
                *cgroup_root = '\0';
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
int get_cgroup_controller_path(pid_t pid, const char *req_controller, char *cgroup_path) {
    char pid_cgroup_file[64];
    FILE *pid_cgroup_fd = NULL;
    // open the /proc/<PID>/cgroup file
    snprintf(pid_cgroup_file, 64, "/proc/%d/cgroup", (int)pid);
    pid_cgroup_fd = fopen(pid_cgroup_file, "r");
    if (!pid_cgroup_fd) {
        fprintf(stderr, "ERROR: Failed to open %s\n", pid_cgroup_file);
        return 1;
    }
    // parse process cgroups looking for req_controller path
    char controllers[64];
    char *controllers_path = malloc(sizeof(char) * (FILENAME_MAX + 1));
    int idx;
    char *controller = NULL;
    char *controller_pos = NULL;
    int found = 0;
    // and find the requested controller path
    while (!found && fscanf(pid_cgroup_fd, "%d:%64[^:]:%"STR(FILENAME_MAX)"[^\n]\n", &idx, controllers, controllers_path) == 3 ){
        controller = strtok_r(controllers, ",", &controller_pos);
        while (controller) {
            if ((strncmp(req_controller, controller, strlen(req_controller) + 1) == 0) && (controller[strlen(req_controller)] == ',' || controller[strlen(req_controller)] == '\0')) {
                // remove slash in the end if present
                if (strlen(controllers_path) && controllers_path[strlen(controllers_path) - 1] == '/') {
                    controllers_path[strlen(controllers_path) - 1] = '\0';
                }
                // check controllers_path starts with '/' if not empty
                if (strlen(controllers_path) && controllers_path[0] != '/') {
                    fprintf(stderr, "WARNING: Found controller path '%s' but it is not started with '/'. Skipping.", controllers_path);
                    continue;
                }
                strncpy(cgroup_path, controllers_path, FILENAME_MAX); cgroup_path[FILENAME_MAX] = '\0';
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
    char *task_path = malloc(sizeof(char) * (FILENAME_MAX + 1));
    // change umask to avoid too restrictive values
    umask(S_IWGRP | S_IWOTH);
    // create cgroup
    int retval = 1;
    do {
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
            break;
        }
        // get cgroup tasks file path
        int ncopied;
        ncopied = snprintf(task_path, FILENAME_MAX, "%s/tasks", cgroup_path);
        if ( ncopied < 0 || ncopied >= FILENAME_MAX ) {
            fprintf(stderr, "ERROR: Failed to get cgroup %s tasks path. Overflow detected.", cgroup_path);
            break;
        }
        // move pid to cgroup
        cgroup_tasks = fopen(task_path, "w");
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
        if (fflush(cgroup_tasks) != 0) {
            fprintf(stderr, "ERROR: Failed to add task to cgroup: %s\n", strerror(errno));
            break;
        }
        fprintf(stderr, "Cgroup %s has been successfully created for PID %d\n", cgroup_path, pid);
        retval = 0;
    } while (0);

    free(task_path);
    if (cgroup_tasks) fclose(cgroup_tasks);
    return retval;
}

// Remove cgroup (moving all processed to parent cgroup)
int move_pids_to_parent_cgroup(const char *cgroup_path, const char *cgroup_root) {
    char *parent_cgroup_path =  malloc(sizeof(char) * (FILENAME_MAX + 1));
    char *current_cgroup_path =  malloc(sizeof(char) * (FILENAME_MAX + 1));
    strncpy(current_cgroup_path, cgroup_path, FILENAME_MAX); current_cgroup_path[FILENAME_MAX] = '\0';
    strncpy(parent_cgroup_path, cgroup_path, FILENAME_MAX); parent_cgroup_path[FILENAME_MAX] = '\0';
    // check for top-level cgroup
    if (strlen(parent_cgroup_path) && parent_cgroup_path[strlen(parent_cgroup_path) - 1] == '/') {
        parent_cgroup_path[strlen(parent_cgroup_path) - 1] = '\0';
    }
    FILE *cgroup_tasks_fd = NULL;
    FILE *pcgroup_tasks_fd = NULL;
    int retval = 1;
    do {
        if ( strcmp(parent_cgroup_path, cgroup_root) == 0 ) {
            fprintf(stderr, "Process is already belongs to top-level cgroup. Nothing to do.\n");
            retval = 0;
            break;
        }
        // crop last part of the path
        char *last_slash = NULL;
        last_slash = strrchr(parent_cgroup_path, '/');
        if (last_slash) { last_slash[0] = '\0'; }
        // paths points to cgroup tasks file
        if (strlen(current_cgroup_path) > (FILENAME_MAX - 6)) { // parent_cgroup_path is less
            fprintf(stderr, "ERROR: Failed to move PIDs, path to cgroup /tasks interface will lead to the overflow.");
            break;
        }
        strcat(current_cgroup_path, "/tasks");
        strcat(parent_cgroup_path, "/tasks");
        // open tasks files for both cgroups
        cgroup_tasks_fd = fopen(current_cgroup_path, "r");
        if (!cgroup_tasks_fd) {
            fprintf(stderr, "ERROR: Failed to open %s for reading. %s\n", current_cgroup_path, strerror(errno));
            break;
        }
        pcgroup_tasks_fd = fopen(parent_cgroup_path, "w");
        if (!pcgroup_tasks_fd) {
            fprintf(stderr, "ERROR: Failed to open %s for writing. %s\n", parent_cgroup_path, strerror(errno));
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
            if (fflush(pcgroup_tasks_fd) != 0) {
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
    } while (0);

    free(parent_cgroup_path);
    free(current_cgroup_path);
    if (pcgroup_tasks_fd) fclose(pcgroup_tasks_fd);
    if (cgroup_tasks_fd) fclose(cgroup_tasks_fd);
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
    do {
        // get cgroup root mount
        if (get_cgroup_mount_root(controller, cgroup_root) != 0) break;
        // get controller-specific path for the parent process
        pid_t ppid = getppid();
        if (get_cgroup_controller_path(ppid, controller, controller_path) != 0) break;
        // print info
        fprintf(stderr, "Found %s controller for PID %d: %s%s\n", controller, (int)ppid, cgroup_root, controller_path);
        // check root
        uid_t euid = geteuid();
        if ( euid ) {
            fprintf(stderr, "WARNING: The command is running as non-root and most probably have no access rights. It is designed with SUID bit in mind.\n");
        }
        // Delete vs Create
        int ncopied;
        if (delete_mode) {
            // delete PPID cgroup (moving all processed to parent cgroup)
            ncopied = snprintf(cgroup_path, FILENAME_MAX, "%s%s", cgroup_root, controller_path);
            if ( ncopied < 0 || ncopied >= FILENAME_MAX ) {
                fprintf(stderr, "ERROR: Cannot construct cgroup path for %s controller in %s. Overflow detected.", cgroup_path, cgroup_root);
                break;
            }
            if (move_pids_to_parent_cgroup(cgroup_path, cgroup_root) != 0 ) break;
            retval = 0;
        } else {
            // create a child cgroup and mode PPID to it
            ncopied = snprintf(cgroup_path, FILENAME_MAX, "%s%s/%s", cgroup_root, controller_path, cgroup_name);
            if ( ncopied < 0 || ncopied >= FILENAME_MAX ) {
                fprintf(stderr, "ERROR: Cannot construct child cgroup %s path. Overflow detected.", cgroup_name);
                break;
            }
            if (move_pid_to_cgroup(ppid, cgroup_path) != 0 ) break;
            printf("%s\n", cgroup_path);
            retval = 0;
        }
    } while (0);
    // free
    free(cgroup_root);
    free(controller_path);
    free(cgroup_path);
    return retval;
}
