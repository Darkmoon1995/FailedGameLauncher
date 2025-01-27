#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define LIBRARY_FILE "/steamapps/libraryfolders.vdf"

// Function to check if a file exists
int file_exists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

// Function to find Steam path using `find` command
int find_steam_path(char *steam_path, size_t size) {
    FILE *fp = popen("find /home/$USER -type d -name 'Steam' 2>/dev/null", "r");
    if (!fp) {
        fprintf(stderr, "Failed to run find command.\n");
        return 0;
    }

    if (fgets(steam_path, size, fp)) {
        // Remove newline from the path
        steam_path[strcspn(steam_path, "\n")] = '\0';
        pclose(fp);
        return 1;
    }

    pclose(fp);
    return 0;
}

int main() {
    char steam_path[512];
    if (!find_steam_path(steam_path, sizeof(steam_path))) {
        printf("Steam directory not found.\n");
        return 1;
    }

    // Create the path to the appcache stats directory
    char *steam_game_path = strcpy(malloc(strlen(steam_path) + strlen("/appcache/stats") + 1), steam_path);
    strcat(steam_game_path, "/appcache/stats");
    printf("Steam Game Finder Path: %s\n", steam_game_path);

    // Find all available game IDs
    DIR *d;
    struct dirent *dir;
    d = opendir(steam_game_path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strstr(dir->d_name, "UserGameStats_")) {
                size_t length = strlen(dir->d_name);
                char *start = strrchr(dir->d_name, '_');  // Find the last '_'
                int end_index = length - 4;  // Adjust for ".bin" or similar suffix
                if (start) {
                    dir->d_name[end_index] = '\0';  // Null-terminate at the calculated end index
                    char *game_id = start + 1;  // Extract the game ID


                    // Build and execute the curl command
                    char command[256];
                    snprintf(command, sizeof(command),
                             "curl -s https://store.steampowered.com/app/%s/ | grep -i \"<title>\"", game_id);

                    FILE *fp = popen(command, "r");
                    if (!fp) {
                        perror("Failed to execute curl command");
                        continue;
                    }

                    // Read and display the command output
                    char result[1024];
                    if (fgets(result, sizeof(result), fp)) {
                        // Remove the first 7 and last 8 characters from the result
                        size_t result_len = strlen(result);
                        if (result_len > 15) {
                            result[result_len - 9] = '\0'; // Remove the last 8 characters
                            memmove(result, result + 9, result_len - 9); // Remove the first 7 characters
                        }
                        printf("%s\n", result);
                    } else {
                        printf("No title found for Game ID %s\n", game_id);
                    }
                    pclose(fp);
                }
            }
        }
        closedir(d);
    } else {
        perror("Failed to open directory");
    }

    free(steam_game_path);
    return 0;
}
