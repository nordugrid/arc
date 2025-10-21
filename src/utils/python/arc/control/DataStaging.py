from .ControlCommon import *
import subprocess
import sys
import re
import glob
import copy
from datetime import datetime, timedelta

try:
    from .Jobs import *
except ImportError:
    JobsControl = None

def complete_job_id(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return JobsControl(arcconf).complete_job(parsed_args)



class DataStagingControl(ComponentControl):
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.DataStaging')
        self.control_dir = None
        self.arcconfig = arcconfig

        # config is mandatory
        if arcconfig is None:
            self.logger.error('Failed to get parsed arc.conf. DataStaging control is not possible.')
            sys.exit(1)
            
        # controldir is mandatory
        self.control_dir = self.arcconfig.get_value('controldir', 'arex').rstrip('/')
        if self.control_dir is None:
            self.logger.critical('Jobs control is not possible without controldir.')
            sys.exit(1)

    def _calc_timewindow(self,args):
        twindow = None
        if args.days:
            twindow =  datetime.timedelta(days=args.days)
        if args.hours:
            if twindow:
                twindow = twindow + datetime.timedelta(hours=args.hours)
            else:
                twindow = datetime.timedelta(hours=args.hours)
        if args.minutes:
            if twindow:
                twindow = twindow + datetime.timedelta(minutes=args.minutes)
            else:
                twindow = datetime.timedelta(minutes=args.minutes)
        if args.seconds:
            if twindow:
                twindow = twindow + datetime.timedelta(seconds=args.seconds)
            else:
                twindow = datetime.timedelta(seconds=args.seconds)

        if twindow:
            twindow_start=datetime.now() - twindow
            return twindow_start
        else:
            return None

    
    def _get_tstamp(self,line):
        """ Extract timestamp from line of type 
        [2016-08-22 15:17:49] [INFO] ......
        or of type
        <Log>[2016-08-22 15:17:49] [INFO] ...... 
        """
        
        timestmp_str = ''

        if '<Log>' in line:
            words = re.split('>',line)[-1]
            words = re.split(' +', words)
            words = [word.strip() for word in words]
            timestmp_str = words[0].strip('[') + ' ' + words[1].strip(']')
        else:
            words = re.split(' +', line)
            words = [word.strip() for word in words]
            timestmp_str = words[0].strip('[') + ' ' + words[1].strip(']')
            
        timestamp = datetime.strptime(timestmp_str,'%Y-%m-%d %H:%M:%S')
        return timestmp_str, timestamp


    def _get_filecount_joblog(self,log_f,jobid,twindow_start=None):
        pass
        
    def _get_timestamps_joblog(self,log_f,jobid,twindow_start=None):
        """ 
        Get the total time from PREPARING to SUBMIT which is the total time in PREPARING.
        Only calculate the time for the jobs that actually have user-deinfed input-files (not pilot-related).
        Last filter here, to ensure that only jobs that actually were in preparing in the timewindow are counted, in the case where a timewindow is used. 
        """

        has_udef_input = None
        ds_time={'start':'','end':'','dt':'','done':False,'failed':False,'noinput':False}
        has_udef_input = self._has_userdefined_inputfiles(jobid)

        if has_udef_input is None:
            print('The grami file for job with id: ', jobid, ' is no longer present. Skipping this job.')
        elif has_udef_input is False:
            ds_time={'start':'','end':'','dt':'','done':False,'failed':False,'noinput':True}
        else:
            # This job has user-defined inputfiles for datastaging
            ds_start = None
            ds_end = None
            with open(log_f,'r') as f:
                for line in f:

                    words = re.split(' +', line)
                    words = [word.strip() for word in words]
                
                    if "ACCEPTED -> PREPARING" in line:
                    
                        timestmp_str = words[0].strip('[')
                        timestmp_str = timestmp_str.strip(']')
                        ds_time['start']=timestmp_str
                        
                        timestamp = datetime.strptime(timestmp_str,'%Y-%m-%dT%H:%M:%SZ')
                        ds_start = timestamp

                    elif "PREPARING -> SUBMIT" in line:

                        timestmp_str = words[0].strip('[')
                        timestmp_str = timestmp_str.strip(']')
                        ds_time['end']=timestmp_str
                        
                        timestamp = datetime.strptime(timestmp_str,'%Y-%m-%dT%H:%M:%SZ')
                        ds_end = timestamp
                        
                        ds_time['done']=True
                        continue

                    elif "PREPARING -> FINISHING" in line and 'failure' in line:
                        ds_time['failed']=True
                        continue
                    
                    else:
                        # if this line does not contain target string, continue to next line
                        continue
                        
            if twindow_start and ds_end:
                # If the datastaging ended before the selected timewindow
                if ds_end < twindow_start:
                    return None

            if ds_start and ds_end:
                # Datastaging done
                ds_time['dt']=str(ds_end - ds_start)
            elif ds_start:
                # Datastaging ongoing
                ds_time['dt']=str(datetime.utcnow() - ds_start)
            else:
                return None
        return ds_time

    def _has_userdefined_inputfiles(self,jobid):
        grami_file = control_path(self.control_dir, jobid, 'grami')
        try:
            with open(grami_file,'r') as f:
                for line in f:
                    if 'joboption_inputfile' in line:
                        fileN = line.split('=')[-1].strip()
                        fileN = fileN.replace("'/","")
                        fileN = fileN.replace("'","")
                        return True

        except OSError:
            return None
        return False



    def _get_filesforjob(self,jobid):
        """ The grami-info only contains filename, not URI
        - grami files only contains user-defined input files """
        grami_file = control_path(self.control_dir, jobid, 'grami')
        all_files_user = []
        try:
            with open(grami_file,'r') as f:
                for line in f:
                    if 'joboption_inputfile' in line:
                        fileN = line.split('=')[-1].strip()
                        fileN = fileN.replace("'/","")
                        fileN = fileN.replace("'","")
                        

                        if len(fileN)>0:
                            all_files_user.append(fileN)
        except OSError:
            # Will just then return empty all_files and all_files_user lists
            pass
                            
        return all_files_user


    def _get_file_events_statistics(self, jobid, file_events):
        """ Get files already downloaded from jobs statistics file
        This includes non-user defined inputfiles like pilot or json site files """

        client_uploads = []
        stat_file = control_path(self.control_dir, jobid, 'statistics')
        try:
            with open(stat_file,'r') as f:
                for line in f:
                    line = line.strip()

                    if line and 'inputfile' in line:
                        words = re.split(',', line)
                        words = [word.strip() for word in words]
                        
                        fileN = words[0].split('inputfile:')[-1].split('/')[-1]

                        # The statistics file can contain files that are not listed in grami file - add these too and initialize
                        if fileN not in file_events.keys():
                            file_events[fileN] = {}
                            file_events[fileN]['staged_in'] = 'no'
                            file_events[fileN]['dtrid_short'] = '-'
                            
                        source = (words[0].split('inputfile:')[-1]).split('=')[-1][:-len(fileN)-1]
                        size = int(words[1].split('=')[-1])/(1024*1024.)
                        start = words[2].split('=')[-1]
                        end = words[3].split('=')[-1]
                        cached = words[4].split('=')[-1]
                        atlasfile = False

                        if 'panda' in fileN or 'pilot' in fileN:
                            atlasfile = True

                        start_dtstr = datetime.strptime(start, '%Y-%m-%dT%H:%M:%SZ')
                        end_dtstr = datetime.strptime(end, '%Y-%m-%dT%H:%M:%SZ')
                        seconds = (end_dtstr - start_dtstr).seconds

                        file_events[fileN] = {'size':size,'source':source,'start':start,'end':end,'seconds':seconds,'cached':cached,'atlasfile':atlasfile, 'staged_in': 'yes'}
                        # To also show <site>.all.json file that is uploaded with the job by the client
                        #if fileN not in all_files and 'json' in fileN:
                        #    client_uploads.append(fileN)
        except IOError:
            # TODO-error message/info message?
            pass
        return file_events#, client_uploads

    def _get_file_events_errors(self,jobid,file_events):
        """ Get more details about ongoing  transfers from jobs errors file"""
        dtr_file = {}
        file_dtr = {}

        log_file = control_path(self.control_dir, jobid, 'errors')
        with open(log_file,'r') as f:
            for line in f:

                words = re.split(' +', line)
                words = [word.strip() for word in words]

                    
                if 'Scheduler received new DTR' in line:
                    
                    # First instance of the new DTR
                    timestmp_str, timestmp_obj = self._get_tstamp(line)
                    dtrid_short = words[5].replace(':','')
                    fileN = words[13].split('/')[-1]
                    fileN = fileN.replace(",","")

                    dtr_file[dtrid_short]=fileN
                    file_dtr[fileN]=dtrid_short

                    
                    # file_events may or may not contain information already - 
                    # from statistics file - _get_file_details_statistics 
                    # But since this is the first occurence of datastaging events for this fileN recorded 
                    # in errors file I only need to check in this if-statement 
                    # whether the dictionary already contains the fileN
                    if fileN not in file_events.keys():
                        file_events[fileN]={}
                        file_events[fileN]['staged_in'] = 'no'

                        
                    file_events[fileN]['dtrid_short']=dtrid_short
                    file_events[fileN]['start_sched']=timestmp_str


                elif 'Started remote Delivery at' in line:
                    # Extract remote datadelivery host

                    dtrid_short = words[5].replace(':','')
                    fileN = dtr_file[dtrid_short]
                    url = words[-1]
                    file_events[fileN]['remote_dds']=url
                    #[2024-08-14 12:29:56] [VERBOSE] [508263/32095] DTR 1504...4a5e: Connecting to Delivery service at http://10.2.1.96:33555/datadeliveryservice

                elif 'Delivery received new DTR' in line:

                    dtr_idx = 5
                    if '<Log>' in line:
                        dtr_idx = 6
                                                
                    # Delivery actually starting - waiting time over
                    timestmp_str, timestmp_obj = self._get_tstamp(line)
                    dtrid_short = words[dtr_idx].replace(':','')
                    fileN = dtr_file[dtrid_short]
                    
                    file_events[fileN]['start_deliver']=timestmp_str

                elif 'Transfer finished' in line:

                    # Transfer done, file downloaded
                    timestmp_str, timestmp_obj = self._get_tstamp(line)
                    dtrid_short = words[5].replace(':','')
                    fileN = dtr_file[dtrid_short]

                    file_events[fileN]['transf_done']=timestmp_str
                    file_events[fileN]['staged_in']='yes'
                    
                    # Calculated avg download speed
                    # Calculated by diff in time between 'Delivery received new DTR' and 'Transfer finished 
                    # messages for the DTR. 
                    
                    if 'size' in file_events[fileN].keys() and 'start_deliver' in file_events[fileN].keys():
                        start_dwnld = datetime.strptime(file_events[fileN]['start_deliver'], "%Y-%m-%d %H:%M:%S")
                        end_dwnld = datetime.strptime(file_events[fileN]['transf_done'], "%Y-%m-%d %H:%M:%S")
                        seconds = (end_dwnld - start_dwnld).seconds
                        try:
                            speed = float(file_events[fileN]['size']/seconds)
                        except ZeroDivisionError:
                            speed = -1
                        
                        file_events[fileN]['speed']=speed
                        file_events[fileN]['seconds']=seconds
 

                elif 'Returning to generator' in line:

                    # DTR done
                    timestmp_str, timestmp_obj = self._get_tstamp(line)
                    dtrid_short = words[5].replace(':','')
                    fileN = dtr_file[dtrid_short]
                    
                    file_events[fileN]['return_gen']=timestmp_str

        return file_events


        
    def _get_file_events(self,jobid):
        """  Extracts information about the files already downloaded for a single job 
        from its jobs statistics file 
        includes download info for non-user defined file such as pilot or site json file """
  
        all_userdef_inputs = self._get_filesforjob(jobid)
        file_events = {}
        file_events = dict.fromkeys(all_userdef_inputs, {})
        for fileN in file_events:
            file_events[fileN]['staged_in'] = 'no'
            file_events[fileN]['dtrid_short'] = '-'
            
        file_events  = self._get_file_events_statistics(jobid, file_events)
        # Get more details about the transfers from jobs errors file
        file_events = self._get_file_events_errors(jobid, file_events)
                    
        return file_events


    def _get_dtrstates(self,args):

        
        dtrlog = self.arcconfig.get_value('statefile', 'arex/data-staging')

        if dtrlog is None:
            self.logger.critical('Job state file name can not be found in arc.conf.')
            return

        dtrlog = dtrlog.rstrip('/')
        state_counter = {}
        num_lines = 0

        if os.path.exists(dtrlog):
            with open(dtrlog,'r') as f:

                for line in f:
                    l = line.split()
                    state = l[1]
                    if state == 'TRANSFERRING':
                        if len(l) == 6:
                            host = l[5]
                            state = state + '_' + host
                        else:
                            state = state + '_local'
                    if state in state_counter:
                        state_counter[state] = state_counter[state] + 1
                    else:
                        state_counter[state] = 1
                    num_lines = num_lines + 1

                state_counter['ARC_STAGING_TOTAL']=num_lines
        
        else:
            self.logger.error('Failed to open DTR state file: %s',dtrlog)

        return  state_counter



    def ascii_histogram(self,data):

        hist = {}

        bin_edges = [0, 1, 5, 15, 30, 60, 180, 360, 720, 1440, 4320]
        bin_labels = ['0-1m', '1-5m', '5-15m', '15-30m', '30-60m', '1-3h', '3-6h', '6-12h', '12-24h', '1d-3d','>3d']
        duration_minutes = []


        # Initialize histogram dict
        for bin_edge in bin_edges:
            hist[bin_edge] = 0

            
        for time_str in data:
            time_delta = None
            try:
                time_delta = datetime.strptime(time_str, '%H:%M:%S')
            except ValueError:
                time_delta = datetime.strptime(time_str, '%H:%M:%S.%f')
            except:
                pass
            # Ignore seconds
            duration_minutes = time_delta.hour*60 + time_delta.minute

            if duration_minutes >= bin_edges[0] and duration_minutes < bin_edges[1]:
                hist[bin_edges[0]] +=1
            elif duration_minutes >= bin_edges[1] and duration_minutes < bin_edges[2]:
                hist[bin_edges[1]] +=1
            elif duration_minutes >= bin_edges[2] and duration_minutes < bin_edges[3]:
                hist[bin_edges[2]] +=1
            elif duration_minutes >= bin_edges[3] and duration_minutes < bin_edges[4]:
                hist[bin_edges[3]] +=1
            elif duration_minutes >= bin_edges[4] and duration_minutes < bin_edges[5]:
                hist[bin_edges[4]] +=1
            elif duration_minutes >= bin_edges[5] and duration_minutes < bin_edges[6]:
                hist[bin_edges[5]] +=1
            elif duration_minutes >= bin_edges[6] and duration_minutes < bin_edges[7]:
                hist[bin_edges[6]] +=1
            elif duration_minutes >= bin_edges[7] and duration_minutes < bin_edges[8]:
                hist[bin_edges[7]] +=1
            elif duration_minutes >= bin_edges[8] and duration_minutes < bin_edges[9]:
                hist[bin_edges[8]] +=1
            elif duration_minutes >= bin_edges[9] and duration_minutes < bin_edges[10]:
                hist[bin_edges[9]] +=1
            else:
                hist[bin_edges[10]] +=1


        # Find the maximum count for scaling
        max_count = max(hist.values())
    
        # Print the histogram
        for bin_num, count in hist.items():
            bin_idx = bin_edges.index(bin_num)
            
            bar = '\u2588' * int(50 * count / max_count)
            print(f'{bin_labels[bin_idx]:>8}: {bar} ({count})')





        
    def show_preparing_jobs(self,args):
        
        out,err=subprocess.Popen(['arcctl','job','list','-s','PREPARING'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False).communicate()
        jobids_preparing = out.decode().split('\n')
        # Remove empty items
        jobids_preparing = [item for item in jobids_preparing if item]

        # Extract information about files being prepared including on what host
        jobs_prep = {}

        # Collect all files for jobs in PREPARING state that are logged as remotely downloaded 
        # Save this in a new dictionary for printing out
        print('\n\n')
        print(f'Datastaging information for jobs in PREPARING state, which are currently being staged in or are in progress or waiting to be staged.')
        fmt = "\t{:<9} {:<14.14} {:<12.12} {:<60} {:<60}  {:<60} {:<15} {:<7} {:<25} {:<25}"
        print(fmt.format('COUNTER', 'ARC-ID', 'DTR-ID', 'FILENAME', 'SOURCE', 'REMOTE-DELIVERY', 'SIZE (MB)', 'SEC (s)', 'START', 'END'))
        counter = 1
        for idx, jobid in enumerate(jobids_preparing):

            # For this jobid - get details about all files currently done in datastaging
            file_dict = self._get_file_events(jobid)
            print('\n')
            for fileN,evts in file_dict.items():

                dtrid = evts['dtrid_short']
                if 'source' not in evts.keys():
                    evts['source'] = '-'
                if 'size' not in evts.keys():
                    evts['size'] = -1
                if 'end' not in evts.keys():
                    evts['end'] = '-'
                if 'seconds' not in evts.keys():
                    evts['seconds'] = -1
                if 'remote_dds' not in evts.keys():
                    evts['remote_dds'] = ''
                if 'start_deliver' not in evts.keys():
                    evts['start_deliver'] = '-'
                    
                try:
                    fmt = "\t{:>4}-{:<4} {:<14.14} {:<12.12} {:<60.60}  {:<60.60} {:<60.60} {:<15.1f} {:<7} {:<25} {:<25}"
                    print(fmt.format(idx+1,counter, jobid, dtrid, fileN, evts['source'], evts['remote_dds'], evts['size'], evts['seconds'], evts['start_deliver'], evts['end']))
                except Exception as e:
                    print(f'Got an exception while printing out information for preparing job {jobid} for {fileN}: {e}')
                counter += 1

        #else:
        #    print(f'There are currently no jobs in preparing state.')


            

    
    def show_remote_delivery_hosts(self,args):
        
        deliveryservices = self.arcconfig.get_value('deliveryservice', 'arex/data-staging')
        out,err=subprocess.Popen(['arcctl','job','list','-s','PREPARING'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False).communicate()
        jobids_preparing = out.decode().split('\n')
        # Remove empty items
        jobids_preparing = [item for item in jobids_preparing if item]

        # Extract information about files being prepared including on what host
        remote_delivery = {}

        # Collect all files for jobs in PREPARING state that are logged as remotely downloaded 
        # Save this in a new dictionary for printing out
        for jobid in jobids_preparing:

            # For this jobid - get details about all files currently done in datastaging
            file_events = self._get_file_events(jobid)
            for fileN,val in file_events.items():

                if 'remote_dds' in val.keys():
                    remote_dds =  val['remote_dds']
                    if remote_dds not in remote_delivery.keys():
                        remote_delivery[remote_dds]={}
                        
                    remote_delivery[remote_dds][fileN] = copy.deepcopy(val)
                    remote_delivery[remote_dds][fileN]['jobid']=jobid
                    


        idx = 0
        print('\n\n')
        if remote_delivery:
            fmt = "\t{:<8} {:<60} {:<14.14} {:<10.10} {:<60} {:<60} {:<15} {:<7} {:<25} {:<25} "
            print(f'Datadelivery service and file info for jobs in PREPARING state, which are currently been staged in or are in progress of being staged in at a remote datadelivery service.')
            print(fmt.format('COUNTER', 'REMOTE-DELIVERY', 'ARC-ID', 'DTR-ID', 'FILENAME', 'SOURCE', 'SIZE (MB)', 'SEC (s)', 'START', 'END'))



            for remote_dds,remote_delivery in remote_delivery.items():
                print('\n')
                for fileN,val in remote_delivery.items():
                    idx += 1

                    jobid = val['jobid']
                    dtrid = val['dtrid_short']
                    if 'source' not in val.keys():
                        val['source'] = '-'
                    if 'size' not in val.keys():
                        val['size'] = -1
                    if 'end' not in val.keys():
                        val['end'] = '-'
                    if 'seconds' not in val.keys():
                        val['seconds'] = -1
                    try:
                        fmt = "\t{:<8} {:<60} {:<14.14} {:<10.10} {:<60} {:<60} {:<15.1f} {:<7} {:<25.25} {:<25.25} "
                        print(fmt.format(idx, remote_dds, jobid,  dtrid, fileN, val['source'], val['size'], val['seconds'], val['start_deliver'], val['end']))
                    except Exception as e:
                        print(f'Got an exception while printing out information for remote datadelivery sites for {fileN} and jobid: {jobid}: {e}')

        else:
            print(f'There are currently no files being staged by any remote datadelivery services')

        
    def show_dtrstates(self,args):

        state_counter = self._get_dtrstates(args)

        # TO-DO print in nice order
        print_order = ['CACHE_WAIT','STAGING_PREPARING_WAIT','STAGE_PREPARE','TRANSFER_WAIT','TRANSFER','PROCESSING_CACHE']

        fmt = "\t{:<25} {:<20} {:<6}"
        if state_counter:
            print('Number of current datastaging processes (files):')
            print(fmt.format('State', 'Data-delivery host', 'Number'))
            
            count_transferring = 0

            # First print the most important states:
            for state in print_order:
                try:
                    print(fmt.format(state, 'N/A', state_counter[state]))
                except KeyError:
                    pass

            # Now print the TRANSFERRING states
            for key, val in state_counter.items():
                if 'TRANSFERRING' in key:
                    state = 'TRANSFERRING'
                    host = key.split('_')[-1]
                    count_transferring += val
                    if 'local' in host:
                        continue
                    print(fmt.format(state, host, val))


            # Print out other states
            for key, val in state_counter.items():
                if 'TRANSFERRING' in key or key in print_order or 'local' in key or key in 'ARC_STAGING_TOTAL': continue
                print(fmt.format(key, 'N/A',val))

            try:
                count_transferring += state_counter['TRANSFERRING_local']
                print(fmt.format('TRANSFERRING', 'local', state_counter['TRANSFERRING_local']))
            except KeyError:
                pass

            # Print out divider
            print('-'*60)
            # Sum up all TRANSFERRING slots
            print(fmt.format('TRANSFERRING TOTAL', 'N/A', count_transferring))

            # Finally print the sum of all dtrs
            print(fmt.format('ARC_STAGING_TOTAL', 'N/A', state_counter['ARC_STAGING_TOTAL']))
                
        else:
            print('No information in %s file: currently no datastaging processes running',dtrlog)
                
        return


    def show_job_time(self,jobid):

        datastaging_time={}
        log_f = control_path(self.control_dir, jobid, 'errors')
        datastaging_time=self._get_timestamps_joblog(log_f,jobid)

        if datastaging_time:
            print(f"\nTotal datastaging duration for jobid {jobid:<50}")
            if datastaging_time['noinput']:
                print('\tThis job has no user-defined input-files, hence no datastaging needed/done.')
            else:
                if datastaging_time['done']:
                    fmt = "\t{:<21} \t{:<21} \t{:<12}"
                    print(fmt.format('Start' 'End' 'Duration'))
                    print(fmt.format(datastaging_time['start'], datastaging_time['end'], datastaging_time['dt']))
                else:
                    fmt = "\t{:<21} \t{:<21} \t{:<12}"
                    print("\tDatastaging still ongoing")
                    print(fmt.format('Start', 'Duration'))
                    print(fmt.format(datastaging_time['start'],datastaging_time['dt']))
        else:
            print(f'No datastaging information for jobid {jobid:<50} - Try arcctl accounting instead - the job might be finished.')



    def show_job_details(self,jobid,file_details):


        done_stagedin = {k: v for k, v in file_details.items() if v.get('staged_in') == 'yes'}
        tobe_stagedin = {k: v for k, v in file_details.items() if v.get('staged_in') == 'no'}
        

        # General info
        print(f'\nInformation  about input-files for jobid {jobid} ')
        
        # Print out a list of all files and if staged-in or not
        print('\nState of input-files:')
        fmt = "\t{:<8} {:<60.60} {:<20} {:<12})"
        print(fmt.format('COUNTER', 'FILENAME', 'DTR-ID', 'STAGED-IN'))
        for idx,fileN in enumerate(file_details.keys()):
            print(fmt.format(idx+1, fileN, file_details[fileN]['dtrid_short'], file_details[fileN]['staged_in']))
        print('\tNote: files uploaded by the client appear to not be staged-in, ignore these as AREX does not handle the stage-in of these files. Examples for ATLAS: queudata.json pandaJobData.out runpilot2-wrapper.sh')
                
        # Print out information about files already staged in
        sorted_dict = sorted(done_stagedin.items(), key = lambda x: x[1]['end'])
        print('\nDetails for files that have been staged in - both downloaded and cached:')
        fmt = "\t{:<8} {:<60} {:<60} {:<15} {:<25} {:<25} {:<10} {:<7}"
        print(fmt.format('COUNTER', 'FILENAME', 'SOURCE', 'SIZE (MB)', 'START', 'END', 'SECONDS', 'CACHED'))
        for idx,item in enumerate(sorted_dict):
            fileN = item[0]
            filedict = item[1]
            fmt = "\t{:<8} {:<60.60} {:<60} {:<15} {:<25} {:<25} {:<10} {:<7}"
            print(fmt.format(idx+1, fileN, filedict['source'], filedict['size'], filedict['start'], filedict['end'], filedict['seconds'], filedict['cached']))

            
        # Print out information about files already downloaded
        # TO-DO find a nice way to sort this, maybe removing the files that do not have all info provided?
        downloads = False
        print('\nFine-grained details for files that have been staged-in by download (not cached):')
        fmt = "\t{:<5.5} {:<15.15} {:<15.15} {:<20.20} {:<20.20} {:<20.20} {:<20.20} {:<20.20} {:<20.20} {:<6} {:<10.10} {}"
        print(fmt.format('COUNT', 'FILENAME', 'SIZE (MB)', 'START', 'END', 'SCHEDULER-START', 'DELIVERY-START', 'TRANSFER-DONE', 'ALL-DONE', '(s)', '(MB/s)', 'DELIVERY-SERVICE'))
        idx = 1
        for key,val in done_stagedin.items():
            if 'remote_dds' not in val:
                val['remote_dds'] = '-'
            if ('start_deliver' in val.keys() and 'start_sched' in val.keys() and 'return_gen' in val.keys() and 'speed' in val.keys()):
                fmt = "\t{:<5.5} {:<15.15} {:<15.3f} {:<20.20} {:<20.20} {:<20.20} {:<20.20} {:<20.20} {:<20.20} {:<6} {:<10.1f} {}"
                print(fmt.format(idx, key, val['size'], val['start'], val['end'], val['start_sched'], val['start_deliver'], val['transf_done'], val['return_gen'], val['seconds'], val['speed'], val['remote_dds']))
                idx += 1
                downloads = True
        if not downloads:
            print('\tNo download info available, probably because all files for this jobs were already in the cache.')




    def show_summary_files(self,args):
        """ 
        Total number and size of files downloaded in the chosen timewindow for jobs in state PREPARING or INLRMS
        Uses the job.<jobid>.statistics file to extract files downloaded, as this gets updated file by file once a download is done.
        """

        n_jobs = 0 

        n_files_all = 0
        n_files_all_user = 0
        n_files =  0
        n_files_atlas =  0
        n_files_user =  0
        n_files_cached = 0
        n_files_cached_atlas = 0
        n_files_cached_user = 0
        n_files_downloaded = 0
        n_files_downloaded_atlas = 0
        n_files_downloaded_user = 0

        size_files = 0
        size_files_atlas = 0
        size_files_user = 0
        size_files_cached = 0
        size_files_cached_atlas = 0
        size_files_cached_user = 0
        size_files_downloaded = 0
        size_files_downloaded_atlas = 0
        size_files_downloaded_user = 0

        datastaging_files={}
        twindow_start = self._calc_timewindow(args)

        if twindow_start:
            print(f"\nThis may take some time... \nFetching the total number of files downloaded for jobs in INLRMS and PREPARING state that were modified after {datetime.strftime(twindow_start,'%Y-%m-%d %H:%M:%S')}")
        else:
            print(f"\nThis may take some time... \nFetching the total number of files downloaded for jobs in INLRMS and PREPARING state")

        out,err=subprocess.Popen(['arcctl','job','list','-s','PREPARING'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False).communicate()
        jobids = out.decode().split('\n')

        out,err=subprocess.Popen(['arcctl','job','list','-s','INLRMS'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False).communicate()
        jobids.extend(out.decode().split('\n'))

        # Remove any empty items
        jobids = [item for item in jobids if item]
        
        for jobid in jobids:
            log_f = control_path(self.control_dir, jobid, 'statistics')
            mtime = None
            try:
                mtime=datetime.fromtimestamp(os.path.getmtime(log_f))
            except OSError:
                # Files got removed in the meantime, skip this job
                continue
        
            # Skip all jobs that are last modified before the users or default timewindow
            if twindow_start and mtime < twindow_start:
                continue


            file_events = {}
            file_events = self._get_file_events(jobid)
            n_jobs += 1
            n_files_all += len(file_events.keys())


            if not n_jobs:
                print('\nNo active jobs were found in the selected timewindow --> exiting.')
                return 

            
            # Collecting information for files that are done in stage-in for this job
            for key, val in file_events.items():
                
                #print(f'jobid: {jobid} fileN: {key} val: {val}')
                try:
                    # Only files in active download during the timewindow are added
                    end = datetime.strptime(val['end'], '%Y-%m-%dT%H:%M:%SZ')
                    if twindow_start and end < twindow_start:
                        continue
                except KeyError:
                    continue
                
                n_files += 1
                # Originally converted to MB, want GB here
                size_files += val['size']/1024.

                if val['atlasfile']:
                    n_files_atlas += 1
                    size_files_atlas += val['size']/1024.
                else:
                    n_files_user += 1
                    size_files_user += val['size']/1024.

                    
                if val['cached'] == 'yes':
                    n_files_cached += 1
                    size_files_cached += val['size']/1024.
                    if val['atlasfile']:
                        n_files_cached_atlas += 1
                        size_files_cached_atlas += val['size']/1024.
                    else:
                        n_files_cached_user += 1
                        size_files_cached_user += val['size']/1024.
                else:
                    n_files_downloaded += 1
                    size_files_downloaded += val['size']/1024.
                    if val['atlasfile']:
                        n_files_downloaded_atlas += 1
                        size_files_downloaded_atlas += val['size']/1024.
                    else:
                        n_files_downloaded_user += 1
                        size_files_downloaded_user += val['size']/1024.


        fmt_header = "{:<50} {:<10} {:<15}"
        fmt_rows =   "{:<50} {:<10} {:<15.1f}"
        if twindow_start:
            print('\nTimewindow start: : '+ datetime.strftime(twindow_start,'%Y-%m-%d %H:%M:%S'))
        print('\nALL JOBS')
        print('Number of jobs active since start of timewindow '+ str(n_jobs))
        print('\tTotal number of inputfiles for these jobs: ' + str(n_files_all))
        #print('\tOut of these, user-defined input-files amounted to: ' + str(n_files_all_user))
    
        if not n_files:
            print('\nNo files were done staging-in during the selected timewindow --> exiting.')
            return

        print('\nFILES DONE IN STAGE-IN')
        print(fmt_header.format('', 'NUMBER', 'SIZE (GB)'))
        print(fmt_rows.format('Total', str(n_files), size_files))

        if n_files_atlas:
            print(fmt_rows.format('    User-defined', str(n_files_user), size_files_user))
            print(fmt_rows.format('    Atlas-files', str(n_files_atlas), size_files_atlas))


        print(fmt_rows.format('Cached Total', str(n_files_cached), size_files_cached))
        if n_files_atlas:
            print(fmt_rows.format('    User-defined', str(n_files_cached_user), size_files_cached_user))
            print(fmt_rows.format('    Atlas-files', str(n_files_cached_atlas), size_files_cached_atlas))
        
        print(fmt_rows.format('Not-cached Total (downloaded)', str(n_files_downloaded), size_files_downloaded))
        if n_files_atlas:
            print(fmt_rows.format('    User-defined', str(n_files_downloaded_user), size_files_downloaded_user))
            print(fmt_rows.format('    Atlas-files', str(n_files_downloaded_atlas), size_files_downloaded_atlas))


        print('\n\nRATIOS')
        print("{:<50} {:<20} {}:<20".format('', 'RATIO amount', 'RATIO size'))
        fmt = "{:<50} {:<20.4} {:<20.4}"
        if n_files_atlas:
            try:
                print(fmt.format('Total User/Atlas', float(n_files_user)/float(n_files_atlas), float(size_files_user/size_files_atlas)))
            except ZeroDivisionError:
                pass
        try:
            print(fmt.format('Total Cached/non-cached', float(n_files_cached)/float(n_files_downloaded), float(size_files_cached/size_files_downloaded)))
        except ZeroDivisionError:
            pass
        if n_files_atlas:
            try:
                print(fmt.format('Total Cached-user/Cached-atlas', float(n_files_cached_user)/float(n_files_cached_atlas), float(size_files_cached_user/size_files_cached_atlas)))
            except ZeroDivisionError:
                pass
            try:
                print(fmt.format('Total Downloaded-user/Downloaded-atlas', float(n_files_downloaded_user)/float(n_files_downloaded_atlas), float(size_files_downloaded_user/size_files_downloaded_atlas)))
            except ZeroDivisionError:
                pass


    def show_summary_jobs(self,args):        
        """ Overview over duration of all datastaging processes for jobs in PREPARING or RUNNING in the chosen timewindow 
        Checks job errors files that have been modified during the timewindow. 
        Checks duration between ACCEPTED -> PREPARING to PREPARING -> FINISHING stages. 
        """
        datastaging_jobs={}
        twindow_start = self._calc_timewindow(args)

        if twindow_start:
            print(f"\nThis may take some time... \nFetching summary of download times for INLRMS and PREPARING jobs modified after {format(datetime.strftime(twindow_start,'%Y-%m-%d %H:%M:%S'))}")
        else:
            print(f"\nThis may take some time... \nFetching summary of download times for all INLRMS and PREPARING jobs")
        
        out,err=subprocess.Popen(['arcctl','job','list','-s','PREPARING'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False).communicate()
        jobids = out.decode().split('\n')

        out,err=subprocess.Popen(['arcctl','job','list','-s','INLRMS'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False).communicate()
        jobids.extend(out.decode().split('\n'))

        for jobid in jobids:
            if not jobid:
                # Protect against empty string
                continue
            log_f = control_path(self.control_dir, jobid, 'errors')
            mtime = None
            try:
                mtime=datetime.fromtimestamp(os.path.getmtime(log_f))
            except OSError:
                # Files got removed in the meantime, skip this job
                continue
        
            # Skip all files that are modified before the users or default timewindow
            if twindow_start and (mtime < twindow_start):
                continue

            try:
                jobdict = self._get_timestamps_joblog(log_f,jobid,twindow_start)
                if jobdict:
                    datastaging_jobs[jobid]=jobdict
            except:
                continue


        done_dict={}
        ongoing_dict={}
        failed_dict={}
        noinput_list=[]
        for key,val in datastaging_jobs.items():
            if not val['failed']:
                if val['noinput']:
                    noinput_list.append(key)
                    continue
                if val['done']:
                    done_dict[key]=val
                else:
                    ongoing_dict[key]=val
            else:
                failed_dict[key]=val
        
        print('\nJobs where no user-defined input-data was defined (no datastaging done/needed): ')
        print('Number of jobs:  '+ str(len(noinput_list)))
        for jobid in noinput_list:
            print(f"\t{jobid:}")
            
        sorted_dict = sorted(done_dict.items(), key = lambda x: x[1]['dt'])
        print('\nSorted list of jobs where datastaging is done:')
        print('Number of jobs:  '+ str(len(sorted_dict)))
        done_list = []
        ongoing_list = []
        if sorted_dict:
            print(f"\t{'ARCID':<60} {'DURATION':<10} {'TIMESTAMP-START':<22} {'TIMESTAMP-DONE':<22}")
            for item in sorted_dict:
                print(f"\t{item[0]:<60} {item[1]['dt']:<10} {item[1]['start']:<22} {item[1]['end']:<22}")
                done_list.append(item[1]['dt'])
        else:
            print('\nNo jobs where datastaging is done.')   

        sorted_dict = sorted(ongoing_dict.items(), key = lambda x: x[1]['dt']) 
        if ongoing_dict:
            print('\nSorted list of jobs where datastaging is ongoing:')
            print('Number of jobs:  ' + str(len(sorted_dict)))
            print(f"\t{'ARCID':<60} {'DURATION':<10}")
            for item in sorted_dict:
                print(f"\t{item[0]:<60} {item[1]['dt'].split('.')[0]:<10}")
                ongoing_list.append(item[1]['dt'])
        else:
            print('\nNo jobs where datastaging is ongoing.')   

        sorted_dict = sorted(failed_dict.items(), key = lambda x: x[1]['dt']) 
        if failed_dict:
            print('\nDatastaging used for failed jobs:')
            print('Number of jobs:  '+ str(len(sorted_dict)))
            print(f"\t{'ARCID':<60} {'DURATION':<10}")
            for item in sorted_dict:
                print(f"\t{item[0]:<60} {item[1]['dt'].split('.')[0]:<10}")


        if args.showhist:
            if done_list:
                print('\nHistogram of datastaging durations for jobs done with datastaging')
                self.ascii_histogram(done_list)
                print('\n')
            if ongoing_list:
                print('\nHistogram of datastaging durations for jobs with ongoing datastaging')
                self.ascii_histogram(ongoing_list)
                print('\n')

            
    def summarycontrol(self,args):
        if args.summaryaction == 'jobs':
            self.show_summary_jobs(args)
        elif args.summaryaction == 'files':
            self.show_summary_files(args)
        else:
            print('Usage ')

                
    def jobcontrol(self,args):
        canonicalize_args_jobid(args)

        if args.jobaction == 'get-totaltime':
            jobid = args.jobid
            self.show_job_time(jobid)
        elif args.jobaction == 'get-details':
            jobid = args.jobid
            file_details = self._get_file_events(jobid)
            self.show_job_details(jobid,file_details)
            self.show_job_time(jobid)
        elif args.jobaction == 'get-all':
            self.show_preparing_jobs(args)
            

    def control(self, args):
        if args.action == 'job' or 'jobaction' in args:
            canonicalize_args_jobid(args)
            self.jobcontrol(args)
        elif args.action == 'summary':
            self.summarycontrol(args)
        elif args.action == 'dtr':
            self.show_dtrstates(args)
        elif args.action == 'remote_dds':
            self.show_remote_delivery_hosts(args)
        else:
            self.logger.critical('Unsupported datastaging action %s', args.action)
            sys.exit(1)

    @staticmethod
    def register_job_parser(dds_job_ctl):

        dds_job_ctl.set_defaults(handler_class=DataStagingControl)
        dds_job_actions = dds_job_ctl.add_subparsers(title='Job Datastaging Menu', dest='jobaction',metavar='ACTION',help='DESCRIPTION')

        dds_job_actions.required = True

        dds_job_total = dds_job_actions.add_parser('get-totaltime', help='Show the total time spent in the preparation stage for the selected job')
        dds_job_total.add_argument('jobid',help='Job ID').completer = complete_job_id
        
        dds_job_details = dds_job_actions.add_parser('get-details', help='Show details related to the  files downloaded for the selected job')
        dds_job_details.add_argument('jobid',help='Job ID').completer = complete_job_id



        dds_job_all = dds_job_actions.add_parser('get-all', help='Show details related to datastaging for all preparing jobs')



        #dds_job_details.add_argument('-d', '--details', help='Detailed info about jobs files', action='store_true')

           
    @staticmethod
    def register_parser(root_parser):

        dds_ctl = root_parser.add_parser('datastaging',help='DataStaging info')
        dds_ctl.set_defaults(handler_class=DataStagingControl)


        dds_actions = dds_ctl.add_subparsers(title='DataStaging Control Actions',dest='action',
                                             metavar='ACTION', help='DESCRIPTION')
        dds_actions.required = True
        
               
        # Job
        dds_job_ctl = dds_actions.add_parser('job',help='Job Datastaging Information for a preparing or running job.')
        DataStagingControl.register_job_parser(dds_job_ctl)

        

        # Summary for jobs in PREPARING or RUNNING
        dds_summary_ctl = dds_actions.add_parser('summary',help='Job Datastaging Summary Information for jobs preparing or running.')
        dds_summary_ctl.set_defaults(handler_class=DataStagingControl)
        dds_summary_actions = dds_summary_ctl.add_subparsers(title='Job Datastaging Summary Menu',dest='summaryaction',metavar='ACTION',help='DESCRIPTION')
        dds_summary_actions.required = True

        dds_summary_jobs = dds_summary_actions.add_parser('jobs',help='Show overview of the duration of datastaging for all jobs INLRMS or PREPARING status. If no timewindow is specified, all active jobs will be accounted for. If a timewindow is specified, only jobs that have had any activity in the errors file in the specified timewindow are included.')
        dds_summary_jobs.add_argument('-d','--days',type=int,help='Timewindow in days.')
        dds_summary_jobs.add_argument('-hr','--hours',type=int,help='Timewindow in hours.')
        dds_summary_jobs.add_argument('-m','--minutes',type=int,help='Timewindow in minutes.')
        dds_summary_jobs.add_argument('-s','--seconds',type=int,help='Timewindow in seconds.')
        dds_summary_jobs.add_argument('-sh','--showhist',action='store_true',help='Print out ASCII histograms of the datastaging summary.')

        dds_summary_files = dds_summary_actions.add_parser('files',help='Show the total number file and and total file-size downloaded for all jobs INLRMS or PREPARING status. If no timewindow is specified, all active jobs will be accounted for. If a timewindow is specified, only jobs that have had any activity in the errors file in the specified timewindow are included.')
        dds_summary_files.add_argument('-d','--days',type=int,help='Timewindow in days.')
        dds_summary_files.add_argument('-hr','--hours',type=int,help='Timewindow in hours.')
        dds_summary_files.add_argument('-m','--minutes',type=int,help='Timewindow in minutes.')
        dds_summary_files.add_argument('-s','--seconds',type=int,help='Timewindow in seconds.')
        

        # Remote datadelivery services
        dds_remote_ctl = dds_actions.add_parser('remote_dds',help='Remote Datastaging file information for preparing jobs.')
        dds_remote_ctl.set_defaults(handler_class=DataStagingControl)

        
        # Data delivery processes
        dds_dtr_ctl = dds_actions.add_parser('dtr',help='Data-delivery transfer (DTR) information')
        dds_dtr_ctl.set_defaults(handler_class=DataStagingControl)
        dds_dtr_actions = dds_dtr_ctl.add_subparsers(title='DTR info menu',dest='dtr',metavar='ACTION',help='DESCRIPTION')
        
        dds_dtr_state = dds_dtr_actions.add_parser('state', help='Show summary of DTR state info')
        dds_dtr_state.add_argument('state')
