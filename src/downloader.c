#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "network.h"
#include "segment.h"
#include "writer.h"

// Create temporary segment file paths
char *createSegmentPath(const char *basePath, int segmentId)
{
    // Allocate memory for the segment path (basePath + ".partN")
    char *segmentPath = malloc(strlen(basePath) + 16);
    if (!segmentPath)
    {
        return NULL;
    }
    sprintf(segmentPath, "%s.part%d", basePath, segmentId);
    return segmentPath;
}

// Function to download a single segment
void *downloadSegmentThread(void *arg)
{
    if (!arg)
        return NULL;

    FileSegment *segment = (FileSegment *)arg;
    downloadSegment(segment);
    return NULL;
}

// Start a download with multiple threads
int startThreadedDownload(const char *url, const char *outputPath, int numThreads, DownloadProgress *progress)
{
    if (!url || !outputPath)
        return -1;

    if (numThreads > MAX_THREADS)
        numThreads = MAX_THREADS;
    if (numThreads <= 0)
        numThreads = 1;

    // Get file size
    long fileSize = getFileSize(url);
    if (fileSize <= 0)
    {
        fprintf(stderr, "Failed to get file size for %s\n", url);
        return -1;
    }

    printf("File size: %ld bytes\n", fileSize);

    // Create segments
    FileSegment *segments = malloc(sizeof(FileSegment) * numThreads);
    if (!segments)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    // Initialize segments
    long segmentSize = fileSize / numThreads;
    for (int i = 0; i < numThreads; i++)
    {
        segments[i].start = i * segmentSize;
        segments[i].end = (i == numThreads - 1) ? fileSize - 1 : (i + 1) * segmentSize - 1;
        segments[i].url = strdup(url);
        segments[i].outputPath = createSegmentPath(outputPath, i);
        segments[i].segmentId = i;
        segments[i].isCompleted = 0;

        if (!segments[i].url || !segments[i].outputPath)
        {
            fprintf(stderr, "Memory allocation failed for segment %d\n", i);
            // Clean up segments that were successfully allocated
            for (int j = 0; j < i; j++)
            {
                free(segments[j].url);
                free(segments[j].outputPath);
            }
            free(segments);
            return -1;
        }
    }

    // Initialize progress tracker
    if (progress)
    {
        pthread_mutex_init(&progress->mutex, NULL);
        progress->downloadedBytes = 0;
        progress->totalBytes = fileSize;
        progress->completedSegments = 0;
        progress->totalSegments = numThreads;
        progress->isPaused = 0;
    }

    // Create threads
    pthread_t *threads = malloc(sizeof(pthread_t) * numThreads);
    if (!threads)
    {
        fprintf(stderr, "Memory allocation failed for threads\n");

        // Clean up segments
        freeSegments(segments, numThreads);
        free(segments);

        return -1;
    }

    // Start download threads
    for (int i = 0; i < numThreads; i++)
    {
        if (pthread_create(&threads[i], NULL, downloadSegmentThread, &segments[i]) != 0)
        {
            fprintf(stderr, "Thread creation failed for segment %d\n", i);
            // Continue with other threads
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Check if all segments were completed
    int allCompleted = 1;
    for (int i = 0; i < numThreads; i++)
    {
        if (!segments[i].isCompleted)
        {
            allCompleted = 0;
            fprintf(stderr, "Segment %d failed to download\n", i);
        }
    }

    // Merge segments if all completed
    if (allCompleted)
    {
        printf("Merging segments...\n");
        if (mergeSegments(outputPath, segments, numThreads) != 0)
        {
            fprintf(stderr, "Error: Failed to merge segments\n");
            allCompleted = 0;
        }
    }

    // Clean up
    free(threads);
    freeSegments(segments, numThreads);
    free(segments);

    if (progress)
    {
        pthread_mutex_destroy(&progress->mutex);
    }

    return allCompleted ? 0 : -1;
}