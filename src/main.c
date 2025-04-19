#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "monitor.h"
#include "segment.h"
#include "network.h"
#include "downloader.h"
#include "writer.h"

void startDownload(const char *url, Monitor *monitor)
{
    if (!url || !monitor)
    {
        fprintf(stderr, "Error: Invalid parameters\n");
        return;
    }

    // Initialize CURL globally once
    curl_global_init(CURL_GLOBAL_ALL);

    printf("Getting file size for: %s\n", url);
    long fileSize = getFileSize(url);
    if (fileSize <= 0)
    {
        fprintf(stderr, "Error: Failed to get file size.\n");
        curl_global_cleanup();
        return;
    }

    printf("File size: %ld bytes\n", fileSize);

    // Ensure downloads directory exists
    if (ensureDownloadDirectory() != 0)
    {
        curl_global_cleanup();
        return;
    }

    // Get base filename for output and prepend downloads directory
    char *filename = get_filename_from_url(url);
    if (!filename)
    {
        fprintf(stderr, "Error: Could not determine output filename\n");
        curl_global_cleanup();
        return;
    }

    // Create full path with downloads directory
    char *baseOutputPath = malloc(strlen("downloads/") + strlen(filename) + 1);
    if (!baseOutputPath)
    {
        fprintf(stderr, "Error: Memory allocation failed\n");
        free(filename);
        curl_global_cleanup();
        return;
    }
    sprintf(baseOutputPath, "downloads/%s", filename);
    free(filename);

    // Add download to monitor
    addDownload(monitor, baseOutputPath);

    // Determine number of threads based on file size
    int numThreads = MAX_THREADS;
    if (fileSize < 1024 * 1024) // If file is smaller than 1MB
        numThreads = 1;
    else if (fileSize < 10 * 1024 * 1024) // If file is smaller than 10MB
        numThreads = 4;

    // Divide file into segments with proper paths
    FileSegment *segments = malloc(numThreads * sizeof(FileSegment));
    if (!segments)
    {
        fprintf(stderr, "Error: Memory allocation failed\n");
        free(baseOutputPath);
        curl_global_cleanup();
        return;
    }

    // Calculate segments
    long segmentSize = fileSize / numThreads;
    for (int i = 0; i < numThreads; i++)
    {
        segments[i].start = i * segmentSize;
        segments[i].end = (i == numThreads - 1) ? fileSize - 1 : (i + 1) * segmentSize - 1;
        segments[i].url = strdup(url);
        segments[i].outputPath = createSegmentPath(baseOutputPath, i);
        segments[i].segmentId = i;
        segments[i].isCompleted = 0;

        if (!segments[i].url || !segments[i].outputPath)
        {
            fprintf(stderr, "Error: Memory allocation failed\n");
            // Cleanup previous segments
            for (int j = 0; j < i; j++)
            {
                free(segments[j].url);
                free(segments[j].outputPath);
            }
            free(segments);
            free(baseOutputPath);
            curl_global_cleanup();
            return;
        }
    }

    // Initialize progress tracking
    DownloadProgress progress;
    if (pthread_mutex_init(&progress.mutex, NULL) != 0)
    {
        fprintf(stderr, "Error: Mutex init failed\n");
        freeSegments(segments, numThreads);
        free(baseOutputPath);
        curl_global_cleanup();
        return;
    }

    progress.downloadedBytes = 0;
    progress.totalBytes = fileSize;
    progress.completedSegments = 0;
    progress.totalSegments = numThreads;
    progress.isPaused = 0;

    // Start download threads
    pthread_t *threads = malloc(numThreads * sizeof(pthread_t));
    if (!threads)
    {
        fprintf(stderr, "Error: Memory allocation failed for threads\n");
        pthread_mutex_destroy(&progress.mutex);
        freeSegments(segments, numThreads);
        free(baseOutputPath);
        curl_global_cleanup();
        return;
    }

    for (int i = 0; i < numThreads; i++)
    {
        if (pthread_create(&threads[i], NULL, downloadSegment, &segments[i]) != 0)
        {
            fprintf(stderr, "Error: Thread creation failed\n");
            // We'll continue with other threads
        }
    }

    // Wait for threads to complete
    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Check if all segments completed successfully
    if (allSegmentsCompleted(segments, numThreads))
    {
        printf("All segments downloaded successfully. Merging...\n");
        if (mergeSegments(baseOutputPath, segments, numThreads) == 0)
        {
            printf("Download completed successfully: %s\n", baseOutputPath);
            updateProgress(monitor, baseOutputPath, 100);
            printf("File saved to downloads.\n");
        }
        else
        {
            fprintf(stderr, "Error: Failed to merge segments\n");
            updateProgress(monitor, baseOutputPath, -1);
        }
    }
    else
    {
        fprintf(stderr, "Error: Some segments failed to download\n");
        updateProgress(monitor, baseOutputPath, -1);
    }

    // Cleanup
    pthread_mutex_destroy(&progress.mutex);
    free(threads);
    freeSegments(segments, numThreads);
    free(segments);
    free(baseOutputPath);
    curl_global_cleanup();
}

int main(int argc, char *argv[])
{
    if (argc != 3 || strcmp(argv[1], "download") != 0)
    {
        printf("Usage: %s download \"link_to_download\"\n", argv[0]);
        return 1;
    }

    const char *url = argv[2];
    Monitor monitor;
    initMonitor(&monitor);

    startDownload(url, &monitor);

    destroyMonitor(&monitor);
    return 0;
}