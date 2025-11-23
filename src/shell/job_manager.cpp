#include "shell/job_manager.h"
#include <iostream>

namespace helix {

void JobManager::addJob(int pid, const std::string& command) {
    Job job;
    job.job_id = next_job_id++;
    job.pgid = pid;
    job.command = command;
    job.status = JobStatus::RUNNING;
    jobs[job.job_id] = job;
}

void JobManager::removeJob(int job_id) {
    jobs.erase(job_id);
}

void JobManager::printJobs() const {
    for (const auto& pair : jobs) {
        const Job& job = pair.second;
        std::string status_str = "Running";
        if (job.status == JobStatus::STOPPED) status_str = "Stopped";
        else if (job.status == JobStatus::DONE) status_str = "Done";
        else if (job.status == JobStatus::TERMINATED) status_str = "Terminated";
        std::cout << "[" << job.job_id << "] " << status_str << " " << job.command << "\n";
    }
}

void JobManager::bringToForeground(int job_id) {
    auto it = jobs.find(job_id);
    if (it == jobs.end()) {
        std::cerr << "fg: job " << job_id << " not found\n";
        return;
    }
    // Placeholder for now - full implementation needs tcsetpgrp and SIGCONT
    std::cout << "Bringing job " << job_id << " to foreground\n";
}

void JobManager::resumeInBackground(int job_id) {
    auto it = jobs.find(job_id);
    if (it == jobs.end()) {
        std::cerr << "bg: job " << job_id << " not found\n";
        return;
    }
    // Placeholder for now - full implementation needs SIGCONT
    std::cout << "Resuming job " << job_id << " in background\n";
}

} // namespace helix
