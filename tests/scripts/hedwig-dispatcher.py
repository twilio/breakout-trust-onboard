#!/usr/bin/python3

import argparse
import sys
import urllib.parse
import urllib.request
import json
import time

TIMEOUT = 30 * 60 # 30 minutes

class HedwigException(Exception):
    pass

class HedwigWorker:
    def __init__(self, host, token, base_repo, base_ref, pr_repo, pr_ref, extra_args=None):
        self.host = host
        self.token = token
        self.base_repo = base_repo
        self.base_ref = base_ref
        self.pr_repo = pr_repo
        self.pr_ref = pr_ref
        self.extra_args = extra_args

    def send_request(self, data):
        req = urllib.request.Request(self.host, data.encode('utf-8'))
        req.add_header("Authorization", "Basic %s" % self.token)
        try:
          result = urllib.request.urlopen(req)
          data = result.read()
          encoding = result.info().get_content_charset('utf-8')
          js = json.loads(data.decode(encoding))
          return js

        except urllib.error.HTTPError as e:
            print ("HTTP Error: ", e)
            return None

    def send_request_json(self, json_dict):
        js = json.dumps(json_dict)
        return self.send_request(urllib.parse.urlencode({'payload' : js}))

    def send_start(self):
        req_payload = {'pr_ref'    : self.pr_ref,
                       'pr_repo'   : self.pr_repo,
                       'base_ref'  : self.base_ref,
                       'base_repo' : self.base_repo}
        if self.extra_args:
            req_payload['extra_args'] = self.extra_args

        req = {'type'    : 'start',
               'payload' : req_payload}

        return self.send_request_json(req)

    def send_check_status(self, uuid):
        req_payload = {'uuid' : uuid}
        req = {'type'    : 'check_status',
               'payload' : req_payload}
        return self.send_request_json(req)

    def send_terminate(self, uuid):
        req_payload = {'uuid' : uuid}
        req = {'type'    : 'terminate',
               'payload' : req_payload}
        return self.send_request_json(req)

    def send_get_log(self, uuid):
        req_payload = {'uuid' : uuid}
        req = {'type'    : 'get_log',
               'payload' : req_payload}
        return self.send_request_json(req)

    def run(self):
        ret = self.send_start()

        if not ret or not 'status' in ret or not 'uuid' in ret or ret['status'] != 'OK':
            raise HedwigException('Failed to start the job')

        uuid = ret['uuid']
        print ('Remote job %s started' % uuid)

        start_time = time.time()
        ci_success = None

        while time.time() - start_time < TIMEOUT:
            time.sleep(10.0)
            print('.', end='') # Keep Travis alive by producing console output
            sys.stdout.flush()
            ret = self.send_check_status(uuid)

            if not ret or not 'status' in ret:
                raise HedwigException('Failed to get job status')

            if ret['status'] == 'queued' or ret['status'] == 'running':
                pass
            elif ret['status'] == 'success' or ret['status'] == 'failure':
                ci_success = (ret['status'] == 'success')
                break
            else:
                raise HedwigException('Unexpected job status')

        if ci_success == None:
            # timeout
            print ('Remote CI job timed out, terminating')

            ret = self.send_terminate(uuid)
            if not ret or not 'status' in ret or ret['status'] != 'OK':
                raise HedwigException('Failed to terminate the job after the timeout')

            ci_success = False

        log_json = self.send_get_log(uuid)

        if not log_json or not 'log' in log_json:
            raise HedwigException('Failed to get job log')

        return (ci_success, log_json['log'])

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--baserepo", help="PR base repo URL")
    parser.add_argument("--prrepo", help="PR branch repo URL")
    parser.add_argument("--baseref", help="PR base ref")
    parser.add_argument("--prref", help="PR branch ref")
    parser.add_argument("--extra-args", help="Additional arguments to the worker's test script")
    parser.add_argument("workers", nargs='+', help="Worker tokens, each of form https://domain.com/resourse@<base64 auth token>")

    print ("Starting Hedwig CI dispatcher")

    args = parser.parse_args()

    if not args.baserepo:
        print ("Base repo is a required argument")
        parser.print_help()
        sys.exit(1)

    if not args.prrepo:
        print ("PR repo is a required argument")
        parser.print_help()
        sys.exit(1)

    if not args.baserepo:
        print ("Base ref is a required argument")
        parser.print_help()
        sys.exit(1)

    if not args.baserepo:
        print ("PR ref is a required argument")
        parser.print_help()
        sys.exit(1)

    if len(args.workers) < 1:
        print ("At least one worker should be defined")
        parser.print_help()
        sys.exit(1)


    host, token = args.workers[0].split('@') # TODO: iterate over all workers choosing the best suitable one
    worker = HedwigWorker(host, token, args.baserepo, args.baseref, args.prrepo, args.prref, args.extra_args)

    try:
        success, log = worker.run()

        if success:
            print("Remote CI job returned SUCCESS")
        else:
            print("Remote CI job returned FAILURE")
        print(log)

        sys.exit(0 if success else 1)

    except HedwigException as e:
        print("Fatal ERROR executing remote CI job:", e)
        sys.exit(1)

if __name__ == "__main__":
    main()
