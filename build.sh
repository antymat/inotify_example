#!/bin/bash -e
gcc src/inotify_example.c -Isrc -pthread -lrt -g -o inotify_example
gcc src/inotify_example_client.c -Isrc -pthread -lrt -g -o inotify_example_client

