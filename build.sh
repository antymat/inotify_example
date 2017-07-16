#!/bin/bash -e
gcc src/inotify_example.c -Isrc -pthread -lrt -g -Wall -o inotify_example
gcc src/inotify_example_client.c -Isrc -pthread -lrt -g -Wall -o inotify_example_client

