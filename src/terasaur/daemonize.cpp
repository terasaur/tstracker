/**
 * Copyright 2012 ibiblio
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <sys/stat.h> // umask()
#include <unistd.h> // fork()
#include <stdlib.h>

namespace terasaur {

/**
 * Perform double fork to daemonize process on Unix platforms.
 * It's a little different than Stevens' C example, more like the Unix
 * Programming FAQ http://www.erlenstar.demon.co.uk/unix/faq_toc.html
 */
void daemonize(void)
{
    // Fork, divorce from parent, and exit parent process
    int pid = fork();
    if (pid == -1) {
        printf("Error during first fork\n");
        exit(1);
    }
    if (pid != 0) {
        exit(0);
    }

    // Become session leader
    setsid();

    // Fork again, become non-session group leader, exit parent process
    pid = fork();
    if (pid == -1) {
        printf("Error during second fork\n");
        exit(1);
    }
    if (pid != 0) {
        exit(0);
    }

    // Don't leave our pwd in a directory, and clear umask bits
    chdir("/");
    umask(0);

    // Close fds 0, 1, and 2, and open new descriptors
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);

    stdin = fopen("/dev/null", "r");
    stdout = fopen("/dev/null", "a+");
    stderr = fopen("/dev/null", "a+");
}
} // namespace terasaur
