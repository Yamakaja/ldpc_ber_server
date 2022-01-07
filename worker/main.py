#!/usr/bin/python

# LDPC BER Tester Worker, manages SD-FECs and BER tester cores
# Provides access through web api

import json
import os
import argparse
import web_api
import worker_test

parser = argparse.ArgumentParser(description='LDPC BER Tester Worker Process.')
parser.add_argument("-c", '--config-file', default="config.json", type=argparse.FileType('r', encoding='utf-8'),
                    help='Configuration file')
parser.add_argument("CMD", choices=["api_server", "status", "test"],
        default="api_server", help="Available commands: [api_server, status, test]", metavar="CMD")
parser.add_argument("-d", "--debug", action="store_true", help="Launch server in debug mode and/or print debug information")
parser.add_argument("-n", "--dry-run", action="store_true", help="Do not initialize hardware")

args = parser.parse_args()
config = json.load(args.config_file)

if args.CMD == "api_server":
    web_api.run(config, args.debug, args.dry_run)

elif args.CMD == "status":
    print("SDFEC status information - not implemnted yet!")

elif args.CMD == "test":
    worker_test.run(config, args.debug, args.dry_run)

