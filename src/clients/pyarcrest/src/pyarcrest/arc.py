"""
Module for interaction with the ARC CE REST interface.

Automatic support for multiple versions of the API is implemented with optional
manual selection of the API version. This is done by defining a base class with
methods closely reflecting the operations specified in the ARC CE REST
interface specification: https://www.nordugrid.org/arc/arc6/tech/rest/rest.html
Additionally, the base class defines some higher level methods, e. g. a method
to upload job input files using multiple threads.

Some operations involved in determining the API version are implemented in class
methods instead of instance methods as instance methods are considered to be
tied to the API version. Determination of API version should therefore be a
static operation.
"""


import concurrent.futures
import datetime
import json
import logging
import os
import queue
import re
import threading
from urllib.parse import urlparse

from pyarcrest.errors import (ARCError, ARCHTTPError, DescriptionParseError,
                              DescriptionUnparseError, InputFileError,
                              InputUploadError, MatchmakingError,
                              MissingDiagnoseFile, MissingOutputFile,
                              NoValueInARCResult)
from pyarcrest.http import HTTPClient
from pyarcrest.x509 import certToPEM, parseProxyPEM, pemToCSR, signProxyCSR

log = logging.getLogger(__name__)


class Result:

    def __init__(self, value, error=False):
        assert isinstance(error, bool)
        self.value = value
        self.error = True if error else False


