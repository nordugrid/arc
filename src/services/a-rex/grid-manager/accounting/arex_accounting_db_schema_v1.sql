/*
 * Main AAR attributes and metrics to search and gather stats for
 */
CREATE TABLE IF NOT EXISTS AAR (
  RecordID          INTEGER PRIMARY KEY AUTOINCREMENT,
  /* Unique job ids */
  JobID             TEXT NOT NULL UNIQUE,
  LocalJobID        TEXT,
  /* Submission data */
  EndpointID        INTEGER NOT NULL,
  QueueID           INTEGER NOT NULL,
  UserID            INTEGER NOT NULL,
  VOID              INTEGER NOT NULL,
  /* Completion data */
  StatusID          INTEGER NOT NULL,
  ExitCode          INTEGER NOT NULL,
  /* Main accounting times to search jobs (as unix timestamp) */
  SubmitTime        INTEGER NOT NULL,
  EndTime           INTEGER NOT NULL,
  /* Used resources */
  NodeCount         INTEGER NOT NULL,
  CPUCount          INTEGER NOT NULL,
  UsedMemory        INTEGER NOT NULL,
  UsedVirtMem       INTEGER NOT NULL,
  UsedWalltime      INTEGER NOT NULL,
  UsedCPUUserTime   INTEGER NOT NULL,
  UsedCPUKernelTime INTEGER NOT NULL,
  UsedScratch       INTEGER NOT NULL,
  StageInVolume     INTEGER NOT NULL,
  StageOutVolume    INTEGER NOT NULL,
  /* Foreign keys constraints */
  FOREIGN KEY(EndpointID) REFERENCES Endpoints(EndpointID),
  FOREIGN KEY(QueueID) REFERENCES Queues(QueueID),
  FOREIGN KEY(UserID) REFERENCES Users(UserID),
  FOREIGN KEY(VOID) REFERENCES WLCGVOs(VOID),
  FOREIGN KEY(StatusID) REFERENCES Status(StatusID)
);
CREATE UNIQUE INDEX IF NOT EXISTS AAR_JobID_IDX ON AAR(JobID);
CREATE INDEX IF NOT EXISTS AAR_LocalJobID_IDX ON AAR(LocalJobID);
CREATE INDEX IF NOT EXISTS AAR_EndpointID_IDX ON AAR(EndpointID);
CREATE INDEX IF NOT EXISTS AAR_QueueID_IDX ON AAR(QueueID);
CREATE INDEX IF NOT EXISTS AAR_UserID_IDX ON AAR(UserID);
CREATE INDEX IF NOT EXISTS AAR_StatusID_IDX ON AAR(StatusID);
CREATE INDEX IF NOT EXISTS AAR_SubmitTime_IDX ON AAR(SubmitTime);
CREATE INDEX IF NOT EXISTS AAR_EndTime_IDX ON AAR(EndTime);

/*
 * Extra tables for AAR normalization
 */

