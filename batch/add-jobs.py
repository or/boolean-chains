#!/usr/bin/env python3
import boto3
import time
import subprocess
import botocore.exceptions
import sys

AWS_PROFILE = "or"
boto3.setup_default_session(profile_name=AWS_PROFILE, region_name="us-east-1")

which = sys.argv[1]

if which == "full":
    JOB_DEFINITION = "boolean-chains-job-definition"
    COMMAND = "full-search"
    JOB_ID = "15-21"
    JOB_PARAMETER_FILE = f"../full-plan-{JOB_ID}.txt"
    JOB_QUEUE = f"boolean-chains-queue-{JOB_ID}"
    S3_OUTPUT_PATH = f"s3://computing-results/boolean-chains/{JOB_ID}"

elif which == "hungry":
    JOB_DEFINITION = "hungry-search-job-definition"
    COMMAND = "hungry-search"
    JOB_ID = "16-22-2"
    JOB_PARAMETER_FILE = f"../hungry-plan-{JOB_ID}.txt"
    JOB_QUEUE = f"hungry-search-queue-{JOB_ID}"
    S3_OUTPUT_PATH = f"s3://computing-results/hungry-search/{JOB_ID}"

else:
    print("first argument must be 'full' or 'hungry'")
    sys.exit(1)

NUMBER_OF_JOBS = int(sys.argv[2])

batch_client = boto3.client("batch")


def with_retry(func, max_retries=7):
    retry = 0
    while True:
        try:
            return func()
        except botocore.exceptions.ClientError as e:
            if "ThrottlingException" in str(e) or "Rate exceeded" in str(e):
                if retry >= max_retries:
                    print("Max retries reached, skipping operation.")
                    return None
                sleep_time = 2**retry
                print(f"Throttled. Retrying in {sleep_time} seconds...")
                time.sleep(sleep_time)
                retry += 1
            else:
                raise


def submit_batch_job(job_name, command_args):
    clean_command_args = command_args.replace(" ", "_")
    output_filename = f"output_{clean_command_args}.txt"
    s3_filename = f"{S3_OUTPUT_PATH}/{output_filename}"

    container_overrides = {
        "command": [
            "/bin/bash",
            "-c",
            f"aws s3 ls '{s3_filename}' && echo 'output already found, bailing out' && exit 0;"
            "date;"
            f"(time /{COMMAND} {command_args}) 2>&1 | tee '/tmp/{output_filename}';"
            f"aws s3 cp '/tmp/{output_filename}' '{s3_filename}'",
        ]
    }

    def submit():
        return batch_client.submit_job(
            jobName=job_name,
            jobQueue=JOB_QUEUE,
            jobDefinition=JOB_DEFINITION,
            containerOverrides=container_overrides,
        )

    return with_retry(submit)


def get_all_jobs(job_queue):
    all_jobs = []

    for status in [
        "SUBMITTED",
        "PENDING",
        "RUNNABLE",
        "STARTING",
        "RUNNING",
        "SUCCEEDED",
        # "FAILED",
    ]:
        response = batch_client.list_jobs(jobQueue=job_queue, jobStatus=status)
        while True:
            all_jobs.extend(response.get("jobSummaryList", []))
            next_token = response.get("nextToken")
            if not next_token:
                break
            response = batch_client.list_jobs(
                jobQueue=job_queue, jobStatus=status, nextToken=next_token
            )

    return all_jobs


print(f"Fetching jobs from queue {JOB_QUEUE}...")
existing_jobs = get_all_jobs(JOB_QUEUE)
duplicates = set()
seen_jobs = set()
for job in existing_jobs:
    job_name = job.get("jobName")
    if job_name in seen_jobs:
        print(f"duplicate: {job_name}")
        duplicates.add(job_name)
    seen_jobs.add(job_name)


def cancel_or_terminate_duplicates():
    for job_name in duplicates:
        jobs = [job for job in existing_jobs if job["jobName"] == job_name]
        by_status = {}
        for job in jobs:
            by_status[job["status"]] = by_status.get(job["status"], []) + [job]

        stats = {}
        for k, v in by_status.items():
            stats[k] = len(v)

        print(f"processing {job_name}: {stats}")
        if "SUCCEEDED" in by_status:
            for job in by_status.get("RUNNING", []):
                print(f"    terminating running {job['jobName']} - {job['jobId']}")
                batch_client.terminate_job(
                    jobId=job["jobId"], reason="Remove duplicates"
                )

            for job in by_status.get("STARTING", []):
                print(f"    terminating starting {job['jobName']} - {job['jobId']}")
                batch_client.terminate_job(
                    jobId=job["jobId"], reason="Remove duplicates"
                )

            for job in by_status.get("RUNNABLE", []):
                print(f"    cancelling runnable {job['jobName']} - {job['jobId']}")
                batch_client.cancel_job(jobId=job["jobId"], reason="Remove duplicates")

        elif "RUNNING" in by_status:
            for job in by_status["RUNNING"][1:]:
                print(f"    terminating running {job['jobName']} - {job['jobId']}")
                batch_client.terminate_job(
                    jobId=job["jobId"], reason="Remove duplicates"
                )

            for job in by_status.get("STARTING", []):
                print(f"    terminating starting {job['jobName']} - {job['jobId']}")
                batch_client.terminate_job(
                    jobId=job["jobId"], reason="Remove duplicates"
                )

            for job in by_status.get("RUNNABLE", []):
                print(f"    cancelling runnable {job['jobName']} - {job['jobId']}")
                batch_client.cancel_job(jobId=job["jobId"], reason="Remove duplicates")

        elif "STARTING" in by_status:
            for job in by_status["STARTING"][1:]:
                print(f"    terminating starting {job['jobName']} - {job['jobId']}")
                batch_client.terminate_job(
                    jobId=job["jobId"], reason="Remove duplicates"
                )

            for job in by_status.get("RUNNABLE", []):
                print(f"    cancelling runnable {job['jobName']} - {job['jobId']}")
                batch_client.cancel_job(jobId=job["jobId"], reason="Remove duplicates")

        elif "RUNNABLE" in by_status:
            for job in by_status["RUNNABLE"][1:]:
                print(f"    cancelling runnable {job['jobName']} - {job['jobId']}")
                batch_client.cancel_job(jobId=job["jobId"], reason="Remove duplicates")


def add_jobs():
    print(f"Submitting jobs to queue {JOB_QUEUE}...")
    process = subprocess.Popen(
        ["../get-random-lines.sh", JOB_PARAMETER_FILE, str(NUMBER_OF_JOBS)],
        stdout=subprocess.PIPE,
        text=True,
    )

    num_added = 0
    for args in process.stdout:
        args = args.strip()
        clean_args = args.replace(" ", "_")
        job_name = f"{JOB_ID}_{clean_args}"
        if job_name in seen_jobs:
            print(f"Job {job_name} already exists, ignoring...")
            continue

        num_added += 1
        print(f"Submitting job: {job_name} ({num_added})")
        submit_batch_job(job_name, args)

    print("Done.")


add_jobs()

# cancel_or_terminate_duplicates()