class ARCRest:

    DIAGNOSE_FILES = [
        "failed", "local", "errors", "description", "diag", "comment",
        "status", "acl", "xml", "input", "output", "input_status",
        "output_status", "statistics"
    ]

    def __init__(self, httpClient, token=None, proxypath=None, apiBase="/arex", sendsize=None, recvsize=None, timeout=None):
        """
        Initialize the base object.

        Note that this class should not be instantiated directly because
        additional implementations of attributes and methods are required from
        derived classes.
        """
        assert token or proxypath
        if token:
            self.token = token
            self.proxypath = None
        elif proxypath:
            self.token = None
            self.proxypath = proxypath
        self.httpClient = httpClient
        self.apiBase = apiBase

        self.sendsize = sendsize
        self.recvsize = recvsize or 2 ** 14  # 16KB default download size
        self.timeout = timeout

    def close(self):
        self.httpClient.close()

    ### Direct operations on ARC CE ###

    def getAPIVersions(self):
        return self.getAPIVersionsStatic(self.httpClient, self.apiBase, self.token)

    def getCEInfo(self):
        status, text = self._requestJSON("GET", "/info")
        if status != 200:
            raise ARCHTTPError(status, text)
        return json.loads(text)

    def getJobsList(self):
        status, text = self._requestJSON("GET", "/jobs")
        if status != 200:
            raise ARCHTTPError(status, text)

        try:
            jsonData = json.loads(text)["job"]
        except json.JSONDecodeError as exc:
            if exc.doc == "":
                jsonData = []
            else:
                raise

        # /rest/1.0 compatibility
        if not isinstance(jsonData, list):
            jsonData = [jsonData]

        return [job["id"] for job in jsonData]

    def createJobs(self, description, queue=None, delegationID=None, isADL=True):
        raise NotImplementedError

    def getJobsInfo(self, jobs):
        responses = self._manageJobs(jobs, "info")
        results = []
        for job, response in zip(jobs, responses):
            code, reason = int(response["status-code"]), response["reason"]
            if code != 200:
                results.append(Result(ARCHTTPError(code, reason), error=True))
            elif "info_document" not in response:
                results.append(Result(NoValueInARCResult(f"No info document in successful info response for job {job}"), error=True))
            else:
                results.append(Result(self._parseJobInfo(response["info_document"])))
        return results

    def getJobsStatus(self, jobs):
        responses = self._manageJobs(jobs, "status")
        results = []
        for job, response in zip(jobs, responses):
            code, reason = int(response["status-code"]), response["reason"]
            if code != 200:
                results.append(Result(ARCHTTPError(code, reason), error=True))
            elif "state" not in response:
                results.append(Result(NoValueInARCResult("No state in successful status response"), error=True))
            else:
                results.append(Result(response["state"]))
        return results

    def killJobs(self, jobs):
        responses = self._manageJobs(jobs, "kill")
        results = []
        for job, response in zip(jobs, responses):
            code, reason = int(response["status-code"]), response["reason"]
            if code != 202:
                results.append(Result(ARCHTTPError(code, reason), error=True))
            else:
                results.append(Result(True))
        return results

    def cleanJobs(self, jobs):
        responses = self._manageJobs(jobs, "clean")
        results = []
        for job, response in zip(jobs, responses):
            code, reason = int(response["status-code"]), response["reason"]
            if code != 202:
                results.append(Result(ARCHTTPError(code, reason), error=True))
            else:
                results.append(Result(True))
        return results

    def restartJobs(self, jobs):
        responses = self._manageJobs(jobs, "restart")
        results = []
        for job, response in zip(jobs, responses):
            code, reason = int(response["status-code"]), response["reason"]
            if code != 202:
                results.append(Result(ARCHTTPError(code, reason), error=True))
            else:
                results.append(Result(True))
        return results

    def getJobsDelegations(self, jobs):
        responses = self._manageJobs(jobs, "delegations")
        results = []
        for job, response in zip(jobs, responses):
            code, reason = int(response["status-code"]), response["reason"]
            if code != 200:
                results.append(Result(ARCHTTPError(code, reason), error=True))
            elif "delegation_id" not in response:
                results.append(Result(NoValueInARCResult("No delegation ID in successful response"), error=True))
            else:
                # /rest/1.0 compatibility
                if isinstance(response["delegation_id"], list):
                    delegations = response["delegation_id"]
                else:
                    delegations = [response["delegation_id"]]
                results.append(Result(delegations))
        return results

    def downloadFile(self, jobid, sessionPath, filePath):
        try:
            self._downloadURL(f"/jobs/{jobid}/session/{sessionPath}", filePath)
        except ARCHTTPError as exc:
            if exc.status == 404:
                raise MissingOutputFile(sessionPath)
            else:
                raise

    def uploadFile(self, jobid, sessionPath, filePath):
        urlPath = f"/jobs/{jobid}/session/{sessionPath}"
        with open(filePath, "rb") as f:
            resp = self._request("PUT", urlPath, data=f)
            text = resp.read().decode()
            if resp.status != 200:
                raise ARCHTTPError(resp.status, text)

    def downloadListing(self, jobid, sessionPath):
        urlPath = f"/jobs/{jobid}/session/{sessionPath}"
        status, text = self._requestJSON("GET", urlPath)
        if status != 200:
            raise ARCHTTPError(status, text)

        # /rest/1.0 compatibility
        # handle empty response
        listing = {}
        try:
            listing = json.loads(text)
        except json.JSONDecodeError as exc:
            if exc.doc != "":
                raise
        # handle inexisting file list ...
        if "file" not in listing:
            listing["file"] = []
        # ... or a single file string
        elif not isinstance(listing["file"], list):
            listing["file"] = [listing["file"]]
        # handle inexsisting dir list ...
        if "dirs" not in listing:
            listing["dirs"] = []
        # ... or a single dir string
        elif not isinstance(listing["dirs"], list):
            listing["dirs"] = [listing["dirs"]]

        return listing

    def downloadDiagnoseFile(self, jobid, name, filePath):
        if name not in self.DIAGNOSE_FILES:
            raise ARCError(f"Invalid control dir file requested: {name}")
        try:
            self._downloadURL(f"/jobs/{jobid}/diagnose/{name}", filePath)
        except ARCHTTPError as exc:
            if exc.status == 404:
                raise MissingDiagnoseFile(name)
            else:
                raise

    def getDelegationsList(self, type=None):
        params = {}

        if type:
            assert type in ("x509", "jwt")
            params["type"] = type

        status, text = self._requestJSON("GET", "/delegations", params=params)
        if status != 200:
            raise ARCHTTPError(status, text)

        # /rest/1.0 compatibility
        try:
            return json.loads(text)["delegation"]
        except json.JSONDecodeError as exc:
            if exc.doc == "":
                return []
            else:
                raise

    # priorities: token over proxypath over whatever the access credential is
    def newDelegation(self, token=None, proxypath=None):
        params = {"action": "new"}
        headers = {}
        tokenDelegation = token or not proxypath and self.token
        if tokenDelegation:
            if not token:
                token = self.token
            headers["X-Token-Delegation"] = f"Bearer {token}"
            params["type"] = "jwt"

        resp = self._request("POST", "/delegations", headers=headers, params=params, token=token)
        respstr = resp.read().decode()

        if token:
            if resp.status != 201:
                raise ARCHTTPError(resp.status, respstr)
            respstr = None
        else:
            if resp.status != 201:
                raise ARCHTTPError(resp.status, respstr)

        return resp.getheader("Location").split("/")[-1], respstr

    def uploadCertDelegation(self, delegationID, cert):
        url = f"/delegations/{delegationID}"
        headers = {"Content-Type": "application/x-pem-file"}
        resp = self._request("PUT", url, data=cert, headers=headers)
        respstr = resp.read().decode()
        if resp.status != 200:
            raise ARCHTTPError(resp.status, respstr)

    def getDelegation(self, delegationID):
        url = f"/delegations/{delegationID}"
        resp = self._request("POST", url, params={"action": "get"})
        respstr = resp.read().decode()
        if resp.status != 200:
            raise ARCHTTPError(resp.status, respstr)
        return respstr

    # returns CSR if proxy cert is used, None otherwise
    #
    # what happens if token is used to renew a proxy delegation?
    def renewDelegation(self, delegationID, token=None, proxypath=None):
        params = {"action": "renew"}
        headers = {}
        tokenDelegation = token or (not proxypath and self.token)
        if tokenDelegation:
            if not token:
                token = self.token
            headers["X-Token-Delegation"] = f"Bearer {token}"

        url = f"/delegations/{delegationID}"
        resp = self._request("POST", url, headers=headers, params=params, token=token)
        respstr = resp.read().decode()

        if token:
            if resp.status != 200:
                raise ARCHTTPError(resp.status, respstr)
            respstr = None
        else:
            if resp.status != 201:
                raise ARCHTTPError(resp.status, respstr)

        return respstr

    def deleteDelegation(self, delegationID):
        url = f"/delegations/{delegationID}"
        resp = self._request("POST", url, params={"action": "delete"})
        respstr = resp.read().decode()
        if resp.status != 202:
            raise ARCHTTPError(resp.status, respstr)

    ### Higher level job operations ###

    def uploadJobFiles(self, jobids, jobInputs, workers=None, sendsize=None, timeout=None):
        if not workers:
            workers = 1
        resultDict = {jobid: [] for jobid in jobids}

        # create upload queue
        uploadQueue = queue.Queue()
        for jobid, inputFiles in zip(jobids, jobInputs):
            try:
                self._addInputTransfers(uploadQueue, jobid, inputFiles)
            except InputFileError as exc:
                resultDict[jobid].append(exc)
                log.debug(f"Skipping job {jobid} due to input file error: {exc}")

        if uploadQueue.empty():
            log.debug("No local inputs to upload")
            return [resultDict[jobid] for jobid in jobids]

        errorQueue = queue.Queue()

        # create REST clients for workers
        numWorkers = min(uploadQueue.qsize(), workers)
        restClients = []
        for i in range(numWorkers):
            restClients.append(self.getClient(
                host=self.httpClient.conn.host,
                port=self.httpClient.conn.port,
                sendsize=sendsize or self.sendsize,
                timeout=timeout or self.timeout,
                token=self.token,
                proxypath=self.proxypath,
                apiBase=self.apiBase,
                version=self.version,
            ))
        log.debug(f"Created {len(restClients)} upload workers")

        # run upload threads on upload queue
        with concurrent.futures.ThreadPoolExecutor(max_workers=numWorkers) as pool:
            futures = []
            for restClient in restClients:
                futures.append(pool.submit(
                    self._uploadTransferWorker,
                    restClient,
                    uploadQueue,
                    errorQueue,
                ))
            concurrent.futures.wait(futures)

        # close HTTP clients
        for restClient in restClients:
            restClient.close()

        # get transfer errors
        while not errorQueue.empty():
            error = errorQueue.get()
            resultDict[error["jobid"]].append(error["error"])
            errorQueue.task_done()

        return [resultDict[jobid] for jobid in jobids]

    def downloadJobFiles(self, downloadDir, jobids, outputFilters={}, diagnoseFiles={}, diagnoseDirs={}, workers=None, recvsize=None, timeout=None):
        if not workers:
            workers = 1
        resultDict = {jobid: [] for jobid in jobids}
        transferQueue = TransferQueue(workers)

        for jobid in jobids:
            cancelEvent = threading.Event()
            # add diagnose files to transfer queue
            try:
                self._addDiagnoseTransfers(transferQueue, jobid, downloadDir, diagnoseFiles, diagnoseDirs, cancelEvent)
            except ARCError as exc:
                resultDict[jobid].append(exc)
                continue
            # add job session directory as a listing transfer
            path = os.path.join(downloadDir, jobid)
            transferQueue.put(Transfer(jobid, "", path, type="listing", cancelEvent=cancelEvent))

        errorQueue = queue.Queue()

        # create REST clients for workers
        restClients = []
        for i in range(workers):
            restClients.append(self.getClient(
                host=self.httpClient.conn.host,
                port=self.httpClient.conn.port,
                recvsize=recvsize or self.recvsize,
                timeout=timeout or self.timeout,
                token=self.token,
                proxypath=self.proxypath,
                apiBase=self.apiBase,
                version=self.version,
            ))

        log.debug(f"Created {len(restClients)} download workers")

        refilters = {jobid: re.compile(refilter) for jobid, refilter in outputFilters.items()}

        with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as pool:
            futures = []
            for restClient in restClients:
                futures.append(pool.submit(
                    self._downloadTransferWorker,
                    restClient,
                    transferQueue,
                    errorQueue,
                    downloadDir,
                    refilters,
                ))
            concurrent.futures.wait(futures)

        for restClient in restClients:
            restClient.close()

        # get transfer errors
        while not errorQueue.empty():
            error = errorQueue.get()
            resultDict[error["jobid"]].append(error["error"])
            errorQueue.task_done()

        return [resultDict[jobid] for jobid in jobids]

    def createDelegation(self, token=None, proxypath=None, proxyLifetime=None):
        delegationID, csr = self.newDelegation(token, proxypath)
        if csr:
            try:
                pem = self._signCSR(csr, proxyLifetime)
                self.uploadCertDelegation(delegationID, pem)
                return delegationID
            except Exception:
                self.deleteDelegation(delegationID)
                raise
        return delegationID

    def refreshDelegation(self, delegationID, token=None, proxypath=None, proxyLifetime=None):
        csr = self.renewDelegation(delegationID, token, proxypath)
        if csr:
            try:
                pem = self._signCSR(csr, proxyLifetime)
                self.uploadCertDelegation(delegationID, pem)
            except Exception:
                self.deleteDelegation(delegationID)
                raise

    def submitJobs(self, descs, queue=None, delegationID=None, processDescs=True, matchDescs=True, uploadData=True, workers=None, sendsize=None, timeout=None):
        raise NotImplementedError

    def matchJob(self, ceInfo, queue=None, runtimes=[], walltime=None):
        if queue:
            self._matchQueue(ceInfo, queue)

            # matching walltime requires queue
            if walltime:
                self._matchWalltime(ceInfo, queue, walltime)

        for runtime in runtimes:
            self._matchRuntime(ceInfo, runtime)

    ### auth API ###

    # TODO: should this error on no credentials?
    def updateCredential(self, token=None, proxypath=None):
        if token:
            # if token is updated, the connection is not required to be
            # recreated
            if self.token:
                self.token = token
                return
            else:
                self.token = token
                self.proxypath = None
        else:
            self.proxypath = proxypath
            if self.token:
                self.token = None

        # Since using proxy certificate as client certificate is part of
        # connection object, the connection has to be recreated whenever the
        # new proxy certificate is used or the proxy authenticated connection
        # is switched to token authentication.
        self.httpClient = self.getHTTPClient(
            host=self.httpClient.conn.host,
            port=self.httpClient.conn.port,
            blocksize=self.sendsize,
            timeout=self.timeout,
            token=token,
            proxypath=proxypath,
        )

    ### Static operations ###

    @classmethod
    def getAPIVersionsStatic(cls, httpClient, apiBase="/arex", token=None):
        status, text = cls._requestJSONStatic(httpClient, "GET", f"{apiBase}/rest", token=token)
        if status != 200:
            raise ARCHTTPError(status, text)
        apiVersions = json.loads(text)

        # /rest/1.0 compatibility
        if not isinstance(apiVersions["version"], list):
            return [apiVersions["version"]]
        else:
            return apiVersions["version"]

    @classmethod
    def getHTTPClient(cls, url=None, host=None, port=None, blocksize=None, timeout=None, token=None, proxypath=None):
        assert token or proxypath
        if token:
            return HTTPClient(url, host, port, blocksize, timeout)
        elif proxypath:
            return HTTPClient(url, host, port, blocksize, timeout, proxypath)

    # TODO: explain the rationale in documentation about the design of the API
    #       version selection mechanism:
    #       - specific API implementations are in classes
    #       - classes cannot be used as values in class variables or method
    #         parameters without the proper ordering of definitions which is
    #         awkward and inflexible
    @classmethod
    def getClient(cls, url=None, host=None, port=None, sendsize=None, recvsize=None, timeout=None, token=None, proxypath=None, apiBase="/arex", version=None, impls=None):
        if not proxypath:
            proxypath = f"/tmp/x509up_u{os.getuid()}"
        httpClient = cls.getHTTPClient(url, host, port, sendsize, timeout, token, proxypath)

        # get API version to implementation class mapping
        implementations = impls
        if not implementations:
            implementations = cls._getImplementations()

        # get API versions from CE
        apiVersions = cls.getAPIVersionsStatic(httpClient, apiBase, token)
        if not apiVersions:
            raise ARCError("No supported API versions on CE")

        # determine the API version to be used based on available
        # implementations and available versions on ARC CE
        if version:
            if version not in implementations:
                raise ARCError(f"No client support for requested API version {version}")
            if version not in apiVersions:
                raise ARCError(f"API version {version} not among CE supported API versions {apiVersions}")
            apiVersion = version
        else:
            # get the highest version of client implementation supported on the
            # ARC CE
            apiVersion = None
            for version in reversed(apiVersions):
                if version in implementations:
                    apiVersion = version
                    break
            if not apiVersion:
                raise ARCError(f"No client support for CE supported API versions: {apiVersions}")

        log.debug(f"API version {apiVersion} selected")
        return implementations[apiVersion](httpClient, token, proxypath, apiBase, sendsize, recvsize, timeout)

    ### Support methods ###

    def _downloadURL(self, url, path, recvsize=None):
        resp = self._request("GET", url)

        if resp.status != 200:
            text = resp.read().decode()
            raise ARCHTTPError(resp.status, text)

        os.makedirs(os.path.dirname(path), exist_ok=True)

        with open(path, "wb") as f:
            data = resp.read(recvsize or self.recvsize)
            while data:
                f.write(data)
                data = resp.read(recvsize or self.recvsize)

    # returns nothing if match successful, raises exception otherwise
    def _matchQueue(self, ceInfo, queue):
        if not self._findQueue(ceInfo, queue):
            raise MatchmakingError(f"Queue {queue} not found")

    # TODO: is it possible for user to just specify the runtime and any version
    #       is OK or vice versa?
    # returns nothing if match successful, raises exception otherwise
    def _matchRuntime(self, ceInfo, runtime):
        runtimes = self._findRuntimes(ceInfo)
        if runtime not in runtimes:
            raise MatchmakingError(f"Runtime {runtime} not found")

    # returns nothing if match successful, raises exception otherwise
    def _matchWalltime(self, ceInfo, queue, walltime):
        queueInfo = self._findQueue(ceInfo, queue)
        if not queueInfo:
            raise MatchmakingError(f"Queue {queue} not found to match walltime")

        if "MaxWallTime" in queueInfo:
            maxWallTime = int(queueInfo["MaxWallTime"])
            if walltime > maxWallTime:
                raise MatchmakingError(f"Walltime {walltime} higher than max walltime {maxWallTime} for queue {queue}")

    def _findQueue(self, ceInfo, queue):
        compShares = ceInfo.get("Domains", {}) \
                           .get("AdminDomain", {}) \
                           .get("Services", {}) \
                           .get("ComputingService", {}) \
                           .get("ComputingShare", [])
        if not compShares:
            return None

        # /rest/1.0 compatibility
        if not isinstance(compShares, list):
            compShares = [compShares]

        for compShare in compShares:
            if compShare.get("Name", None) == queue:
                # Queues are defined as ComputingShares. There are some shares
                # that are mapped to another share. Such a share is never a
                # queue externally. So if the name of the such share is used as
                # a queue, the result has to be empty.
                if "MappingPolicy" in compShare:
                    return None
                else:
                    return compShare
        return None

    def _findRuntimes(self, ceInfo):
        appenvs = ceInfo.get("Domains", {}) \
                        .get("AdminDomain", {}) \
                        .get("Services", {}) \
                        .get("ComputingService", {}) \
                        .get("ComputingManager", {}) \
                        .get("ApplicationEnvironments", {}) \
                        .get("ApplicationEnvironment", [])

        # /rest/1.0 compatibility
        if not isinstance(appenvs, list):
            appenvs = [appenvs]

        runtimes = []
        for env in appenvs:
            if "AppName" in env:
                envname = env["AppName"]
                if "AppVersion" in env:
                    envname += f"-{env['AppVersion']}"
                runtimes.append(envname)
        return runtimes

    def _signCSR(self, csrPEM, lifetime=None):
        with open(self.proxypath) as f:
            proxyPEM = f.read()
        certPEM, _, chainPEM = parseProxyPEM(proxyPEM)
        chainPEM = certPEM + chainPEM
        csr = pemToCSR(csrPEM)
        cert = signProxyCSR(csr, self.proxypath, lifetime=lifetime)
        return certToPEM(cert) + chainPEM

    def _addInputTransfers(self, uploadQueue, jobid, inputFiles):
        cancelEvent = threading.Event()
        transfers = []
        for name, source in inputFiles.items():
            try:
                path = isLocalInputFile(name, source)
            except ValueError as exc:
                raise InputFileError(f"Error parsing source {source} of input {name}: {exc}")
            if not path:
                continue
            if not os.path.isfile(path):
                raise InputFileError(f"Source {source} of input {name} is not a file")
            transfers.append(Transfer(jobid, name, path, cancelEvent=cancelEvent))
        # no exception raised, add transfers to queue
        for transfer in transfers:
            uploadQueue.put(transfer)

    def _addDiagnoseTransfers(self, transferQueue, jobid, downloadDir, diagnoseFiles, diagnoseDirs, cancelEvent):
        diagnoseList = diagnoseFiles.get(jobid, self.DIAGNOSE_FILES)
        diagnoseDir = diagnoseDirs.get(jobid, "gmlog")
        transfers = []
        for diagFile in diagnoseList:
            assert diagFile in self.DIAGNOSE_FILES
            path = os.path.join(downloadDir, jobid, diagnoseDir, diagFile)
            transfers.append(Transfer(jobid, diagFile, path, type="diagnose", cancelEvent=cancelEvent))
        # no exception raised, add transfers to queue
        for transfer in transfers:
            transferQueue.put(transfer)

    # When name is "", it means the root of the session dir. In this case,
    # slash must not be added to it.
    def _addTransfersFromListing(self, transferQueue, jobid, listing, name, path, cancelEvent, refilter=None):
        for f in listing["file"]:
            newpath = os.path.join(path, f)
            if name:
                newname = f"{name}/{f}"
            else:
                newname = f
            if not refilter or refilter.match(newname):
                transferQueue.put(Transfer(jobid, newname, newpath, type="file", cancelEvent=cancelEvent))

        for d in listing["dirs"]:
            newpath = os.path.join(path, d)
            if name:
                newname = f"{name}/{d}"
            else:
                newname = d
            if not refilter or refilter.match(newname):
                transferQueue.put(Transfer(jobid, newname, newpath, type="listing", cancelEvent=cancelEvent))

    def _requestJSON(self, method, endpoint, headers={}, token=None, jsonData=None, data=None, params={}):
        headers["Accept"] = "application/json"
        resp = self._request(method, endpoint, headers, token, jsonData, data, params)
        text = resp.read().decode()
        return resp.status, text

    def _manageJobs(self, jobs, action):
        if not jobs:
            return []

        # JSON data for request
        tomanage = [{"id": job} for job in jobs]

        # /rest/1.0 compatibility
        if len(tomanage) == 1:
            jsonData = {"job": tomanage[0]}
        else:
            jsonData = {"job": tomanage}

        # execute action and get JSON result
        status, text = self._requestJSON("POST", "/jobs", jsonData=jsonData, params={"action": action})
        if status != 201:
            raise ARCHTTPError(status, text)
        jsonData = json.loads(text)

        # /rest/1.0 compatibility
        if not isinstance(jsonData["job"], list):
            return [jsonData["job"]]
        else:
            return jsonData["job"]

    # TODO: think about what to log and how
    def _submitJobs(self, descs, queue=None, delegationID=None, processDescs=True, matchDescs=True, uploadData=True, workers=None, sendsize=None, timeout=None, v1_0=False):
        import arc
        ceInfo = self.getCEInfo()

        if not delegationID:
            delegationID = self.createDelegation()

        # A list of tuples of index and input file dict for every job
        # description to be submitted. The index is the description's
        # position in the given parameter of job descriptions and is
        # required to create properly aligned results.
        tosubmit = []

        # A dict of a key that is index in given descs list and a value that
        # is either a list of exceptions for failed submission or a tuple of
        # jobid and state for successful submission.
        resultDict = {}

        jobdescs = arc.JobDescriptionList()
        bulkdesc = ""
        for i in range(len(descs)):
            # parse job description
            if not arc.JobDescription.Parse(descs[i], jobdescs):
                resultDict[i] = Result(DescriptionParseError("Failed to parse description"), error=True)
                continue
            arcdesc = jobdescs[-1]

            # get queue, runtimes and walltime from description
            jobqueue = arcdesc.Resources.QueueName
            if not jobqueue:
                jobqueue = queue
                if jobqueue and v1_0:
                    # set queue in job description
                    arcdesc.Resources.QueueName = jobqueue
            runtimes = [str(env) for env in arcdesc.Resources.RunTimeEnvironment.getSoftwareList()]
            if not runtimes:
                runtimes = []
            walltime = arcdesc.Resources.TotalWallTime.range.max
            if walltime == -1:
                walltime = None

            # do matchmaking
            if matchDescs:
                try:
                    self.matchJob(ceInfo, jobqueue, runtimes, walltime)
                except MatchmakingError as error:
                    resultDict[i] = Result(error, error=True)
                    continue

            if v1_0:
                # add delegation ID to description
                arcdesc.DataStaging.DelegationID = delegationID

            # process job description
            if processDescs:
                self._processJobDescription(arcdesc)

            # get input files from description
            inputFiles = self._getArclibInputFiles(arcdesc)

            # unparse modified description, remove xml version node because it
            # is not accepted by ARC CE, add to bulk description
            unparseResult = arcdesc.UnParse("emies:adl")
            if not unparseResult[0]:
                resultDict[i] = Result(DescriptionUnparseError("Could not unparse processed description"), error=True)
                continue
            descstart = unparseResult[1].find("<ActivityDescription")
            bulkdesc += unparseResult[1][descstart:]

            tosubmit.append((i, inputFiles))

        if not tosubmit:
            return [resultDict[i] for i in range(len(descs))]

        # merge into bulk description
        if len(tosubmit) > 1:
            bulkdesc = f"<ActivityDescriptions>{bulkdesc}</ActivityDescriptions>"

        # submit jobs to ARC
        # TODO: handle exceptions
        results = self.createJobs(bulkdesc, queue, delegationID)

        uploadIXs = []  # a list of job indexes for proper result processing
        uploadIDs = []  # a list of jobids for which to upload files
        uploadInputs = []  # a list of job input file dicts for upload

        for (jobix, inputFiles), result in zip(tosubmit, results):
            if result.error:
                resultDict[jobix] = Result(result.value, error=True)
            else:
                jobid, state = result.value
                resultDict[jobix] = Result((jobid, state))
                uploadIDs.append(jobid)
                uploadInputs.append(inputFiles)
                uploadIXs.append(jobix)

        # upload jobs' local input data
        if uploadData:
            errors = self.uploadJobFiles(uploadIDs, uploadInputs, workers, sendsize, timeout or self.timeout)
            for jobix, uploadErrors in zip(uploadIXs, errors):
                if uploadErrors:
                    jobid, state = resultDict[jobix].value
                    resultDict[jobix] = Result(InputUploadError(jobid, state, uploadErrors), error=True)

        return [resultDict[i] for i in range(len(descs))]

    def _request(self, method, endpoint, headers={}, token=None, jsonData=None, data=None, params={}):
        if not token:
            token = self.token
        endpoint = f"{self.apiPath}{endpoint}"
        return self.httpClient.request(method, endpoint, headers, token, jsonData, data, params)

    ### Static support methods ###

    @classmethod
    def _requestJSONStatic(cls, httpClient, method, endpoint, headers={}, token=None, jsonData=None, data=None, params={}):
        headers["Accept"] = "application/json"
        resp = httpClient.request(method, endpoint, headers, token, jsonData, data, params)
        text = resp.read().decode()
        return resp.status, text

    @classmethod
    def _getImplementations(cls):
        return {"1.0": ARCRest_1_0, "1.1": ARCRest_1_1}

    @classmethod
    def _uploadTransferWorker(cls, restClient, uploadQueue, errorQueue):
        while True:
            try:
                upload = uploadQueue.get(block=False)
            except queue.Empty:
                break
            uploadQueue.task_done()

            if upload.cancelEvent.is_set():
                log.debug(f"Skipping upload for cancelled job {upload.jobid}")
                continue

            try:
                restClient.uploadFile(upload.jobid, upload.name, upload.path)
            except Exception as exc:
                upload.cancelEvent.set()
                errorQueue.put({"jobid": upload.jobid, "error": exc})
                log.debug(f"Error uploading {upload.path} for job {upload.jobid}: {exc}")

    # TODO: add bail out parameter for cancelEvent?
    @classmethod
    def _downloadTransferWorker(cls, restClient, transferQueue, errorQueue, downloadDir, outputFilters={}):
        while True:
            try:
                transfer = transferQueue.get()
            except TransferQueueEmpty:
                break

            jobid, name, path = transfer.jobid, transfer.name, transfer.path
            if transfer.cancelEvent.is_set():
                log.debug(f"Skipping download for cancelled job {jobid}")
                continue

            try:
                if transfer.type in ("file", "diagnose"):
                    try:
                        if transfer.type == "file":
                            restClient.downloadFile(jobid, name, path)
                        elif transfer.type == "diagnose":
                            restClient.downloadDiagnoseFile(jobid, name, path)
                    except Exception as exc:
                        errorQueue.put({"jobid": jobid, "error": exc})
                        log.error(f"Download {transfer.type} {name} to {path} for job {jobid} failed: {exc}")

                elif transfer.type == "listing":
                    try:
                        listing = restClient.downloadListing(jobid, name)
                    except Exception as exc:
                        errorQueue.put({"jobid": jobid, "error": exc})
                        log.error(f"Download listing {name} for job {jobid} failed: {exc}")
                    else:
                        refilter = outputFilters.get(jobid, None)
                        # create new transfer jobs
                        restClient._addTransfersFromListing(
                            transferQueue, jobid, listing, name, path, transfer.cancelEvent, refilter=refilter,
                        )

            # every possible exception needs to be handled, otherwise the
            # threads will lock up
            except BaseException as exc:
                errorQueue.put({"jobid": jobid, "error": exc})
                log.debug(f"Download name {name} and path {path} for job {jobid} failed: {exc}")

    @classmethod
    def _getArclibInputFiles(cls, desc):
        inputFiles = {}
        for infile in desc.DataStaging.InputFiles:
            source = None
            if len(infile.Sources) > 0:
                source = infile.Sources[0].fullstr()
            inputFiles[infile.Name] = source
        return inputFiles

    @classmethod
    def _processJobDescription(cls, jobdesc):
        import arc
        exepath = jobdesc.Application.Executable.Path
        if exepath and exepath.startswith("/"):  # absolute paths are on compute nodes
            exepath = ""
        inpath = jobdesc.Application.Input
        outpath = jobdesc.Application.Output
        errpath = jobdesc.Application.Error
        logpath = jobdesc.Application.LogDir

        exePresent = False
        stdinPresent = False
        for infile in jobdesc.DataStaging.InputFiles:
            if exepath == infile.Name:
                exePresent = True
            elif inpath == infile.Name:
                stdinPresent = True

        stdoutPresent = False
        stderrPresent = False
        logPresent = False
        for outfile in jobdesc.DataStaging.OutputFiles:
            if outpath == outfile.Name:
                stdoutPresent = True
            elif errpath == outfile.Name:
                stderrPresent = True
            elif logpath == outfile.Name or logpath == outfile.Name[:-1]:
                logPresent = True

        if exepath and not exePresent:
            infile = arc.InputFileType()
            infile.Name = exepath
            jobdesc.DataStaging.InputFiles.append(infile)

        if inpath and not stdinPresent:
            infile = arc.InputFileType()
            infile.Name = inpath
            jobdesc.DataStaging.InputFiles.append(infile)

        if outpath and not stdoutPresent:
            outfile = arc.OutputFileType()
            outfile.Name = outpath
            jobdesc.DataStaging.OutputFiles.append(outfile)

        if errpath and not stderrPresent:
            outfile = arc.OutputFileType()
            outfile.Name = errpath
            jobdesc.DataStaging.OutputFiles.append(outfile)

        if logpath and not logPresent:
            outfile = arc.OutputFileType()
            if not logpath.endswith('/'):
                outfile.Name = f'{logpath}/'
            else:
                outfile.Name = logpath
            jobdesc.DataStaging.OutputFiles.append(outfile)

    @classmethod
    def _parseJobInfo(cls, infoDocument):
        jobInfo = {}
        infoDict = infoDocument.get("ComputingActivity", {})

        COPY_KEYS = ["Name", "Type", "LocalIDFromManager", "Owner", "LocalOwner", "StdIn", "StdOut", "StdErr", "LogDir", "Queue"]
        for key in COPY_KEYS:
            if key in infoDict:
                jobInfo[key] = infoDict[key]

        INT_KEYS = ["UsedTotalWallTime", "UsedTotalCPUTime", "RequestedTotalWallTime", "RequestedTotalCPUTime", "RequestedSlots", "ExitCode", "WaitingPosition", "UsedMainMemory"]
        for key in INT_KEYS:
            if key in infoDict:
                jobInfo[key] = int(infoDict[key])

        TSTAMP_KEYS = ["SubmissionTime", "EndTime", "WorkingAreaEraseTime", "ProxyExpirationTime"]
        for key in TSTAMP_KEYS:
            if key in infoDict:
                jobInfo[key] = datetime.datetime.strptime(infoDict[key], "%Y-%m-%dT%H:%M:%SZ")

        VARIABLE_KEYS = ["Error", "ExecutionNode"]
        for key in VARIABLE_KEYS:
            if key in infoDict:
                jobInfo[key] = infoDict[key]
                # /rest/1.0 compatibility
                if not isinstance(jobInfo[key], list):
                    jobInfo[key] = [jobInfo[key]]

        states = infoDict.get("State", [])
        # /rest/1.0 compatibility
        if not isinstance(states, list):
            states = [states]
        # get state from a list of states in different systems
        for state in states:
            if state.startswith("arcrest:"):
                jobInfo["state"] = state[len("arcrest:"):]

        restartStates = infoDict.get("RestartState", [])
        # /rest/1.0 compatibility
        if not isinstance(restartStates, list):
            restartStates = [restartStates]
        # get restart state from a list of restart states in different systems
        for state in restartStates:
            if state.startswith("arcrest:"):
                jobInfo["restartState"] = state[len("arcrest:"):]

        return jobInfo


