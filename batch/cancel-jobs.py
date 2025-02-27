#!/usr/bin/env python3
import boto3
import sys

AWS_PROFILE = "or"
boto3.setup_default_session(profile_name=AWS_PROFILE, region_name="us-east-1")


def cancel_runnable_jobs(queue_name):
    # Initialize Batch client
    batch_client = boto3.client("batch")

    # List all RUNNABLE jobs in the specified queue
    response = batch_client.list_jobs(jobQueue=queue_name, jobStatus="RUNNABLE")

    job_ids = [job["jobId"] for job in response.get("jobSummaryList", [])]

    if not job_ids:
        print(f"No RUNNABLE jobs found in queue {queue_name}")
        return

    # Cancel each job
    for job_id in job_ids:
        print(f"Cancelling job {job_id}")
        batch_client.cancel_job(jobId=job_id, reason="Batch cancellation requested")
        print(f"Job {job_id} cancelled")

    # Check for additional pages of results
    while "nextToken" in response:
        response = batch_client.list_jobs(
            jobQueue=queue_name, jobStatus="RUNNABLE", nextToken=response["nextToken"]
        )

        job_ids = [job["jobId"] for job in response.get("jobSummaryList", [])]

        for job_id in job_ids:
            print(f"Cancelling job {job_id}")
            batch_client.cancel_job(jobId=job_id, reason="Batch cancellation requested")
            print(f"Job {job_id} cancelled")

    print(f"All RUNNABLE jobs in queue {queue_name} have been cancelled")


# Example usage
if __name__ == "__main__":
    queue_name = sys.argv[1]
    cancel_runnable_jobs(queue_name)
