#!/usr/bin/env python3
import boto3

AWS_PROFILE = "or"
boto3.setup_default_session(profile_name=AWS_PROFILE, region_name="us-east-1")
emr_client = boto3.client("emr")

NUM_CORE_INSTANCES = 2


def create_cluster():
    response = emr_client.run_job_flow(
        Name="Computational-Cluster",
        ReleaseLabel="emr-7.7.0",
        Applications=[
            {"Name": "Hadoop"},
        ],
        Instances={
            "InstanceGroups": [
                {
                    "InstanceRole": "MASTER",
                    "InstanceCount": 1,
                    "InstanceType": "c4.large",
                },
                {
                    "InstanceRole": "CORE",
                    "InstanceCount": NUM_CORE_INSTANCES,
                    "InstanceType": "c7i-flex.xlarge",
                },
            ],
            "KeepJobFlowAliveWhenNoSteps": True,
            "Ec2KeyName": "emr",
        },
        JobFlowRole="EMR_EC2_DefaultRole",
        ServiceRole="EMR_DefaultRole",
        AutoTerminationPolicy={"IdleTimeout": 3600},
        StepConcurrencyLevel=NUM_CORE_INSTANCES,
        # Configurations=[
        #     {
        #         "Classification": "yarn-site",
        #         "Properties": {
        #             "yarn.nodemanager.resource.cpu-vcores": "2",
        #             "yarn.scheduler.maximum-allocation-vcores": "2",
        #         },
        #     }
        # ],
    )
    return response["JobFlowId"]


CLUSTER_ID = create_cluster()

if not CLUSTER_ID:
    print("Error: CLUSTER_ID is empty!")
    exit(1)

print(f"Created cluster: {CLUSTER_ID}")
