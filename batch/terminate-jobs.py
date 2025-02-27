#!/usr/bin/env python3
import boto3
import sys

AWS_PROFILE = "or"
boto3.setup_default_session(profile_name=AWS_PROFILE, region_name="us-east-1")


def terminate_running_jobs(queue_name):
    # Initialize Batch client
    batch_client = boto3.client("batch")

    # List all RUNNING jobs in the specified queue
    response = batch_client.list_jobs(jobQueue=queue_name, jobStatus="RUNNING")

    job_ids = [job["jobId"] for job in response.get("jobSummaryList", [])]

    if not job_ids:
        print(f"No RUNNING jobs found in queue {queue_name}")
        return

    # Terminate each job
    for job_id in job_ids:
        print(f"Terminating job {job_id}")
        batch_client.terminate_job(jobId=job_id, reason="Batch termination requested")
        print(f"Job {job_id} terminated")

    # Check for additional pages of results
    while "nextToken" in response:
        response = batch_client.list_jobs(
            jobQueue=queue_name, jobStatus="RUNNING", nextToken=response["nextToken"]
        )

        job_ids = [job["jobId"] for job in response.get("jobSummaryList", [])]

        for job_id in job_ids:
            print(f"Terminating job {job_id}")
            batch_client.terminate_job(
                jobId=job_id, reason="Batch termination requested"
            )
            print(f"Job {job_id} terminated")

    print(f"All RUNNING jobs in queue {queue_name} have been terminated")


# Example usage
if __name__ == "__main__":
    queue_name = sys.argv[1]
    terminate_running_jobs(queue_name)
