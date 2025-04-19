#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include "common.h"

// Helper function to create segment paths
char* createSegmentPath(const char* outputPath, int segmentId);

// Function to download a single segment in a thread
void* downloadSegmentThread(void* arg);

// Function to start a multithreaded download
int startThreadedDownload(const char* url, const char* outputPath, int numThreads, DownloadProgress* progress);

#endif // DOWNLOADER_H