#include "shell/job_manager.h"
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
#include <cerrno>
#include <cstring>

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

    Job& job = it->second;
    std::cout << job.command << "\n";

    // Give terminal control to the job's process group
    if (tcsetpgrp(STDIN_FILENO, job.pgid) == -1) {
        std::cerr << "fg: failed to give terminal control to job\n";
        return;
    }

    // If the job was stopped, send SIGCONT to resume it
    if (job.status == JobStatus::STOPPED) {
        if (kill(-job.pgid, SIGCONT) == -1) {
            std::cerr << "fg: failed to resume job\n";
            return;
        }
    }

    // Update job status to running
    job.status = JobStatus::RUNNING;

    // Wait for the job to complete or be stopped
    int status;
    pid_t result;
    do {
        result = waitpid(-job.pgid, &status, WUNTRACED);
    } while (result == -1 && errno == EINTR);

    // Restore terminal control to the shell
    pid_t shell_pgid = getpgrp();
    tcsetpgrp(STDIN_FILENO, shell_pgid);

    // Update job status based on how it terminated
    if (result > 0) {
        if (WIFSTOPPED(status)) {
            // Job was stopped (Ctrl+Z)
            job.status = JobStatus::STOPPED;
            std::cout << "\n[" << job.job_id << "] Stopped " << job.command << "\n";
        } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
            // Job completed or was terminated
            job.status = WIFEXITED(status) ? JobStatus::DONE : JobStatus::TERMINATED;
            // Remove completed job
            jobs.erase(it);
        }
    }
}

void JobManager::resumeInBackground(int job_id) {
    auto it = jobs.find(job_id);
    if (it == jobs.end()) {
        std::cerr << "bg: job " << job_id << " not found\n";
        return;
    }

    Job& job = it->second;

    // Only resume if the job is stopped
    if (job.status != JobStatus::STOPPED) {
        std::cerr << "bg: job " << job_id << " is not stopped\n";
        return;
    }

    // Send SIGCONT to resume the job in the background
    if (kill(-job.pgid, SIGCONT) == -1) {
        std::cerr << "bg: failed to resume job: " << strerror(errno) << "\n";
        return;
    }

    // Update job status to running
    job.status = JobStatus::RUNNING;
    std::cout << "[" << job.job_id << "] " << job.command << " &\n";
}

void JobManager::updateJobStatus(pid_t pid, JobStatus status) {
    // Find job by PID and update its status
    // Note: This is called from signal handler, so must be signal-safe (no I/O)
    for (auto& pair : jobs) {
        if (pair.second.pgid == pid) {
            pair.second.status = status;
            break;
        }
    }
}

void JobManager::checkCompletedJobs() {
    // Check for completed background jobs using waitpid with WNOHANG
    // This is called from SIGCHLD handler, so must be signal-safe
    // Note: We mark jobs as completed but don't print or erase here

    int status;
    pid_t pid;

    // WNOHANG means return immediately if no child has exited
    // -1 means wait for any child process
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        JobStatus job_status;

        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            // Process has terminated
            job_status = WIFEXITED(status) ? JobStatus::DONE : JobStatus::TERMINATED;
            updateJobStatus(pid, job_status);
        } else if (WIFSTOPPED(status)) {
            // Process was stopped (Ctrl+Z)
            updateJobStatus(pid, JobStatus::STOPPED);
        }
    }
}

void JobManager::printAndCleanCompletedJobs() {
    // Print notifications for completed jobs and remove them
    // This is called from main loop, so it's safe to use I/O
    for (auto it = jobs.begin(); it != jobs.end(); ) {
        const Job& job = it->second;

        if (job.status == JobStatus::DONE || job.status == JobStatus::TERMINATED) {
            // Print notification
            std::cout << "[" << job.job_id << "] ";
            if (job.status == JobStatus::DONE) {
                std::cout << "Done";
            } else {
                std::cout << "Terminated";
            }
            std::cout << " " << job.command << "\n";

            // Remove the completed job
            it = jobs.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace helix
