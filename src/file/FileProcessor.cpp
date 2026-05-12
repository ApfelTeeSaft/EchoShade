#include "echoshade/FileProcessor.hpp"
#include <atomic>

namespace echoshade {

// Stub
struct FileProcessor::Impl {
    FileProcessorConfig              cfg;
    std::shared_ptr<PlatformProfile> profile;
    std::atomic<bool>                cancelled{false};
};

FileProcessor::FileProcessor(const FileProcessorConfig& cfg,
                             std::shared_ptr<PlatformProfile> profile)
    : impl_(std::make_unique<Impl>())
{
    impl_->cfg     = cfg;
    impl_->profile = std::move(profile);
}

FileProcessor::~FileProcessor() = default;

bool FileProcessor::processFile(const FileJob& /*job*/) {
    // TODO: read audio, apply DSPPipeline, write output.
    return false;
}

bool FileProcessor::processBatch(const std::vector<FileJob>& jobs,
                                 ProgressCallback cb) {
    impl_->cancelled.store(false);
    int done = 0;
    for (const auto& job : jobs) {
        if (impl_->cancelled.load()) break;
        processFile(job);
        ++done;
        if (cb) cb(done, static_cast<int>(jobs.size()), job.inputPath);
    }
    return !impl_->cancelled.load();
}

void FileProcessor::cancel() {
    impl_->cancelled.store(true);
}

} // namespace echoshade
