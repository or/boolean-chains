#!/usr/bin/env python3
import boto3
import time
import subprocess
import botocore.exceptions

AWS_PROFILE = "or"
JOB_QUEUE = "boolean-chains-queue"
JOB_DEFINITION = "boolean-chains-job-definition"
S3_BINARY_PATH = "s3://computing-results/binaries/boolean-chains-full-fast"
S3_OUTPUT_PATH = "s3://computing-results/boolean-chains/12-18"

boto3.setup_default_session(profile_name=AWS_PROFILE, region_name="us-east-1")
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

    container_overrides = {
        "command": [
            "/bin/bash",
            "-c",
            f"aws s3 cp {S3_BINARY_PATH} /tmp/binary; "
            f"chmod +x /tmp/binary; "
            "date;"
            f"date; (time /tmp/binary {command_args}) 2>&1 | tee /tmp/{output_filename}; "
            f"aws s3 cp /tmp/{output_filename} {S3_OUTPUT_PATH}/{output_filename}",
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


print("Submitting jobs to AWS Batch...")
process = subprocess.Popen(
    ["../target/boolean-chains-full-fast-plan", "2"], stdout=subprocess.PIPE, text=True
)
for args in process.stdout:
    args = args.strip()
    job_name = f"Process_{args.replace(' ', '_')}"

    print(f"Submitting job: {job_name}")
    submit_batch_job(job_name, args)

print("Done.")