class ARCRest_1_0(ARCRest):

    def __init__(self, httpClient, token=None, proxypath=None, apiBase="/arex", sendsize=None, recvsize=None, timeout=None):
        assert not token
        super().__init__(httpClient, None, proxypath, apiBase, sendsize, recvsize, timeout)
        self.version = "1.0"
        self.apiPath = f"{self.apiBase}/rest/{self.version}"

    def createJobs(self, description, queue=None, delegationID=None, isADL=True):
        contentType = "application/xml" if isADL else "application/rsl"
        status, text = self._requestJSON(
            "POST",
            "/jobs",
            data=description,
            headers={"Content-Type": contentType},
            params={"action": "new"},
        )
        if status != 201:
            raise ARCHTTPError(status, text)
        jsonData = json.loads(text)

        # /rest/1.0 compatibility
        if not isinstance(jsonData["job"], list):
            responses = [jsonData["job"]]
        else:
            responses = jsonData["job"]

        results = []
        for response in responses:
            code, reason = int(response["status-code"]), response["reason"]
            if code != 201:
                results.append(Result(ARCHTTPError(code, reason), error=True))
            else:
                results.append(Result((response["id"], response["state"])))
        return results

    def submitJobs(self, descs, queue=None, delegationID=None, processDescs=True, matchDescs=True, uploadData=True, workers=None, sendsize=None, timeout=None):
        return self._submitJobs(descs, queue, delegationID, processDescs, matchDescs, uploadData, workers, sendsize, timeout, v1_0=True)

    def getDelegationsList(self, type=None):
        return super().getDelegationsList(type=None)