/* Submission endpoints (limited enum of types and URLs) */
CREATE TABLE IF NOT EXISTS Endpoints (
  EndpointID    INTEGER PRIMARY KEY AUTOINCREMENT,
  Interface     TEXT NOT NULL,
  URL           TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS Endpoints_Interface_IDX ON Endpoints(Interface);

/* Queues (limited enum on particular resource) */
CREATE TABLE IF NOT EXISTS Queues (
  QueueID       INTEGER PRIMARY KEY AUTOINCREMENT,
  Name          TEXT NOT NULL UNIQUE
);
CREATE UNIQUE INDEX IF NOT EXISTS Queues_Name_IDX ON Queues(Name);

/* Users */
CREATE TABLE IF NOT EXISTS Users (
  UserID        INTEGER PRIMARY KEY AUTOINCREMENT,
  Name          TEXT NOT NULL UNIQUE
);
CREATE UNIQUE INDEX IF NOT EXISTS Users_Name_IDX ON Users(Name);

/* WLCG VOs */
CREATE TABLE IF NOT EXISTS WLCGVOs (
  VOID          INTEGER PRIMARY KEY AUTOINCREMENT,
  Name          TEXT NOT NULL UNIQUE
);
CREATE UNIQUE INDEX IF NOT EXISTS WLCGVOs_Name_IDX ON WLCGVOs(Name);

/* Status */
CREATE TABLE IF NOT EXISTS Status (
  StatusID      INTEGER PRIMARY KEY AUTOINCREMENT,
  Name          TEXT NOT NULL UNIQUE
);
CREATE UNIQUE INDEX IF NOT EXISTS Status_Name_IDX ON Status(Name);

/*
 * Extra data recorded for the job in dedicated tables
 */

/* User token attributes */
CREATE TABLE IF NOT EXISTS AuthTokenAttributes (
  RecordID      INTEGER NOT NULL,
  AttrKey       TEXT NOT NULL,
  AttrValue     TEXT,
  FOREIGN KEY(RecordID) REFERENCES AAR(RecordID)
);
CREATE INDEX IF NOT EXISTS AuthTokenAttributes_RecordID_IDX ON AuthTokenAttributes(RecordID);
CREATE INDEX IF NOT EXISTS AuthTokenAttributes_AttrKey_IDX ON AuthTokenAttributes(AttrKey);

/* Event timestamps for the job */
CREATE TABLE IF NOT EXISTS JobEvents (
  RecordID      INTEGER NOT NULL,
  EventKey      TEXT NOT NULL, -- including: submit, stageinstart, stageinstop, lrmssubmit, lrmsstart, lrmsend, stageoutstart, stageoutend, finish
  EventTime     TEXT NOT NULL,
  FOREIGN KEY(RecordID) REFERENCES AAR(RecordID)
);
CREATE INDEX IF NOT EXISTS JobEvents_RecordID_IDX ON JobEvents(RecordID);
CREATE INDEX IF NOT EXISTS JobEvents_EventKey_IDX ON JobEvents(EventKey);

/* RTEs */
CREATE TABLE IF NOT EXISTS RunTimeEnvironments (
  RecordID      INTEGER NOT NULL,
  RTEName       TEXT NOT NULL, -- TODO: should we record arguments, versions, default/enabled?
  FOREIGN KEY(RecordID) REFERENCES AAR(RecordID)
);
CREATE INDEX IF NOT EXISTS RunTimeEnvironments_RecordID_IDX ON RunTimeEnvironments(RecordID);
CREATE INDEX IF NOT EXISTS RunTimeEnvironments_RTEName_IDX ON RunTimeEnvironments(RTEName);

/* Data transfers info */
CREATE TABLE IF NOT EXISTS DataTransfers (
  RecordID      INTEGER NOT NULL,
  URL           TEXT NOT NULL,
  FileSize      INTEGER NOT NULL,
  TransferStart INTEGER NOT NULL,
  TransferEnd   INTEGER NOT NULL,
  TransferType  INTEGER NOT NULL, -- download, download from cahce, upload
  FOREIGN KEY(RecordID) REFERENCES AAR(RecordID)
);
CREATE INDEX IF NOT EXISTS DataTransfers_RecordID_IDX ON DataTransfers(RecordID);
CREATE INDEX IF NOT EXISTS DataTransfers_URL_IDX ON DataTransfers(URL);

/* Extra arbitrary text attributes affiliated with AAR */
CREATE TABLE IF NOT EXISTS JobExtraInfo (
  RecordID      INTEGER NOT NULL,
  InfoKey       TEXT NOT NULL, -- including: jobname, lrms, nodename, clienthost, localuser, projectname, systemsoftware, wninstance, benchmark
  InfoValue     TEXT,
  FOREIGN KEY(RecordID) REFERENCES AAR(RecordID)
);
CREATE INDEX IF NOT EXISTS JobExtraInfo_RecordID_IDX ON JobExtraInfo(RecordID);
CREATE INDEX IF NOT EXISTS JobExtraInfo_InfoKey_IDX ON JobExtraInfo(InfoKey);

/* 
 * Database common config parameters 
 */
CREATE TABLE IF NOT EXISTS DBConfig (
   KeyName      TEXT PRIMARY KEY, 
   KeyValue     TEXT NOT NULL
);
INSERT INTO DBConfig VALUES ('DBSCHEMA_VERSION', '1');
