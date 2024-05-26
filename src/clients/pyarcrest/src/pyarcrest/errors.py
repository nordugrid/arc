class X509Error(Exception):
    """Base error for x509 module."""


class HTTPClientError(Exception):
    """Base error for http module."""


class ARCError(Exception):
    """Base error for arc module."""


class ARCHTTPError(ARCError):
    """Job operation error."""

    def __init__(self, status, text):
        super().__init__(status, text)
        self.status = status
        self.text = text

    def __str__(self):
        return f"Operation error: {self.status} {self.text}"


class DescriptionParseError(ARCError):
    """Description parsing error."""


class DescriptionUnparseError(ARCError):
    """Description unparsing error."""


class InputFileError(ARCError):
    """Error with input file in job description."""


class NoValueInARCResult(ARCError):
    """Error with result from ARC."""


class MatchmakingError(ARCError):
    """Matchmaking error."""


class InputUploadError(ARCError):
    """Input file upload error."""

    def __init__(self, jobid, state, errors):
        super().__init__(jobid, state, errors)
        self.jobid = jobid
        self.state = state
        self.errors = errors

    def __str__(self):
        return f"Input upload error(s) for job {self.jobid} in state {self.state}:\n" \
               + '\n'.join([str(error) for error in self.errors])


class MissingResultFile(ARCError):
    """
    Missing job result file.

    There needs to be a distinction between a missing output and diagnose file.
    In some use cases a missing diagnose file might not want to be considered
    as an error in would require to be handled explicitly.
    """

    def __init__(self, filename):
        super().__init__(filename)
        self.filename = filename

    def __str__(self):
        return f"Missing result file {self.filename}"


class MissingOutputFile(MissingResultFile):
    """Mising job output file."""

    def __str__(self):
        return f"Missing output file {self.filename}"


class MissingDiagnoseFile(MissingResultFile):
    """Missing job diagnose file."""

    def __str__(self):
        return f"Missing diagnose file {self.filename}"
