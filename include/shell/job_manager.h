#ifndef HELIX_JOB_MANAGER_H
#define HELIX_JOB_MANAGER_H

#include "shell/interfaces.h"
#include "types.h"
#include <map>

namespace helix {

// JobManager - Manages background and foreground jobs
// Implements IJobManager interface (Dependency Inversion Principle)
// Responsibilities:
// - Track active jobs
// - Bring jobs to foreground
// - Resume jobs in background
// - Print job status
class JobManager : public IJobManager {
public:
    JobManager() = default;
    ~JobManager() override = default;

    // Add a new job
    void addJob(int pid, const std::string& command) override;

    // Remove a job
    void removeJob(int job_id) override;

    // Print all jobs
    void printJobs() const override;

    // Bring a job to foreground
    void bringToForeground(int job_id) override;

    // Resume a job in background
    void resumeInBackground(int job_id) override;

    // Get jobs map (for direct access)
    const std::map<int, Job>& getJobs() const override { return jobs; }

private:
    std::map<int, Job> jobs;
    int next_job_id = 1;
};

} // namespace helix

#endif // HELIX_JOB_MANAGER_H
