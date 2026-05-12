#pragma once
#include "PlatformProfile.hpp"
#include "PerturbationEngine.hpp"
#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace echoshade {

struct FileProcessorConfig {
    std::string              outputSuffix   = "_protected";
    bool                     preserveMeta   = true;
    PerturbationConfig       perturbation;
};

struct FileJob {
    std::string inputPath;
    std::string outputPath;   // empty = auto-generate from inputPath + suffix
};

using ProgressCallback = std::function<void(int jobsDone, int jobsTotal,
                                             const std::string& currentFile)>;

/// Offline audio file protection pipeline.
/// Reads audio files, applies perturbation, writes protected output.
class FileProcessor {
public:
    explicit FileProcessor(const FileProcessorConfig& cfg,
                           std::shared_ptr<PlatformProfile> profile);
    ~FileProcessor();

    FileProcessor(const FileProcessor&)            = delete;
    FileProcessor& operator=(const FileProcessor&) = delete;

    /// Process a single file. Returns true on success.
    bool processFile(const FileJob& job);

    /// Process a batch of files. Calls progress callback after each job.
    bool processBatch(const std::vector<FileJob>& jobs,
                      ProgressCallback cb = nullptr);

    /// Cancel an in-progress batch.
    void cancel();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace echoshade
