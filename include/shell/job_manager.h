#ifndef HELIX_JOB_MANAGER_H
#define HELIX_JOB_MANAGER_H

#include "types.h"
#include <map>

namespace helix {

// JobManager - Manages background and foreground jobs
// Responsibilities:
// - Track active jobs
// - Bring jobs to foreground
// - Resume jobs in background
// - Print job status
class JobManager {
public:
    JobManager() = default;
    ~JobManager() = default;

    // Add a new job
    void addJob(int pid, const std::string& command);

    // Remove a job
    void removeJob(int job_id);

    // Print all jobs
    void printJobs() const;

    // Bring a job to foreground
    void bringToForeground(int job_id);

    // Resume a job in background
    void resumeInBackground(int job_id);

    // Get jobs map (for direct access)
    const std::map<int, Job>& getJobs() const { return jobs; }

private:
    std::map<int, Job> jobs;
    int next_job_id = 1;
};

} // namespace helix

#endif // HELIX_JOB_MANAGER_H