class ARCRest_1_1(ARCRest):

    def __init__(self, httpClient, token=None, proxypath=None, apiBase="/arex", sendsize=None, recvsize=None, timeout=None):
        super().__init__(httpClient, token, proxypath, apiBase, sendsize, recvsize, timeout)
        self.version = "1.1"
        self.apiPath = f"{self.apiBase}/rest/{self.version}"

    def createJobs(self, description, queue=None, delegationID=None, isADL=True):
        params = {"action": "new"}
        if queue:
            params["queue"] = queue
        if delegationID:
            params["delegation_id"] = delegationID
        headers = {"Content-Type": "application/xml" if isADL else "application/rsl"}
        status, text = self._requestJSON(
            "POST",
            "/jobs",
            data=description,
            headers=headers,
            params=params,
        )
        if status != 201:
            raise ARCHTTPError(status, text)
        responses = json.loads(text)["job"]

        results = []
        for response in responses:
            code, reason = int(response["status-code"]), response["reason"]
            if code != 201:
                results.append(Result(ARCHTTPError(code, reason), error=True))
            else:
                results.append(Result((response["id"], response["state"])))
        return results

    def submitJobs(self, descs, queue=None, delegationID=None, processDescs=True, matchDescs=True, uploadData=True, workers=None, sendsize=None, timeout=None):
        return self._submitJobs(descs, queue, delegationID, processDescs, matchDescs, uploadData, workers, sendsize, timeout)


