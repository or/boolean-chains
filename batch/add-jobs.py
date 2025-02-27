#!/usr/bin/env python3
import boto3
import time
import subprocess
import botocore.exceptions
import sys

AWS_PROFILE = "or"
boto3.setup_default_session(profile_name=AWS_PROFILE, region_name="us-east-1")

which = sys.argv[1] if len(sys.argv) > 1 else None
if which == "ec2":
    JOB_DEFINITION = "boolean-chains-job-definition-ec2"
elif which == "fargate":
    JOB_DEFINITION = "boolean-chains-job-definition"
else:
    print("first argument must be 'ec2' or 'fargate'")
    sys.exit(1)

JOB_QUEUE = sys.argv[2]
JOB_ID = sys.argv[3]

S3_OUTPUT_PATH = f"s3://computing-results/boolean-chains/{JOB_ID}"

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
            "date;"
            f"(time /boolean-chains-full-fast {command_args}) 2>&1 | tee /tmp/{output_filename};"
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
    clean_args = args.replace(" ", "_")
    job_name = f"{JOB_ID}_{clean_args}"

    print(f"Submitting job: {job_name}")
    submit_batch_job(job_name, args)

print("Done.")
