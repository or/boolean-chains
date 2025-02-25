#!/usr/bin/env python3
import boto3
import time
import subprocess
import botocore.exceptions
import sys

AWS_PROFILE = "or"
boto3.setup_default_session(profile_name=AWS_PROFILE, region_name="us-east-1")
emr_client = boto3.client("emr")

CLUSTER_ID = sys.argv[1]

print("Waiting for cluster to be up...")
emr_client.get_waiter("cluster_running").wait(ClusterId=CLUSTER_ID)
print("Cluster is up.")


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


def add_step(steps, step_name, command_args):
    clean_command_args = command_args.replace(" ", "_")
    output_filename = f"output_{clean_command_args}.txt"

    steps.append(
        {
            "Name": step_name,
            "ActionOnFailure": "CONTINUE",
            "HadoopJarStep": {
                "Jar": "command-runner.jar",
                "Args": [
                    "/bin/bash",
                    "-c",
                    f"aws s3 cp s3://emr-cluster-result/binaries/boolean-chains-full-fast /tmp/binary; "
                    f"chmod +x /tmp/binary; "
                    f"/tmp/binary {command_args} > /tmp/{output_filename} 2>&1 ; "
                    f"aws s3 cp /tmp/{output_filename} s3://emr-cluster-result/boolean-chains/{CLUSTER_ID}/{output_filename}",
                ],
            },
        }
    )


def add_steps(cluster_id, steps):
    def add():
        return emr_client.add_job_flow_steps(JobFlowId=cluster_id, Steps=steps)

    return with_retry(add)


def list_steps(cluster_id):
    response = emr_client.list_steps(ClusterId=cluster_id)
    return set(step["Name"] for step in response.get("Steps", []))


existing_steps = list_steps(CLUSTER_ID)
steps = []
print("Adding steps...")
process = subprocess.Popen(
    ["../target/boolean-chains-full-fast-plan", "2"], stdout=subprocess.PIPE, text=True
)
for args in process.stdout:
    args = args.strip()
    step_name = f"Process {args}"

    if step_name in existing_steps:
        print(f"Step '{step_name}' already exists, skipping...")
        continue

    print(f"Adding new step: {step_name}")
    add_step(steps, step_name, args)
    if len(steps) >= 50:
        add_steps(CLUSTER_ID, steps)
        steps = []


print("Done.")