class Transfer:

    def __init__(self, jobid, name, path, type="file", cancelEvent=None):
        self.jobid = jobid
        self.name = name
        self.path = path
        self.type = type
        self.cancelEvent = cancelEvent
        if not self.cancelEvent:
            self.cancelEvent = threading.Event()


class ARCJob:

    def __init__(self, id=None, descstr=None):
        self.id = id
        self.descstr = descstr
        self.name = None
        self.delegid = None
        self.state = None
        self.errors = []
        self.downloadFiles = []
        self.inputFiles = {}

        self.ExecutionNode = None
        self.UsedTotalWallTime = None
        self.UsedTotalCPUTime = None
        self.RequestedTotalWallTime = None
        self.RequestedTotalCPUTime = None
        self.RequestedSlots = None
        self.ExitCode = None
        self.Type = None
        self.LocalIDFromManager = None
        self.WaitingPosition = None
        self.Owner = None
        self.LocalOwner = None
        self.StdIn = None
        self.StdOut = None
        self.StdErr = None
        self.LogDir = None
        self.Queue = None
        self.UsedMainMemory = None
        self.SubmissionTime = None
        self.EndTime = None
        self.WorkingAreaEraseTime = None
        self.ProxyExpirationTime = None
        self.RestartState = []
        self.Error = []

    def updateFromInfo(self, infoDocument):
        infoDict = infoDocument.get("ComputingActivity", {})
        if not infoDict:
            return

        if "Name" in infoDict:
            self.name = infoDict["Name"]

        # get state from a list of activity states in different systems
        for state in infoDict.get("State", []):
            if state.startswith("arcrest:"):
                self.state = state[len("arcrest:"):]

        if "Error" in infoDict:
            # /rest/1.0 compatibility
            if isinstance(infoDict["Error"], list):
                self.Error = infoDict["Error"]
            else:
                self.Error = [infoDict["Error"]]

        if "ExecutionNode" in infoDict:
            # /rest/1.0 compatibility
            if isinstance(infoDict["ExecutionNode"], list):
                self.ExecutionNode = infoDict["ExecutionNode"]
            else:
                self.ExecutionNode = [infoDict["ExecutionNode"]]
            # throw out all non ASCII characters from nodes
            for i in range(len(self.ExecutionNode)):
                self.ExecutionNode[i] = ''.join([i for i in self.ExecutionNode[i] if ord(i) < 128])

        if "UsedTotalWallTime" in infoDict:
            self.UsedTotalWallTime = int(infoDict["UsedTotalWallTime"])

        if "UsedTotalCPUTime" in infoDict:
            self.UsedTotalCPUTime = int(infoDict["UsedTotalCPUTime"])

        if "RequestedTotalWallTime" in infoDict:
            self.RequestedTotalWallTime = int(infoDict["RequestedTotalWallTime"])

        if "RequestedTotalCPUTime" in infoDict:
            self.RequestedTotalCPUTime = int(infoDict["RequestedTotalCPUTime"])

        if "RequestedSlots" in infoDict:
            self.RequestedSlots = int(infoDict["RequestedSlots"])

        if "ExitCode" in infoDict:
            self.ExitCode = int(infoDict["ExitCode"])

        if "Type" in infoDict:
            self.Type = infoDict["Type"]

        if "LocalIDFromManager" in infoDict:
            self.LocalIDFromManager = infoDict["LocalIDFromManager"]

        if "WaitingPosition" in infoDict:
            self.WaitingPosition = int(infoDict["WaitingPosition"])

        if "Owner" in infoDict:
            self.Owner = infoDict["Owner"]

        if "LocalOwner" in infoDict:
            self.LocalOwner = infoDict["LocalOwner"]

        if "StdIn" in infoDict:
            self.StdIn = infoDict["StdIn"]

        if "StdOut" in infoDict:
            self.StdOut = infoDict["StdOut"]

        if "StdErr" in infoDict:
            self.StdErr = infoDict["StdErr"]

        if "LogDir" in infoDict:
            self.LogDir = infoDict["LogDir"]

        if "Queue" in infoDict:
            self.Queue = infoDict["Queue"]

        if "UsedMainMemory" in infoDict:
            self.UsedMainMemory = int(infoDict["UsedMainMemory"])

        if "SubmissionTime" in infoDict:
            self.SubmissionTime = datetime.datetime.strptime(
                infoDict["SubmissionTime"],
                "%Y-%m-%dT%H:%M:%SZ"
            )

        if "EndTime" in infoDict:
            self.EndTime = datetime.datetime.strptime(
                infoDict["EndTime"],
                "%Y-%m-%dT%H:%M:%SZ"
            )

        if "WorkingAreaEraseTime" in infoDict:
            self.WorkingAreaEraseTime = datetime.datetime.strptime(
                infoDict["WorkingAreaEraseTime"],
                "%Y-%m-%dT%H:%M:%SZ"
            )

        if "ProxyExpirationTime" in infoDict:
            self.ProxyExpirationTime = datetime.datetime.strptime(
                infoDict["ProxyExpirationTime"],
                "%Y-%m-%dT%H:%M:%SZ"
            )

        if "RestartState" in infoDict:
            self.RestartState = infoDict["RestartState"]

    def getArclibInputFiles(self, desc):
        self.inputFiles = {}
        for infile in desc.DataStaging.InputFiles:
            source = None
            if len(infile.Sources) > 0:
                source = infile.Sources[0].fullstr()
            self.inputFiles[infile.Name] = source


class TransferQueue:

    def __init__(self, numWorkers):
        self.queue = queue.Queue()
        self.lock = threading.Lock()
        self.barrier = threading.Barrier(numWorkers)

    def put(self, val):
        with self.lock:
            self.queue.put(val)
            self.barrier.reset()

    def get(self):
        while True:
            with self.lock:
                if not self.queue.empty():
                    val = self.queue.get()
                    self.queue.task_done()
                    return val

            try:
                self.barrier.wait()
            except threading.BrokenBarrierError:
                continue
            else:
                raise TransferQueueEmpty()


class TransferQueueEmpty(Exception):
    pass


def isLocalInputFile(name, source):
    """
    Return path if local or empty string if remote URL.

    Raises:
        - ValueError: source cannot be parsed
    """
    if not source:
        return name
    url = urlparse(source)
    if url.scheme not in ("file", None, "") or url.hostname:
        return ""
    return url.path
